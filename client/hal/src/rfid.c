/**
 * @file rfid.c
 * @brief Implementation of the functions defined in @ref rfid.h.
 * @author Louie Lu 30129418.
 */

#include "hal/rfid.h"

/////////////////// Function Prototypes /////////////////////////

/**
 * Format a register address according to Section 8.1.2.3 of MFRC522 datasheet.
 *
 * "When performing register operations on the MFRC522, an address byte must
 * meet the following format:
 *      - The MSB defines the mode used (1 = read, 0 = write).
 *      - Bits 6 to 1 define the address.
 *      - The LSB is always set to 0."
 *
 * @param regAddr The register address to access.
 * @param mode Specifies read or write.
 * @return byte The formatted register address.
 */
static byte
Rfid_formatRegAddr(byte regAddr, const char mode);

/**
 * Read a register address on the MFRC522 using SPI.
 * @param regAddr The register address to read.
 * @return byte The register's content.
 */
static byte
Rfid_readReg(byte regAddr);

/**
 * Write a value to a register address on the MFRC522 using SPI.
 * @param regAddr The register address to write.
 * @param value The value to write.
 * @return TODO:
 */
static void
Rfid_writeReg(byte regAddr, byte value);

/**
 * Set a bitmask. This is an alternate method of writing a register. Useful when
 * you want to keep a register's contents mostly the same, but want to flip just
 * a few certain bits as specified by the bitmask.
 * @param regAddr The register address to write.
 * @param bitmask The bitmask to set.
 * @return TODO: Something.
 */
static void
Rfid_setBitmask(byte regAddr, byte bitmask);

//  * ====================   (1)   =================   (2)   ==============
//  * |    C Program     | ------> | PCD (MFRC522) | ------> | PICC (MF1) |
//  * | transceiveBuffer | <------ |  FIFO buffer  | <------ |    UID     |
//  * ====================   (4)   =================   (3)   ==============

/**
 * Transmit a PICC command through the PCD's FIFO buffer, then receive the
 * PICC's response from the PCD's FIFO buffer.
 *
 * @param transceiveBuffer A bi-directional buffer. Before calling this
 * function, transceiveBuffer must contain the PICC command to transmit. When
 * the function ends, the buffer will be overwritten with the PICC's response.
 * @param bufferLen The relevant length of transceiveBuffer, i.e. the number of
 * bytes in the PICC command.
 * @param numRxBits An address that will hold the length of the PICC's response
 * in bits.
 * @return int A PICC status code.
 */
static int
Rfid_transceive(byte* transceiveBuffer, int bufferLen, int* numRxBits);

/**
 * Perform a checksum according to Section 2 of AN10927 MIFARE Document.
 * We employ this checksum as a final check to verify a UID has been transmitted
 * correctly (i.e. accounting for potential noise).
 *
 * @param uid An array of 5 bytes obtained from a PICC_ANTICOLLISION command.
 * The first 4 bytes comprise the UID. The 5th byte is the BCC (Block Check
 * Character). The BCC was "hard-coded" during manufacturing, and was calculated
 * as the XOR over the intended UID's 4 bytes.
 * @return int A PICC status code: whether the checksum matches the BCC.
 */
static int
Rfid_performChecksum(byte* uid);

////////////////////// Prototype Implementations /////////////////////////
static byte
Rfid_formatRegAddr(byte regAddr, const char mode)
{
    assert(mode == 'r' || mode == 'w');

    // << 1     simultaneously places the register address in Bits 6-1,
    //          and makes the LSB a 0.
    // | 0x80   makes the MSB a 1 for reading.

    // For example, if we want to read VERSION_REG (0x37):
    // 0x37         = 0011 0111
    // << 1         = 0110 1110
    // | 0x80       = 1110 1110

    switch (mode) {
        case 'r':
            return regAddr << 1 | 0x80;
        case 'w':
            return regAddr << 1;
        default:
            return 0xFF;
    }
}

static byte
Rfid_readReg(byte regAddr)
{
    byte formattedRegAddr = Rfid_formatRegAddr(regAddr, 'r');
    return Spi_readReg(formattedRegAddr);
}

