/**
 * @file spi.c
 * @brief Implementation of the functions defined in @ref spi.h.
 * @author Louie Lu 30129418.
 */

#include "hal/spi.h"

static int spiFileDesc;

void
Spi_init(char* spiDevice)
{
    // Open Device
    spiFileDesc = open(spiDevice, O_RDWR);
    if (spiFileDesc < 0) {
        printf("Error: Can't open device %s\n", spiDevice);
        exit(1);
    }
    // Set port parameters
    // Set SPI mode: Necessary
    int spiMode = SPI_MODE_DEFAULT;
    int errorCheck = ioctl(spiFileDesc, SPI_IOC_WR_MODE, &spiMode);
    if (errorCheck < 0) {
        printf("Error: Set SPI mode failed\n");
        exit(1);
    }
    // Set Max Speed (Hz): Optional
    // TODO: Double check if this clock speed actually affects the readings.
    // SPI_CLK_DIV_16 and _64 have both been recommended from different sources.
    int speedHz = SPEED_HZ_DEFAULT;
    errorCheck = ioctl(spiFileDesc, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz);
    if (errorCheck < 0) {
        printf("Error: Set max speed failed\n");
        exit(1);
    }
}

void
Spi_shutdown(void)
{
    close(spiFileDesc);
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
        perror("readReg");
        exit(-1);
    }

    // Section 8.1.2.1 of the MFRC522 datasheet:
    // "MISO byte order:
    //      Byte 0: X (Do Not Care)
    //      Byte 1: Data 0"
    // In other words, ignore the first byte in rxBuf.
    return rxBuf[1];
}

void
Spi_writeReg(byte regAddr, byte value)
{
    // Prepare 2 buffers.
    // Section 8.1.2.2 of the MFRC522 datasheet:
    // txBuf contains the reg we want to access, then the value we want to
    // write. In the case of a write, rxBuf doesn't really matter. (TODO: (?))
    byte txBuf[SPI_BUFF_SIZE] = { regAddr, value };
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
        perror("writeReg");
        exit(-1);
    }
}