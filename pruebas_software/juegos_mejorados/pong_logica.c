/* pong_logica.c -- implementacion real de la mecanica de Pong (ver
 * pong_logica.h). Pelota rebota en paredes superior/inferior y en las 2
 * paletas; sale por la izquierda o la derecha para anotar punto.
 *
 * Prueba aislada (sin SDL, sin hal.h): gcc -DPONG_LOGICA_TEST pong_logica.c -o test_pong_logica.exe
 */
#include "pong_logica.h"

static uint32_t pong_xorshift32(uint32_t *estado) {
  uint32_t x = *estado;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *estado = x;
  return x;
}

static void pong_sacar(EstadoPong *estado) {
  estado->pelota_x = PONG_ANCHO / 2 - PONG_PELOTA_LADO / 2;
  estado->pelota_y = PONG_ALTO / 2 - PONG_PELOTA_LADO / 2;

  uint32_t r = pong_xorshift32(&estado->prng_estado);
  estado->pelota_vx = (r & 0x1u) ? 1 : -1;
  estado->pelota_vy = (int8_t)(((r >> 1) & 0x1u) ? 1 : -1);
}

void pong_inicializar(EstadoPong *estado, uint32_t semilla) {
  estado->paleta_izq_y = PONG_ALTO / 2 - PONG_PALETA_ALTO / 2;
  estado->paleta_der_y = PONG_ALTO / 2 - PONG_PALETA_ALTO / 2;
  estado->puntos_izq = 0;
  estado->puntos_der = 0;
  estado->juego_terminado = 0;
  estado->prng_estado = semilla ? semilla : 1u;
  pong_sacar(estado);
}

static void pong_clamp_paleta(int16_t *y) {
  if (*y < 0) *y = 0;
  if (*y > PONG_ALTO - PONG_PALETA_ALTO) *y = PONG_ALTO - PONG_PALETA_ALTO;
}

void pong_actualizar(EstadoPong *estado, AccionPong accion_jugador) {
  if (estado->juego_terminado) {
    return;
  }

  switch (accion_jugador) {
    case PONG_ACCION_ARRIBA: estado->paleta_izq_y -= PONG_PALETA_VELOCIDAD; break;
    case PONG_ACCION_ABAJO:  estado->paleta_izq_y += PONG_PALETA_VELOCIDAD; break;
    default: break;
  }
  pong_clamp_paleta(&estado->paleta_izq_y);

  /* IA simple: sigue el centro de la pelota, mas lenta que el jugador a proposito */
  int16_t centro_paleta_der = estado->paleta_der_y + PONG_PALETA_ALTO / 2;
  if (centro_paleta_der < estado->pelota_y) {
    estado->paleta_der_y += PONG_IA_VELOCIDAD;
  } else if (centro_paleta_der > estado->pelota_y) {
    estado->paleta_der_y -= PONG_IA_VELOCIDAD;
  }
  pong_clamp_paleta(&estado->paleta_der_y);

  estado->pelota_x += estado->pelota_vx;
  estado->pelota_y += estado->pelota_vy;

  if (estado->pelota_y <= 0) {
    estado->pelota_y = 0;
    estado->pelota_vy = (int8_t)(-estado->pelota_vy);
  } else if (estado->pelota_y >= PONG_ALTO - PONG_PELOTA_LADO) {
    estado->pelota_y = PONG_ALTO - PONG_PELOTA_LADO;
    estado->pelota_vy = (int8_t)(-estado->pelota_vy);
  }

  /* colision con la paleta izquierda (AABB real). El limite inferior
     `pelota_x >= 0` es necesario: sin el, una pelota que YA salio del campo
     de juego (x negativo, punto ya perdido) podia seguir "rebotando" si la
     paleta se movia hacia esa altura en el mismo instante -- encontrado por
     la prueba de abajo, no es un caso hipotetico. */
  if (estado->pelota_vx < 0 &&
      estado->pelota_x >= 0 &&
      estado->pelota_x <= PONG_PALETA_ANCHO &&
      estado->pelota_y + PONG_PELOTA_LADO > estado->paleta_izq_y &&
      estado->pelota_y < estado->paleta_izq_y + PONG_PALETA_ALTO) {
    estado->pelota_x = PONG_PALETA_ANCHO;
    estado->pelota_vx = (int8_t)(-estado->pelota_vx);
  }

  /* colision con la paleta derecha -- mismo limite superior simetrico */
  if (estado->pelota_vx > 0 &&
      estado->pelota_x + PONG_PELOTA_LADO <= PONG_ANCHO &&
      estado->pelota_x + PONG_PELOTA_LADO >= PONG_ANCHO - PONG_PALETA_ANCHO &&
      estado->pelota_y + PONG_PELOTA_LADO > estado->paleta_der_y &&
      estado->pelota_y < estado->paleta_der_y + PONG_PALETA_ALTO) {
    estado->pelota_x = (int16_t)(PONG_ANCHO - PONG_PALETA_ANCHO - PONG_PELOTA_LADO);
    estado->pelota_vx = (int8_t)(-estado->pelota_vx);
  }

  if (estado->pelota_x < 0) {
    estado->puntos_der++;
    if (estado->puntos_der >= PONG_PUNTOS_PARA_GANAR) {
      estado->juego_terminado = 1;
    } else {
      pong_sacar(estado);
    }
  } else if (estado->pelota_x > PONG_ANCHO) {
    estado->puntos_izq++;
    if (estado->puntos_izq >= PONG_PUNTOS_PARA_GANAR) {
      estado->juego_terminado = 1;
    } else {
      pong_sacar(estado);
    }
  }
}

