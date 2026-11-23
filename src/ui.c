#include "ui.h"
#include <ctype.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#define KEY_ESC 27

static const char *SHARP_NOTES_ASCII[]   = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *FLAT_NOTES_ASCII[]    = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
static const char *SHARP_NOTES_UNICODE[] = {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"};
static const char *FLAT_NOTES_UNICODE[]  = {"C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B"};

static const char *WAVE_NAMES[] = {"Sine", "Triangle", "Sawtooth", "Square"};

static bool key_uses_flats(int root, bool is_minor) {
  if (is_minor) {
    // Minor keys on the flat side of the circle of fifths: Dm, Gm, Cm, Fm, Bbm, Ebm
    return (root == 2 || root == 7 || root == 0 || root == 5 || root == 10 || root == 3);
  } else {
    // Major keys on the flat side of the circle of fifths: F, Bb, Eb, Ab, Db, Gb
    return (root == 5 || root == 10 || root == 3 || root == 8 || root == 1 || root == 6);
  }
}

static const char *get_note_name(int note_idx, bool use_flats, bool use_unicode) {
  if (use_unicode) {
    return use_flats ? FLAT_NOTES_UNICODE[note_idx] : SHARP_NOTES_UNICODE[note_idx];
  }
  return use_flats ? FLAT_NOTES_ASCII[note_idx] : SHARP_NOTES_ASCII[note_idx];
}

// Prints a note label padded to `width` terminal columns (accounting for multi-byte ♯/♭)
static void print_padded_note(const char *lbl, int width) {
  int visual_len = 0;
  for (const unsigned char *p = (const unsigned char *)lbl; *p; p++) {
    if ((*p & 0xC0) != 0x80) {
      visual_len++;
    }
  }
  printw("%s", lbl);
  for (int i = visual_len; i < width; i++) {
    printw(" ");
  }
}

static int key_to_semitone_offset(int lower_ch, bool is_minor) {
  switch (lower_ch) {
  case 'a':
    return 0;
  case 's':
    return 2;
  case 'd':
    return is_minor ? 3 : 4;
  case 'f':
    return 5;
  case 'g':
    return 7;
  case 'h':
    return is_minor ? 8 : 9;
  case 'j':
    return is_minor ? 10 : 11;
  case 'w':
    return 1;
  case 'e':
    return is_minor ? 4 : 3;
  case 't':
    return 6;
  case 'y':
    return is_minor ? 9 : 8;
  case 'u':
    return is_minor ? 11 : 10;
  default:
    return -1;
  }
}

static double get_note_data(int root, int offset, int octave, bool is_minor,
                            bool use_unicode, char *buf, size_t buf_size) {
  int total_semitones = root + offset;
  int note_idx = total_semitones % 12;
  int note_octave = octave + (total_semitones / 12);

  int midi_note = (note_octave + 1) * 12 + note_idx;
  double freq = 440.0 * pow(2.0, (midi_note - 69) / 12.0);

  if (buf && buf_size > 0) {
    bool use_flats = key_uses_flats(root, is_minor);
    snprintf(buf, buf_size, "%s%d", get_note_name(note_idx, use_flats, use_unicode),
             note_octave);
  }
  return freq;
}

static void draw_piano(int start_y, int start_x, int root, int octave,
                       bool is_minor, int active_key, bool use_unicode) {
  char w_lbl[12], e_lbl[12], t_lbl[12], y_lbl[12], u_lbl[12];
  char a_lbl[12], s_lbl[12], d_lbl[12], f_lbl[12], g_lbl[12], h_lbl[12], j_lbl[12];

  get_note_data(root, 1, octave, is_minor, use_unicode, w_lbl, sizeof(w_lbl));
  get_note_data(root, is_minor ? 4 : 3, octave, is_minor, use_unicode, e_lbl, sizeof(e_lbl));
  get_note_data(root, 6, octave, is_minor, use_unicode, t_lbl, sizeof(t_lbl));
  get_note_data(root, is_minor ? 9 : 8, octave, is_minor, use_unicode, y_lbl, sizeof(y_lbl));
  get_note_data(root, is_minor ? 11 : 10, octave, is_minor, use_unicode, u_lbl, sizeof(u_lbl));

  get_note_data(root, 0, octave, is_minor, use_unicode, a_lbl, sizeof(a_lbl));
  get_note_data(root, 2, octave, is_minor, use_unicode, s_lbl, sizeof(s_lbl));
  get_note_data(root, is_minor ? 3 : 4, octave, is_minor, use_unicode, d_lbl, sizeof(d_lbl));
  get_note_data(root, 5, octave, is_minor, use_unicode, f_lbl, sizeof(f_lbl));
  get_note_data(root, 7, octave, is_minor, use_unicode, g_lbl, sizeof(g_lbl));
  get_note_data(root, is_minor ? 8 : 9, octave, is_minor, use_unicode, h_lbl, sizeof(h_lbl));
  get_note_data(root, is_minor ? 10 : 11, octave, is_minor, use_unicode, j_lbl, sizeof(j_lbl));

  const char *v = use_unicode ? "│" : "|";

  // Row 0 (57 cols)
  if (use_unicode) {
    mvprintw(start_y, start_x,
             "┌───────┬───────┬───────┬───────┬───────┬───────┬───────┐");
  } else {
    mvprintw(start_y, start_x,
             "+-------+-------+-------+-------+-------+-------+-------+");
  }

  // Row 1 (57 cols): Black key letter bindings
  mvprintw(start_y + 1, start_x, "%s     ", v);
  if (active_key == 'w') attron(A_REVERSE | A_BOLD);
  printw("[ W ]");
  if (active_key == 'w') attroff(A_REVERSE | A_BOLD);
  printw("   ");
  if (active_key == 'e') attron(A_REVERSE | A_BOLD);
  printw("[ E ]");
  if (active_key == 'e') attroff(A_REVERSE | A_BOLD);
  printw("     %s     ", v);
  if (active_key == 't') attron(A_REVERSE | A_BOLD);
  printw("[ T ]");
  if (active_key == 't') attroff(A_REVERSE | A_BOLD);
  printw("   ");
  if (active_key == 'y') attron(A_REVERSE | A_BOLD);
  printw("[ Y ]");
  if (active_key == 'y') attroff(A_REVERSE | A_BOLD);
  printw("   ");
  if (active_key == 'u') attron(A_REVERSE | A_BOLD);
  printw("[ U ]");
  if (active_key == 'u') attroff(A_REVERSE | A_BOLD);
  printw("     %s", v);

  // Row 2 (57 cols): Black key note names (column-safe padding)
  mvprintw(start_y + 2, start_x, "%s     ", v);
  if (active_key == 'w') attron(A_REVERSE | A_BOLD);
  printw(" ");
  print_padded_note(w_lbl, 4);
  if (active_key == 'w') attroff(A_REVERSE | A_BOLD);
  printw("   ");
  if (active_key == 'e') attron(A_REVERSE | A_BOLD);
  printw(" ");
  print_padded_note(e_lbl, 4);
  if (active_key == 'e') attroff(A_REVERSE | A_BOLD);
  printw("     %s     ", v);
  if (active_key == 't') attron(A_REVERSE | A_BOLD);
  printw(" ");
  print_padded_note(t_lbl, 4);
  if (active_key == 't') attroff(A_REVERSE | A_BOLD);
  printw("   ");
  if (active_key == 'y') attron(A_REVERSE | A_BOLD);
  printw(" ");
  print_padded_note(y_lbl, 4);
  if (active_key == 'y') attroff(A_REVERSE | A_BOLD);
  printw("   ");
  if (active_key == 'u') attron(A_REVERSE | A_BOLD);
  printw(" ");
  print_padded_note(u_lbl, 4);
  if (active_key == 'u') attroff(A_REVERSE | A_BOLD);
  printw("     %s", v);

  // Row 3 (57 cols)
  if (use_unicode) {
    mvprintw(start_y + 3, start_x,
             "│     └─┬─┘   └─┬─┘     │     └─┬─┘   └─┬─┘   └─┬─┘     │");
  } else {
    mvprintw(start_y + 3, start_x,
             "|     +-+-+   +-+-+     |     +-+-+   +-+-+   +-+-+     |");
  }

  const char white_keys[7] = {'a', 's', 'd', 'f', 'g', 'h', 'j'};
  const char *white_lbls[7] = {a_lbl, s_lbl, d_lbl, f_lbl, g_lbl, h_lbl, j_lbl};

  // Row 4 (57 cols)
  if (use_unicode) {
    mvprintw(start_y + 4, start_x,
             "│       │       │       │       │       │       │       │");
  } else {
    mvprintw(start_y + 4, start_x,
             "|       |       |       |       |       |       |       |");
  }

  // Row 5 (57 cols): White key letter bindings
  move(start_y + 5, start_x);
  for (int i = 0; i < 7; i++) {
    printw("%s", v);
    if (active_key == white_keys[i]) attron(A_REVERSE | A_BOLD);
    printw(" [ %c ] ", toupper(white_keys[i]));
    if (active_key == white_keys[i]) attroff(A_REVERSE | A_BOLD);
  }
  printw("%s", v);

  // Row 6 (57 cols): White key note names (column-safe padding)
  move(start_y + 6, start_x);
  for (int i = 0; i < 7; i++) {
    printw("%s", v);
    if (active_key == white_keys[i]) attron(A_REVERSE | A_BOLD);
    printw("  ");
    print_padded_note(white_lbls[i], 5);
    if (active_key == white_keys[i]) attroff(A_REVERSE | A_BOLD);
  }
  printw("%s", v);

    // Row 7 (57 cols)
  if (use_unicode) {
    mvprintw(start_y + 7, start_x,
             "└───────┴───────┴───────┴───────┴───────┴───────┴───────┘");
  } else {
    mvprintw(start_y + 7, start_x,
             "+-------+-------+-------+-------+-------+-------+-------+");
  }
}

static void draw_interface(const char *current_note, int root, int octave,
                           bool is_minor, int active_key,
                           const SynthState *state) {
  clear();
  if (state->use_unicode) {
    mvprintw(1, 2, "═══ SYNTH IN TERMINAL (sit) ═══");
  } else {
    mvprintw(1, 2, "=== SYNTH IN TERMINAL (sit) ===");
  }

  bool use_flats = key_uses_flats(root, is_minor);
  mvprintw(2, 2, "Key: %s %s  |  Octave: %d  |  Wave: %s  |  LPF: %.0f Hz",
           get_note_name(root, use_flats, state->use_unicode),
           is_minor ? "Minor" : "Major", octave,
           WAVE_NAMES[state->waveform], state->lpf_cutoff);

  // Status line for WAV / Key Recording / Playback
  if (state->is_recording) {
    attron(A_BOLD);
    mvprintw(3, 2, "[WAV REC] %s", state->rec_filename);
    attroff(A_BOLD);
  } else if (state->is_key_recording) {
    attron(A_BOLD);
    mvprintw(3, 2, "[KEY REC] %s", state->seq_filename);
    attroff(A_BOLD);
  } else if (state->is_playing) {
    attron(A_BOLD);
    mvprintw(3, 2, "[PLAYING] %s (Press ESC to stop)", state->seq_filename);
    attroff(A_BOLD);
  } else if (state->seq_filename[0] != '\0') {
    mvprintw(3, 2, "Last Seq: %s", state->seq_filename);
  } else if (state->rec_filename[0] != '\0') {
    mvprintw(3, 2, "Saved WAV: %s", state->rec_filename);
  }

  draw_piano(5, 2, root, octave, is_minor,
             state->note_active ? active_key : -1, state->use_unicode);

  mvprintw(14, 2, "Controls: [Shift+Key] Root | [Z/X] Oct | [M] Maj/Min | [V] Wave");
  mvprintw(15, 2, "          [[ / ]] LPF Cut  | [R] WAV   | [O] KeyRec  | [P] Play | [ESC] Quit");

  if (state->note_active) {
    attron(A_BOLD);
    mvprintw(17, 2, "Active Note: ");
    print_padded_note(current_note, 4);
    printw(" (%.2f Hz)", state->frequency);
    attroff(A_BOLD);
  } else {
    mvprintw(17, 2, "Active Note: --");
  }
  refresh();
}

void ui_init(void) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  set_escdelay(25);
  timeout(150);
}

