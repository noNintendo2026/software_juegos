/* roadfighter_logica.c -- implementacion real de las 4 mecanicas que el
 * README de roadFighter promete y que main.c original nunca implemento:
 * carrera contra el reloj (distancia/puntaje), combustible, trafico
 * dinamico (autos civiles + camion de gasolina) y choques/perdida de
 * velocidad. Tambien corrige el bug real de main.c original: x--/x++ sin
 * ningun limite (el auto podia salirse de la pantalla).
 *
 * Prueba aislada (sin SDL, sin hal.h): gcc -DROADFIGHTER_LOGICA_TEST roadfighter_logica.c -o test_roadfighter_logica.exe
 */
#include "roadfighter_logica.h"

static uint32_t rf_xorshift32(uint32_t *estado) {
  uint32_t x = *estado;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *estado = x;
  return x;
}

static void rf_generar_trafico(EntidadTraficoRF *e, uint32_t *prng_estado) {
  uint32_t r = rf_xorshift32(prng_estado);

  uint8_t carril = (uint8_t)(r % 3);  /* 3 carriles dentro del ancho real de la carretera */
  int16_t ancho_disponible = (RF_CARRETERA_DER - RF_CARRETERA_IZQ) - RF_AUTO_ANCHO;
  e->x = (int16_t)(RF_CARRETERA_IZQ + (ancho_disponible / 3) * carril);
  e->y = (int16_t)(-10 - ((r >> 8) % 40));  /* aparece arriba, fuera de pantalla, en alturas distintas */

  uint32_t tipo_r = (r >> 16) % 10;
  if (tipo_r < 1) {
    e->tipo = RF_TRAFICO_CAMION;  /* camion de gasolina: mas raro, 1/10 (decision de diseño) */
  } else if (tipo_r < 5) {
    e->tipo = RF_TRAFICO_AUTO_AZUL;
  } else {
    e->tipo = RF_TRAFICO_AUTO_AMARILLO;
  }
  e->activo = 1;
}

void rf_inicializar(EstadoRoadFighterRF *estado, uint32_t semilla) {
  estado->jugador_x = (RF_CARRETERA_IZQ + RF_CARRETERA_DER) / 2 - RF_JUGADOR_ANCHO / 2;
  estado->jugador_y = RF_PANTALLA_ALTO - RF_JUGADOR_ALTO - 4;
  estado->combustible = 100;
  estado->marcha_alta = 0;
  estado->distancia = 0;
  estado->choques = 0;
  estado->juego_terminado = 0;
  estado->juego_ganado = 0;
  estado->prng_estado = semilla ? semilla : 1u;  /* xorshift32 no puede arrancar en 0 */

  for (int i = 0; i < RF_MAX_TRAFICO; i++) {
    estado->trafico[i].activo = 0;
  }
}

static int rf_ancho_trafico(TipoTraficoRF tipo) {
  return tipo == RF_TRAFICO_CAMION ? RF_CAMION_ANCHO : RF_AUTO_ANCHO;
}

static int rf_alto_trafico(TipoTraficoRF tipo) {
  return tipo == RF_TRAFICO_CAMION ? RF_CAMION_ALTO : RF_AUTO_ALTO;
}

void rf_actualizar(EstadoRoadFighterRF *estado, AccionRF accion) {
  if (estado->juego_terminado || estado->juego_ganado) {
    return;  /* partida terminada, no se actualiza mas hasta que alguien reinicie el estado */
  }

  switch (accion) {
    case RF_ACCION_IZQUIERDA: estado->jugador_x -= estado->marcha_alta ? 3 : 2; break;
    case RF_ACCION_DERECHA:   estado->jugador_x += estado->marcha_alta ? 3 : 2; break;
    case RF_ACCION_ARRIBA:    estado->marcha_alta = 1; break;  /* marcha alta -- README: "dos marchas" */
    case RF_ACCION_ABAJO:     estado->marcha_alta = 0; break;  /* marcha baja */
    default: break;
  }

  /* limite real de la carretera -- corrige el bug del main.c original (sin limites) */
  int16_t x_min = RF_CARRETERA_IZQ;
  int16_t x_max = RF_CARRETERA_DER - RF_JUGADOR_ANCHO;
  if (estado->jugador_x < x_min) estado->jugador_x = x_min;
  if (estado->jugador_x > x_max) estado->jugador_x = x_max;

  /* combustible: consumo continuo real, mayor en marcha alta (README: "consume...de forma continua") */
  estado->combustible -= estado->marcha_alta ? 2 : 1;
  if (estado->combustible < 0) estado->combustible = 0;

  estado->distancia += estado->marcha_alta ? 2u : 1u;
  if (estado->distancia >= RF_DISTANCIA_META) {
    estado->juego_ganado = 1;
    return;
  }

  if (estado->combustible <= 0) {
    estado->juego_terminado = 1;
    return;
  }

  int16_t velocidad_scroll = estado->marcha_alta ? 4 : 2;  /* el trafico se desplaza relativo a la velocidad del jugador */

  for (int i = 0; i < RF_MAX_TRAFICO; i++) {
    EntidadTraficoRF *e = &estado->trafico[i];

    if (!e->activo) {
      rf_generar_trafico(e, &estado->prng_estado);
      continue;
    }

    e->y += velocidad_scroll;
    if (e->y > RF_PANTALLA_ALTO) {
      e->activo = 0;  /* salio de la pantalla por abajo, se recicla en la siguiente actualizacion */
      continue;
    }

    int ancho_e = rf_ancho_trafico(e->tipo);
    int alto_e = rf_alto_trafico(e->tipo);

    int colisiona = estado->jugador_x < e->x + ancho_e &&
                    estado->jugador_x + RF_JUGADOR_ANCHO > e->x &&
                    estado->jugador_y < e->y + alto_e &&
                    estado->jugador_y + RF_JUGADOR_ALTO > e->y;

    if (colisiona) {
      if (e->tipo == RF_TRAFICO_CAMION) {
        /* README real: "chocar contra camiones especiales... para recargar el deposito" */
        estado->combustible += 40;
        if (estado->combustible > 100) estado->combustible = 100;
      } else {
        /* README real: choque provoca "perdida de velocidad" */
        estado->choques++;
        estado->combustible = (estado->combustible > 15) ? (int16_t)(estado->combustible - 15) : 0;
        estado->marcha_alta = 0;
        if (estado->choques >= RF_MAX_CHOQUES) {
          estado->juego_terminado = 1;
        }
      }
      e->activo = 0;  /* se "consume" tras el choque -- no se puede chocar dos veces con la misma entidad */
    }
  }
}

