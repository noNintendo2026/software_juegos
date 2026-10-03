/* display_spi.c -- driver SPI real para el Display-Driver (punto 2 de los 8
 * problemas: en el hal.h original, sendSprite/sendBackground dentro del
 * bloque #ifdef FPGA eran cuerpos vacios con el codigo real comentado --
 * `// DIR_X = x; // SPR_DIR = spriteAddress;` -- nunca se implemento el envio
 * real por SPI).
 *
 * Protocolo real documentado por el equipo de Display-Driver (su repo,
 * "01 - Display Driver timing controller.md"): "La logica del sistema no
 * transmite comandos de alto nivel... sino que efectua escrituras directas
 * sobre coordenadas especificas de la memoria de video", mediante una
 * "trama de 32 bits (Comando + Coordenada + Color)" enviada por SPI (CPU =
 * maestro, FPGA del display = esclavo).
 *
 * Lo que el equipo documento: QUE la trama es de 32 bits y QUE lleva esos 3
 * campos. Lo que NO documento: el ancho exacto de cada campo dentro de esos
 * 32 bits. El reparto de abajo es ASUMIDO (ver PENDIENTES_EXTERNOS.md, punto
 * 6 actualizado) a partir de la unica restriccion real confirmada: la
 * pantalla es de 64x64 (protocolo HUB75 -- 64 pulsos de CLK x 32
 * direcciones A-E x 2 mitades simultaneas), asi que X e Y necesitan 6 bits
 * cada uno como minimo para cubrir 0-63:
 *
 *   bits [31:28] Comando (4 bits)       -- ASUMIDO, unico valor usado hoy: CMD_SET_PIXEL
 *   bits [27:22] X (6 bits, 0-63)        -- ASUMIDO
 *   bits [21:16] Y (6 bits, 0-63)        -- ASUMIDO
 *   bits [15:0]  Color RGB565 (16 bits)  -- formato YA CONFIRMADO (ver hal_shared.h)
 *
 * El acceso al registro (MEM_DISPLAY_BASE + REG_OFFSET_DATA/STATUS) reusa el
 * mismo patron CSR que el equipo de UART SI documento para su propio
 * periferico (escribir TX_DATA dispara el envio; STATUS bit0 = TX_BUSY). Se
 * copia ese patron aqui por consistencia entre perifericos, pero sigue
 * ASUMIDO para Display-Driver en particular hasta que ese equipo lo
 * confirme.
 *
 * Las funciones publicas (sendSprite/sendBackground/sendLetter/sendString)
 * solo se definen bajo FPGA: en el simulador cada juego ya usa su propio
 * hal.h con SDL (funcionando), asi que aqui no se declara nada para ese
 * caso -- evita simbolos duplicados si algun dia este archivo se linkea
 * junto a un juego.
 *
 * Prueba aislada de la logica de empaquetado de bits (no requiere FPGA ni
 * toca memoria real): gcc -DDISPLAY_SPI_TEST display_spi.c -o test_display_spi.exe
 */
#include <string.h>
#include <stdint.h>
#include "hal_shared.h"
#include "basic_font.h"

#define DISPLAY_CMD_SET_PIXEL 0x1u    /* ASUMIDO -- unico comando documentado: escritura directa de un pixel */
#define DISPLAY_COLORKEY      0x0843u /* mismo colorkey "transparente" que ya usa el simulador SDL en hal.h -- se preserva para que sprites/letras se vean igual en ambos mundos */

/* Pura (sin tocar hardware) para poder probarla sin -DFPGA y sin memoria real. */
uint32_t display_construir_trama(uint8_t x, uint8_t y, uint16_t colorRGB565) {
  uint32_t trama = 0;
  trama |= ((uint32_t)(DISPLAY_CMD_SET_PIXEL & 0xFu)) << 28;
  trama |= ((uint32_t)(x & 0x3Fu)) << 22;
  trama |= ((uint32_t)(y & 0x3Fu)) << 16;
  trama |= (uint32_t)colorRGB565;
  return trama;
}

#ifdef FPGA

static void esperar_tx_libre(void) {
  /* ASUMIDO: mismo bit0=TX_BUSY que documento el equipo de UART para su propio STATUS */
  while (MMIO32(MEM_DISPLAY_BASE + REG_OFFSET_STATUS) & 0x1u) {
    /* esperar a que termine de transmitir la trama anterior */
  }
}

