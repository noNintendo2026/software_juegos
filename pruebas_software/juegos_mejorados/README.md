# juegos_mejorados

Contenidos pendientes (punto 7 de los 8 problemas para conectar hardware real a los juegos).
No se modifica ninguna carpeta de los compañeros — estos archivos solo **incluyen** (no
copian) los headers públicos de `roadFighter/include/` para reusar sus sprites y su `hal.h`
("no importa si repetimos codigo", instrucción explícita del dueño de este subgrupo).

## roadFighter: de "mover un auto sin límites" a las 4 mecánicas reales del README

El `main.c` original de `roadFighter/` solo mueve el auto libremente por la pantalla, sin
límites, sin combustible, sin tráfico y sin choques — a pesar de que los sprites para todo
esto (`BlueCar`, `YellowCar`, `Truck`, `CarDestroyed`, `Crash1/2/3`) ya estaban en
`roadFighter/include/assets.h` sin usar.

- **`roadfighter_logica.h` / `.c`** — lógica real y **pura** (sin SDL, sin `hal.h`) de las 4
  mecánicas del README: carrera contra el reloj (puntaje = distancia), combustible (se
  consume solo, se recarga chocando con el camión), tráfico dinámico (autos azules/amarillos +
  camión, generados con un PRNG con semilla explícita para que sea reproducible en pruebas), y
  choques (pierden velocidad, 3 choques = fin del juego). También corrige el bug real del
  `main.c` original: el auto podía salirse de la pantalla (`x--`/`x++` sin límites).
  Probado: 4/4 `[OK]`.
- **`RoadFighter_Mejorado.c`** — versión jugable (SDL2) que conecta esa lógica con
  `sendSprite`/`sendBackground`/`sendString` de `hal.h` y los sprites reales ya existentes.
  Compila limpio y corre estable (verificado).

## pong: la carpeta no tenía un juego de Pong real

Confirmado con `diff`: `pong/Space_Invaders.c` es **copia byte-idéntica** de
`space_invaders/Space_Invaders.c`. No existe ningún Pong en el repositorio.

- **`pong_logica.h` / `.c`** — Pong clásico implementado desde cero (mecánica genérica de
  dominio público, sin ningún asset ni elemento gráfico de ningún Pong real): pelota + 2
  paletas, rebote en paredes y paletas, puntaje, fin del juego a 5 puntos. Un jugador controla
  la paleta izquierda; la paleta derecha la controla una IA simple y vencible (todavía no hay
  un segundo dispositivo de entrada confiable para 2 jugadores locales — ver
  `PENDIENTES_EXTERNOS.md`). La propia prueba de este archivo encontró y permitió corregir un
  bug real: la pelota podía "rebotar" aunque ya hubiera pasado la línea de gol, si la paleta se
  movía hacia esa altura en el mismo instante. Probado: 3/3 `[OK]`.
- **`Pong_Real.c`** — versión jugable (SDL2). Paletas y pelota son rectángulos sólidos
  generados en código (sin ningún asset externo). Compila limpio y corre estable (verificado).

## Cómo compilar (ejemplo con MinGW + SDL2, mismo patrón usado en el resto de `hal_real/`)

```bash
# Desde esta carpeta, con roadFighter/ y SDL2-<version>-mingw/ como hermanos del repo:
gcc RoadFighter_Mejorado.c roadfighter_logica.c -o RoadFighter_Mejorado.exe \
    -I../../roadFighter/include -I../../SDL2-2.30.9/i686-w64-mingw32/include/SDL2 \
    -I../../SDL2-2.30.9/i686-w64-mingw32/include -L../../SDL2-2.30.9/i686-w64-mingw32/lib \
    -lmingw32 -lSDL2main -lSDL2
# copiar SDL2.dll junto al .exe antes de correrlo

gcc Pong_Real.c pong_logica.c -o Pong_Real.exe \
    -I../../roadFighter/include -I../../SDL2-2.30.9/i686-w64-mingw32/include/SDL2 \
    -I../../SDL2-2.30.9/i686-w64-mingw32/include -L../../SDL2-2.30.9/i686-w64-mingw32/lib \
    -lmingw32 -lSDL2main -lSDL2
```

## Lo que queda sin tocar (honesto, no es parte de esta pasada)

`snake/snake.c` ya es un juego completo y jugable, pero es standalone de consola de Windows
(`conio.h`/`windows.h`), no usa `hal.h` ni el contrato HAL compartido — por lo tanto no puede
correr contra el hardware real (pantalla 64x64, PS/2, etc.) sin una reescritura completa de su
capa de entrada/salida. Se documenta aquí como brecha conocida, pero no se tocó en esta pasada
por ser la de menor prioridad frente a la conectividad de hardware real (puntos 1-6 y parte del
8, ya resueltos).
