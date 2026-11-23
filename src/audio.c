#include "audio.h"
#include <math.h>
#include <portaudio.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define M_PI 3.14159265358979323846

static PaStream *stream = NULL;
static FILE *wav_file = NULL;
static uint32_t data_bytes_written = 0;

// Standard 44-byte PCM WAV header
typedef struct {
  char riff[4];             // "RIFF"
  uint32_t file_size;       // Total file size - 8
  char wave[4];             // "WAVE"
  char fmt[4];              // "fmt "
  uint32_t fmt_size;        // 16 for PCM
  uint16_t audio_format;    // 1 for PCM
  uint16_t num_channels;    // 2 (Stereo)
  uint32_t sample_rate;     // 44100
  uint32_t byte_rate;       // SampleRate * NumChannels * BitsPerSample/8
  uint16_t block_align;     // NumChannels * BitsPerSample/8
  uint16_t bits_per_sample; // 16
  char data[4];             // "data"
  uint32_t data_size;       // Number of bytes in data section
} WavHeader;

static void start_wav_recording(SynthState *state) {
  mkdir("recordings", 0755);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  snprintf(state->rec_filename, sizeof(state->rec_filename),
           "recordings/sit_%04d%02d%02d_%02d%02d%02d.wav", t->tm_year + 1900,
           t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

  wav_file = fopen(state->rec_filename, "wb");
  if (!wav_file) {
    state->is_recording = false;
    return;
  }

  WavHeader header = {.riff = {'R', 'I', 'F', 'F'},
                      .file_size = 0,
                      .wave = {'W', 'A', 'V', 'E'},
                      .fmt = {'f', 'm', 't', ' '},
                      .fmt_size = 16,
                      .audio_format = 1,
                      .num_channels = 2,
                      .sample_rate = SAMPLE_RATE,
                      .byte_rate = SAMPLE_RATE * 2 * 2,
                      .block_align = 4,
                      .bits_per_sample = 16,
                      .data = {'d', 'a', 't', 'a'},
                      .data_size = 0};

  fwrite(&header, sizeof(WavHeader), 1, wav_file);
  data_bytes_written = 0;
}

static void stop_wav_recording(void) {
  if (!wav_file)
    return;

  uint32_t file_size = data_bytes_written + sizeof(WavHeader) - 8;
  fseek(wav_file, 4, SEEK_SET);
  fwrite(&file_size, sizeof(uint32_t), 1, wav_file);

  fseek(wav_file, 40, SEEK_SET);
  fwrite(&data_bytes_written, sizeof(uint32_t), 1, wav_file);

  fclose(wav_file);
  wav_file = NULL;
}

static double generate_wave(Waveform wf, double phase) {
  double norm = phase / (2.0 * M_PI); // 0.0 to 1.0
  switch (wf) {
  case WAVE_SINE:
    return sin(phase);
  case WAVE_TRIANGLE:
    return 1.0 - 4.0 * fabs(round(norm - 0.25) - (norm - 0.25));
  case WAVE_SAWTOOTH:
    return 2.0 * (norm - floor(norm + 0.5));
  case WAVE_SQUARE:
    return (phase < M_PI) ? 1.0 : -1.0;
  default:
    return 0.0;
  }
}

static double apply_lowpass(double input, SynthState *state) {
  double dt = 1.0 / SAMPLE_RATE;
  double rc = 1.0 / (2.0 * M_PI * state->lpf_cutoff);
  double alpha = dt / (rc + dt);
  state->lpf_state += alpha * (input - state->lpf_state);
  return state->lpf_state;
}

static int audio_callback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData) {
  SynthState *state = (SynthState *)userData;
  float *out = (float *)outputBuffer;
  (void)inputBuffer;
  (void)timeInfo;
  (void)statusFlags;

  if (state->is_recording && !wav_file) {
    start_wav_recording(state);
  } else if (!state->is_recording && wav_file) {
    stop_wav_recording();
  }

  for (unsigned long i = 0; i < framesPerBuffer; i++) {
    double target_amp = state->note_active ? 0.3 : 0.0;
    state->amplitude += (target_amp - state->amplitude) * 0.005;

    double raw = generate_wave(state->waveform, state->phase);
    if (state->waveform == WAVE_SAWTOOTH || state->waveform == WAVE_SQUARE) {
      raw = apply_lowpass(raw, state);
    } else {
      state->lpf_state = raw;
    }

    float sample = (float)(raw * state->amplitude);
    *out++ = sample;
    *out++ = sample;

    if (wav_file) {
      // Convert float (-1.0 to 1.0) to 16-bit signed PCM
      int16_t pcm = (int16_t)(sample * 32767.0f);
      int16_t stereo_frame[2] = {pcm, pcm};
      fwrite(stereo_frame, sizeof(int16_t), 2, wav_file);
      data_bytes_written += sizeof(stereo_frame);
    }

    state->phase += (2.0 * M_PI * state->frequency) / SAMPLE_RATE;
    if (state->phase >= 2.0 * M_PI) {
      state->phase -= 2.0 * M_PI;
    }
  }
  return paContinue;
}

bool audio_init(SynthState *state) {
  if (Pa_Initialize() != paNoError)
    return false;

  PaError err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE,
                                     FRAMES_PER_BUFFER, audio_callback, state);
  if (err != paNoError)
    return false;

  return Pa_StartStream(stream) == paNoError;
}

void audio_cleanup(void) {
  if (wav_file) {
    stop_wav_recording();
  }
  if (stream) {
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
  }
  Pa_Terminate();
}
