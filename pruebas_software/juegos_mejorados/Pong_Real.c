/* Pong_Real.c -- version jugable de Pong (ver pong_logica.h/.c). La carpeta
 * pong/ del repositorio real no tiene un juego de Pong -- pong/Space_Invaders.c
 * es copia byte-identica de space_invaders/Space_Invaders.c (confirmado con
 * `diff`, sin diferencias). Este archivo si implementa Pong desde cero.
 *
 * Usa el hal.h de roadFighter (ya tiene sendString/sendLetter con parametro
 * de color, igual que hal_shared.h) solo como headers -- no se modifica esa
 * carpeta. Paleta y pelota son rectangulos solidos generados en codigo, sin
 * ningun asset externo ni elemento grafico de ningun Pong real.
 *
 * Compilar (ejemplo, ajustar ruta de SDL2 mingw):
 *   gcc Pong_Real.c pong_logica.c -o Pong_Real.exe \
 *       -I<roadFighter>/include -I<SDL2>/include/SDL2 -I<SDL2>/include \
 *       -L<SDL2>/lib -lmingw32 -lSDL2main -lSDL2
 *   (copiar SDL2.dll junto al .exe antes de correrlo)
 */
#include "hal.h"
#include "pong_logica.h"
#include <stdio.h>

static uint16_t sprite_paleta[PONG_PALETA_ANCHO * PONG_PALETA_ALTO];
static uint16_t sprite_pelota[PONG_PELOTA_LADO * PONG_PELOTA_LADO];
static uint16_t fondo_negro[64 * 64];

static void generar_sprites_solidos(void) {
  for (int i = 0; i < PONG_PALETA_ANCHO * PONG_PALETA_ALTO; i++) {
    sprite_paleta[i] = 0xFFFF;  /* blanco solido -- rectangulo generico, sin ningun asset */
  }
  for (int i = 0; i < PONG_PELOTA_LADO * PONG_PELOTA_LADO; i++) {
    sprite_pelota[i] = 0xFFFF;
  }
  for (int i = 0; i < 64 * 64; i++) {
    fondo_negro[i] = 0x0000;
  }
}

static AccionPong accion_hal_a_pong(Action a) {
  switch (a) {
    case ACTION_UP:   return PONG_ACCION_ARRIBA;
    case ACTION_DOWN: return PONG_ACCION_ABAJO;
    default:          return PONG_ACCION_NINGUNA;
  }
}

int main(int argc, char *argv[]) {
  if (initScreen(argc, argv) != 0) {
    return 1;
  }
  generar_sprites_solidos();

  EstadoPong estado;
  pong_inicializar(&estado, (uint32_t)7);

  int running = 1;
  char texto_puntaje[16];

  while (running) {
    Action accion_hal = readControls(&running, &default_keys);
    pong_actualizar(&estado, accion_hal_a_pong(accion_hal));

    SDL_SetRenderDrawColor(global_renderer, 0, 0, 0, 255);
    SDL_RenderClear(global_renderer);

    sendBackground(fondo_negro);
    sendSprite(sprite_paleta, 0, (uint8_t)estado.paleta_izq_y, PONG_PALETA_ANCHO, PONG_PALETA_ALTO);
    sendSprite(sprite_paleta, 64 - PONG_PALETA_ANCHO, (uint8_t)estado.paleta_der_y, PONG_PALETA_ANCHO, PONG_PALETA_ALTO);
    sendSprite(sprite_pelota, (uint8_t)estado.pelota_x, (uint8_t)estado.pelota_y, PONG_PELOTA_LADO, PONG_PELOTA_LADO);

    snprintf(texto_puntaje, sizeof(texto_puntaje), "%d-%d", estado.puntos_izq, estado.puntos_der);
    sendString(texto_puntaje, 26, 2, 5, 7, (struct rgb){255, 255, 255});

    if (estado.juego_terminado) {
      const char *ganador = estado.puntos_izq > estado.puntos_der ? "GANASTE" : "PERDISTE";
      sendString((char *)ganador, 10, 30, 5, 7, (struct rgb){255, 0, 0});
    }

    SDL_RenderPresent(global_renderer);
    SDL_Delay(16);
  }

  SDL_Quit();
  return 0;
}
