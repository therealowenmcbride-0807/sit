#ifndef SYNTH_H
#define SYNTH_H

#include <stdbool.h>

typedef enum {
  WAVE_SINE = 0,
  WAVE_TRIANGLE,
  WAVE_SAWTOOTH,
  WAVE_SQUARE,
  WAVE_COUNT
} Waveform;

typedef struct {
  double phase;
  double frequency;
  double amplitude;
  bool note_active;
  Waveform waveform;

  // UI display mode
  bool use_unicode;

  // Low-pass filter
  double lpf_cutoff;
  double lpf_state;

  // Audio WAV recording
  bool is_recording;
  char rec_filename[128];

  // Key sequence recording & playback
  bool is_key_recording;
  bool is_playing;
  char seq_filename[128];
} SynthState;

#endif