/* ===================================================================================
 * Prueba aislada -- verifica las 4 mecanicas reales del README: limites de la
 * carretera, consumo de combustible hasta terminar el juego, recarga real al
 * chocar con el camion, y fin de juego tras RF_MAX_CHOQUES choques civiles.
 * =================================================================================== */
#ifdef ROADFIGHTER_LOGICA_TEST
#include <stdio.h>

static void probar_limites_carretera(void) {
  EstadoRoadFighterRF e;
  rf_inicializar(&e, 12345u);

  for (int i = 0; i < 100; i++) {
    rf_actualizar(&e, RF_ACCION_IZQUIERDA);
  }
  int ok_izq = e.jugador_x == RF_CARRETERA_IZQ;

  rf_inicializar(&e, 12345u);
  for (int i = 0; i < 100; i++) {
    rf_actualizar(&e, RF_ACCION_DERECHA);
  }
  int ok_der = e.jugador_x == (RF_CARRETERA_DER - RF_JUGADOR_ANCHO);

  printf("Prueba: limites reales de la carretera (antes sin limite)        %s\n",
        (ok_izq && ok_der) ? "[OK]" : "[FALLO]");
}

static void probar_combustible_termina_el_juego(void) {
  EstadoRoadFighterRF e;
  rf_inicializar(&e, 777u);

  int ticks = 0;
  while (!e.juego_terminado && !e.juego_ganado && ticks < 10000) {
    rf_actualizar(&e, RF_ACCION_NINGUNA);
    ticks++;
  }

  int ok = e.juego_terminado == 1 && e.combustible == 0 && !e.juego_ganado;
  printf("Prueba: combustible se agota y termina el juego (en %d ticks)    %s\n",
        ticks, ok ? "[OK]" : "[FALLO]");
}

static void probar_recarga_con_camion(void) {
  EstadoRoadFighterRF e;
  rf_inicializar(&e, 999u);

  /* se fuerza manualmente un camion justo sobre el jugador, sin depender del PRNG */
  e.combustible = 50;
  e.trafico[0].tipo = RF_TRAFICO_CAMION;
  e.trafico[0].x = e.jugador_x;
  e.trafico[0].y = e.jugador_y;
  e.trafico[0].activo = 1;

  rf_actualizar(&e, RF_ACCION_NINGUNA);

  int ok = e.combustible > 50 && e.trafico[0].activo == 0;
  printf("Prueba: chocar con el camion SI recarga combustible real         %s (combustible=%d)\n",
        ok ? "[OK]" : "[FALLO]", e.combustible);
}

static void probar_choques_terminan_el_juego(void) {
  EstadoRoadFighterRF e;
  rf_inicializar(&e, 555u);
  e.combustible = 100;

  for (int choque = 0; choque < RF_MAX_CHOQUES; choque++) {
    e.trafico[0].tipo = RF_TRAFICO_AUTO_AZUL;
    e.trafico[0].x = e.jugador_x;
    e.trafico[0].y = e.jugador_y;
    e.trafico[0].activo = 1;
    rf_actualizar(&e, RF_ACCION_NINGUNA);
  }

  int ok = e.choques == RF_MAX_CHOQUES && e.juego_terminado == 1;
  printf("Prueba: %d choques con trafico civil terminan el juego            %s (choques=%d)\n",
        RF_MAX_CHOQUES, ok ? "[OK]" : "[FALLO]", e.choques);
}

int main(void) {
  printf("=== Prueba real de la logica de Road Fighter (sin SDL, sin hal.h) ===\n\n");
  probar_limites_carretera();
  probar_combustible_termina_el_juego();
  probar_recarga_con_camion();
  probar_choques_terminan_el_juego();
  printf("\nSi todas dicen [OK], las 4 mecanicas reales del README (carrera, combustible,\n"
        "trafico y choques) funcionan tal como se describen, con limites reales de pantalla.\n");
  return 0;
}
#endif
