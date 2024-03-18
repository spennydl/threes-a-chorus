#include "hal/rfid.h"

static byte tagData[NUM_BYTES_IN_PICC_BLOCK] = { 0 };
static int status;

// Function prototypes
static void
printByteArray(byte* tagData);

static void
printByteArray(byte* tagData)
{
    printf("{ ");
    for (int i = 0; i < NUM_BYTES_IN_PICC_UID; i++) {
        printf("%2X", tagData[i]);

        // Avoid the trailing comma while printing.
        if (i < NUM_BYTES_IN_PICC_UID - 1) {
            printf(", ");
        }
    }
    printf(" }");
    puts("");
    fflush(stdout);
}

int
Rfid_example(void)
{
    Rfid_init();

    while (true) {
        status = Rfid_searchForTag();
        printf("status: %d\n", status);
        if (status != PICC_OK) {
            // TODO if you don't sleep like this then it gets messed up pretty
            // often
            // Timeutils_sleepForMs(1000);
        }

        if (status == PICC_OK) {
            do {
                status = Rfid_getTagUid(tagData);
            } while (status != PICC_OK);

            puts("Tag found. Printing tag UID...");
            printByteArray(tagData);
        }
        // Else, keep searching.

        // TODO: sleeping is probably the biggest deterrent of the infinite loop
        // error, but you can still run into inf loop errors by waving it abck &
        // forth. 1000ms feels pretty safe (99% chance of being fine), 100ms
        // can still run into errors.
        Timeutils_sleepForMs(1000);
    }
    puts("Ending program...");

    Rfid_shutdown();

    return 0;
}