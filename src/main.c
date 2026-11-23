#include "audio.h"
#include "synth.h"
#include "ui.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");

  SynthState state = {.phase = 0.0,
                      .frequency = 440.0,
                      .amplitude = 0.0,
                      .note_active = false,
                      .waveform = WAVE_SINE,
                      .use_unicode = true,
                      .lpf_cutoff = 1200.0,
                      .lpf_state = 0.0,
                      .is_recording = false,
                      .rec_filename = "",
                      .is_key_recording = false,
                      .is_playing = false,
                      .seq_filename = ""};

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--ascii") == 0 || strcmp(argv[i], "-a") == 0) {
      state.use_unicode = false;
    } else if (strcmp(argv[i], "--unicode") == 0 ||
               strcmp(argv[i], "-u") == 0) {
      state.use_unicode = true;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf("Usage: %s [--ascii | --unicode] [recording.sit]\n", argv[0]);
      return 0;
    } else {
      snprintf(state.seq_filename, sizeof(state.seq_filename), "%s", argv[i]);
      state.is_playing = true;
    }
  }

  if (!audio_init(&state)) {
    fprintf(stderr, "Failed to initialize audio.\n");
    return 1;
  }

  ui_init();
  ui_run(&state);
  ui_cleanup();

  audio_cleanup();
  return 0;
}
