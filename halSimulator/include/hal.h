#pragma once
#include "basic_font.h"
#include <SDL2/SDL.h>
#include <stdint.h>

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

void sendString(char *string, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  // for (;;)
  // sendLetter();
}

void sendLetter(char s, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  // sendSprite(sprite_buffer, x, y, w, h);
}

#endif
