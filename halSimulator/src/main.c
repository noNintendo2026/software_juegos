#include "assets.h"
#include "hal.h"

typedef enum { SCREEN_MENU, SCREEN_GAME, SCREEN_SETTINGS } AppScreen;

int main(int argc, char *argv[]) {
  if (initTestingScreen(argc, argv) != 0) {
    return 1;
  }

  int running = 1;
  uint8_t x = 0;
  uint8_t y = 0;

  AppScreen current_screen = SCREEN_GAME;

  while (running) {
    Action action = readControls(&running, &default_keys);

    switch (current_screen) {
    case SCREEN_MENU:
      if (action == ACTION_START) {
        current_screen = SCREEN_GAME;
      }
      break;

    case SCREEN_GAME:
      if (action == ACTION_SELECT) {
        current_screen = SCREEN_MENU;
      } else {
        switch (action) {
        case ACTION_UP:
          y--;
          break;
        case ACTION_DOWN:
          y++;
          break;
        case ACTION_LEFT:
          x--;
          break;
        case ACTION_RIGHT:
          x++;
          break;
        case ACTION_START:
          current_screen = SCREEN_MENU;
          printf("Current Screen: MENU");
        default:
          break;
        }
      }
      break;
    case SCREEN_SETTINGS:
      if (action == ACTION_SELECT) {
        current_screen = SCREEN_MENU;
      }
      break;
    }

    SDL_SetRenderDrawColor(global_renderer, 0, 0, 0, 255);
    SDL_RenderClear(global_renderer);

    switch (current_screen) {
    case SCREEN_MENU:
      sendBackground(bc_sample);
      break;

    case SCREEN_GAME:
      sendBackground(bc_sample);
      sendSprite(sample, x, y, 8, 8);
      break;

    case SCREEN_SETTINGS:
      sendBackground(bc_sample);
      break;
    }

    SDL_RenderPresent(global_renderer);
    SDL_Delay(16);
  }

  SDL_Quit();
  return 0;
}
