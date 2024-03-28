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

#include "hal/spi.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

// Terminology:
// PCD  (Proximity Coupling Device)         == RFID Reader == MFRC522
// PICC (Proximity Integrated Circuit Card) == RFID Tag    == MIFARE Classic 1K

/****************************/
/*  MFRC522 Register Codes  */
/****************************/

// These are regs that are essential to the RFID reading loop. All of these regs
// are worth fully comprehending through the datasheet.
#define PCD_COMMAND_REG 0x01
#define PCD_COM_IRQ_REG 0x04
#define PCD_ERROR_REG 0x06
#define PCD_FIFO_DATA_REG 0x09
#define PCD_FIFO_LEVEL_REG 0x0A
#define PCD_CONTROL_REG 0x0C
#define PCD_BIT_FRAMING_REG 0x0D

// These are regs we use once, then don't touch again. They are not as important
// to understand, but they still perform crucial init functions.
#define PCD_MODE_REG 0x11
#define PCD_TX_CONTROL_REG 0x14
#define PCD_TX_ASK_REG 0x15

// A reg for debugging.
#define PCD_VERSION_REG 0x37

/***************************/
/*  MFRC522 Command Codes  */
/***************************/

#define PCD_IDLE_CMD 0x00
#define PCD_TRANSCEIVE_CMD 0x0C

/****************************/
/*  MF1S503x Command Codes  */
/****************************/

#define PICC_REQA_CMD 0x26
#define PICC_ANTICOLLISION_CMD_A 0x93
#define PICC_ANTICOLLISION_CMD_B 0x20

/*****************************/
/*         Bitmasks          */
/*****************************/

#define RX_IRQ_BITMASK 0x20
#define ERROR_BITMASK 0x13
#define INVALID_BYTE_BITMASK 0x07

/*****************************/
/*     RFID Status Codes     */
/*****************************/

typedef enum
{
    RFID_OK = 0,
    RFID_RW_ERR,
    RFID_TIMEOUT_ERR,
    RFID_INVALID_BYTE_ERR,
    RFID_UNEXPECTED_RESPONSE_ERR,
    RFID_CHECKSUM_ERR,
    RFID_OTHER_ERR,
} Rfid_StatusCode;

/****************************/
/*        Num Bytes         */
/****************************/

#define NUM_BITS_IN_BYTE 8
#define NUM_BYTES_IN_PICC_UID 4

// ATQA = Answer to Request A, i.e. a tag's response to a REQA command.
// Section 9.4 of MF1 datasheet:
// The ATQA is always 2 bytes long: { 0x04, 0x00 }.
#define NUM_BYTES_IN_ATQA 2
#define PICC_ATQA 0x04

#define NUM_BYTES_IN_REQA_CMD 1
#define NUM_BYTES_IN_ANTICOLLISION_CMD 2

//////////////////// Functions ////////////////////////

/**
 * Initialize the RFID reader and its thread.
 * @return Rfid_StatusCode Whether init succeeded.
 */
Rfid_StatusCode
Rfid_init(void);

/**
 * Shutdown the RFID reader.
 * @return Rfid_StatusCode Whether shutdown succeeded.
 */
Rfid_StatusCode
Rfid_shutdown(void);

/**
 * Get the current tag's "ID" (read: not UID), which we define as the first byte
 * of the *actual* UID. For our project, just the first byte is enough to
 * distinguish our tags, and one byte is simpler to work with than an array.
 * @return byte The first byte of the UID of the tag that the module is
 * currently reading, or 0xFF if no tag is currently being read.
 */
byte
Rfid_getCurrentTagId(void);

/**
 * Print the firmware version of an MFRC522, which can be 0x91 or 0x92.
 * Mostly here for debugging purposes.
 */
void
Rfid_printFirmwareVersion(void);