/**
 * @file button.h
 * @brief Read a button connected to gpio.
 * @author Spencer Leslie 301571329
 */
#pragma once

/** Button events. */
typedef enum
{
    /** Button is up. */
    BUTTON_UP = 0,
    /** Button is down. */
    BUTTON_DOWN,
    /** Button was just pressed. */
    BUTTON_PRESS,
    /** Button was just released. */
    BUTTON_RELEASE
} ButtonEvent;

/** External-facing button type. */
typedef struct Button Button;

/**
 * Open a button on a gpio pin.
 *
 * @param linuxPinNumber Linux GPIO number.
 * @param bbgHeader Which header (8 or 9) the pin is on.
 * @param bbgPinNumber The BBG header pin number of the pin.
 * @return Pointer to a button or NULL on error.
 */
Button*
Button_open(unsigned char linuxPinNumber,
            unsigned char bbgHeader,
            unsigned char bbgPinNumber);

/**
 * Updates the button state and returns detected and debounced ButtonEvents.
 *
 * This should be called on a regular interval of once every ~10ms.
 *
 * @param button The button to read from.
 * @return ButtonEvent Detected button event.
 */
ButtonEvent
Button_getState(Button* button);

/**
 * Close and free the button.
 *
 * @param button The button to close and free.
 */
void
Button_close(Button* button);
