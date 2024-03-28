#pragma once

typedef enum
{
    BUTTON_UP = 0,
    BUTTON_DOWN,
    BUTTON_PRESS,
    BUTTON_RELEASE
} ButtonEvent;

typedef struct Button Button;

Button*
Button_open(unsigned char linux_pin_number,
            unsigned char bbg_header,
            unsigned char bbg_pin_number);

ButtonEvent
Button_getState(Button* button);

void
Button_close(Button* button);
