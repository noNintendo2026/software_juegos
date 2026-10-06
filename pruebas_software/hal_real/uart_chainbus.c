/* uart_chainbus.c -- capa fisica UART real + armado de tramas Chain Bus
 * (punto 8 de los 8 problemas: mostrar el puntaje por UART en las pantallas
 * M5Stack Chain Mono).
 *
 * Capa fisica -- CONFIRMADA por el equipo de UART en su propio repo
 * ("protocolo uart y juego/readme.md"): 8N1, 115200 bps (coincide con el
 * hardware Chain Mono, no hace falta traducir baud rate). Registros CSR
 * PROPUESTOS por ese mismo equipo (su propia tabla, no un supuesto nuestro):
 *
 *   0x400000  TX_DATA  (escritura)  -- byte a transmitir
 *   0x400004  RX_DATA  (lectura)    -- byte recibido
 *   0x400008  STATUS   (lectura)    -- bit0 = TX_BUSY, bit1 = RX_READY
 *
 * Capa de aplicacion -- Chain Bus, formato fijo del FABRICANTE (M5Stack) para
 * las pantallas Chain Mono, documentado por el equipo de UART citando la
 * referencia oficial de M5Stack:
 *
 *   Cabecera(2)=AA 55 | Longitud(2) | Index_id(1) | Cmd(1) | Payload(var) | CRC(var) | Cola(2)=55 AA
 *
 * LO QUE FALTA CONFIRMAR (ver PENDIENTES_EXTERNOS.md):
 *   - El algoritmo EXACTO de CRC. El equipo solo documento "checksum de la
 *     trama" y remitio a la referencia oficial de M5Stack (U218 UART Protocol
 *     Reference) sin copiar el algoritmo. NO se inventa aqui un CRC real --
 *     se deja una funcion placeholder, marcada explicitamente, que hay que
 *     reemplazar antes de probar contra una pantalla Chain Mono real.
 *   - Los valores EXACTOS de `Cmd` para cada operacion (solo se documento
 *     `0x10` como ejemplo de "fijar modo de pantalla"; el comando real para
 *     "mostrar texto ASCII" o "escribir pixeles" no se documento con su
 *     valor numerico). Por eso estas funciones reciben `cmd` como parametro
 *     en vez de decidirlo internamente.
 *
 * Prueba aislada (no requiere FPGA): verifica que el armado de trama respeta
 * la cabecera/cola fijas y la longitud real del payload -- la unica parte
 * que SI esta confirmada por completo.
 * gcc -DUART_CHAINBUS_TEST uart_chainbus.c -o test_uart_chainbus.exe
 */
#include <stdint.h>
#include <string.h>
#include "hal_shared.h"

#define UART_REG_TX_DATA  (MEM_UART_BASE + 0x00u)  /* CONFIRMADO por el equipo de UART */
#define UART_REG_RX_DATA  (MEM_UART_BASE + 0x04u)  /* CONFIRMADO */
#define UART_REG_STATUS   (MEM_UART_BASE + 0x08u)  /* CONFIRMADO */
#define UART_STATUS_TX_BUSY   0x1u
#define UART_STATUS_RX_READY  0x2u

#define CHAINBUS_CABECERA_0 0xAAu  /* CONFIRMADO: fija */
#define CHAINBUS_CABECERA_1 0x55u
#define CHAINBUS_COLA_0     0x55u  /* CONFIRMADO: fija */
#define CHAINBUS_COLA_1     0xAAu
#define CHAINBUS_TAM_CRC    1u     /* ASUMIDO: 1 byte; el tamano real del CRC del Chain Bus no esta confirmado */

/* PENDIENTE DE CONFIRMAR: esto NO es el CRC real de M5Stack, es un relleno
 * para que la trama tenga el formato correcto mientras se confirma el
 * algoritmo oficial (ver PENDIENTES_EXTERNOS.md). Sirve para probar el
 * armado de la trama, NO para comunicarse con hardware real todavia. */
static uint8_t chainbus_crc_PENDIENTE_CONFIRMAR(const uint8_t *datos, size_t n) {
  uint8_t suma = 0;
  for (size_t i = 0; i < n; i++) {
    suma = (uint8_t)(suma ^ datos[i]);
  }
  return suma;
}

/* Arma una trama Chain Bus completa en `buffer` (debe tener espacio para
 * 2+2+1+1+payload_len+CHAINBUS_TAM_CRC+2 bytes). Devuelve el tamano total
 * real de la trama armada. Pura -- no toca hardware, se puede probar sin FPGA. */
