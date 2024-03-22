#include "hal/button.h"
#include "hal/gpio.h"
#include <stdint.h>
#include <stdlib.h>

#define BUTTON_EVENT_MASK 0xC7
#define BUTTON_PRESS_BITS 0x07
#define BUTTON_RELEASE_BITS 0xC0
#define BUTTON_DOWN_BITS 0xFF
#define BUTTON_UP_BITS 0x00

struct Button
{
    int state;
    gpio gpio;
    uint8_t history;
};

Button*
Button_open(unsigned char linux_pin_number,
            unsigned char bbg_header,
            unsigned char bbg_pin_number)
{
    Button* button = malloc(sizeof(Button));
    button->history = 0;
    button->state = BUTTON_UP;
    button->gpio.bbg_header = bbg_header;
    button->gpio.bbg_pin_number = bbg_pin_number;
    button->gpio.linux_pin_number = linux_pin_number;
    button->gpio.direction = GPIO_IN;

    if (gpio_open(&button->gpio) < 0) {
        free(button);
        return NULL;
    }
    return button;
}

ButtonEvent
Button_getState(Button* button)
{
    int val;
    if (gpio_read(&button->gpio, &val) < 0) {
        return -1;
    }

    button->history <<= 1;
    button->history |= val;

    if (button->history == BUTTON_DOWN_BITS) {
        button->state = BUTTON_DOWN;
    } else if (button->history == BUTTON_UP_BITS) {
        button->state = BUTTON_UP;
    }

    uint8_t masked = button->history & BUTTON_EVENT_MASK;
    if (masked == BUTTON_PRESS_BITS) {
        button->state = BUTTON_PRESS;
        button->history = BUTTON_DOWN_BITS;
    } else if (masked == BUTTON_RELEASE_BITS) {
        button->state = BUTTON_RELEASE;
        button->history = BUTTON_UP_BITS;
    }

    return button->state;
}

void
Button_close(Button* button)
{
    gpio_close(&button->gpio);
    free(button);
}
