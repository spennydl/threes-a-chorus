/**
 * @file rfid.h
 * @brief Read the UID of a MIFARE Classic 1K tag from an MFRC522 reader.
 * @author Louie Lu 301291418.
 *
 * ////////////////////// Datasheets /////////////////////////
 *
 * MFRC522 RFID reader
 * http://nxp.com/docs/en/data-sheet/MFRC522.pdf
 *
 * MIFARE Classic 1K RFID tag
 * http://mouser.com/datasheet/2/302/MF1S503x-89574.pdf
 *
 * ISO/IEC 14443, the underlying technology that MIFARE uses
 * http://emutag.com/iso/14443-2.pdf (Part 2)
 * http://wg8.de/wg8n1496_17n3613_Ballot_FCD14443-3.pdf (Part 3)
 *
 * AN10927, a document on MIFARE UIDs
 * http://nxp.com/docs/en/application-note/AN10927.pdf
 *
 * /////////////////////// References ///////////////////////////
 *
 * miguelbalboa's authoritative Arduino library for the MFRC522.
 * http://github.com/miguelbalboa/rfid
 *
 */

#pragma once

// Note:
// PCD  (Proximity Coupling Device)         == RFID Reader == MFRC522
// PICC (Proximity Integrated Circuit Card) == RFID Tag    == MIFARE Classic 1K

#define NUM_BYTES_IN_PICC_BLOCK 16
#define NUM_BYTES_IN_PICC_UID 4

/**********************/
/* Register Hex Codes */
/**********************/

// These are regs that are integral to the RFID reading loop.
// All of these regs are worth fully comprehending through the datasheet.
#define PCD_COMMAND_REG 0x01
#define PCD_COM_IRQ_REG 0x04
#define PCD_ERROR_REG 0x06
#define PCD_FIFO_DATA_REG 0x09
#define PCD_FIFO_LEVEL_REG 0x0A
#define PCD_CONTROL_REG 0x0C
#define PCD_BIT_FRAMING_REG 0x0D
#define PCD_VERSION_REG 0x37

// These are regs we initialize once, then don't touch again.
// It is okay, and expected, to not understand these regs.
#define PCD_MODE_REG 0x11
#define PCD_TX_MODE_REG 0x12
#define PCD_RX_MODE_REG 0x13
#define PCD_TX_CONTROL_REG 0x14
#define PCD_TX_ASK_REG 0x15
#define PCD_MOD_WIDTH_REG 0x24

// These are regs related to frame timings.
// They fall in a similar category to the init regs.
#define PCD_T_MODE_REG 0x2A
#define PCD_T_PRESCALER_REG 0x2B
#define PCD_T_RELOAD_REG_H 0x2C
#define PCD_T_RELOAD_REG_L 0x2D

/*
 * Command Codes
 * These are specific codes we send to issue a command.
 */

// PCD | RFID Reader | MFRC522
#define PCD_IDLE_CMD 0x00
#define PCD_TRANSCEIVE_CMD 0x0C
#define PCD_MF_AUTHENT_CMD 0x0E

// PICC | RFID Tag | MF1S503x
#define PICC_REQA_CMD 0x26
#define PICC_ANTICOLLISION_CMD_A 0x93
#define PICC_ANTICOLLISION_CMD_B 0x20

/*
 * Error Codes
 */
// TODO: Rename these to RFID errors (not PICC exclusive).
#define PICC_OK 0
#define PICC_NO_TAG_ERR 1
#define PICC_BAD_FIFO_READ_ERR 2
#define PICC_CHECKSUM_ERR 3
#define PICC_OTHER_ERR 4

#include "hal/spi.h"

/**
 * Initialize the RFID reader.
 * @return TODO:
 */
void
Rfid_init(void);

/**
 * Shutdown the RFID reader.
 * @return TODO:
 */
void
Rfid_shutdown(void);

/**
 * Search for a PICC by sending a REQA command from the PCD.
 * @return int A PICC status code: whether a tag has been found.
 */
int
Rfid_searchForTag(void);

/**
 * Retrieve the UID of a PICC following a successful REQA command.
 * @param buffer The buffer to store the UID in.
 * @return int A PICC status code.
 */
int
Rfid_getTagUid(byte* buffer);

/**
 * Get the current tag's ID, which we define as the first byte of the UID.
 * For our project, merely the first byte is enough to distinguish our tags.
 * @return byte The first byte of the UID of the tag that the module is
 * currently reading, or 0xFF if no tag is currently being read.
 */
byte
Rfid_getCurrentTagId();

/**
 * Print the firmware version of an MFRC522, which can be 0x91 or 0x92.
 * Mostly here for debugging purposes.
 */
void
Rfid_printFirmwareVersion(void);
