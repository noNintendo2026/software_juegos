/* roadfighter_logica.h -- logica real del juego Road Fighter (punto 7:
 * "contenidos pendientes" -- el main.c original de roadFighter solo mueve
 * el auto libremente sin limites, sin combustible, sin trafico y sin
 * colisiones, aunque su propio README promete las 4 mecanicas clasicas:
 * carrera contra el reloj, combustible, trafico dinamico y choques).
 *
 * Deliberadamente SIN dependencia de hal.h/SDL -- así se puede compilar y
 * probar esta logica sola, sin ventana ni libreria grafica (mismo criterio
 * ya usado en ps2_keyboard.c/ps2_mouse.c/display_spi.c de hal_real/). El
 * archivo que sí dibuja en pantalla (RoadFighter_Mejorado.c) traduce las
 * acciones de hal.h a AccionRF y llama estas funciones.
 *
 * Los tamaños de los sprites (8x12 autos, 9x21 camion) y los limites de la
 * carretera (64x64, SimplerRoad/Road ya existentes en roadFighter/assets.h)
 * son los reales del repositorio -- no se inventaron dimensiones nuevas.
 */
#pragma once
#include <stdint.h>

typedef enum {
  RF_ACCION_NINGUNA = 0,
  RF_ACCION_ARRIBA,
  RF_ACCION_ABAJO,
  RF_ACCION_IZQUIERDA,
  RF_ACCION_DERECHA
} AccionRF;

typedef enum {
  RF_TRAFICO_AUTO_AZUL,
  RF_TRAFICO_AUTO_AMARILLO,
  RF_TRAFICO_CAMION  /* el camion de combustible real que menciona el README */
} TipoTraficoRF;

#define RF_MAX_TRAFICO      5
#define RF_MAX_CHOQUES      3     /* 3 choques con trafico civil = fin del juego */
#define RF_CARRETERA_IZQ    4     /* borde izquierdo real de la carretera dentro de los 64px (deja pasto/borde) */
#define RF_CARRETERA_DER    60    /* borde derecho real */
#define RF_JUGADOR_ANCHO    8     /* RedCar real: 8x12 (ver roadFighter/include/assets.h) */
#define RF_JUGADOR_ALTO     12
#define RF_AUTO_ANCHO       8     /* BlueCar/YellowCar reales: 8x12 */
#define RF_AUTO_ALTO        12
#define RF_CAMION_ANCHO     9     /* Truck real: 9x21 */
#define RF_CAMION_ALTO      21
#define RF_PANTALLA_ALTO    64
#define RF_DISTANCIA_META   2000  /* longitud de la etapa (decision de diseño, no un dato de hardware) */

typedef struct {
  TipoTraficoRF tipo;
  int16_t x, y;
  uint8_t activo;  /* 0 = fuera de juego, se puede reciclar en la siguiente actualizacion */
} EntidadTraficoRF;

typedef struct {
  int16_t jugador_x;
  int16_t jugador_y;
  int16_t combustible;      /* 0-100 */
  uint8_t marcha_alta;       /* 0=baja, 1=alta -- las "dos marchas" reales del README */
  uint32_t distancia;        /* puntaje = distancia recorrida */
  uint8_t choques;
  uint8_t juego_terminado;   /* se quedo sin combustible o choco demasiadas veces */
  uint8_t juego_ganado;      /* llego a RF_DISTANCIA_META */
  EntidadTraficoRF trafico[RF_MAX_TRAFICO];
  uint32_t prng_estado;      /* xorshift32 -- semilla explicita para que el trafico sea reproducible en pruebas */
} EstadoRoadFighterRF;

void rf_inicializar(EstadoRoadFighterRF *estado, uint32_t semilla);
void rf_actualizar(EstadoRoadFighterRF *estado, AccionRF accion);
