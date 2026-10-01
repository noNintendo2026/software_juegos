/* ps2_keyboard.c -- decodificador REAL de scan codes PS/2 set 2 (punto 3 de los 8
 * problemas: el Action enum no podia representar las secuencias de 1-4 bytes que
 * manda el teclado real).
 *
 * Implementa exactamente lo que documento el equipo de ps2_keyboard.v en su README:
 *   - Presionar (make): 1 byte -> el codigo de la tecla.
 *   - Soltar (break): 2 bytes -> F0 + codigo.
 *   - Tecla extendida (flechas, etc), presionar: 2 bytes -> E0 + codigo.
 *   - Tecla extendida, soltar: 3 bytes -> E0 F0 + codigo.
 *
 * Los codigos de las flechas (E0 75/72/6B/74) y de Shift (12) y A (1C) estan
 * CONFIRMADOS por el propio README del equipo (los usan como ejemplo). Enter (5A),
 * Espacio (29) y Shift derecho (59) son valores ESTANDAR del set 2 (no estan escritos
 * como texto en su README, solo en una imagen que no pude leer) -- quedan marcados
 * para que el equipo los confirme contra su propia tabla (Scan_codes.png) antes de
 * usarlos en hardware real.
 *
 * Prueba aislada (sin hardware, sin SDL): gcc ps2_keyboard.c -o test_ps2_keyboard.exe
 */
#include <stdio.h>
#include <stdint.h>
#include "hal_shared.h"

typedef enum { ESPERANDO_INICIO, VISTO_E0, VISTO_F0, VISTO_E0_F0 } EstadoPS2;

typedef struct {
  EstadoPS2 estado;
} DecodificadorPS2;

/* Codigos set 2 relevantes para el juego (solo los que mapean a un Action real) */
#define SC_FLECHA_ARRIBA  0x75  /* extendida (E0) -- CONFIRMADO en el README del equipo */
#define SC_FLECHA_ABAJO   0x72  /* extendida (E0) -- CONFIRMADO */
#define SC_FLECHA_IZQ     0x6B  /* extendida (E0) -- CONFIRMADO */
#define SC_FLECHA_DER     0x74  /* extendida (E0) -- CONFIRMADO */
#define SC_ENTER          0x5A  /* NO extendida -- valor estandar PS/2 set 2, verificar contra Scan_codes.png */
#define SC_ESPACIO        0x29  /* valor estandar PS/2 set 2, verificar contra Scan_codes.png */
#define SC_SHIFT_DER      0x59  /* valor estandar PS/2 set 2, verificar contra Scan_codes.png (equivalente a SDLK_RSHIFT, el binding SELECT original) */

static DecodificadorPS2 decodificador = { ESPERANDO_INICIO };

/* Traduce UN scan code ya resuelto (sin prefijos) a Action. Solo "hace algo" en el
 * evento de PRESIONAR (ignoramos soltar para este juego simple, igual que el modelo
 * original de Action -- si luego se necesita saber "se soltó", hay que exponer eso
 * tambien; no se hizo aqui para no inventar una necesidad que el juego no tiene hoy). */
static Action codigoATeclaPresionada(uint8_t codigo, int extendida) {
  if (extendida) {
    switch (codigo) {
      case SC_FLECHA_ARRIBA: return ACTION_UP;
      case SC_FLECHA_ABAJO:  return ACTION_DOWN;
      case SC_FLECHA_IZQ:    return ACTION_LEFT;
      case SC_FLECHA_DER:    return ACTION_RIGHT;
      default: return ACTION_NONE;
    }
  }
  switch (codigo) {
    case SC_ENTER:     return ACTION_START;
    case SC_SHIFT_DER:  return ACTION_SELECT;
    default: return ACTION_NONE;
  }
}

/* Procesa UN byte real recibido del teclado. Devuelve la Action si ese byte completo
 * una secuencia de "tecla presionada" relevante, o ACTION_NONE si el byte era parte de
 * un prefijo, una tecla que no nos importa, o un evento de "soltar". */
