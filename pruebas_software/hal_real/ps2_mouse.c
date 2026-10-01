/* ps2_mouse.c -- decodificador REAL de paquetes PS/2 del mouse (punto 3 de los 8
 * problemas: no existia ningun tipo de dato para representar movimiento continuo X/Y
 * ni botones de mouse -- el Action enum es solo para eventos discretos).
 *
 * Implementa exactamente el formato que documento el equipo de ps2_mouse en su README:
 *   Byte 1: [Y overflow][X overflow][Y sign][X sign][1][boton centro][boton der][boton izq]
 *   Byte 2: movimiento X (8 bits, con el signo ya indicado en byte 1 + overflow)
 *   Byte 3: movimiento Y (8 bits, idem)
 *   (modo IntelliMouse, 4 bytes: Byte 4 trae la rueda en complemento a 2 de 4 bits)
 *
 * El movimiento real es de 9 bits en complemento a dos (8 bits de datos + 1 bit de
 * signo en el byte de estado) -- se reconstruye el valor con signo real, no solo se
 * usa el byte crudo.
 *
 * Prueba aislada (sin hardware): gcc -DPS2_MOUSE_TEST ps2_mouse.c -o test_ps2_mouse.exe
 */
#include <stdio.h>
#include <stdint.h>
#include "hal_shared.h"

typedef struct {
  int byte_index;        /* 0,1,2 (o 3 en modo IntelliMouse) */
  uint8_t buffer[4];
  int intellimouse;       /* 1 si se activo el modo de 4 bytes (rueda) */
} DecodificadorMouse;

static DecodificadorMouse decodificadorMouse = { 0, {0, 0, 0, 0}, 0 };

/* Reconstruye un entero con signo real a partir del byte de movimiento + su bit de
 * signo/overflow del byte de estado, tal como describe el protocolo real documentado. */
static int16_t reconstruir_movimiento(uint8_t byte_mov, int bit_signo, int bit_overflow) {
  if (bit_overflow) {
    /* overflow real: el protocolo no garantiza un valor util, se satura */
    return bit_signo ? -256 : 255;
  }
  if (bit_signo) {
    /* complemento a dos extendido de 8 a 16 bits */
    return (int16_t)((int16_t)byte_mov - 256);
  }
  return (int16_t)byte_mov;
}

void ps2_mouse_activar_intellimouse(void) {
  decodificadorMouse.intellimouse = 1;
}

/* Procesa UN byte real recibido del mouse. Devuelve un MouseState con
 * packet_valido=1 SOLO en el byte que completa un paquete entero (3 o 4 bytes). */
MouseState ps2_mouse_procesar_byte(uint8_t byte) {
  MouseState resultado = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  int tam_paquete = decodificadorMouse.intellimouse ? 4 : 3;

  /* Sincronizacion real exigida por el protocolo: el bit 3 del primer byte SIEMPRE
     es 1 -- si no lo es, no estamos alineados con el inicio real de un paquete y hay
     que resincronizar (se descarta todo y se espera un byte valido de "byte 1"). */
  if (decodificadorMouse.byte_index == 0 && (byte & 0x08) == 0) {
    return resultado;  /* no es un byte 1 valido, se ignora, seguimos esperando */
  }

  decodificadorMouse.buffer[decodificadorMouse.byte_index] = byte;
  decodificadorMouse.byte_index++;

  if (decodificadorMouse.byte_index < tam_paquete) {
    return resultado;  /* paquete incompleto todavia */
  }

  /* Paquete completo -- decodificar segun el protocolo real documentado */
  uint8_t b1 = decodificadorMouse.buffer[0];
  uint8_t b2 = decodificadorMouse.buffer[1];
  uint8_t b3 = decodificadorMouse.buffer[2];

  resultado.left   = (b1 & 0x01) ? 1 : 0;
  resultado.right  = (b1 & 0x02) ? 1 : 0;
  resultado.middle = (b1 & 0x04) ? 1 : 0;

  int signoX = (b1 & 0x10) ? 1 : 0;
  int signoY = (b1 & 0x20) ? 1 : 0;
  int overflowX = (b1 & 0x40) ? 1 : 0;
  int overflowY = (b1 & 0x80) ? 1 : 0;

  resultado.dx = reconstruir_movimiento(b2, signoX, overflowX);
  /* el protocolo real documenta Y positivo hacia arriba; la mayoria de pantallas
     tratan Y positivo hacia abajo, por eso se invierte aqui, en el unico lugar donde
     corresponde (no en el juego). */
  resultado.dy = (int16_t)(-reconstruir_movimiento(b3, signoY, overflowY));

  if (decodificadorMouse.intellimouse) {
    uint8_t b4 = decodificadorMouse.buffer[3];
    int8_t z4 = (int8_t)(b4 & 0x0F);
    if (z4 & 0x08) z4 = (int8_t)(z4 - 16);  /* complemento a 2 de 4 bits, protocolo real */
    resultado.wheel = z4;
    resultado.button4 = (b4 & 0x10) ? 1 : 0;
    resultado.button5 = (b4 & 0x20) ? 1 : 0;
  }

  resultado.packet_valido = 1;
  decodificadorMouse.byte_index = 0;  /* listo para el siguiente paquete */
  return resultado;
}

