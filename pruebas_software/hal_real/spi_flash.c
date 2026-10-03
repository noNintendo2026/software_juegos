/* spi_flash.c -- driver real de la memoria SPI Flash (punto 8 de los 8
 * problemas: almacenamiento de juegos / guardado, mapeado en 0x420000-0x42FFFF).
 *
 * Comandos y secuencias de trama -- CONFIRMADOS por el equipo de
 * spi_flash_ctrl en su propio README (tablas de comandos + diagramas de
 * secuencia para READ y FAST READ):
 *
 *   06h Write Enable      | 04h Write Disable   | 05h Read Status Register 1
 *   35h Read Status Reg 2 | C7h/60h Chip Erase
 *   03h READ (sin dummy)  | 0Bh FAST READ (1 byte dummy)
 *   02h Page Program (<=256 bytes) | 20h Sector Erase (4KB)
 *   52h Block Erase (32KB) | D8h Block Erase (64KB)
 *
 * Secuencia real de una transaccion (confirmada): CS baja -> Comando (1 byte)
 * -> Direccion de 24 bits (3 bytes, MSB primero) -> [dummy si aplica] ->
 * Datos -> CS sube.
 *
 * LO QUE NO esta confirmado (ver PENDIENTES_EXTERNOS.md): el equipo
 * documento el protocolo SPI generico de una flash, pero no publico el mapa
 * de registros CSR de SU controlador (`spi_flash_ctrl`) dentro de
 * 0x420000-0x42FFFF. Este driver ASUME el mismo patron ya usado para los
 * demas perifericos (registro de datos tipo shift-register full-duplex +
 * bit de CS en el registro de control + bit de ocupado en status) -- es el
 * diseño mas comun para un controlador SPI genérico, pero sigue siendo un
 * supuesto, no un hecho confirmado por ese equipo.
 *
 * El bit exacto de "WIP" (Write In Progress) dentro del Status Register 1
 * tampoco lo documento el equipo con su posicion exacta -- se asume bit0,
 * que es la convencion mas comun en memorias SPI NOR (ej. la familia
 * Winbond W25Qxx que citan como referencia), pero deberia confirmarse contra
 * el datasheet real del chip que efectivamente se vaya a usar.
 *
 * Prueba aislada (no requiere FPGA): verifica SOLO la parte pura y 100%
 * confirmada -- el armado de comando+direccion de 24 bits, MSB primero.
 * gcc -DSPI_FLASH_TEST spi_flash.c -o test_spi_flash.exe
 */
#include <stdint.h>
#include "hal_shared.h"

#define SPI_FLASH_CMD_WRITE_ENABLE   0x06u
#define SPI_FLASH_CMD_WRITE_DISABLE  0x04u
#define SPI_FLASH_CMD_READ_STATUS1   0x05u
#define SPI_FLASH_CMD_READ_STATUS2   0x35u
#define SPI_FLASH_CMD_CHIP_ERASE     0xC7u
#define SPI_FLASH_CMD_READ           0x03u
#define SPI_FLASH_CMD_FAST_READ      0x0Bu
#define SPI_FLASH_CMD_PAGE_PROGRAM   0x02u
#define SPI_FLASH_CMD_SECTOR_ERASE_4K   0x20u
#define SPI_FLASH_CMD_BLOCK_ERASE_32K   0x52u
#define SPI_FLASH_CMD_BLOCK_ERASE_64K   0xD8u

#define SPI_FLASH_STATUS1_WIP_BIT  0x01u  /* ASUMIDO -- convencion comun SPI NOR, confirmar contra el datasheet real */

/* Pura -- arma Comando(1) + Direccion de 24 bits MSB primero(3) en `buffer`.
 * Confirmado por el equipo: "Direccion: 3 bytes (24 bits), MSB primero".
 * Se puede probar sin FPGA ni hardware real. */
void spi_flash_armar_cmd_direccion(uint8_t *buffer, uint8_t comando, uint32_t direccion24) {
  buffer[0] = comando;
  buffer[1] = (uint8_t)((direccion24 >> 16) & 0xFFu);
  buffer[2] = (uint8_t)((direccion24 >> 8) & 0xFFu);
  buffer[3] = (uint8_t)(direccion24 & 0xFFu);
}

