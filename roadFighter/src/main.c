#include "assets.h"
#include "hal.h"

typedef enum { SCREEN_MENU, SCREEN_GAME, SCREEN_SETTINGS } AppScreen;

int main(int argc, char *argv[]) {
  if (initScreen(argc, argv) != 0) {
    return 1;
  }

  int running = 1;
  uint8_t x = 30;
  uint8_t y = 64 - 12;
  uint8_t gameIndex = 0;
  uint8_t currentRow = 0;
  uint8_t currentCol = 0;

  const uint8_t selectorPositionsX[] = {6, 32};
  const uint8_t selectorPositionsY[] = {12, 38};
  const uint8_t gamesCount = 4;

  uint8_t selectory = selectorPositionsY[0];
  uint8_t selectorx = selectorPositionsX[0];

  AppScreen current_screen = SCREEN_GAME;

  while (running) {
    Action action = readControls(&running, &default_keys);

    switch (current_screen) {
    case SCREEN_MENU:
      if (action == ACTION_START) {
        current_screen = SCREEN_GAME;
      } else {
        switch (action) {
        case ACTION_UP:
          if (currentRow == 1)
            currentRow--;
          break;
        case ACTION_DOWN:
          if (currentRow == 0)
            currentRow++;
          break;
        case ACTION_LEFT:
          if (currentCol == 1)
            currentCol--;
          break;
        case ACTION_RIGHT:
          if (currentCol == 0)
            currentCol++;
          break;
        case ACTION_START:
          current_screen = SCREEN_MENU;
        default:
          break;
        }
      }
      selectory = selectorPositionsY[currentRow];
      selectorx = selectorPositionsX[currentCol];
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
      sendBackground(GameSelector);
      sendSprite(Selector, selectorx, selectory, 24, 24);
      sendString("Menu", 20, 2, 5, 7, (struct rgb){0, 255, 0});
      break;
    case SCREEN_GAME:
      sendBackground(SimplerRoad);
      sendSprite(RedCar, x, y, 8, 12);
      break;

    case SCREEN_SETTINGS:
      sendBackground(Road);
      break;
    }

    SDL_RenderPresent(global_renderer);
    SDL_Delay(16);
  }

  SDL_Quit();
  return 0;
}
