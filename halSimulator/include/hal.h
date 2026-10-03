#pragma once
#include "basic_font.h"
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

typedef enum {
  ACTION_NONE = 0,
  ACTION_UP,
  ACTION_DOWN,
  ACTION_RIGHT,
  ACTION_LEFT,
  ACTION_START,
  ACTION_SELECT
} Action;

struct keys {
  SDL_Keycode UP;
  SDL_Keycode DOWN;
  SDL_Keycode RIGHT;
  SDL_Keycode LEFT;
  SDL_Keycode START;
  SDL_Keycode SELECT;
};

struct keys default_keys = {.UP = SDLK_UP,
                            .DOWN = SDLK_DOWN,
                            .RIGHT = SDLK_RIGHT,
                            .LEFT = SDLK_LEFT,
                            .START = SDLK_RETURN,
                            .SELECT = SDLK_RSHIFT};

// -- Screen hal
struct rgb hex2rgb(uint16_t hexColor);
void sendSprite(const uint16_t *sprite, uint8_t x, uint8_t y, uint8_t w,
                uint8_t h);
void sendBackground(const uint16_t *sprite);
int initScreen(int argc, char *argv[]);

void sendLetter(char s, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// -- Controls hal
Action readControls(int *running, struct keys *bindings);
static SDL_Renderer *global_renderer = NULL;

typedef int SoundID;
enum { SOUND_INVALID_ID = -1 };
SoundID initSound(const char *filepath);
void playSound(SoundID audio_id);

#ifndef FPGA
Action readControls(int *running, struct keys *bindings) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      *running = 0;
    } else if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == bindings->UP)
        return ACTION_UP;
      if (event.key.keysym.sym == bindings->DOWN)
        return ACTION_DOWN;
      if (event.key.keysym.sym == bindings->RIGHT)
        return ACTION_RIGHT;
      if (event.key.keysym.sym == bindings->LEFT)
        return ACTION_LEFT;
      if (event.key.keysym.sym == bindings->START)
        return ACTION_START;
      if (event.key.keysym.sym == bindings->SELECT)
        return ACTION_SELECT;
    }
  }
  return ACTION_NONE;
};

struct rgb hex2rgb(uint16_t hexColor) {
  struct rgb color;

  uint8_t r5 = (hexColor >> 11) & 0x1F;
  uint8_t g6 = (hexColor >> 5) & 0x3F;
  uint8_t b5 = hexColor & 0x1F;

  color.r = (r5 * 255) / 31;
  color.g = (g6 * 255) / 63;
  color.b = (b5 * 255) / 31;

  return color;
}

void sendSprite(const uint16_t *sprite, uint8_t x, uint8_t y, uint8_t w,
                uint8_t h) {
  if (!global_renderer)
    return;

  int scale = 8;

  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col++) {
      uint16_t pixelColor = sprite[row * w + col];

      if (pixelColor == 0x0843) {
        continue;
      }

      struct rgb color = hex2rgb(pixelColor);

      SDL_SetRenderDrawColor(global_renderer, color.r, color.g, color.b, 255);

      SDL_Rect pixelRect = {.x = (x + col) * scale,
                            .y = (y + row) * scale,
                            .w = scale,
                            .h = scale};

      SDL_RenderFillRect(global_renderer, &pixelRect);
    }
  }
}

void sendBackground(const uint16_t *sprite) {
  sendSprite(sprite, 0, 0, 64, 64);
};

void sendLetter(char s, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  if (s < 32 || s > 126) {
    s = '?';
  }

  int font_index = (s - 32) * w;
  uint16_t sprite_buffer[35];

  for (uint8_t col = 0; col < w; col++) {
    uint8_t column_data = usg_font_5x7[font_index + col];
    for (uint8_t row = 0; row < h; row++) {

      int bit = (column_data >> row) & 1;

      sprite_buffer[row * w + col] = bit ? 0xFFF : 0x0843;
    }
  }
  sendSprite(sprite_buffer, x, y, w, h);
}

void sendString(char *string, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  uint8_t text_spacing = 2;
  int len = strlen(string);
  for (uint8_t i = 0; i < len; i++) {
    sendLetter(string[i], x + (i * (w + text_spacing)), y, w, h);
  }
}

int initScreen(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 1;
  }
  SDL_Window *window = NULL;
  if (SDL_CreateWindowAndRenderer(64 * 8, 64 * 8, 0, &window,
                                  &global_renderer) < 0) {
    SDL_Quit();
    return 1;
  }
  SDL_SetWindowTitle(window, "USG Library - SDL2 Simulation");
  return 0;
};

enum { SOUND_VOICE_COUNT = 16, SOUND_ASSET_COUNT = 64 };
typedef struct {
  Sint16 *samples;
  Uint32 sample_count;
} SoundAsset;

typedef struct {
  SoundID audio_id;
  Uint32 position;
  int active;
} SoundVoice;

static SDL_AudioDeviceID sound_device = 0;
static SoundAsset sound_assets[SOUND_ASSET_COUNT];
static int sound_asset_count = 0;
static SoundVoice sound_voices[SOUND_VOICE_COUNT];

