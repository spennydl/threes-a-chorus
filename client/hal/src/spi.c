/**
 * @file spi.c
 * @brief Implementation of the functions defined in @ref spi.h.
 * @author Louie Lu 30129418.
 */

#include "hal/spi.h"

static int spiFileDesc;

Spi_StatusCode
Spi_init(char* spiDevice)
{
    // Open device
    spiFileDesc = open(spiDevice, O_RDWR);
    if (spiFileDesc < 0) {
        perror("Spi_init open");
        exit(SPI_ERR);
    }
    // Set port parameters: necessary
    int spiMode = SPI_MODE_DEFAULT;
    int errorCheck = ioctl(spiFileDesc, SPI_IOC_WR_MODE, &spiMode);
    if (errorCheck < 0) {
        perror("Spi_init set mode");
        exit(SPI_ERR);
    }
    // Set max speed (hz): optional
    int speedHz = SPEED_HZ_DEFAULT;
    errorCheck = ioctl(spiFileDesc, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz);
    if (errorCheck < 0) {
        perror("Spi_init set speed");
        exit(SPI_ERR);
    }

    return SPI_OK;
}

Spi_StatusCode
Spi_shutdown(void)
{
    int result = close(spiFileDesc);
    if (result < 0) {
        return SPI_ERR;
    }
    return SPI_OK;
}

byte
Spi_readReg(byte regAddr)
{
    // Prepare 2 buffers.

    // txBuf contains the reg we want to access.
    // Following a successful read, rxBuf will contain the reg content.
    byte txBuf[SPI_BUFF_SIZE] = { regAddr, 0 };
    byte rxBuf[SPI_BUFF_SIZE] = { 0, 0 };

    // Initialize an SPI struct.
    struct spi_ioc_transfer spiTransaction;
    memset(&spiTransaction, 0, sizeof(struct spi_ioc_transfer));

    // Set the SPI struct fields, passing in our created buffers.
    spiTransaction.tx_buf = (unsigned long)txBuf;
    spiTransaction.rx_buf = (unsigned long)rxBuf;
    spiTransaction.len = SPI_BUFF_SIZE;

    // Transmit to the register.
    int status =
      ioctl(spiFileDesc, SPI_IOC_MESSAGE(SPI_NUM_TRANSFERS), &spiTransaction);
    if (status < 0) {
        perror("Spi_readReg");
    }

    // Section 8.1.2.1 of the MFRC522 datasheet:
    // "MISO byte order:
    //      Byte 0: X (Do Not Care)
    //      Byte 1: Data 0"
    // In other words, ignore the first byte in rxBuf.
    return rxBuf[1];
}

Spi_StatusCode
Spi_writeReg(byte regAddr, byte value)
{
    // Section 8.1.2.2 of the MFRC522 datasheet:
    // txBuf[0] contains the reg we want to access. After come the value(s) we
    // want to write. In the case of a write, we don't need an rxBuf.
    byte txBuf[SPI_BUFF_SIZE] = { regAddr, value };

    // Initialize an SPI struct.
    struct spi_ioc_transfer spiTransaction;
    memset(&spiTransaction, 0, sizeof(struct spi_ioc_transfer));

    // Set the SPI struct fields, passing in our created buffer.
    spiTransaction.tx_buf = (unsigned long)txBuf;
    spiTransaction.len = SPI_BUFF_SIZE;

    // Transmit to the register.
    int status =
      ioctl(spiFileDesc, SPI_IOC_MESSAGE(SPI_NUM_TRANSFERS), &spiTransaction);
    if (status < 0) {
        perror("Spi_writeReg");
        return SPI_ERR;
    }

    return SPI_OK;
}