#include "assets.h"
#include "hal.h"

typedef enum { SCREEN_MENU, SCREEN_GAME, SCREEN_SETTINGS } AppScreen;

int main(int argc, char *argv[]) {
  if (initScreen(argc, argv) != 0) {
    return 1;
  }
  SoundID move_sound =
      initSound("../assets/mixkit-video-game-retro-click-237.wav");
  SoundID menu_sound = initSound("../assets/mixkit-game-ball-tap-2073.wav");

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
          playSound(move_sound);
          y--;
          break;
        case ACTION_DOWN:
          playSound(move_sound);
          y++;
          break;
        case ACTION_LEFT:
          playSound(move_sound);
          x--;
          break;
        case ACTION_RIGHT:
          playSound(move_sound);
          x++;
          break;
        case ACTION_START:
          current_screen = SCREEN_MENU;
          playSound(menu_sound);
          break;
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
      // FEATURE: We need something to be able to center it or select the text
      // from the center.
      sendString("Menu", 5, 5, 5, 7);
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
