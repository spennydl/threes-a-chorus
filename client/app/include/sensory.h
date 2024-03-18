#pragma once

typedef float Sensory_SensoryIndex;

#define SENSORY_OK 0
#define SENSORY_EACCEL -1
#define SENSORY_EULTSON -2

int
Sensory_initialize(void);

int
Sensory_beginSensing(void);

Sensory_SensoryIndex
Sensory_getSensoryIndex(void);

void
Sensory_close(void);