static void escribir_pixel_real(uint8_t x, uint8_t y, uint16_t colorRGB565) {
  if (x >= DISPLAY_WIDTH_REAL || y >= DISPLAY_HEIGHT_REAL) {
    return;  /* fuera de la pantalla real de 64x64 -- no tiene sentido mandarlo */
  }
  esperar_tx_libre();
  MMIO32(MEM_DISPLAY_BASE + REG_OFFSET_DATA) = display_construir_trama(x, y, colorRGB565);
}

void sendSprite(const uint16_t *sprite, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  for (uint8_t row = 0; row < h; row++) {
    for (uint8_t col = 0; col < w; col++) {
      uint16_t pixelColor = sprite[(uint16_t)row * w + col];
      if (pixelColor == DISPLAY_COLORKEY) {
        continue;  /* transparente, igual que en el simulador SDL (hal.h) */
      }
      escribir_pixel_real((uint8_t)(x + col), (uint8_t)(y + row), pixelColor);
    }
  }
}

void sendBackground(const uint16_t *sprite) {
  sendSprite(sprite, 0, 0, DISPLAY_WIDTH_REAL, DISPLAY_HEIGHT_REAL);
}

void sendLetter(char s, uint8_t x, uint8_t y, uint8_t w, uint8_t h, struct rgb color) {
  if (s < 32 || s > 126) {
    s = '?';
  }
  uint16_t colorOn = rgb2hex(color);
  int font_index = (s - 32) * 5;  /* el font real (usg_font_5x7) es de 5 columnas por caracter */

  for (uint8_t col = 0; col < w && col < 5; col++) {
    uint8_t column_data = usg_font_5x7[font_index + col];
    for (uint8_t row = 0; row < h && row < 7; row++) {
      int bit = (column_data >> row) & 1;
      if (!bit) {
        continue;  /* fondo transparente -- mismo comportamiento que el simulador SDL */
      }
      escribir_pixel_real((uint8_t)(x + col), (uint8_t)(y + row), colorOn);
    }
  }
}

void sendString(char *string, uint8_t x, uint8_t y, uint8_t w, uint8_t h, struct rgb color) {
  uint8_t text_spacing = 2;  /* mismo espaciado que usa el simulador SDL en hal.h */
  size_t len = strlen(string);
  for (size_t i = 0; i < len; i++) {
    sendLetter(string[i], (uint8_t)(x + (i * (w + text_spacing))), y, w, h, color);
  }
}

#endif /* FPGA */

/* ===================================================================================
 * Prueba aislada -- verifica que display_construir_trama empaqueta y permite
 * recuperar exactamente los mismos campos (round-trip), para las esquinas
 * reales de una pantalla de 64x64 mas un caso de color arbitrario.
 * =================================================================================== */
#ifdef DISPLAY_SPI_TEST
#include <stdio.h>

static void probar(const char *nombre, uint8_t x, uint8_t y, uint16_t color) {
  uint32_t trama = display_construir_trama(x, y, color);

  uint8_t cmd_leido   = (uint8_t)((trama >> 28) & 0xFu);
  uint8_t x_leido     = (uint8_t)((trama >> 22) & 0x3Fu);
  uint8_t y_leido     = (uint8_t)((trama >> 16) & 0x3Fu);
  uint16_t color_leido = (uint16_t)(trama & 0xFFFFu);

  int ok = cmd_leido == DISPLAY_CMD_SET_PIXEL && x_leido == x && y_leido == y &&
          color_leido == color;

  printf("Prueba: %-28s trama=0x%08X -> cmd=%u x=%2u y=%2u color=0x%04X  %s\n",
        nombre, trama, cmd_leido, x_leido, y_leido, color_leido, ok ? "[OK]" : "[FALLO]");
}

int main(void) {
  printf("=== Prueba real del empaquetado de trama SPI del display (sin hardware) ===\n\n");

  probar("Esquina (0,0), negro",       0,  0, 0x0000);
  probar("Esquina (63,0), blanco",     63, 0, 0xFFFF);
  probar("Esquina (0,63)",             0, 63, 0x1234);
  probar("Esquina (63,63)",            63, 63, 0x07E0); /* verde puro RGB565 */
  probar("Centro (32,32), colorkey",   32, 32, DISPLAY_COLORKEY);

  printf("\nSi todas dicen [OK], la trama de 32 bits (comando+X+Y+color) se puede\n"
        "empaquetar y recuperar sin perder ningun campo, para toda la pantalla real de 64x64.\n");
  return 0;
}
#endif
