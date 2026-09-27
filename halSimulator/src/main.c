#include "assets.h"
#include "hal.h"

int main(int argc, char *argv[]) {
  if (initTestingScreen(argc, argv) != 0) {
    return 1;
  }

  int running = 1;
  uint8_t scalar = 8;
  uint8_t x = 0;
  uint8_t y = 0;
  SDL_Event event;

  while (running) {
    Action action = readControls(&running, &default_keys);
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
      break;
    default:
      break;
    }
    SDL_SetRenderDrawColor(global_renderer, 0, 0, 0, 255);
    SDL_RenderClear(global_renderer);

    sendBackground(bc_sample);
    sendSprite(sample, x, y, 8, 8);

    SDL_RenderPresent(global_renderer);

    SDL_Delay(16);
  }

  SDL_Quit();
  return 0;
}
