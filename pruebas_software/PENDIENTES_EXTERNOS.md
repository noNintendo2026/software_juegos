# Pendientes que dependen de otros equipos

Esta lista nace de implementar el HAL real (`hal_real/`) contra el hardware. Cada punto es
algo que **no podemos decidir desde software** porque depende de una decisión o un dato que
solo tiene el equipo responsable de esa parte. No se inventó ninguna respuesta — donde no
había protocolo documentado, se dejó explícitamente sin implementar en el código.

## 1. Control NES — protocolo sin documentar

**Equipo:** `nes_controller.v`
**Estado:** su repositorio solo tiene el título, sin README de protocolo.
**Pregunta concreta:** ¿el control se lee por desplazamiento serial (shift register, como un
control NES real: pulso `LATCH` + 8 pulsos de `CLOCK` leyendo `DATA` bit a bit) o ya lo
decodifican ustedes en la FPGA y nos entregan un byte/registro con el estado de los botones
directo en `0x450000`? Si es lo segundo, ¿qué bit corresponde a cada botón?
**Por qué bloquea:** sin esto no podemos declarar ninguna función `pollNES()` en `hal_shared.h`
sin inventar un contrato — y el juego no puede usar ese control todavía.

## 2. I2C — protocolo propio sin documentar

**Equipo:** `I2C_Master`
**Estado:** su README remite a una plantilla externa, sin especificar cómo se usa para
guardar/leer puntajes en este proyecto.
**Pregunta concreta:** ¿qué dirección I2C (7 bits) tiene el dispositivo de almacenamiento
(EEPROM u otro) conectado? ¿Qué protocolo de escritura/lectura espera (dirección de memoria
interna + datos, como una EEPROM típica tipo 24LCxx) y cuál es el tamaño disponible para
guardar puntajes?
**Por qué bloquea:** sin esto no se puede implementar guardado de puntajes persistente en
ningún juego.

## 3. I2S — ¿existe un registro de "disparo" en tiempo real?

**Equipo:** `I2S_tx.v`
**Estado:** confirmado que el audio se "hornea" en una ROM de Verilog (`wav2mem.py`) ANTES de
sintetizar el bitstream — no se decide en tiempo de ejecución qué sonido reproducir, igual que
el framebuffer no se genera dinámicamente.
**Pregunta concreta:** dado que el contenido ya está fijo en la ROM al sintetizar, ¿existe
igualmente un registro mapeado en `0x470000` que la CPU pueda escribir para decirle "empieza a
reproducir ahora" (por ejemplo, un registro de control con un bit de "start"), o el audio se
reproduce automáticamente sin intervención de la CPU?
**Por qué bloquea:** determina si `hal_shared.h` necesita o no una función tipo
`playSound()`/`triggerAudio()`. Por ahora no se declaró ninguna, para no inventar un contrato
que no exista.

## 4. Pantalla — ¿4 pantallas = una sola imagen de 128x128 (o similar) o 4 imágenes independientes?

**Equipo:** `Display-Driver` (y quien coordine las 4 FPGAs de pantalla)
**Estado:** confirmado 64x64 píxeles reales por pantalla (64 pulsos de CLK x 32 combinaciones
de A-E x 2 mitades simultáneas, protocolo HUB75). El `.github` menciona 4 pantallas
independientes, cada una con su propia FPGA.
**Pregunta concreta:** para el próximo juego o versión que use más de una pantalla, ¿las 4
pantallas forman una sola superficie lógica más grande (por ejemplo 128x128, tiled) que la CPU
dirige con una sola coordenada global, o cada pantalla es completamente independiente y la CPU
le manda tramas SPI distintas a cada una por separado?
**Por qué bloquea:** no bloquea los 4 juegos actuales (cada uno asume una sola pantalla de
64x64), pero sí cualquier extensión futura que use más de una pantalla a la vez.

## 5. Toolchain de compilación C para RISC-V (RV32I / femtorv32)

**Estado:** se buscó en los 14 repositorios de la organización y no existe ningún compilador
cruzado, linker script, ni Makefile de compilación de C para RISC-V — solo Makefiles de
simulación de Verilog (`iverilog`/`vvp`) para un template de ejemplo ajeno a este proyecto.
**Pregunta concreta:** ¿quién es responsable de proveer el toolchain (`riscv32-unknown-elf-gcc`
o similar), el linker script que ubique el código en `0x000000` (BRAM) y el mecanismo real
para cargar el binario compilado en la FPGA (JTAG, SPI flash boot, etc.)?
**Por qué bloquea:** sin esto, ningún código en C (ni `hal_real/`, ni los juegos) puede
ejecutarse en hardware real todavía — es un bloqueador de infraestructura que no corresponde
resolver solo desde el subgrupo de software de juegos.

## 6. Offsets de registro dentro de cada rango de memoria (ASUMIDOS, no confirmados)

