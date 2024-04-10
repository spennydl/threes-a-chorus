#pragma once

#include "sensory.h"

#include <stdint.h>

/* ******************************************
 * Uncomment the following for Zen Cape RED *
 * ****************************************** */

///////////////// accel.c ////////////////////////
#define ACCEL_ADDR 0x18
#define ACCEL_ACTIVE_MODE (1 << 5) | 0x7
#define ACCEL_CTRL_REG1 0x20
#define ACCEL_DATA_READ 0x28 + 0x80
#define ACCEL_NUM_BYTES_TO_READ 6
#define ACCEL_INDEX_ADJUST 0
//////////////// segDisplay.c  /////////////////////
#define SEGDISPLAY_I2C_ADDRESS 0x20
#define SEGDISPLAY_DIRA_REG 0x02
#define SEGDISPLAY_DIRB_REG 0x03
#define SEGDISPLAY_OUTA_REG 0x00
#define SEGDISPLAY_OUTB_REG 0x01

static const uint8_t EYES_HAPPY[4] = { 0x83, 0x48, 0x83, 0x48 };
static const uint8_t EYES_SAD[4] = { 0x06, 0x10, 0x10, 0x18 };
static const uint8_t EYES_ANGRY[4] = { 0x10, 0x02, 0x04, 0x02 };
static const uint8_t EYES_OVERSTIMULATED[4] = { 0x14, 0x05, 0x14, 0x05 };
static const uint8_t EYES_NEUTRAL[4] = { 0x21, 0x02, 0x01, 0x82 };
static const uint8_t EYES_SINGING[4] = { 0x81, 0x50, 0x81, 0x50 };

static const uint8_t BLINK_HAPPY[4] = { 0x01, 0x10, 0x01, 0x10 };
static const uint8_t BLINK_SAD[4] = { 0x06, 0x00, 0x10, 0x08 };
static const uint8_t BLINK_ANGRY[4] = { 0x00, 0x14, 0x00, 0x11 };
static const uint8_t BLINK_OVERSTIMULATED[4] = { 0x02, 0x08, 0x02, 0x08 };
static const uint8_t BLINK_NEUTRAL[4] = { 0x21, 0x10, 0x01, 0x90 };
static const uint8_t BLINK_SINGING[4] = { 0x81, 0x50, 0x81, 0x50 };

/* ********************************************
 comment the following for Zen Cape GREEN *
 ******************************************/

///////////////// accel.c ////////////////////////
// #define ACCEL_ADDR 0x1C
// #define ACCEL_ACTIVE_MODE 0x01
// #define ACCEL_CTRL_REG1 0x2A
// #define ACCEL_DATA_READ 0x01
// #define ACCEL_NUM_BYTES_TO_READ 7
// #define ACCEL_INDEX_ADJUST 2
//
////////////////// segDisplay.c  /////////////////////
// #define SEGDISPLAY_I2C_ADDRESS 0x20
// #define SEGDISPLAY_DIRA_REG 0x00
// #define SEGDISPLAY_DIRB_REG 0x01
// #define SEGDISPLAY_OUTA_REG 0x14
// #define SEGDISPLAY_OUTB_REG 0x15
//
// static const uint8_t EYES_HAPPY[4] = { 0x0C, 0x91, 0x0C, 0x91 };
// static const uint8_t EYES_SAD[4] = { 0x19, 0x20, 0x40, 0x30 };
// static const uint8_t EYES_ANGRY[4] = { 0x40, 0x04, 0x10, 0x04 };
// static const uint8_t EYES_OVERSTIMULATED[4] = { 0x50, 0x0A, 0x50, 0x0A };
// static const uint8_t EYES_NEUTRAL[4] = { 0x84, 0x04, 0x06, 0x04 };
// static const uint8_t EYES_SINGING[4] = { 0x04, 0xA1, 0x04, 0xA1 };
//
// static const uint8_t BLINK_HAPPY[4] = { 0x04, 0x20, 0x04, 0x20 };
// static const uint8_t BLINK_SAD[4] = { 0x19, 0x00, 0x40, 0x10 };
// static const uint8_t BLINK_ANGRY[4] = { 0x00, 0x28, 0x00, 0x22 };
// static const uint8_t BLINK_OVERSTIMULATED[4] = { 0x08, 0x10, 0x08, 0x10 };
// static const uint8_t BLINK_NEUTRAL[4] = { 0x84, 0x20, 0x06, 0x20 };
// static const uint8_t BLINK_SINGING[4] = { 0x04, 0xA1, 0x04, 0xA1 };
//
/* ********************************************
 *    Define your board's personality here!   *
 * ********************************************/

/** Choose one of three emotions to project when idle. **/
// #define EMOTION_IDLE EMOTION_HAPPY
// #define EMOTION_IDLE EMOTION_NEUTRAL
#define EMOTION_IDLE EMOTION_SAD

/** Choose a synth voice for each emotion. **/
// (1) Happy
// #define VOICE_HAPPY FM_BELL_PARAMS
#define VOICE_HAPPY FM_CHIRP_PARAMS

// (2) Sad
#define VOICE_SAD FM_CRY_PARAMS
// #define VOICE_SAD FM_SHINYDRONE_PARAMS
// #define VOICE_SAD FM_YOI_PARAMS

// (3) Angry
// #define VOICE_ANGRY FM_BIG_PARAMS
#define VOICE_ANGRY FM_SAWBLADE_PARAMS

// (4) Overstimulated
#define VOICE_OVERSTIMULATED FM_BEEPBOOP_PARAMS
// #define VOICE_OVERSTIMULATED FM_GLITCHBOOP_PARAMS

// (5) Neutral
// #define VOICE_NEUTRAL FM_AHH_PARAMS
// #define VOICE_NEUTRAL FM_BASS_PARAMS
// #define VOICE_NEUTRAL FM_BRASS_PARAMS
// #define VOICE_NEUTRAL FM_DEFAULT_PARAMS
#define VOICE_NEUTRAL FM_SHINYDRONE_PARAMS

/** Set your sensory preference constants. **/
static const Sensory_Preferences sensoryPreferences = { .cAccelLow = 5,
                                                        .cAccelHigh = 5,
                                                        .cPot = 5,
                                                        .cDistance = 5,
                                                        .cLight = 0,
                                                        .cButton = 5 };

/** TODO: ... Set your interaction function. **/
