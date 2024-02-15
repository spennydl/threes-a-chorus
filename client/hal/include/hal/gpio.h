/**
 * @file gpio.h
 * @brief Interface to the GPIO pins.
 *
 * Allows configuring and opening a GPIO pin for reading either immediately or
 * in edge-triggered mode.
 *
 * @author Spencer Leslie 301571329
 */
#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include <stddef.h>

/** GPIO pin direction. Input or output. */
typedef enum gpio_pin_direction
{
    GPIO_IN,
    GPIO_OUT
} gpio_pin_direction;

/**
 * @brief GPIO error flags.
 *
 * Errors encountered by this module will almost always be file IO errors
 * and errno will thus be set when any of these errors occur. Callers to this
 * module should check errno when errors occur. These errors are included and
 * set to provide some indication of where in this module the error occurred.
 */
#define GPIO_EOK 1
#define GPIO_EOPEN -1
#define GPIO_EWRITE -2
#define GPIO_EDIR -3
#define GPIO_ECONFIG -4
#define GPIO_EREAD -5
#define GPIO_ENODATA -6
#define GPIO_ESEEK -7
#define GPIO_ECTL -8
#define GPIO_EWAIT -9

/** Externally Facing GPIO handle */
typedef struct
{
    unsigned char linux_pin_number;
    unsigned char bbg_header;
    unsigned char bbg_pin_number;
    gpio_pin_direction direction;
    void* __gpio;
} gpio;

/**
 * Open a GPIO pin according to the provided gpio_spec.
 * @param pin The GPIO spec describing the desired pin.
 * @return gpio The GPIO pin or NULL if opening failed.
 */
int
gpio_open(gpio* pin);

/**
 * Update an open GPIO pin and change its direction.
 * @param pin The pin to update.
 * @param direction The new direction.
 * @return bool Whether or not the update succeeded.
 */
int
gpio_set_direction(gpio* pin, gpio_pin_direction direction);

/**
 * Set whether or not a GPIO pin should be considered active low.
 * @param pin The pin to update.
 * @param active_low Whether or not the pin is active low.
 * @return bool Whether or not the operation succeeded.
 */
int
gpio_set_active_low(gpio* pin, bool active_low);

/**
 * Read the current state of the GPIO pin.
 * @param pin The pin to read.
 * @param out_value Pointer to an integer to write the GPIO state to.
 * @return bool Whether or not the read succeeded.
 */
int
gpio_read(gpio* pin, int* restrict out_value);

/**
 * Wait on a list of pins until one of them changes state.
 * Once a state change occurs, the state change will be written to the
 * index of out_vals corresponding to the index of the changed pin
 * in pins. Waits as long as timeout_ms.
 * @param pins An array of GPIO pins to wait on.
 * @param n_pins Number of pins in pins.
 * @params out_vals An array of n_pins integers to write the output values to.
 * @param timeout_ms Timeout in milliseconds. Waits forever if the timeout is
 * negative, and returns immediately if the timeout is 0.
 * @return int The number of events received, or 0 on timeout, or -1 on error.
 */
int
gpio_wait(gpio* pins, size_t n_pins, int* out_vals, unsigned long timeout_ms);

/**
 * Close and free the GPIO pin.
 * @param pin A double pointer to the pin to close.
 */
void
gpio_close(gpio* pin);

/**
 * Write to a GPIO pin.
 * @param pin The pin to write to.
 * @value The value to write.
 */
int
gpio_write(gpio* pin, bool value);

#endif // GPIO_H
