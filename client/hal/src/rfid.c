/**
 * @file rfid.c
 * @brief Implementation of the functions defined in @ref rfid.h.
 * @author Louie Lu 30129418.
 */

#include "hal/rfid.h"

// NOTE: miguelbalboa's implementation gives more thought to this deadline
// value, factoring in calculations from the datasheets and setting appropriate
// values to timing registers. Our pared implementation does none of this, so
// this value is basically arbitrary. We'll use theirs for good luck.
#define TRANSCEIVE_DEADLINE 35

// Flexible. Works well anywhere from 1 - 100.
#define RFID_POLL_MS 100

#define RFID_NO_TAG_ID 0xFF

static atomic_uchar currentTagId;
static byte tagDataBuffer[NUM_BYTES_IN_PICC_UID + 1]; // + 1 for BCC
static Rfid_StatusCode rfidStatus;

static pthread_t rfid_thread;
static bool init;
static bool run;

////////////////////// Function Prototypes /////////////////////////

/**
 * Format a register address according to Section 8.1.2.3 of MFRC522
 * datasheet.
 *
 * "When accessing a register address on the MFRC522, an address byte must
 * meet the following format:
 *      - The MSB defines the mode used (1 = read, 0 = write).
 *      - Bits 6 to 1 define the address.
 *      - The LSB is always set to 0."
 *
 * @param regAddr The register address to access.
 * @param mode Specifies read ('r') or write ('w').
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
 * @return Rfid_StatusCode Whether the write succeeded.
 */
static Rfid_StatusCode
Rfid_writeReg(byte regAddr, byte value);

/**
 * Set a bitmask. This is an alternate method of writing a register. Useful when
 * you want to keep a register's contents mostly the same, but want to flip just
 * a few certain bits as specified by the bitmask.
 * @param regAddr The register address to write.
 * @param bitmask The bitmask to set.
 * @return Rfid_StatusCode Whether the write succeeded.
 */
static Rfid_StatusCode
Rfid_setBitmask(byte regAddr, byte bitmask);

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
 * @return Rfid_StatusCode Whether the transceive was error-free.
 */
static Rfid_StatusCode
Rfid_transceive(byte* transceiveBuffer, int bufferLen, int* numRxBits);

//  * ====================   (1)   =================   (2)   ==============
//  * |    C Program     | ------> | PCD (MFRC522) | ------> | PICC (MF1) |
//  * | transceiveBuffer | <------ |  FIFO buffer  | <------ |    UID     |
//  * ====================   (4)   =================   (3)   ==============

/**
 * Search for a PICC by sending a single REQA command from the PCD. This
 * function should be called repeatedly, as each call sends a new probing wave.
 * @return Rfid_StatusCode Whether a tag has been found.
 */
static Rfid_StatusCode
Rfid_searchForTag(void);

/**
 * Retrieve the UID of a PICC following a successful REQA command by sending an
 * ANTICOLLISION command from the PCD.
 * @param buffer The buffer to store the 4-byte UID in.
 * @return Rfid_StatusCode Whether a UID has been retrieved.
 */
static Rfid_StatusCode
Rfid_getTagUid(byte* buffer);

/**
 * Perform a checksum according to Section 2 of AN10927 MIFARE Document.
 * We employ this checksum as a final check to verify a UID has been transmitted
 * correctly (i.e. accounting for potential noise).
 *
 * @param uidBuffer An array of 5 bytes obtained from a PICC_ANTICOLLISION
 * command. The first 4 bytes comprise the UID. The 5th byte is the BCC (Block
 * Check Character). The BCC was "hard-coded" during manufacturing, and was
 * calculated as the XOR over the intended UID's 4 bytes.
 * @return Rfid_StatusCode Whether the checksum matches the BCC.
 */
static Rfid_StatusCode
Rfid_performChecksum(byte* uidBuffer);

/**
 * A threaded function that updates the current tag ID by repeatedly running
 * Rfid_searchForTag() and Rfid_getTagUid(). If there is no tag in the RF field,
 * the current tag ID is set to 0xFF.
 */
static void*
_updateCurrentTagId(void* args);

////////////////////// Prototype Implementations ////////////////////////////
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

static Rfid_StatusCode
Rfid_writeReg(byte regAddr, byte value)
{
    byte formattedRegAddr = Rfid_formatRegAddr(regAddr, 'w');
    int result = Spi_writeReg(formattedRegAddr, value);
    if (result != SPI_OK) {
        return RFID_RW_ERR;
    }
    return RFID_OK;
}

static Rfid_StatusCode
Rfid_setBitmask(byte regAddr, byte bitmask)
{
    byte current = Rfid_readReg(regAddr);
    int result = Rfid_writeReg(regAddr, current | bitmask);
    if (result != SPI_OK) {
        return RFID_RW_ERR;
    }
    return RFID_OK;
}

