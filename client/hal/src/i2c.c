#include "hal/i2c.h"
#include "hal/fileio.h"
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
static const int _i2c_bus_pins[][2] = { { 18, 17 }, { 20, 19 } };

/// Internal i2c information. This gets passed back to the user
/// hidden within the \ref i2c_bus_handle.
typedef struct
{
    int fd; ///< File descriptor representing the open bus.
    i2c_device_address selected_address; ///< Currently selected device address.
} _i2c_bus;

///
/// Allocates a new \ref _i2c_bus.
/// \return _i2c_bus* Pointer to the allocated \ref _i2c_bus. Will be NULL
///         on allocation failure.
static _i2c_bus*
_i2c_bus_alloc(void);

///
/// Configure pins for i2c.
/// \param busno The \ref i2c_bus_number to configure.
/// \return bool Whether or not configuration succeeded.
static bool
_i2c_config_pins_for_i2c(i2c_bus_number busno);

static _i2c_bus*
_i2c_bus_alloc(void)
{
    _i2c_bus* b = (_i2c_bus*)malloc(sizeof(_i2c_bus));
    if (b) {
        memset(b, 0, sizeof(_i2c_bus));
    }
    return b;
}

static bool
_i2c_config_pins_for_i2c(i2c_bus_number busno)
{
    const int* bus_pins = NULL;
    switch (busno) {
        case I2C_BUS1:
            bus_pins = _i2c_bus_pins[0];
            break;
        case I2C_BUS2:
            bus_pins = _i2c_bus_pins[1];
            break;
        default:
            return false;
            break;
    }

    for (int i = 0; i < 2; i++) {
        char command[FILEIO_SMALL_BUFSIZE];
        snprintf(command, FILEIO_SMALL_BUFSIZE, I2C_CONFIGPIN_FMT, bus_pins[i]);
        int exit_code = fileio_pipe_command(command);
        if (exit_code != 0) {
            return I2C_ECONFIGPIN;
        }
    }
    return I2C_EOK;
}

int
i2c_bus_open(i2c_bus_number busno, i2c_bus_handle* handle_out)
{
    if (busno < 0 || busno >= I2C_N_BUSSES || handle_out == NULL) {
        return I2C_EBADARG;
    }

    if (!_i2c_config_pins_for_i2c(busno)) {
        return false;
    }
    char i2c_bus_file[FILEIO_SMALL_BUFSIZE];
    snprintf(i2c_bus_file, FILEIO_SMALL_BUFSIZE, I2C_BUS_FMT, busno);

    int fd = open(i2c_bus_file, O_RDWR);
    if (fd < 0) {
        return I2C_EOPEN;
    }

    _i2c_bus* bus = _i2c_bus_alloc();
    if (!bus) {
        close(fd);
        return I2C_EALLOC;
    }
    bus->fd = fd;
    bus->selected_address = I2C_NO_ADDRESS;

    handle_out->__i2c = bus;
    return I2C_EOK;
}

void
i2c_bus_close(i2c_bus_handle* handle)
{
    if (handle != NULL) {
        _i2c_bus* bus = handle->__i2c;
        close(bus->fd);
        free(bus);
        handle->__i2c = NULL;
    }
}

int
i2c_select_device(i2c_bus_handle* handle, i2c_device_address device_addr)
{
    if (handle == NULL) {
        return I2C_EBADARG;
    }
    _i2c_bus* bus = handle->__i2c;

    if (ioctl(bus->fd, I2C_SLAVE, device_addr) < 0) {
        return I2C_EIOCTL;
    }
    bus->selected_address = device_addr;
    return I2C_EOK;
}

int
i2c_write(i2c_bus_handle* handle,
          i2c_device_register reg,
          const uint8_t* data,
          size_t bytes)
{
    if (handle == NULL) {
        return I2C_EBADARG;
    }
    _i2c_bus* bus = handle->__i2c;

    // Write the address plus all of the bytes.
    // I2C registers are contiguous, so we can write
    // to one and continue writing bytes.
    size_t total_bytes = bytes + 1;
    uint8_t buf[total_bytes];
    buf[0] = reg;
    memcpy(buf + 1, data, bytes);

    if (write(bus->fd, buf, total_bytes) < 0) {
        return I2C_EWRITE;
    }
    return I2C_EOK;
}
