#pragma once

#include "hal/i2c.h"

#include "com/config.h"
#include "com/fileio.h"
#include "com/timeutils.h"
#include "com/utils.h"

#include "singer.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/*
 * Display a pattern that represents an emotion. Internally, an emotion
 * corresponds with two patterns - one for stationary eyes, and one for
 * blinking. The time between blinks is randomized, as well as the duration
 * of the blink.
 *
 * Each displayEmotion call displays the pattern just once.
 */
void
SegDisplay_displayEmotion(Emotion emotion);