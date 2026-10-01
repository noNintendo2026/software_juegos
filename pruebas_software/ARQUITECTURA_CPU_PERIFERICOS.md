# Arquitectura real: CPU ↔ Periféricos (noNintendo2026)

Diagrama armado a partir del mapa de memoria del `.github` de la organización y de la
documentación real de cada repositorio de periférico (ps2_keyboard.v, ps2_mouse, I2S_tx.v,
Display-Driver, UART, I2C_Master, nes_controller.v). Vive aquí, en la carpeta donde estamos
trabajando el software aparte del repositorio de GitHub, para no interferir con los equipos de
hardware que siguen escribiendo en sus propias carpetas.

## Diagrama de flujo completo

```mermaid
flowchart TB
    CPU["RISC-V RV32I / femtorv32\n(caja negra, corre el juego en C)"]

    subgraph BUS["Bus de direcciones y registros mapeados en memoria"]
        direction TB
        R1["0x000000-0x3FFFFF\nBRAM (software_juegos)"]
        R2["0x400000-0x40FFFF\nUART"]
        R3["0x420000-0x42FFFF\nSPI Flash"]
        R4["0x430000-0x43FFFF\nPS/2 Teclado"]
        R5["0x440000-0x44FFFF\nPS/2 Mouse"]
        R6["0x450000-0x45FFFF\nNES Controller"]
        R7["0x460000-0x46FFFF\nI2C Master"]
        R8["0x470000-0x47FFFF\nI2S Audio"]
        R9["0x480000-0x4FFFFF\nDisplay Driver"]
    end

    CPU <--> BUS

    R1 -. "hal.h / nuestro codigo\nvive aqui" .-> CODE["space_invaders / roadFighter\n/ snake / pong (C, usa hal.h)"]

    R2 <-->|"Serial\nasincrono"| UARTDEV["Debug UART\n+ MultijugadorRed\n(otra FPGA)"]
    R3 <-->|"SPI"| FLASH["SPI Flash\n(juegos guardados)"]

    R4 <--|"PS/2: CLK+DATA\nscan codes set 2\nmake/break (F0)/ext(E0)"| KBD["Teclado PS/2"]
    R5 <--|"PS/2: CLK+DATA\npaquete 3-4 bytes\nX,Y (2's compl), botones"| MOUSE["Mouse PS/2\n(IntelliMouse)"]
    R6 <-.->|"protocolo aun\nSIN documentar"| NES["Control NES"]

    R7 <-->|"I2C (SDA/SCL)"| I2CDEV["EEPROM / periferico I2C\n(guardado de puntajes)"]
    R8 -->|"BCLK, LRCLK, SD\n(solo salida)"| DAC["DAC I2S -> parlante"]
    R9 -->|"SPI: 32 bits\nComando+Coord+RGB565"| DISP_FPGA["FPGA del Display\n(framebuffer propio)"]
    DISP_FPGA -->|"HUB75: RGB x2, CLK,\nOE, LAT, A-E\n(interno, no es cosa\nde software)"| LED["Matriz LED fisica\n(x4 pantallas)"]

    style CODE fill:#1a3d6d,color:#fff,stroke:#0d2847
    style R1 fill:#1a3d6d,color:#fff,stroke:#0d2847
    style R6 stroke-dasharray: 5 5
    style NES stroke-dasharray: 5 5
```

## Qué significa cada flecha para nuestro código (hal.h)

| Periférico | Dirección | Lo que hal.h debe hacer | Estado real hoy |
|---|---|---|---|
| **Display** (`0x480000`) | CPU → Display FPGA (SPI) | Por cada píxel: mandar una trama SPI de 32 bits = Comando + Coordenada (X,Y) + Color **RGB565** | `sendSprite`/`sendBackground` son stubs vacíos; el color (`hex2rgb`/`rgb2hex`) ya está bien, en RGB565 |
| **Teclado** (`0x430000`) | Teclado → CPU (PS/2) | Leer bytes de scan code set 2, detectar prefijos `F0` (soltar) / `E0` (extendida), traducir a `Action` | `readControls()` asume 1 byte = 1 tecla; no alcanza para secuencias de 2-4 bytes |
| **Mouse** (`0x440000`) | Mouse → CPU (PS/2) | Leer paquete de 3-4 bytes, decodificar X/Y (complemento a 2) y botones | No existe ningún tipo de dato para mouse en el HAL todavía |
| **NES** (`0x450000`) | Control → CPU | — | Protocolo aún sin documentar por ese equipo; no se puede diseñar esta parte todavía |
| **I2C** (`0x460000`) | CPU ↔ dispositivo I2C | Guardar/leer puntajes | Ese equipo remite a una plantilla externa; falta protocolo propio documentado |
| **I2S** (`0x470000`) | Solo salida, CPU → DAC | — | El audio se "hornea" en una ROM de Verilog ANTES de sintetizar (`wav2mem.py`); el HAL probablemente no necesita `playSound()`, como mucho un registro de "disparo" (hay que preguntarle a ese equipo si existe) |
| **UART** (`0x400000`) | Bidireccional | Debug y, por separado, multijugador por red entre FPGAs | Protocolo base documentado en su repo, pendiente de revisar en detalle |
| **SPI Flash** (`0x420000`) | CPU ↔ memoria | Guardar/cargar juegos | No analizado en detalle todavía |

## Nota sobre dónde vive este archivo

Este diagrama está en `workspace/space_invaders_sdl2_port/`, la misma carpeta separada donde
hicimos el port de Space Invaders a SDL2 — **no** está en el clon del repositorio de GitHub, para
no interferir con el trabajo de los compañeros de hardware. Cuando el equipo de software decida
que es momento de subir estos cambios al repositorio real, ustedes deciden cómo y cuándo
integrarlo.
