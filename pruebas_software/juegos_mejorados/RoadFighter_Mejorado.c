/* RoadFighter_Mejorado.c -- version jugable de Road Fighter con las 4
 * mecanicas reales del README (carrera contra el reloj, combustible,
 * trafico dinamico, choques), usando hal.h/assets.h ya existentes de
 * roadFighter/ (no se modifica esa carpeta, solo se incluyen sus headers
 * publicos -- "no importa si repetimos codigo", instruccion explicita del
 * usuario) y la logica pura de roadfighter_logica.c (ya probada sin SDL).
 *
 * Compilar (ejemplo, ajustar ruta de SDL2 mingw):
 *   gcc RoadFighter_Mejorado.c roadfighter_logica.c -o RoadFighter_Mejorado.exe \
 *       -I<roadFighter>/include -I<SDL2>/include/SDL2 -I<SDL2>/include \
 *       -L<SDL2>/lib -lmingw32 -lSDL2main -lSDL2
 *   (copiar SDL2.dll junto al .exe antes de correrlo)
 */
#include "assets.h"  /* se resuelve via -I al include/ real de roadFighter, no se copia ni se modifica esa carpeta */
#include "hal.h"
#include "roadfighter_logica.h"
#include <stdio.h>

static AccionRF accion_hal_a_rf(Action a) {
  switch (a) {
    case ACTION_UP:    return RF_ACCION_ARRIBA;
    case ACTION_DOWN:  return RF_ACCION_ABAJO;
    case ACTION_LEFT:  return RF_ACCION_IZQUIERDA;
    case ACTION_RIGHT: return RF_ACCION_DERECHA;
    default:           return RF_ACCION_NINGUNA;
  }
}

static void dibujar_trafico(const EntidadTraficoRF *e) {
  if (!e->activo || e->y <= -30 || e->y >= RF_PANTALLA_ALTO) {
    return;  /* todavia fuera de pantalla, no hay nada que mandar a dibujar */
  }
  uint8_t x = (uint8_t)(e->x < 0 ? 0 : e->x);
  uint8_t y = (uint8_t)(e->y < 0 ? 0 : e->y);
  switch (e->tipo) {
    case RF_TRAFICO_AUTO_AZUL:     sendSprite(BlueCar, x, y, 8, 12); break;
    case RF_TRAFICO_AUTO_AMARILLO: sendSprite(YellowCar, x, y, 8, 12); break;
    case RF_TRAFICO_CAMION:        sendSprite(Truck, x, y, 9, 21); break;
  }
}

int main(int argc, char *argv[]) {
  if (initScreen(argc, argv) != 0) {
    return 1;
  }

  EstadoRoadFighterRF estado;
  rf_inicializar(&estado, (uint32_t)42);

  int running = 1;
  char texto_combustible[16];
  char texto_distancia[16];

  while (running) {
    Action accion_hal = readControls(&running, &default_keys);
    rf_actualizar(&estado, accion_hal_a_rf(accion_hal));

    SDL_SetRenderDrawColor(global_renderer, 0, 0, 0, 255);
    SDL_RenderClear(global_renderer);

    sendBackground(SimplerRoad);

    for (int i = 0; i < RF_MAX_TRAFICO; i++) {
      dibujar_trafico(&estado.trafico[i]);
    }

    sendSprite(RedCar, (uint8_t)estado.jugador_x, (uint8_t)estado.jugador_y, RF_JUGADOR_ANCHO, RF_JUGADOR_ALTO);

    snprintf(texto_combustible, sizeof(texto_combustible), "F%d", estado.combustible);
    sendString(texto_combustible, 1, 1, 4, 6, (struct rgb){255, 255, 0});

    snprintf(texto_distancia, sizeof(texto_distancia), "D%u", estado.distancia);
    sendString(texto_distancia, 1, 8, 4, 6, (struct rgb){255, 255, 255});

    if (estado.juego_terminado) {
      sendString("FIN", 22, 28, 5, 7, (struct rgb){255, 0, 0});
    } else if (estado.juego_ganado) {
      sendString("GANO", 18, 28, 5, 7, (struct rgb){0, 255, 0});
    }

    SDL_RenderPresent(global_renderer);
    SDL_Delay(16);
  }

  SDL_Quit();
  return 0;
}