static Rfid_StatusCode
Rfid_transceive(byte* transceiveBuffer, int bufferLen, int* numRxBits)
{
    // Reset regs that may have been set by a previous transceive call.
    Rfid_writeReg(PCD_COMMAND_REG, PCD_IDLE_CMD); // Cancel any active command.
    Rfid_writeReg(PCD_COM_IRQ_REG, 0x7F);         // Clear all IRQ bits.
    Rfid_writeReg(PCD_FIFO_LEVEL_REG, 0x80);      // Flush FIFO buffer.

    // Write the PICC command (already stored by the user in transceiveBuffer)
    // into the PCD's FIFO buffer. NOTE: Some PICC commands, such as
    // PICC_ANTICOLLISION, are comprised of two bytes - hence the for loop.
    for (int i = 0; i < bufferLen; i++) {
        Rfid_writeReg(PCD_FIFO_DATA_REG, transceiveBuffer[i]);
    }

    // Execute the TRANSCEIVE command.
    Rfid_writeReg(PCD_COMMAND_REG, PCD_TRANSCEIVE_CMD);

    // Section 10.3.1.8 of MFRC522 datasheet:
    // TRANSCEIVE command requires one more special step to *actually* start.
    // Set MSB of BIT_FRAMING_REG ("StartSend" bit) to 1 == begin transceiving.
    Rfid_setBitmask(PCD_BIT_FRAMING_REG, 0x80);

    // Section 9.3.1.5 of MFRC522 datasheet:
    // The 5th bit in COM_IRQ_REG changes to a 1 when the receiver has detected
    // the end of a valid data stream (i.e. a tag's response).

    // So we set a generous deadline of 35ms for a tag to receive and respond
    // to our command. During these 35ms, we repeatedly check if COM_IRQ_REG's
    // 5th bit has been set to 1 using the RX_IRQ_BITMASK 0x20 (0010 0000).
    long long deadline = Timeutils_getTimeInMs() + TRANSCEIVE_DEADLINE;

    bool rxValidData = false;
    do {
        byte comIrqRegValue = Rfid_readReg(PCD_COM_IRQ_REG);
        if (comIrqRegValue & RX_IRQ_BITMASK) {
            rxValidData = true;
            break;
        }
    } while (Timeutils_getTimeInMs() < deadline);

    // Checking the results after the deadline is up.
    // We perform 3 error checks:

    // (1) Timed out, i.e. no tag in range. (This is a regular occurrence.)
    if (!rxValidData) {
        return RFID_TIMEOUT_ERR;
    }

    // (2) Errors detected from the RFID's ERROR_REG. The ERROR_BITMASK 0x13
    // captures a few different error types, ranging from FIFO buffer overflow
    // to internal error checking failures. These errors happen rarely.
    byte errorRegValue = Rfid_readReg(PCD_ERROR_REG);
    if (errorRegValue & ERROR_BITMASK) {
        return RFID_OTHER_ERR;
    }

    // (3) An incomplete or noisy transmission occurred, and the data stream is
    // incomplete. You can recreate this error with a decent chance of success
    // if you bring the PICC to, and away from, the PCD rapidly and repeatedly.
    // This type of error is monitored and captured in the CONTROL_REG.

    // Section 9.3.1.13 of MFRC522 datasheet: Bits [2:0] of the CONTROL_REG
    // indicate the number of valid bits in the last received byte. If this
    // value is 000, the whole byte is valid. If not, the data is incomplete.
    bool invalidByte = Rfid_readReg(PCD_CONTROL_REG) & INVALID_BYTE_BITMASK;
    if (invalidByte) {
        return RFID_INVALID_BYTE_ERR;
    }

    // If we pass our (3) error checks, we are free to read the received data.

    // Determine how many bytes are in the FIFO buffer by reading
    // the FIFO_LEVEL_REG, and set the numRxBits that the user passed in.
    byte numFIFOBytes = Rfid_readReg(PCD_FIFO_LEVEL_REG);
    *numRxBits = numFIFOBytes * NUM_BITS_IN_BYTE;

    // Retrieve the data from the FIFO buffer, which is multiple bytes long.
    // Section 8.3.1 of MFRC522 datasheet: Every time we read from
    // FIFO_DATA_REG, we read 1 byte, and the internal FIFO buffer pointer
    // is decremented automatically. In other words, we don't have to worry
    // about manually accessing different parts of the FIFO buffer.
    for (int i = 0; i < numFIFOBytes; i++) {
        transceiveBuffer[i] = Rfid_readReg(PCD_FIFO_DATA_REG);
    }

    return RFID_OK;
}

