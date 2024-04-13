/**
 * @file i2c.c
 * @brief Implementation of i2c communication module.
 * @author Spencer Leslie 301571329
 */
#include "hal/i2c.h"
#include "com/fileio.h"
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/// Format string for files representing an i2c bus
#define I2C_BUS_FMT "/dev/i2c-%d"
/// Format string for config-pin command.
#define I2C_CONFIGPIN_FMT "config-pin P9_%d i2c"
/// Special value representing no selected device address.
#define I2C_NO_ADDRESS 128

/// Pins of available busses.
static const int _I2C_busPins[][2] = { { 18, 17 }, { 20, 19 } };

/// Internal i2c information. This gets passed back to the user
/// hidden within the \ref I2C_BusHandle.
typedef struct
{
    int fd; ///< File descriptor representing the open bus.
    I2C_DeviceAddress selectedAddress; ///< Currently selected device address.
} _I2C_Bus;

///
/// Allocates a new \ref _I2C_Bus.
/// \return _I2C_Bus* Pointer to the allocated \ref _I2C_Bus. Will be NULL
///         on allocation failure.
static _I2C_Bus*
_I2C_busAlloc(void);

///
/// Configure pins for i2c.
/// \param busno The \ref I2C_Bus to configure.
/// \return bool Whether or not configuration succeeded.
static bool
_I2C_configPinsForI2c(I2C_Bus busno);

static _I2C_Bus*
_I2C_busAlloc(void)
{
    _I2C_Bus* b = (_I2C_Bus*)malloc(sizeof(_I2C_Bus));
    if (b) {
        memset(b, 0, sizeof(_I2C_Bus));
    }
    return b;
}

static bool
_I2C_configPinsForI2c(I2C_Bus busno)
{
    const int* busPins = NULL;
    switch (busno) {
        case I2C_BUS1:
            busPins = _I2C_busPins[0];
            break;
        case I2C_BUS2:
            busPins = _I2C_busPins[1];
            break;
        default:
            return false;
            break;
    }

    for (int i = 0; i < 2; i++) {
        char command[FILEIO_SMALL_BUFSIZE];
        snprintf(command, FILEIO_SMALL_BUFSIZE, I2C_CONFIGPIN_FMT, busPins[i]);
        int exitCode = fileio_pipe_command(command);
        if (exitCode != 0) {
            return I2C_ECONFIGPIN;
        }
    }
    return I2C_EOK;
}

int
I2C_openBus(I2C_Bus busno, I2C_BusHandle* outHandle)
{
    if (busno < 0 || busno >= I2C_N_BUSSES || outHandle == NULL) {
        return I2C_EBADARG;
    }

    if (!_I2C_configPinsForI2c(busno)) {
        return false;
    }
    char i2cBusFile[FILEIO_SMALL_BUFSIZE];
    snprintf(i2cBusFile, FILEIO_SMALL_BUFSIZE, I2C_BUS_FMT, busno);

    int fd = open(i2cBusFile, O_RDWR);
    if (fd < 0) {
        return I2C_EOPEN;
    }

    _I2C_Bus* bus = _I2C_busAlloc();
    if (!bus) {
        close(fd);
        return I2C_EALLOC;
    }
    bus->fd = fd;
    bus->selectedAddress = I2C_NO_ADDRESS;

    outHandle->__i2c = bus;
    return I2C_EOK;
}

void
I2C_closeBus(I2C_BusHandle* handle)
{
    if (handle != NULL) {
        _I2C_Bus* bus = handle->__i2c;
        close(bus->fd);
        free(bus);
        handle->__i2c = NULL;
    }
}

int
I2C_selectDevice(I2C_BusHandle* handle, I2C_DeviceAddress device_addr)
{
    if (handle == NULL) {
        return I2C_EBADARG;
    }
    _I2C_Bus* bus = handle->__i2c;

    if (ioctl(bus->fd, I2C_SLAVE, device_addr) < 0) {
        return I2C_EIOCTL;
    }
    bus->selectedAddress = device_addr;
    return I2C_EOK;
}

int
I2C_write(I2C_BusHandle* handle,
          I2C_RegisterAddress reg,
          const uint8_t* data,
          size_t bytes)
{
    if (handle == NULL) {
        return I2C_EBADARG;
    }
    _I2C_Bus* bus = handle->__i2c;

    // Write the address plus all of the bytes.
    // I2C registers are contiguous, so we can write
    // to one and continue writing bytes.
    size_t totalBytes = bytes + 1;
    uint8_t buf[totalBytes];
    buf[0] = reg;
    memcpy(buf + 1, data, bytes);

    if (write(bus->fd, buf, totalBytes) < 0) {
        return I2C_EWRITE;
    }
    return I2C_EOK;
}

int
I2C_read(I2C_BusHandle* handle,
         I2C_RegisterAddress reg,
         uint8_t* data,
         size_t nBytes)
{
    if (handle == NULL) {
        return I2C_EBADARG;
    }
    _I2C_Bus* bus = handle->__i2c;

    if (write(bus->fd, &reg, 1) < 0) {
        return I2C_EWRITE;
    }

    if (read(bus->fd, data, nBytes) < 0) {
        return I2C_EREAD;
    }

    return I2C_EOK;
}
