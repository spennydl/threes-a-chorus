#pragma once
/// \file i2c.h
/// \brief Provides utilities for communicating with devices
///        over I2C. Currently, only writing to other devices is supported.
/// \author Spencer Leslie 301571329

#include <stddef.h>
#include <stdint.h>

#define I2C_EOK 1
#define I2C_EOPEN -1
#define I2C_EIOCTL -2
#define I2C_EWRITE -3
#define I2C_ECONFIGPIN -4
#define I2C_EALLOC -5
#define I2C_EBADARG -6
#define I2C_EREAD -7

/// Available I2C busses.
typedef enum
{
    I2C_BUS0 = 0,
    I2C_BUS1,
    I2C_BUS2,
    I2C_N_BUSSES
} I2C_Bus;

/// External handle to an open I2C bus.
typedef struct
{
    void* __i2c;
} I2C_BusHandle;

/// Represents the address of a device on the I2C bus.
typedef uint8_t I2C_DeviceAddress;
/// Represents the address of a device's register.
typedef uint8_t I2C_RegisterAddress;

///
/// Opens an I2C bus.
/// \param bus The \ref I2C_Bus of the bus to open.
/// \param handle A \ref I2C_BusHandle that will be filled out when
///        the handle is successfully opened.
/// \return bool Whether or not opening the bus succeeded. Check errno on error.
int
I2C_openBus(I2C_Bus bus, I2C_BusHandle* outHandle);

///
/// Close a bus.
/// \param bus A \ref I2C_BusHandle to the bus to close.
void
I2C_closeBus(I2C_BusHandle* bus);

///
/// Select a device on the bus to write to. This needs to be called
/// before \ref i2c_write.
/// \param bus The \ref I2C_BusHandle.
/// \param device The \ref I2C_DeviceAddress of the device.
/// \return bool Whether or not selecting the device what successful.
///         Check errno on error.
int
I2C_selectDevice(I2C_BusHandle* bus, I2C_DeviceAddress device);

///
/// Write `n_bytes` bytes to the selected device starting at register `reg`.
/// \param bus The \ref I2C_BusHandle.
/// \param reg The \ref I2C_RegisterAddress to write to.
/// \param data Pointer to \ref n_bytes of data to write.
/// \param n_bytes Number of bytes of data to write.
/// \return bool Whether or not writing succeeded.
///         Check errno on error.
int
I2C_write(I2C_BusHandle* bus,
          I2C_RegisterAddress reg,
          const uint8_t* data,
          size_t nBytes);

int
I2C_read(I2C_BusHandle* bus,
         I2C_RegisterAddress reg,
         uint8_t* data,
         size_t n_bytes);
