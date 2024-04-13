/**
 * @file rfidExample.h
 * 
 * An example that is meant to closely mimic how the RFID module will actually be used in our project. 
 * Reads continuously for a tag, returning the UID of the tag if found, or 0xFF if no tag is in range.
*/

#include "hal/rfid.h"

// Arbitrary
#define POLL_TAG_ID_MS 100

byte currentTagId;

void
Rfid_example(void)
{
    Rfid_init();

    while (true) {
        currentTagId = Rfid_getCurrentTagId();
        printf("0x%2X\n", currentTagId);

        /*
        if (currentTagId == ' ... ' ) {
            setSong( ... )
        }
        */

        Timeutils_sleepForMs(POLL_TAG_ID_MS);
    }
}