size_t chainbus_armar_trama(uint8_t *buffer, uint8_t index_id, uint8_t cmd,
                            const uint8_t *payload, uint16_t payload_len) {
  size_t i = 0;
  buffer[i++] = CHAINBUS_CABECERA_0;
  buffer[i++] = CHAINBUS_CABECERA_1;
  buffer[i++] = (uint8_t)((payload_len >> 8) & 0xFFu);  /* longitud, MSB primero */
  buffer[i++] = (uint8_t)(payload_len & 0xFFu);
  buffer[i++] = index_id;
  buffer[i++] = cmd;

  size_t inicio_payload = i;
  for (uint16_t p = 0; p < payload_len; p++) {
    buffer[i++] = payload[p];
  }

  uint8_t crc = chainbus_crc_PENDIENTE_CONFIRMAR(&buffer[inicio_payload], payload_len);
  buffer[i++] = crc;  /* CHAINBUS_TAM_CRC == 1 */

  buffer[i++] = CHAINBUS_COLA_0;
  buffer[i++] = CHAINBUS_COLA_1;

  return i;
}

#ifdef FPGA

static void uart_tx_byte(uint8_t byte) {
  while (MMIO32(UART_REG_STATUS) & UART_STATUS_TX_BUSY) {
    /* esperar a que termine de transmitir el byte anterior */
  }
  MMIO32(UART_REG_TX_DATA) = byte;
}

static int uart_rx_byte_disponible(void) {
  return (MMIO32(UART_REG_STATUS) & UART_STATUS_RX_READY) != 0;
}

static uint8_t uart_rx_byte(void) {
  return (uint8_t)MMIO32(UART_REG_RX_DATA);
}

/* Envia una trama Chain Bus completa, byte a byte, por UART real. */
void uart_chainbus_enviar(uint8_t index_id, uint8_t cmd, const uint8_t *payload, uint16_t payload_len) {
  uint8_t trama[16 + 256];  /* cabecera+longitud+index+cmd+hasta 256 de payload+crc+cola */
  size_t tam = chainbus_armar_trama(trama, index_id, cmd, payload, payload_len);
  for (size_t i = 0; i < tam; i++) {
    uart_tx_byte(trama[i]);
  }
}

/* El Chain Bus real espera una respuesta (Operation_status) por cada
 * comando -- la lee pero no la interpreta todavia (eso depende de conocer
 * el formato real de esa respuesta, no documentado en detalle). */
int uart_chainbus_leer_respuesta(uint8_t *respuesta_byte) {
  if (!uart_rx_byte_disponible()) {
    return 0;
  }
  *respuesta_byte = uart_rx_byte();
  return 1;
}

#endif /* FPGA */

/* ===================================================================================
 * Prueba aislada -- verifica SOLO lo que esta confirmado: cabecera fija,
 * longitud correcta, index_id/cmd en su lugar, cola fija. No verifica el CRC
 * contra un valor "real" porque todavia no existe un algoritmo confirmado.
 * =================================================================================== */
#ifdef UART_CHAINBUS_TEST
#include <stdio.h>

static void probar(const char *nombre, uint8_t index_id, uint8_t cmd,
                   const uint8_t *payload, uint16_t payload_len) {
  uint8_t buffer[32];
  size_t tam = chainbus_armar_trama(buffer, index_id, cmd, payload, payload_len);
  size_t tam_esperado = 2 + 2 + 1 + 1 + payload_len + CHAINBUS_TAM_CRC + 2;

  int ok = tam == tam_esperado &&
          buffer[0] == CHAINBUS_CABECERA_0 && buffer[1] == CHAINBUS_CABECERA_1 &&
          buffer[2] == (uint8_t)((payload_len >> 8) & 0xFFu) &&
          buffer[3] == (uint8_t)(payload_len & 0xFFu) &&
          buffer[4] == index_id && buffer[5] == cmd &&
          buffer[tam - 2] == CHAINBUS_COLA_0 && buffer[tam - 1] == CHAINBUS_COLA_1;

  printf("Prueba: %-38s tam=%2lu/%2lu  %s\n", nombre, (unsigned long)tam, (unsigned long)tam_esperado, ok ? "[OK]" : "[FALLO]");
}

int main(void) {
  printf("=== Prueba real del armado de trama Chain Bus (sin hardware) ===\n\n");
  printf("NOTA: el CRC usado aqui es un relleno (XOR), NO el algoritmo real de\n"
        "M5Stack -- ver uart_chainbus.c y PENDIENTES_EXTERNOS.md.\n\n");

  uint8_t payload_vacio[1] = {0};
  probar("Payload vacio, index 0, cmd 0x10", 0, 0x10, payload_vacio, 0);

  uint8_t payload_score[3] = {'0', '1', '0'};  /* ej. puntaje "010" como digitos ASCII */
  probar("Puntaje \"010\" como 3 bytes ASCII", 1, 0x20, payload_score, 3);

  uint8_t payload_grande[20];
  memset(payload_grande, 0xAB, sizeof(payload_grande));
  probar("Payload de 20 bytes", 3, 0x30, payload_grande, 20);

  printf("\nSi todas dicen [OK], la cabecera/longitud/index_id/cmd/cola de la trama se\n"
        "arman exactamente como documento el equipo de UART -- falta reemplazar el CRC\n"
        "placeholder por el algoritmo real de M5Stack antes de probar contra hardware.\n");
  return 0;
}
#endif
