/**
 * @file segDisplay.h
 * @brief Provides control for the BeagleBone's 14-seg display.
 * @author Louie Lu 301291418.
 */

#pragma once

#include "hal/i2c.h"

#include "hal/fileio.h"

#include "tcp.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Our server uses Zen Cape Green, so the green variables follow.
#define SEGDISPLAY_I2C_ADDRESS 0x20
#define SEGDISPLAY_DIRA_REG 0x00
#define SEGDISPLAY_DIRB_REG 0x01
#define SEGDISPLAY_OUTA_REG 0x14
#define SEGDISPLAY_OUTB_REG 0x15

typedef enum
{
    PATTERN_LEFT_TOP = 0,
    PATTERN_LEFT_BOT,
    PATTERN_RIGHT_TOP,
    PATTERN_RIGHT_BOT,
    MAX_NUM_PATTERNS,
} Pattern;

int
SegDisplay_init(void);
void
SegDisplay_shutdown(void);

void
SegDisplay_displayStatus(Server_Status status);