/* ===================================================================================
 * Prueba aislada -- rebote en paredes, rebote en paletas, y que el juego
 * termine cuando alguien llega a PONG_PUNTOS_PARA_GANAR.
 * =================================================================================== */
#ifdef PONG_LOGICA_TEST
#include <stdio.h>

static void probar_rebote_pared_superior(void) {
  EstadoPong e;
  pong_inicializar(&e, 1u);
  e.pelota_y = 0;
  e.pelota_vy = -3;
  pong_actualizar(&e, PONG_ACCION_NINGUNA);
  int ok = e.pelota_vy > 0;
  printf("Prueba: pelota rebota en la pared superior                %s\n", ok ? "[OK]" : "[FALLO]");
}

static void probar_rebote_paleta_izquierda(void) {
  EstadoPong e;
  pong_inicializar(&e, 2u);
  e.paleta_izq_y = 20;
  e.pelota_x = PONG_PALETA_ANCHO + 1;
  e.pelota_y = 22;
  e.pelota_vx = -1;
  e.pelota_vy = 0;
  pong_actualizar(&e, PONG_ACCION_NINGUNA);
  int ok = e.pelota_vx > 0;
  printf("Prueba: pelota rebota en la paleta izquierda (jugador)     %s\n", ok ? "[OK]" : "[FALLO]");
}

static void probar_punto_y_fin_del_juego(void) {
  EstadoPong e;
  pong_inicializar(&e, 3u);
  e.puntos_der = (uint8_t)(PONG_PUNTOS_PARA_GANAR - 1);
  /* paleta lejos de donde va a pasar la pelota -- asi se prueba un fallo
     real (sin rebote), no un rebote accidental como en la version anterior
     de esta prueba (que SI encontro el bug ya corregido arriba). */
  e.paleta_izq_y = 0;
  e.pelota_x = 1;
  e.pelota_y = PONG_ALTO - 1;
  e.pelota_vx = -1;
  e.pelota_vy = 0;

  int ticks = 0;
  while (!e.juego_terminado && ticks < 100) {
    pong_actualizar(&e, PONG_ACCION_NINGUNA);
    ticks++;
  }

  int ok = e.puntos_der == PONG_PUNTOS_PARA_GANAR && e.juego_terminado == 1;
  printf("Prueba: fallo real (sin rebote) anota punto y termina el juego (puntos_der=%d)  %s\n",
        e.puntos_der, ok ? "[OK]" : "[FALLO]");
}

int main(void) {
  printf("=== Prueba real de la logica de Pong (sin SDL, sin hal.h) ===\n\n");
  probar_rebote_pared_superior();
  probar_rebote_paleta_izquierda();
  probar_punto_y_fin_del_juego();
  printf("\nSi todas dicen [OK], la pelota rebota correctamente en paredes y paletas,\n"
        "y el juego termina al llegar a %d puntos.\n", PONG_PUNTOS_PARA_GANAR);
  return 0;
}
#endif