void ui_run(SynthState *state) {
  char current_note[12] = "--";
  int root = 0;
  int octave = 4;
  bool is_minor = false;
  int active_key = -1;
  int ch;

  FILE *seq_rec_file = NULL;
  FILE *seq_play_file = NULL;

  if (state->is_playing && state->seq_filename[0] != '\0') {
    seq_play_file = fopen(state->seq_filename, "r");
    if (!seq_play_file) {
      state->is_playing = false;
    }
  }

  while (1) {
    draw_interface(current_note, root, octave, is_minor, active_key, state);

    if (state->is_playing && seq_play_file) {
      int live_ch = getch();
      if (live_ch == KEY_ESC) {
        fclose(seq_play_file);
        seq_play_file = NULL;
        state->is_playing = false;
        state->note_active = false;
        active_key = -1;
        continue;
      }

      if (fscanf(seq_play_file, "%d\n", &ch) != 1) {
        fclose(seq_play_file);
        seq_play_file = NULL;
        state->is_playing = false;
        state->note_active = false;
        active_key = -1;
        snprintf(current_note, sizeof(current_note), "--");
        continue;
      }
    } else {
      ch = getch();
    }

    if (state->is_key_recording && seq_rec_file) {
      int lower_check = (ch == ERR) ? ERR : tolower(ch);
      if (ch != KEY_ESC && lower_check != 'o' && lower_check != 'p') {
        fprintf(seq_rec_file, "%d\n", ch);
      }
    }

    if (ch == ERR) {
      state->note_active = false;
      active_key = -1;
      snprintf(current_note, sizeof(current_note), "--");
      continue;
    }

    if (ch == KEY_ESC) {
      break;
    }

    int lower_ch = tolower(ch);

    if (lower_ch == 'z') {
      if (octave > 1)
        octave--;
    } else if (lower_ch == 'x') {
      if (octave < 7)
        octave++;
    } else if (lower_ch == 'm') {
      is_minor = !is_minor;
    } else if (lower_ch == 'v') {
      state->waveform = (state->waveform + 1) % WAVE_COUNT;
    } else if (ch == '[') {
      if (state->lpf_cutoff > 150.0)
        state->lpf_cutoff -= 100.0;
    } else if (ch == ']') {
      if (state->lpf_cutoff < 8000.0)
        state->lpf_cutoff += 100.0;
    } else if (lower_ch == 'r') {
      state->is_recording = !state->is_recording;
    } else if (lower_ch == 'o' && !state->is_playing) {
      state->is_key_recording = !state->is_key_recording;
      if (state->is_key_recording) {
        mkdir("recordings", 0755);
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        snprintf(state->seq_filename, sizeof(state->seq_filename),
                 "recordings/sit_%04d%02d%02d_%02d%02d%02d.sit",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                 t->tm_min, t->tm_sec);
        seq_rec_file = fopen(state->seq_filename, "w");
        if (!seq_rec_file) {
          state->is_key_recording = false;
        }
      } else if (seq_rec_file) {
        fclose(seq_rec_file);
        seq_rec_file = NULL;
      }
    } else if (lower_ch == 'p' && !state->is_key_recording &&
               state->seq_filename[0] != '\0') {
      if (!state->is_playing) {
        seq_play_file = fopen(state->seq_filename, "r");
        if (seq_play_file) {
          state->is_playing = true;
        }
      } else {
        fclose(seq_play_file);
        seq_play_file = NULL;
        state->is_playing = false;
      }
    } else {
      if (isupper(ch)) {
        int new_root = key_to_semitone_offset(lower_ch, false);
        if (new_root >= 0) {
          root = new_root;
        }
      } else {
        int offset = key_to_semitone_offset(lower_ch, is_minor);
        if (offset >= 0) {
          state->frequency = get_note_data(root, offset, octave, is_minor,
                                           state->use_unicode, current_note,
                                           sizeof(current_note));
          state->note_active = true;
          active_key = lower_ch;
        }
      }
    }
  }

  if (seq_rec_file)
    fclose(seq_rec_file);
  if (seq_play_file)
    fclose(seq_play_file);
}

void ui_cleanup(void) { endwin(); }
