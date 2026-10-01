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

- **`PENDIENTES_EXTERNOS.md`** — lista concreta de lo que falta confirmar con otros equipos
  (protocolo NES, protocolo I2C, registro de disparo de I2S, combinación de las 4 pantallas,
  toolchain de RISC-V, offsets de registro asumidos, 3 scan codes sin confirmar en texto).
  Son preguntas, no respuestas inventadas.

## Nota

Esta carpeta vive en una rama (`software-juan-camilo-pruebas`), no en `main`, precisamente para no
interrumpir el trabajo de nadie — el equipo decide cuándo y cómo fusionarla.
