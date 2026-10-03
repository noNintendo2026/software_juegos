/* pong_logica.h -- logica real de un juego de Pong (punto 7: "contenidos
 * pendientes" -- la carpeta pong/ del repositorio no tiene un juego de Pong
 * real: pong/Space_Invaders.c es una copia byte-identica de
 * space_invaders/Space_Invaders.c, confirmado con `diff`).
 *
 * Implementa el Pong clasico (pelota + 2 paletas) desde cero: mecanica
 * generica de dominio publico, no se reproduce ningun elemento grafico o de
 * marca del Pong original de Atari -- las paletas y la pelota son
 * rectangulos solidos simples generados en codigo, sin ningun asset externo.
 *
 * Un jugador controla la paleta izquierda (ARRIBA/ABAJO). La paleta derecha
 * la controla una IA simple (sigue la pelota con velocidad limitada) --
 * decision honesta: el HAL todavia no tiene un segundo dispositivo de
 * entrada confiable para 2 jugadores locales (ver PENDIENTES_EXTERNOS.md,
 * el Chain Bus/UART para 2 FPGAs via red todavia no esta terminado).
 *
 * Sin dependencia de hal.h/SDL -- se puede probar sola (mismo criterio que
 * roadfighter_logica.c).
 */
#pragma once
#include <stdint.h>

typedef enum {
  PONG_ACCION_NINGUNA = 0,
  PONG_ACCION_ARRIBA,
  PONG_ACCION_ABAJO
} AccionPong;

#define PONG_ANCHO            64
#define PONG_ALTO             64
#define PONG_PALETA_ANCHO     2
#define PONG_PALETA_ALTO      12
#define PONG_PALETA_VELOCIDAD 2
#define PONG_PELOTA_LADO      3
#define PONG_PUNTOS_PARA_GANAR 5
#define PONG_IA_VELOCIDAD     1  /* mas lenta que el jugador -- IA vencible a proposito */

typedef struct {
  int16_t paleta_izq_y;   /* jugador humano */
  int16_t paleta_der_y;   /* IA */
  int16_t pelota_x, pelota_y;
  int8_t pelota_vx, pelota_vy;
  uint8_t puntos_izq, puntos_der;
  uint8_t juego_terminado;
  uint32_t prng_estado;   /* xorshift32 -- para el angulo inicial de saque, reproducible en pruebas */
} EstadoPong;

void pong_inicializar(EstadoPong *estado, uint32_t semilla);
void pong_actualizar(EstadoPong *estado, AccionPong accion_jugador);