/* HAL real: lee el registro de datos PS/2 del mouse solo cuando hay byte nuevo --
 * offsets ASUMIDOS, mismo patron que el teclado (ver hal_shared.h). */
MouseState pollMouse(void) {
#ifdef FPGA
  uint32_t estado = MMIO32(MEM_PS2_MOUSE_BASE + REG_OFFSET_STATUS);
  if ((estado & 0x1u) == 0) {
    MouseState vacio = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    return vacio;
  }
  uint8_t byte = (uint8_t)MMIO32(MEM_PS2_MOUSE_BASE + REG_OFFSET_DATA);
  return ps2_mouse_procesar_byte(byte);
#else
  MouseState vacio = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  return vacio;
#endif
}

/* ===================================================================================
 * Prueba aislada -- construye paquetes reales segun el formato documentado y verifica
 * que el decodificador los interprete correctamente.
 * =================================================================================== */
#ifdef PS2_MOUSE_TEST
static void probar(const char *nombre, uint8_t b1, uint8_t b2, uint8_t b3,
                   int16_t dx_esp, int16_t dy_esp, int izq_esp, int der_esp) {
  decodificadorMouse.byte_index = 0;
  ps2_mouse_procesar_byte(b1);
  ps2_mouse_procesar_byte(b2);
  MouseState r = ps2_mouse_procesar_byte(b3);

  int ok = r.packet_valido && r.dx == dx_esp && r.dy == dy_esp &&
          r.left == izq_esp && r.right == der_esp;

  printf("Prueba: %-32s dx=%4d dy=%4d izq=%d der=%d  (esperado dx=%4d dy=%4d izq=%d der=%d)  %s\n",
        nombre, r.dx, r.dy, r.left, r.right, dx_esp, dy_esp, izq_esp, der_esp,
        ok ? "[OK]" : "[FALLO]");
}

int main(void) {
  printf("=== Prueba real del decodificador de mouse PS/2 (sin hardware) ===\n\n");

  /* Movimiento simple a la derecha (+10,+0), sin botones:
     byte1 = 0000_1000 (bit3=1 obligatorio, sin signos, sin botones) = 0x08
     byte2 = +10 = 0x0A ; byte3 = 0 */
  probar("Mover +10 en X, sin botones", 0x08, 0x0A, 0x00, 10, 0, 0, 0);

  /* Movimiento negativo en X (-10): signoX=1 (bit4), byte2 = 256-10 = 246 = 0xF6 */
  probar("Mover -10 en X", 0x18, 0xF6, 0x00, -10, 0, 0, 0);

  /* Boton izquierdo presionado, sin movimiento: byte1 = 0000_1001 = 0x09 */
  probar("Boton izquierdo, sin mover", 0x09, 0x00, 0x00, 0, 0, 1, 0);

  /* Boton derecho + movimiento Y real hacia arriba (el protocolo dice +, nuestra
     salida lo invierte a negativo porque en pantalla "arriba" es Y decreciente):
     byte1 = 0000_1010 (boton der) = 0x0A ; byte3 = +5 = 0x05 */
  probar("Boton derecho + Y arriba real (+5 protocolo)", 0x0A, 0x00, 0x05, 0, -5, 0, 1);

  printf("\nSi todas dicen [OK], el decodificador reconstruye correctamente el movimiento\n"
        "con signo real y el estado de los botones segun el protocolo documentado.\n");
  return 0;
}
#endif
