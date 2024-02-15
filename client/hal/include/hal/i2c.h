/// \file i2c.h
/// \brief Provides utilities for communicating with devices
///        over I2C. Currently, only writing to other devices is supported.
/// \author Spencer Leslie 301571329
#ifndef SPENNYDL_I2C_H
#define SPENNYDL_I2C_H

#include <stddef.h>
#include <stdint.h>

#define I2C_EOK 1
#define I2C_EOPEN -1
#define I2C_EIOCTL -2
#define I2C_EWRITE -3
#define I2C_ECONFIGPIN -4
#define I2C_EALLOC -5
#define I2C_EBADARG -6

/// Available I2C busses.
typedef enum
{
    I2C_BUS0 = 0,
    I2C_BUS1,
    I2C_BUS2,
    I2C_N_BUSSES
} i2c_bus_number;

/// External handle to an open I2C bus.
typedef struct
{
    void* __i2c;
} i2c_bus_handle;

/// Represents the address of a device on the I2C bus.
typedef uint8_t i2c_device_address;
/// Represents the address of a device's register.
typedef uint8_t i2c_device_register;

///
/// Opens an I2C bus.
/// \param bus The \ref i2c_bus_number of the bus to open.
/// \param handle A \ref i2c_bus_handle that will be filled out when
///        the handle is successfully opened.
/// \return bool Whether or not opening the bus succeeded. Check errno on error.
int
i2c_bus_open(i2c_bus_number bus, i2c_bus_handle* empty_handle);

///
/// Close a bus.
/// \param bus A \ref i2c_bus_handle to the bus to close.
void
i2c_bus_close(i2c_bus_handle* bus);

///
/// Select a device on the bus to write to. This needs to be called
/// before \ref i2c_write.
/// \param bus The \ref i2c_bus_handle.
/// \param device The \ref i2c_device_address of the device.
/// \return bool Whether or not selecting the device what successful.
///         Check errno on error.
int
i2c_select_device(i2c_bus_handle* bus, i2c_device_address device);

///
/// Write `n_bytes` bytes to the selected device starting at register `reg`.
/// \param bus The \ref i2c_bus_handle.
/// \param reg The \ref i2c_device_register to write to.
/// \param data Pointer to \ref n_bytes of data to write.
/// \param n_bytes Number of bytes of data to write.
/// \return bool Whether or not writing succeeded.
///         Check errno on error.
int
i2c_write(i2c_bus_handle* bus,
          i2c_device_register reg,
          const uint8_t* data,
          size_t n_bytes);

#endif // SPENNYDL_I2C_H
