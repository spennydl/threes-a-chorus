
/**
 * @file eyesExample.h
 * 
 * Displays all 6 moods in sequence, each mood lasting 5 seconds. 
 * To use, claer main.c and replace with eyesExample().
 */

#include "com/timeutils.h"
#include "hal/i2c.h"
#include "hal/segDisplay.h"

#define MOOD_DURATION_ms 5000

void
eyesExample(void)
{

    srand(time(NULL));

    SegDisplay_init();

    for (int i = 0; i < 6; i++) {
        int mood = i;
        printf("Current mood: %d\n", mood);

        long long currTime = Timeutils_getTimeInMs();

        while (currTime + MOOD_DURATION_ms > Timeutils_getTimeInMs()) {
            SegDisplay_displayEmotion(mood);
        }
    }
}