/**
 * @file spi.h
 * @brief Provides control for BBG's SPI bus.
 * @author Louie Lu 301291418.
 *
 * Reference:
 * SPI Guide for Linux C on BeagleBone Green
 * http://opencoursehub.cs.sfu.ca/bfraser/grav-cms/cmpt433/links/files/2022-student-howtos-ensc351/SPI-On-BBG.pdf
 */

#pragma once

#include "com/timeutils.h"

#include <assert.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SPI_DEV_BUS1_CS0 "/dev/spidev1.0"
#define SPI_MODE_DEFAULT 0
#define SPEED_HZ_DEFAULT 4000000

#define SPI_BUFF_SIZE 2
#define SPI_NUM_TRANSFERS 1

typedef uint8_t byte;

typedef enum
{
    SPI_OK = 0,
    SPI_ERR,
} Spi_StatusCode;

/**
 * Initialize a SPI bus.
 * @param spiDevice The pathname of the SPI bus to initialize.
 * @return Spi_StatusCode Whether init succeeded.
 */
Spi_StatusCode
Spi_init(char* spiDevice);

/**
 * Shutdown the SPI bus.
 * @return Spi_StatusCode Whether shutdown succeeded.
 */
Spi_StatusCode
Spi_shutdown(void);

/**
 * Read and return a SPI device's register content.
 * @param regAddr The register address to read.
 * @return byte The register's content.
 */
byte
Spi_readReg(byte regAddr);

/**
 * Write a value to a SPI device's register.
 * @param regAddr The register address to write.
 * @param value The value to write.
 * @return Spi_StatusCode Whether the write succeeded.
 */
Spi_StatusCode
Spi_writeReg(byte regAddr, byte value);