static void mixSound(void *userdata, Uint8 *stream, int length) {
  (void)userdata;
  Sint16 *output = (Sint16 *)stream;
  int output_samples = length / (int)sizeof(Sint16);
  SDL_memset(stream, 0, length);

  for (int voice = 0; voice < SOUND_VOICE_COUNT; ++voice) {
    if (!sound_voices[voice].active)
      continue;
    for (int i = 0; i < output_samples && sound_voices[voice].active; ++i) {
      SoundAsset *asset = &sound_assets[sound_voices[voice].audio_id];
      Sint32 mixed = output[i] + asset->samples[sound_voices[voice].position++];
      if (mixed > INT16_MAX)
        mixed = INT16_MAX;
      if (mixed < INT16_MIN)
        mixed = INT16_MIN;
      output[i] = (Sint16)mixed;
      if (sound_voices[voice].position >= asset->sample_count)
        sound_voices[voice].active = 0;
    }
  }
}

SoundID initSound(const char *filepath) {
  if (sound_asset_count >= SOUND_ASSET_COUNT) {
    printf("Sound cache is full (maximum %d sounds)\n", SOUND_ASSET_COUNT);
    return SOUND_INVALID_ID;
  }
  if (SDL_WasInit(SDL_INIT_AUDIO) == 0 &&
      SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    printf("SDL audio init failed: %s\n", SDL_GetError());
    return SOUND_INVALID_ID;
  }

  SDL_AudioSpec source_spec;
  Uint8 *wav_buffer = NULL;
  Uint32 wav_length = 0;
  if (SDL_LoadWAV(filepath, &source_spec, &wav_buffer, &wav_length) == NULL) {
    printf("Failed to load WAV (%s): %s\n", filepath, SDL_GetError());
    return SOUND_INVALID_ID;
  }

  if (sound_device == 0) {
    SDL_AudioSpec wanted = {0};
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 2048;
    wanted.callback = mixSound;
    sound_device = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if (sound_device == 0) {
      printf("Failed to open audio device: %s\n", SDL_GetError());
      SDL_FreeWAV(wav_buffer);
      return SOUND_INVALID_ID;
    }
  }

  SDL_AudioCVT cvt;
  if (SDL_BuildAudioCVT(&cvt, source_spec.format, source_spec.channels,
                        source_spec.freq, AUDIO_S16SYS, 2, 44100) < 0) {
    printf("Failed to prepare WAV conversion: %s\n", SDL_GetError());
    SDL_FreeWAV(wav_buffer);
    return SOUND_INVALID_ID;
  }
  cvt.len = (int)wav_length;
  cvt.buf = (Uint8 *)SDL_malloc((size_t)cvt.len * cvt.len_mult);
  if (cvt.buf == NULL) {
    SDL_FreeWAV(wav_buffer);
    return SOUND_INVALID_ID;
  }
  SDL_memcpy(cvt.buf, wav_buffer, wav_length);
  SDL_FreeWAV(wav_buffer);
  if (SDL_ConvertAudio(&cvt) < 0) {
    printf("Failed to convert WAV: %s\n", SDL_GetError());
    SDL_free(cvt.buf);
    return SOUND_INVALID_ID;
  }

  SoundID audio_id = sound_asset_count++;
  sound_assets[audio_id].samples = (Sint16 *)cvt.buf;
  sound_assets[audio_id].sample_count = (Uint32)cvt.len_cvt / sizeof(Sint16);
  if (sound_asset_count == 1) {
    SDL_memset(sound_voices, 0, sizeof(sound_voices));
    SDL_PauseAudioDevice(sound_device, 0);
  }
  return audio_id;
}

void playSound(SoundID audio_id) {
  if (sound_device == 0 || audio_id < 0 || audio_id >= sound_asset_count ||
      sound_assets[audio_id].samples == NULL ||
      sound_assets[audio_id].sample_count == 0)
    return;
  SDL_LockAudioDevice(sound_device);
  int voice = 0;
  while (voice < SOUND_VOICE_COUNT && sound_voices[voice].active)
    ++voice;
  if (voice == SOUND_VOICE_COUNT)
    voice = 0;
  sound_voices[voice].audio_id = audio_id;
  sound_voices[voice].position = 0;
  sound_voices[voice].active = 1;
  SDL_UnlockAudioDevice(sound_device);
}

#endif

#ifdef FPGA

void sendSprite(const uint16_t *spriteAddress, uint8_t x, uint8_t y, uint8_t w,
                uint8_t h) {
  // DIR_X = x;
  // DIR_Y = y;
  // BC = 0;
  // SPR_DIR = spriteAddress;
};

void sendBackground(const uint16_t *spriteAddress) {
  // DIR_X = 0;
  // DIR_Y = 0;
  // BC = 1;
  // SPR_DIR = spriteAddress;
}

typedef enum { KEYBOARD, MOUSE, NESS } controller;

Action readControls(controller controller) {
  // FEATURE: Maybe another parameter on the function to look for each player.
  switch (controller) {
  case KEYBOARD:
    // return readFromDirection();
    break;
  case MOUSE:
    // return readFromDirection();
    break;
  case NESS:
    // return readFromDirection();
    break;
  }
  return ACTION_NONE;
};

// SoundID initSound(const char *filepath) {
//   (void)filepath;
//   return SOUND_INVALID_ID;
// }
// void playSound(SoundID audio_id) { (void)audio_id; }

void sendString(char *string, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  // for (;;)
  // sendLetter();
}

void sendLetter(char s, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  // sendSprite(sprite_buffer, x, y, w, h);
}

#endif