static Rfid_StatusCode
Rfid_searchForTag(void)
{
    assert(init);

    int numRxBits;
    byte buffer[NUM_BYTES_IN_ATQA] = { PICC_REQA_CMD, 0 };

    // Section 9.1 of MF1 datasheet &
    // Section 9.3.1.4 of MFRC522 datasheet
    // The PICC_REQA_CMD is meant to be 7 bits, so we need to bit-orient.
    Rfid_writeReg(PCD_BIT_FRAMING_REG, 0x07);

    // Execute TRANSCEIVE with REQA
    int status = Rfid_transceive(buffer, NUM_BYTES_IN_REQA_CMD, &numRxBits);

    if (status != RFID_OK) {
        return status;
    }

    // Validate the tag's response
    // (1) Check the length
    if (numRxBits != NUM_BYTES_IN_ATQA * NUM_BITS_IN_BYTE) {
        return RFID_UNEXPECTED_RESPONSE_ERR;
    }

    // (2) Check the content
    if (buffer[0] != PICC_ATQA) {
        return RFID_UNEXPECTED_RESPONSE_ERR;
    }

    return RFID_OK;
}

static Rfid_StatusCode
Rfid_getTagUid(byte* buffer)
{
    assert(init);

    int numRxBits;
    buffer[0] = PICC_ANTICOLLISION_CMD_A;
    buffer[1] = PICC_ANTICOLLISION_CMD_B;

    Rfid_writeReg(PCD_BIT_FRAMING_REG, 0x00);

    // Execute TRANSCEIVE with ANTICOLLISION
    int status =
      Rfid_transceive(buffer, NUM_BYTES_IN_ANTICOLLISION_CMD, &numRxBits);

    if (status != RFID_OK) {
        return status;
    }

    // Validate the tag's response
    // (1) Check the length
    if (numRxBits !=
        (NUM_BYTES_IN_PICC_UID + 1) * NUM_BITS_IN_BYTE) { // + 1 for the BCC
        return RFID_UNEXPECTED_RESPONSE_ERR;
    }

    // (2) Check the content
    Rfid_StatusCode checksumResult = Rfid_performChecksum(buffer);
    return checksumResult;
}

static Rfid_StatusCode
Rfid_performChecksum(byte* uidBuffer)
{
    byte checksum = 0x00;

    // XOR over all 4 UID bytes.
    for (int i = 0; i < NUM_BYTES_IN_PICC_UID; i++) {
        checksum ^= uidBuffer[i];
    }

    // Verify that the checksum is equal to the BCC, which is the last byte
    // in the uidBuffer.
    if (checksum != uidBuffer[NUM_BYTES_IN_PICC_UID]) {
        return RFID_CHECKSUM_ERR;
    }

    return RFID_OK;
}

static void*
_updateCurrentTagId(void* args)
{
    (void)args;

    // Through tests, I have found that a tag ID of 0xFF is sometimes returned
    // due to RFID_TIMEOUT_ERR, even when the tag is permanently sitting on top
    // of the reader. I don't believe this is caused by unintended noise, as the
    // 0xFF is returned in a predictable, wavelike manner. I am unable to
    // determine the cause, though.

    // To combat this problem, I employ the N samples past threshold strategy,
    // looking for 3 consecutive 0xFFs before conceding that there is no tag.
    long long countTimeout = 0;
    const int N = 3;

    while (run) {
        rfidStatus = Rfid_searchForTag();

        if (rfidStatus != RFID_OK) {
            countTimeout += 1;
            if (countTimeout > N) {
                currentTagId = RFID_NO_TAG_ID;
            }
        }

        else {
            rfidStatus = Rfid_getTagUid(tagDataBuffer);
            if (rfidStatus != RFID_OK) {
                countTimeout += 1;
                if (countTimeout > N) {
                    currentTagId = RFID_NO_TAG_ID;
                }
            } else {
                currentTagId = tagDataBuffer[0];
                countTimeout = 0; // Reset
            }
        }

        Timeutils_sleepForMs(RFID_POLL_MS);
    }
    return NULL;
}

/////////////////////// External Functions /////////////////////////
Rfid_StatusCode
Rfid_init(void)
{
    // Init SPI bus.
    Spi_init(SPI_DEV_BUS1_CS0);

    // Set RFID behaviour, then activate it.
    Rfid_writeReg(PCD_MODE_REG, 0x20);         // TxWaitRF = True
    Rfid_writeReg(PCD_TX_ASK_REG, 0x40);       // Force 100% ASK.
    Rfid_setBitmask(PCD_TX_CONTROL_REG, 0x03); // Turn antenna on.

    // Set internal variables.
    currentTagId = 0xFF;
    init = true;
    run = true;

    // Begin the RFID reader thread.
    pthread_create(&rfid_thread, NULL, _updateCurrentTagId, NULL);

    return RFID_OK;
}

Rfid_StatusCode
Rfid_shutdown(void)
{
    run = false;
    pthread_join(rfid_thread, NULL);

    int result = Spi_shutdown();
    if (result != SPI_OK) {
        return RFID_RW_ERR;
    }
    return RFID_OK;
}

byte
Rfid_getCurrentTagId(void)
{
    assert(init);
    return currentTagId;
}

void
Rfid_printFirmwareVersion(void)
{
    assert(init);

    byte version = Rfid_readReg(PCD_VERSION_REG);
    puts("--------------------------------");
    printf(" MFRC522 firmware version: 0x%x\n", version);
    puts("--------------------------------");
}