#ifdef FPGA

#define SPI_FLASH_CONTROL_CS_BIT 0x01u  /* ASUMIDO: bit0 de REG_OFFSET_CONTROL = CS (0=activo/bajo, 1=inactivo) */
#define SPI_FLASH_STATUS_BUSY_BIT 0x01u /* ASUMIDO: bit0 de REG_OFFSET_STATUS = transferencia de byte en curso */

static void spi_flash_cs(int activo) {
  uint32_t control = MMIO32(MEM_SPI_FLASH_BASE + REG_OFFSET_CONTROL);
  if (activo) {
    control &= ~SPI_FLASH_CONTROL_CS_BIT;  /* CS activo en bajo, protocolo real confirmado */
  } else {
    control |= SPI_FLASH_CONTROL_CS_BIT;
  }
  MMIO32(MEM_SPI_FLASH_BASE + REG_OFFSET_CONTROL) = control;
}

/* Transferencia full-duplex de un byte: escribe `tx`, espera a que termine el
 * shift y devuelve lo que llego por MISO en el mismo ciclo (patron SPI
 * estandar: un byte enviado = un byte recibido). */
static uint8_t spi_flash_transferir_byte(uint8_t tx) {
  MMIO32(MEM_SPI_FLASH_BASE + REG_OFFSET_DATA) = tx;
  while (MMIO32(MEM_SPI_FLASH_BASE + REG_OFFSET_STATUS) & SPI_FLASH_STATUS_BUSY_BIT) {
    /* esperar a que termine de desplazar el byte */
  }
  return (uint8_t)MMIO32(MEM_SPI_FLASH_BASE + REG_OFFSET_DATA);
}

void spi_flash_write_enable(void) {
  spi_flash_cs(1);
  spi_flash_transferir_byte(SPI_FLASH_CMD_WRITE_ENABLE);
  spi_flash_cs(0);
}

uint8_t spi_flash_leer_status1(void) {
  spi_flash_cs(1);
  spi_flash_transferir_byte(SPI_FLASH_CMD_READ_STATUS1);
  uint8_t estado = spi_flash_transferir_byte(0x00u);
  spi_flash_cs(0);
  return estado;
}

int spi_flash_ocupado(void) {
  return (spi_flash_leer_status1() & SPI_FLASH_STATUS1_WIP_BIT) != 0;
}

/* READ (03h) -- sin dummy, confirmado. Lee `len` bytes consecutivos (la
 * flash avanza su propia direccion interna mientras CS siga bajo). */
void spi_flash_leer(uint32_t direccion24, uint8_t *destino, uint32_t len) {
  uint8_t cab[4];
  spi_flash_armar_cmd_direccion(cab, SPI_FLASH_CMD_READ, direccion24);

  spi_flash_cs(1);
  for (int i = 0; i < 4; i++) {
    spi_flash_transferir_byte(cab[i]);
  }
  for (uint32_t i = 0; i < len; i++) {
    destino[i] = spi_flash_transferir_byte(0x00u);
  }
  spi_flash_cs(0);
}

/* FAST READ (0Bh) -- 1 byte dummy antes de los datos, confirmado. */
void spi_flash_leer_rapido(uint32_t direccion24, uint8_t *destino, uint32_t len) {
  uint8_t cab[4];
  spi_flash_armar_cmd_direccion(cab, SPI_FLASH_CMD_FAST_READ, direccion24);

  spi_flash_cs(1);
  for (int i = 0; i < 4; i++) {
    spi_flash_transferir_byte(cab[i]);
  }
  spi_flash_transferir_byte(0x00u);  /* dummy, 8 ciclos, confirmado */
  for (uint32_t i = 0; i < len; i++) {
    destino[i] = spi_flash_transferir_byte(0x00u);
  }
  spi_flash_cs(0);
}

