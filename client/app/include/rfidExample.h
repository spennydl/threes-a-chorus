#include "hal/rfid.h"

// Arbitrary
#define POLL_TAG_ID_MS 100

byte currentTagId;

// This example is meant to closely mimic how the RFID module will actually be
// used in our project (minus the printf).
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