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

#include "hal/timeutils.h"

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

/**
 * Initialize a SPI bus.
 * @param spiDevice The pathname of the SPI bus to initialize.
 * @return TODO: Something.
 */
void
Spi_init(char* spiDevice);

/**
 * Shutdown the SPI bus.
 * @return TODO: Something.
 */
void
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
 * @return TODO: Something.
 */
void
Spi_writeReg(byte regAddr, byte value);