static void
Rfid_writeReg(byte regAddr, byte value)
{
    byte formattedRegAddr = Rfid_formatRegAddr(regAddr, 'w');
    Spi_writeReg(formattedRegAddr, value);
}

static void
Rfid_setBitmask(byte regAddr, byte bitmask)
{
    byte current = Rfid_readReg(regAddr);
    Rfid_writeReg(regAddr, current | bitmask);
}

static int
Rfid_transceive(byte* transceiveBuffer, int bufferLen, int* numRxBits)
{
    // Reset regs that may have been set by a previous transceive call.
    Rfid_writeReg(PCD_COMMAND_REG, PCD_IDLE_CMD); // Cancel any active command.
    Rfid_writeReg(PCD_COM_IRQ_REG, 0x7F);         // Clear all IRQ bits.
    Rfid_writeReg(PCD_FIFO_LEVEL_REG, 0x80);      // Flush FIFO buffer.

    // Write the PICC command (already stored in transceiveBuffer) into the
    // PCD's FIFO buffer. NOTE: Some PICC commands, such as PICC_ANTICOLLISION,
    // are comprised of two bytes - hence the for loop.
    for (int i = 0; i < bufferLen; i++) {
        Rfid_writeReg(PCD_FIFO_DATA_REG, transceiveBuffer[i]);
    }

    // Execute the TRANSCEIVE command.
    Rfid_writeReg(PCD_COMMAND_REG, PCD_TRANSCEIVE_CMD);
    // Set StartSend=1 == begin transceiving
    Rfid_setBitmask(PCD_BIT_FRAMING_REG, 0x80);

    // Wait until the command completes. We do this by polling and checking if
    // the COM_IRQ_REG has a certain bit pattern that signifies it's been
    // interrupted by a tag read.

    // More specifically, it checks if either
    // the RxIRq bit has been set (receiver has detected end of a valid data
    // stream), or the IdleIRq bit has been set (if a command terminates).

    byte waitIrq = 0x30;
    long long deadline = Timeutils_getTimeInMs() + 36;

    bool completed = false;
    do {
        byte n = Rfid_readReg(PCD_COM_IRQ_REG);
        if (n & waitIrq) {
            completed = true;
            break;
        }
        // 0x01 is the bit pattern that signifies a timer-based interrupt, i.e.
        // no tag found in the time limit. (TODO: diff btwn this and <
        // deadline?)
        if (n & 0x01) {
            return PICC_NO_TAG_ERR;
        }
    } while (Timeutils_getTimeInMs() < deadline);

    if (!completed) {
        // puts("!completed");
        return PICC_NO_TAG_ERR;
    }

    // Check if the command produced errors.
    byte errorRegValue = Rfid_readReg(PCD_ERROR_REG);
    if (errorRegValue &
        0x13) { // TODO: 0x13 does not check for collisions. Alternate: 0x1D.
        return PICC_OTHER_ERR;
    }

    // Determine how many bytes are in the FIFO buffer.
    byte numFIFOBytes = Rfid_readReg(PCD_FIFO_LEVEL_REG);
    int lastBits = Rfid_readReg(PCD_CONTROL_REG) & 0x07;

    // printf("numFIFOBytes: %x\n", numFIFOBytes);

    // Basically analogous to checking 7 vs 8 valid bits.
    if (lastBits) {
        *numRxBits = (numFIFOBytes - 1) * 8 + lastBits;
        // seems like whenever this happens, we get the FIFO_ERR
        // puts("lastBits True");
    } else {
        *numRxBits = numFIFOBytes * 8; // 2*8. *8 because 8 bits in 1 byte.
                                       // numRxBits is in # bits.
        // puts("lastBits False");
    }

    if (numFIFOBytes == 0) {
        numFIFOBytes = 1;
        // puts("numFIFOBytes = 0");
    }
    if (numFIFOBytes > NUM_BYTES_IN_PICC_BLOCK) {
        numFIFOBytes = NUM_BYTES_IN_PICC_BLOCK;
        // puts("numFIFOBytes > 16");
    }

    // printf("Final numFIFOBytes: %x\n", numFIFOBytes);

    // Retrieve the data from the FIFO buffer. The data is multiple bytes long.
    // Section 8.3.1 of MFRC522 datasheet:
    // Every time we read from FIFO_DATA_REG, we read 1 byte, and the internal
    // FIFO buffer pointer is decremented automatically.
    for (int i = 0; i < numFIFOBytes; i++) {
        transceiveBuffer[i] = Rfid_readReg(PCD_FIFO_DATA_REG);
    }

    return PICC_OK;
}