**Equipos:** todos los de periféricos (`ps2_keyboard.v`, `ps2_mouse`, `Display-Driver`,
`spi_flash_ctrl`, etc.)
**Estado:** cada equipo documentó el protocolo (temporización, formato de trama) pero ninguno
documentó el offset exacto de sus registros de datos/estado/control dentro de su rango de
64KB. En `hal_shared.h` se propuso el patrón estándar (`+0x00` datos, `+0x04` estado,
`+0x08` control) y se marcó explícitamente como **ASUMIDO**. El equipo de UART es la única
excepción parcial: sí publicó una tabla de registros "propuestos" (`0x400000 TX_DATA`,
`0x400004 RX_DATA`, `0x400008 STATUS`), pero la palabra "propuestos" indica que tampoco está
100% confirmado que el RTL ya los implemente exactamente así.
**Pregunta concreta:** ¿cada equipo puede confirmar (o corregir) esos offsets dentro de su
propio rango antes de que probemos contra hardware real? En particular, `spi_flash_ctrl` no
documentó ningún registro CSR propio — solo el protocolo SPI genérico de la memoria Flash —
así que ahí ni siquiera hay una "propuesta" que confirmar, hay que pedirla desde cero.
**Por qué bloquea:** si los offsets reales son distintos, el código seguirá compilando pero
leerá/escribirá en el registro equivocado al conectar hardware real — un bug silencioso.

## 8. Formato exacto de la trama SPI del Display (ancho de cada campo)

**Equipo:** `Display-Driver`
**Estado:** confirmado que la trama es de 32 bits y lleva "Comando + Coordenada + Color
(RGB565)" (cita textual de su README), pero NO se documentó cuántos bits ocupa cada campo
dentro de esos 32. En `hal_real/display_spi.c` se implementó un reparto ASUMIDO: 4 bits de
comando + 6 bits de X + 6 bits de Y + 16 bits de color (los 6 bits de X/Y son el mínimo
necesario para cubrir 0-63 en la resolución real confirmada de 64x64).
**Pregunta concreta:** ¿cuál es el ancho y la posición real de cada campo dentro de la trama
de 32 bits? ¿Existe más de un valor de Comando (por ejemplo, para "limpiar pantalla" o
"rellenar región"), o el único comando real es "escribir un píxel"?
**Por qué bloquea:** si el reparto de bits real es distinto, cada píxel se dibujará en la
coordenada o el color equivocado al conectar hardware real, aunque el código compile y corra
sin errores.

## 9. Algoritmo de CRC y opcodes de comando del Chain Bus (M5Stack, marcador de puntaje)

**Equipo:** `UART` (capa de aplicación Chain Bus, sobre la física UART ya bien documentada)
**Estado:** el formato de trama (cabecera `AA 55`, longitud, `Index_id`, `Cmd`, payload, CRC,
cola `55 AA`) sí está confirmado por el equipo. Lo que NO está confirmado es (a) el algoritmo
exacto de CRC — el README solo dice "checksum de la trama" y remite a la referencia oficial
de M5Stack (`docs.m5stack.com/en/protocol/U218/UART`) sin copiar el algoritmo — y (b) los
valores numéricos de `Cmd` para cada operación real (solo documentaron `0x10` como ejemplo de
"fijar modo de pantalla"; no hay un `Cmd` documentado para "mostrar texto" o "escribir
píxeles"). En `hal_real/uart_chainbus.c` se implementó el armado de trama completo pero con
un CRC de **relleno** (XOR simple), marcado explícitamente como NO real.
**Pregunta concreta:** ¿alguien del equipo puede revisar la referencia oficial de M5Stack
(enlace arriba) y traer el algoritmo de CRC exacto y la tabla completa de `Cmd` que ya usaron
o piensan usar para mostrar el puntaje?
**Por qué bloquea:** sin el CRC correcto, el módulo Chain Mono real rechazará todas las
tramas (los checksums no coincidirán) y el marcador de puntaje no se va a mostrar nunca,
aunque toda la lógica de UART y de armado de trama esté bien.

## 7. Scan codes estándar del teclado sin confirmar en texto

**Equipo:** `ps2_keyboard.v`
**Estado:** su README confirma en texto los códigos de las flechas (`E0 75/72/6B/74`) y el
ejemplo de Shift+A, pero Enter (`5A`), Espacio (`29`) y Shift derecho (`59`) son valores
estándar del set 2 que solo aparecen en una imagen (`Scan_codes.png`) referenciada, no en texto
legible.
**Pregunta concreta:** ¿pueden confirmar esos 3 códigos contra su propia tabla antes de usarlos
en hardware real?
**Por qué bloquea:** si alguno de esos 3 valores estándar no coincide con la tabla real que
usaron, los botones START/SELECT del juego no responderían en hardware aunque el resto del
teclado funcione bien.