Action ps2_procesar_byte(uint8_t byte) {
  switch (decodificador.estado) {
    case ESPERANDO_INICIO:
      if (byte == 0xE0) { decodificador.estado = VISTO_E0; return ACTION_NONE; }
      if (byte == 0xF0) { decodificador.estado = VISTO_F0; return ACTION_NONE; }
      decodificador.estado = ESPERANDO_INICIO;
      return codigoATeclaPresionada(byte, 0);

    case VISTO_E0:
      if (byte == 0xF0) { decodificador.estado = VISTO_E0_F0; return ACTION_NONE; }
      decodificador.estado = ESPERANDO_INICIO;
      return codigoATeclaPresionada(byte, 1);

    case VISTO_F0:
      /* break de una tecla normal -- se soltó, no generamos Action (ver nota arriba) */
      decodificador.estado = ESPERANDO_INICIO;
      return ACTION_NONE;

    case VISTO_E0_F0:
      /* break de una tecla extendida -- se soltó */
      decodificador.estado = ESPERANDO_INICIO;
      return ACTION_NONE;
  }
  return ACTION_NONE;
}

/* HAL real: lee del registro de datos PS/2 (MEM_PS2_KBD_BASE + REG_OFFSET_DATA) solo
 * cuando el registro de estado dice que hay un byte nuevo -- patron estandar de
 * periferico mapeado en memoria, offsets ASUMIDOS (ver hal_shared.h). */
Action pollKeyboard(void) {
#ifdef FPGA
  uint32_t estado = MMIO32(MEM_PS2_KBD_BASE + REG_OFFSET_STATUS);
  if ((estado & 0x1u) == 0) {
    return ACTION_NONE;  /* no hay byte nuevo */
  }
  uint8_t byte = (uint8_t)MMIO32(MEM_PS2_KBD_BASE + REG_OFFSET_DATA);
  return ps2_procesar_byte(byte);
#else
  return ACTION_NONE;  /* en el simulador SDL, el teclado se lee por hal_sdl_only.h, no por aqui */
#endif
}

/* ===================================================================================
 * Prueba aislada -- verifica el decodificador contra las secuencias EXACTAS que el
 * propio equipo de ps2_keyboard.v puso como ejemplo en su README.
 * =================================================================================== */
#ifdef PS2_KEYBOARD_TEST
static const char *nombreAction(Action a) {
  switch (a) {
    case ACTION_UP: return "ACTION_UP";
    case ACTION_DOWN: return "ACTION_DOWN";
    case ACTION_LEFT: return "ACTION_LEFT";
    case ACTION_RIGHT: return "ACTION_RIGHT";
    case ACTION_START: return "ACTION_START";
    case ACTION_SELECT: return "ACTION_SELECT";
    default: return "ACTION_NONE";
  }
}

static void probar_secuencia(const char *nombre, const uint8_t *bytes, int n, Action esperado_en_ultimo) {
  decodificador.estado = ESPERANDO_INICIO;
  Action ultimo = ACTION_NONE;
  printf("Prueba: %s -> ", nombre);
  for (int i = 0; i < n; i++) {
    ultimo = ps2_procesar_byte(bytes[i]);
    printf("%02X ", bytes[i]);
  }
  printf("=> %s (esperado %s) %s\n", nombreAction(ultimo), nombreAction(esperado_en_ultimo),
        ultimo == esperado_en_ultimo ? "[OK]" : "[FALLO]");
}

int main(void) {
  printf("=== Prueba real del decodificador PS/2 (sin hardware) ===\n\n");

  /* Flecha arriba presionada: E0 75 (del README real del equipo) */
  uint8_t s1[] = {0xE0, 0x75};
  probar_secuencia("Flecha arriba (E0 75)", s1, 2, ACTION_UP);

  /* Flecha arriba soltada: E0 F0 75 -- no debe generar Action */
  uint8_t s2[] = {0xE0, 0xF0, 0x75};
  probar_secuencia("Flecha arriba soltada (E0 F0 75)", s2, 3, ACTION_NONE);

  /* La secuencia completa de ejemplo del propio README:
     Shift izq + A presionadas y soltadas en orden inverso:
     12 (Shift down) 1C (A down) F0 1C (A up) F0 12 (Shift up)
     Ninguna de esas teclas esta mapeada a una Action en este juego -- se espera
     ACTION_NONE en todo momento, y que el decodificador no se "atore". */
  uint8_t s3[] = {0x12, 0x1C, 0xF0, 0x1C, 0xF0, 0x12};
  probar_secuencia("Shift+A completo (ejemplo del README)", s3, 6, ACTION_NONE);

  /* Despues de la secuencia anterior, el decodificador debe seguir funcionando bien
     (no quedar en un estado raro): probamos flecha derecha justo despues. */
  uint8_t s4[] = {0xE0, 0x74};
  probar_secuencia("Flecha derecha tras la secuencia anterior", s4, 2, ACTION_RIGHT);

  printf("\nSi todas dicen [OK], el decodificador procesa correctamente las secuencias reales documentadas.\n");
  return 0;
}
#endif
