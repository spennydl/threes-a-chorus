#pragma once

#include "das/fm.h"

int
FmPlayer_initialize(FmSynthParams* params);

FmSynthesizer*
FmPlayer_synth(void);

void
FmPlayer_close(void);
