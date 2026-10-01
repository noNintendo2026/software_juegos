# pruebas_software

Carpeta de trabajo y pruebas del subgrupo de software (Juan Camilo). Aquí va código de prueba,
análisis y documentación propia **sin afectar** las carpetas de los demás juegos
(`pong`, `roadFighter`, `snake`, `space_invaders`, `halSimulator`) ni el trabajo de los compañeros
de hardware. Puede haber código duplicado respecto a otras carpetas a propósito — es más
importante tener un espacio seguro para probar que evitar la repetición.

## Contenido actual

- **`Space_Invaders_SDL2.c`** — port real de `space_invaders/Space_Invaders.c` de SDL 1.2 a SDL2
  (el original no compilaba contra SDL2: usaba `SDL_SetVideoMode`, `SDL_Flip`, `SDL_GetKeyState`,
  etc., funciones que SDL2 eliminó). Verificado: compila sin errores y corre sin crashear con
  sprites de prueba (el original crasheaba porque los 9-10 archivos `.bmp` reales nunca se subieron
  al repositorio). Para compilar necesita las librerías de desarrollo de SDL2 (no incluidas aquí
  por tamaño — ver la guía de compilación de `halSimulator/forDev.md`).

- **`ARQUITECTURA_CPU_PERIFERICOS.md`** — diagrama (Mermaid) del flujo completo CPU ↔ periféricos,
  armado a partir del mapa de memoria real del `.github` de la organización y de la documentación
  de protocolo de cada repositorio de periférico (`ps2_keyboard.v`, `ps2_mouse`, `I2S_tx.v`,
  `Display-Driver`, etc.). Sirve para que el equipo de software entienda qué debe implementar
  `hal.h` contra el hardware real.

- **`hal_real/`** — implementación real del HAL contra hardware, empezando a resolver los 8
  problemas de "conectar hardware real a los juegos" identificados al analizar el repo:
  - `hal_shared.h` — contrato **unificado** del HAL (arregla el bug real de que
    `readControls()` tenía firmas distintas en el bloque de simulador y en el de hardware),
    con el mapa de memoria real confirmado, offsets de registro **ASUMIDOS** (marcados
    explícitamente, pendientes de confirmar con cada equipo — ver `PENDIENTES_EXTERNOS.md`),
    el macro `MMIO32()` para acceso real al bus, y los nuevos tipos `MouseState`/`pollMouse()`.
  - `ps2_keyboard.c` — decodificador real de scan codes PS/2 set 2 (make/break/extendida),
    probado contra la secuencia de ejemplo que el propio equipo de `ps2_keyboard.v` documentó
    en su README (4/4 pruebas `[OK]`, ver comentarios del archivo para compilar la prueba).
  - `ps2_mouse.c` — decodificador real de paquetes PS/2 del mouse (3-4 bytes, X/Y en
    complemento a 2, botones, modo IntelliMouse con rueda), probado con paquetes construidos
    según el protocolo documentado por el equipo de `ps2_mouse` (4/4 pruebas `[OK]`).
  - `basic_font.h` — copia exacta del font `usg_font_5x7` del propio equipo (ya usado por el
    simulador SDL), reutilizado para que las letras se vean igual en el simulador y en
    hardware real.
  - `display_spi.c` — driver real de `sendSprite`/`sendBackground`/`sendLetter`/`sendString`
    contra el protocolo SPI del `Display-Driver` (trama de 32 bits Comando+Coordenada+Color).
    Antes estos eran cuerpos vacíos (comentados) en el bloque `#ifdef FPGA` del `hal.h`
    original. El reparto exacto de bits dentro de la trama de 32 bits es **ASUMIDO** (ver
    `PENDIENTES_EXTERNOS.md`, punto 8). Probado: 5/5 `[OK]` (empaquetado/recuperación de la
    trama para las 4 esquinas reales de la pantalla de 64x64 + un caso de colorkey).
  - `uart_chainbus.c` — capa física UART real (según el CSR propuesto por ese equipo:
    `TX_DATA`/`RX_DATA`/`STATUS`) + armado de tramas Chain Bus (protocolo del fabricante
    M5Stack para las pantallas Chain Mono del marcador de puntaje). El CRC usado es un
    **relleno explícito**, no el algoritmo real de M5Stack (ver `PENDIENTES_EXTERNOS.md`,
    punto 9). Probado: 3/3 `[OK]` (cabecera, longitud, index/cmd y cola de la trama).
  - `spi_flash.c` — driver real de la memoria SPI Flash (comandos `READ`/`FAST READ`/
    `Write Enable`/`Page Program`/`Sector`/`Block Erase`, todos documentados por el equipo de
    `spi_flash_ctrl`). El mapa de registros CSR del controlador es **ASUMIDO** (ese equipo
    solo documentó el protocolo SPI genérico de la memoria, no su propio controlador — ver
    `PENDIENTES_EXTERNOS.md`, punto 6). Probado: 4/4 `[OK]`, incluyendo el ejemplo exacto de
    lectura que el equipo puso en su propio README.

- **`PENDIENTES_EXTERNOS.md`** — lista concreta de lo que falta confirmar con otros equipos
  (protocolo NES, protocolo I2C, registro de disparo de I2S, combinación de las 4 pantallas,
  toolchain de RISC-V, offsets de registro asumidos, 3 scan codes sin confirmar en texto).
  Son preguntas, no respuestas inventadas.

## Nota

Esta carpeta vive en una rama (`software-juan-camilo-pruebas`), no en `main`, precisamente para no
interrumpir el trabajo de nadie — el equipo decide cuándo y cómo fusionarla.
