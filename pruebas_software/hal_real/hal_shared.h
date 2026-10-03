/* hal_shared.h -- contrato UNIFICADO del HAL (punto 1 de los 8 problemas reales).
 *
 * Arregla el bug real encontrado: antes, readControls() tenia una firma distinta en el
 * bloque #ifndef FPGA (simulador) que en el bloque #ifdef FPGA (hardware real) -- el
 * codigo del juego escrito contra una NO compilaba contra la otra. Esta version define
 * UNA sola firma por funcion, valida para ambos mundos, y separa claramente lo que es
 * "solo del simulador" (ventana SDL, bindings de teclado de PC) de lo que es real HAL
 * (entradas/salidas que tambien existen en el hardware).
 *
 * Direcciones reales del mapa de memoria (del .github de la organizacion,
 * noNintendo2026) -- version ASUMIDA del layout de registros DENTRO de cada rango: cada
 * equipo de hardware solo documento el PROTOCOLO (temporizacion, formato de trama), no
 * el offset exacto de sus registros dentro de su rango de 64KB. Los offsets de abajo son
 * una PROPUESTA razonable (registro de datos + registro de estado/control, el patron
 * estandar de un periferico mapeado en memoria), NO un hecho confirmado -- hay que
 * validarlos con cada equipo antes de usarlos contra hardware real. Se marcan con
 * "ASUMIDO" explicitamente para no mezclarlos con lo que si esta confirmado.
 */
#pragma once
#include <stdint.h>

/* ===================================================================================
 * Mapa de memoria real (CONFIRMADO -- .github/profile/README.md de la organizacion)
 * =================================================================================== */
#define MEM_BRAM_BASE        0x000000u  /* software_juegos: BRAM, arranque/firmware/juegos */
#define MEM_UART_BASE         0x400000u  /* UART: debug + MultijugadorRed */
#define MEM_SPI_FLASH_BASE    0x420000u  /* spi_flash_ctrl: almacenamiento de juegos */
#define MEM_PS2_KBD_BASE      0x430000u  /* ps2_keyboard.v */
#define MEM_PS2_MOUSE_BASE    0x440000u  /* ps2_mouse */
#define MEM_NES_BASE          0x450000u  /* nes_controller.v -- protocolo AUN SIN DOCUMENTAR */
#define MEM_I2C_BASE          0x460000u  /* I2C_Master: guardado de puntajes */
#define MEM_I2S_BASE          0x470000u  /* I2S_tx.v: audio (solo salida) */
#define MEM_DISPLAY_BASE      0x480000u  /* Display-Driver: framebuffer via SPI */

/* ===================================================================================
 * Offsets de registro DENTRO de cada rango -- ASUMIDOS, pendientes de confirmar
 * =================================================================================== */
#define REG_OFFSET_DATA       0x00u   /* ASUMIDO: registro de datos (escribir/leer byte o palabra) */
#define REG_OFFSET_STATUS     0x04u   /* ASUMIDO: bit 0 = dato listo (RX) o listo para enviar (TX) */
#define REG_OFFSET_CONTROL    0x08u   /* ASUMIDO: bit 0 = iniciar transmision / habilitar */

/* Acceso real a un registro mapeado en memoria (unico lugar del codigo que toca el bus
 * directamente -- antes NINGUN archivo .c del repo hacia esto, era el hueco real del
 * punto 6). */
#define MMIO32(direccion) (*(volatile uint32_t *)(uintptr_t)(direccion))

/* ===================================================================================
 * Color -- YA CORRECTO en el hal.h original, se conserva igual (RGB565 real)
 * =================================================================================== */
struct rgb {
  uint8_t r, g, b;
};

static inline struct rgb hex2rgb(uint16_t hexColor) {
  struct rgb c;
  uint8_t r5 = (hexColor >> 11) & 0x1F;
  uint8_t g6 = (hexColor >> 5) & 0x3F;
  uint8_t b5 = hexColor & 0x1F;
  c.r = (uint8_t)((r5 * 255) / 31);
  c.g = (uint8_t)((g6 * 255) / 63);
  c.b = (uint8_t)((b5 * 255) / 31);
  return c;
}

static inline uint16_t rgb2hex(struct rgb c) {
  uint16_t r5 = (uint16_t)((c.r * 31) / 255);
  uint16_t g6 = (uint16_t)((c.g * 63) / 255);
  uint16_t b5 = (uint16_t)((c.b * 31) / 255);
  return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

/* ===================================================================================
 * Entradas -- contrato UNICO, igual en simulador y en hardware real
 * =================================================================================== */
typedef enum {
  ACTION_NONE = 0,
  ACTION_UP,
  ACTION_DOWN,
  ACTION_RIGHT,
  ACTION_LEFT,
  ACTION_START,
  ACTION_SELECT
} Action;

/* FIX real: antes readControls() tenia 2 firmas incompatibles. Ahora es UNA sola,
 * valida para los dos mundos. El simulador ignora internamente los detalles de SDL
 * (bindings, running) -- esos quedan en una funcion aparte, SOLO del simulador, que no
 * es parte del contrato real de hardware (ver hal_sdl_only.h). */
Action pollKeyboard(void);

/* NUEVO (punto 3: no existia ningun tipo de dato para el mouse). Basado en el protocolo
 * real documentado por el equipo de ps2_mouse: paquete de 3-4 bytes, X/Y en complemento
 * a 2, hasta 5 botones (modo IntelliMouse). */
typedef struct {
  int16_t dx;           /* movimiento relativo en X, con signo, protocolo real PS/2 */
  int16_t dy;           /* movimiento relativo en Y, con signo */
  int8_t  wheel;         /* rueda, IntelliMouse (-8..+7), 0 si no hay rueda */
  uint8_t left : 1;
  uint8_t right : 1;
  uint8_t middle : 1;
  uint8_t button4 : 1;   /* IntelliMouse 5 botones */
  uint8_t button5 : 1;
  uint8_t packet_valido : 1;  /* 0 si no habia paquete completo nuevo todavia */
} MouseState;

MouseState pollMouse(void);

/* PENDIENTE real: el control NES no tiene protocolo documentado por ese equipo todavia
 * (su repo solo tiene el titulo). No se declara ninguna funcion aqui a proposito --
 * agregarla ahora seria inventar un contrato sin base real. Ver PENDIENTES_EXTERNOS.md. */

/* ===================================================================================
 * Salida de pantalla -- incluye funciones compartidas (iguales en ambos mundos)
 * =================================================================================== */
void sendSprite(const uint16_t *sprite, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void sendBackground(const uint16_t *sprite);
void sendLetter(char s, uint8_t x, uint8_t y, uint8_t w, uint8_t h, struct rgb color);
void sendString(char *string, uint8_t x, uint8_t y, uint8_t w, uint8_t h, struct rgb color);

/* Resolucion real confirmada por el protocolo HUB75 (64 pulsos de CLK por fila x 32
 * combinaciones de A-E x 2 mitades simultaneas = 64 filas). Point 2 de los 8 problemas:
 * Space Invaders todavia usa 800x600 / sprites de 30x30, incompatible con esto. */
#define DISPLAY_WIDTH_REAL  64
#define DISPLAY_HEIGHT_REAL 64