static int
Rfid_performChecksum(byte* uidBuffer)
{
    byte checksum = 0x00;

    // XOR all 4 UID bytes.
    for (int i = 0; i < NUM_BYTES_IN_PICC_UID; i++) {
        checksum ^= uidBuffer[i];
    }

    // Verify that the checksum is equal to the BCC, which is the last byte in
    // the uidBuffer.
    if (checksum != uidBuffer[NUM_BYTES_IN_PICC_UID]) {
        return PICC_CHECKSUM_ERR;
    }
    return PICC_OK;
}

/////////////////// External Functions /////////////////////////
void
Rfid_init(void)
{
    // Init SPI bus.
    Spi_init(SPI_DEV_BUS1_CS0);

    // Reset regs.
    Rfid_writeReg(PCD_TX_MODE_REG, 0x00);
    Rfid_writeReg(PCD_RX_MODE_REG, 0x00);
    Rfid_writeReg(PCD_MOD_WIDTH_REG, 0x26);

    // Set timer regs.
    Rfid_writeReg(PCD_T_MODE_REG, 0x80);
    Rfid_writeReg(PCD_T_PRESCALER_REG, 0xA9);
    Rfid_writeReg(PCD_T_RELOAD_REG_H, 0x03);
    Rfid_writeReg(PCD_T_RELOAD_REG_L, 0xE8);

    // Set mode regs (and turn antenna on).
    Rfid_writeReg(PCD_TX_ASK_REG, 0x40);
    Rfid_writeReg(PCD_MODE_REG, 0x3D);
    Rfid_setBitmask(PCD_TX_CONTROL_REG, 0x03);
}

void
Rfid_shutdown(void)
{
    // TODO: Anything else...?
    Spi_shutdown();
}

int
Rfid_searchForTag(void)
{
    byte buffer[2] = { PICC_REQA_CMD, 0 };
    int numRxBits;

    // Section 9.1 of MF1 datasheet; Section 9.3.1.4 of MFRC522 datasheet
    // The PICC_REQA_CMD is 7 bits, so we need to bit-orient.
    Rfid_writeReg(PCD_BIT_FRAMING_REG, 0x07);

    int status = Rfid_transceive(buffer, 1, &numRxBits);

    if (status != PICC_OK) {
        return status;
    }

    if (buffer[0] != 0x04) {
        // puts("Not 0x04");
        return PICC_OTHER_ERR; // TODO: ERR type
    }

    if (numRxBits != 16) { // 16 *BITS*, i.e. 2 bytes.
        // puts("RESULT LEN IS WRONG!");
        return PICC_BAD_FIFO_READ_ERR; // TODO rename, maybe more of a timing
                                       // error than anything
    }

    return status;
}

int
Rfid_getTagUid(byte* buffer)
{
    int numRxBits;

    Rfid_writeReg(PCD_BIT_FRAMING_REG, 0x00);

    buffer[0] = PICC_ANTICOLLISION_CMD_A;
    buffer[1] = PICC_ANTICOLLISION_CMD_B;

    // TODO:
    // This *always* returns 40 bits exactly? It always
    // needs to call transceive only literally once? Not convinced...
    int status = Rfid_transceive(buffer, 2, &numRxBits);

    // TODO: Magic number, also maybe a new ERR type
    if (numRxBits != 40) {
        return PICC_BAD_FIFO_READ_ERR;
    }

    if (status == PICC_OK) {
        int checksumResult = Rfid_performChecksum(buffer);
        if (checksumResult != PICC_OK) {
            return PICC_CHECKSUM_ERR;
        }
    }
    return PICC_OK;
}

void
Rfid_printFirmwareVersion()
{
    byte version = Rfid_readReg(PCD_VERSION_REG);
    puts("--------------------------------");
    printf(" MFRC522 firmware version: 0x%x\n", version);
    puts("--------------------------------");
}