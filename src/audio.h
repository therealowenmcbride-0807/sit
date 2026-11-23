#ifndef AUDIO_H
#define AUDIO_H

#include "synth.h"

bool audio_init(SynthState *state);
void audio_cleanup(void);

#endif