/* Page Program (02h) -- hasta 256 bytes, confirmado. El llamador debe haber
 * hecho spi_flash_write_enable() antes (secuencia real documentada). */
void spi_flash_escribir_pagina(uint32_t direccion24, const uint8_t *datos, uint32_t len) {
  if (len > 256u) {
    len = 256u;  /* el comando real no programa mas de una pagina de 256 bytes */
  }
  uint8_t cab[4];
  spi_flash_armar_cmd_direccion(cab, SPI_FLASH_CMD_PAGE_PROGRAM, direccion24);

  spi_flash_cs(1);
  for (int i = 0; i < 4; i++) {
    spi_flash_transferir_byte(cab[i]);
  }
  for (uint32_t i = 0; i < len; i++) {
    spi_flash_transferir_byte(datos[i]);
  }
  spi_flash_cs(0);
}

static void spi_flash_borrar_con_comando(uint8_t comando, uint32_t direccion24) {
  uint8_t cab[4];
  spi_flash_armar_cmd_direccion(cab, comando, direccion24);
  spi_flash_cs(1);
  for (int i = 0; i < 4; i++) {
    spi_flash_transferir_byte(cab[i]);
  }
  spi_flash_cs(0);
}

void spi_flash_borrar_sector_4k(uint32_t direccion24)  { spi_flash_borrar_con_comando(SPI_FLASH_CMD_SECTOR_ERASE_4K, direccion24); }
void spi_flash_borrar_bloque_32k(uint32_t direccion24) { spi_flash_borrar_con_comando(SPI_FLASH_CMD_BLOCK_ERASE_32K, direccion24); }
void spi_flash_borrar_bloque_64k(uint32_t direccion24) { spi_flash_borrar_con_comando(SPI_FLASH_CMD_BLOCK_ERASE_64K, direccion24); }

#endif /* FPGA */

/* ===================================================================================
 * Prueba aislada -- verifica el armado de Comando+Direccion de 24 bits contra
 * el ejemplo EXACTO que el propio equipo puso en su README (leer un uint32_t
 * desde 0x000100: "-> 03h -> 00h 01h 00h").
 * =================================================================================== */
#ifdef SPI_FLASH_TEST
#include <stdio.h>

static void probar(const char *nombre, uint8_t comando, uint32_t direccion,
                   uint8_t b0_esp, uint8_t b1_esp, uint8_t b2_esp, uint8_t b3_esp) {
  uint8_t buf[4];
  spi_flash_armar_cmd_direccion(buf, comando, direccion);
  int ok = buf[0] == b0_esp && buf[1] == b1_esp && buf[2] == b2_esp && buf[3] == b3_esp;
  printf("Prueba: %-40s %02X %02X %02X %02X  (esperado %02X %02X %02X %02X)  %s\n",
        nombre, buf[0], buf[1], buf[2], buf[3], b0_esp, b1_esp, b2_esp, b3_esp,
        ok ? "[OK]" : "[FALLO]");
}

int main(void) {
  printf("=== Prueba real del armado de Comando+Direccion SPI Flash (sin hardware) ===\n\n");

  /* Ejemplo EXACTO del README del equipo: leer desde 0x000100 con READ (03h)
     -> "03h -> 00h 01h 00h" */
  probar("READ (03h) desde 0x000100 (ejemplo del README)", 0x03u, 0x000100u, 0x03u, 0x00u, 0x01u, 0x00u);

  probar("READ (03h) desde 0x000000", 0x03u, 0x000000u, 0x03u, 0x00u, 0x00u, 0x00u);
  probar("FAST READ (0Bh) desde 0xFFFFFF", 0x0Bu, 0xFFFFFFu, 0x0Bu, 0xFFu, 0xFFu, 0xFFu);
  probar("Page Program (02h) en 0x001000", 0x02u, 0x001000u, 0x02u, 0x00u, 0x10u, 0x00u);

  printf("\nSi todas dicen [OK], el armado de Comando+Direccion de 24 bits (MSB primero)\n"
        "coincide exactamente con el ejemplo documentado por el equipo de spi_flash_ctrl.\n");
  return 0;
}
#endif
