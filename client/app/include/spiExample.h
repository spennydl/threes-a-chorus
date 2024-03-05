#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define SPI_DEV_BUS1_CS0 "/dev/spidev1.0"

#define SPI_MODE_DEFAULT 0
#define SPEED_HZ_DEFAULT 4000000

int
SPI_initPort(char* spiDevice)
{
    // Open Device
    int spiFileDesc = open(spiDevice, O_RDWR);
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
    int speedHz = SPEED_HZ_DEFAULT;
    errorCheck = ioctl(spiFileDesc, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz);
    if (errorCheck < 0) {
        printf("Error: Set max speed failed\n");
        exit(1);
    }
    return spiFileDesc;
}

int
SPI_example(void)
{
    int spifd = SPI_initPort(SPI_DEV_BUS1_CS0);

    uint8_t txBuf[] = { (0x37 << 1) | 0x80, 0 };
    uint8_t rxBuf[] = { 0, 0 };

    struct spi_ioc_transfer spiTransaction;
    memset(&spiTransaction, 0, sizeof(struct spi_ioc_transfer));

    spiTransaction.tx_buf = (unsigned long)&txBuf;
    spiTransaction.rx_buf = (unsigned long)&rxBuf;
    spiTransaction.len = 2;

    printf("Reading from version register with formatted address %x...\n",
           txBuf[0]);
    printf("We expect the version to be either 0x91 or 0x92\n");
    printf(".....\n");

    int status = ioctl(spifd, SPI_IOC_MESSAGE(1), &spiTransaction);
    if (status < 0) {
        printf("Error: SPI Transfer failed\n");
        perror("SPI error");
    } else {
        printf("Received version: %x\n", rxBuf[1]);
    }
    return 0;
}
