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
}

int
Rfid_example(void)
{
    Rfid_init();
    Rfid_printFirmwareVersion(); // for good luck :)

    puts("When testing, please perform the following:\n");
    puts("1) Obtain a 4-byte UID, without error, from the white card tag.");
    puts("2) Swipe the tag at least twice, to ensure it returns the same UID.");
    puts("3) Repeat for the blue key fob tag.");
    puts("4) Reboot the BeagleBone and repeat steps 1-3.");

    puts("");
    puts("Searching for a tag...");
    puts("");

    while (true) {
        status = Rfid_searchForTag();

        if (status == PICC_OK) {
            do {
                status = Rfid_getTagUid(tagData);
            } while (status != PICC_OK);
            break;
        }
        // Else, keep searching.
    }
    puts("Tag found. Printing tag UID...");
    printByteArray(tagData);
    puts("Ending program...");

    Rfid_shutdown();

    return 0;
}