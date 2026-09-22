//Imports de librerias
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

//Definicion de constantes
#define dimX 55
#define dimY 25
#define tamCuerpo 3
#define scoreMov 0.25
#define scoreFru 50
#define MAX_NOMBRE 50
#define MAX_PUNTAJES 100

int offsetX = 0;
int offsetY = 0;
char dirActual = 'a';
char dirActual2 = 'd';
int  modo2Jugadores = 0;
int  pedidoPausa = 0;

// Codigos de teclas
#define TECLA_ARRIBA   72
#define TECLA_ABAJO    80
#define TECLA_IZQ      75
#define TECLA_DER      77
#define TECLA_ENTER    13
#define TECLA_ESC      27

/*Definicion de estructuras*/
typedef struct { int x, y; } Fruta;

typedef struct Serpiente {
    int x, y;
    struct Serpiente* sig;
} Serpiente;

typedef struct {
    char nombre[MAX_NOMBRE];
    int puntaje;
} EntradaRanking;

//Prototipos
int  menuPrincipal();
int  seleccionarModo();
void iniciarJuego();
void pedirNombres(char nombre1[MAX_NOMBRE], char nombre2[MAX_NOMBRE]);
void guardarPuntaje(const char nombre[MAX_NOMBRE], int puntaje);
void generarTablero(char t[dimY][dimX]);
void iniciarMapa(char tablero[dimY][dimX]);
void mostrarTablero(char tablero[dimY][dimX]);
Serpiente* crearNodo(int x, int y);
Serpiente* crearSerpienteEn(int cx, int cy);
void actualizarSerpiente(Serpiente* cabeza, int color, char dir);
void dibujarSerpiente(Serpiente* cabeza, int color, char dir);
Serpiente* moverSerpiente(Serpiente* cabeza, char dir);
void procesarEntrada();
int  menuPausa();
void borrarCursor();
void gotoxy(int x, int y);
void gotoxyTablero(int x, int y);
void calcularOffset();
void generarFruta(Fruta* f, Serpiente* s1, Serpiente* s2);
void dibujarFruta(Fruta f);
void agrandarSerpiente(Serpiente* cabeza, int* tamano);
bool comeFruta(Fruta f, Serpiente* cabeza);
bool colisionCuerpoPropio(Serpiente* cabeza);
bool colisionConOtra(Serpiente* cabeza, Serpiente* otra);
bool cabezasJuntas(Serpiente* a, Serpiente* b);
void imprimirPuntos(int p1, int p2);
void sonidoComerFruta();
void dibujarGameOver(const char* n1, int p1, const char* n2, int p2, int ganador, int invertido);
void juegoTerminado(const char* n1, int p1, const char* n2, int p2, int ganador, Serpiente* s1, Serpiente* s2);
void mostrarCreditos();
void maximizarConsola();
void vaciarLista(Serpiente* cabeza);
void mostrarRanking();
void mostrarInstrucciones();
void salidaJuego();
void dibujarMarco();
void dibujarMenuPrincipal(int seleccion);
void dibujarModoMenu(int seleccion);
void dibujarMenuPausa(int seleccion);

int main() {
    maximizarConsola();
    borrarCursor();

    int corriendo = 1;
    while (corriendo) {
        int op = menuPrincipal();
        switch (op) {
        case 0: {  // Jugar
            int modo = seleccionarModo();
            if (modo == 1) { modo2Jugadores = 0; iniciarJuego(); }
            else if (modo == 2) { modo2Jugadores = 1; iniciarJuego(); }
            // Si modo == 0 → el usuario aprieto ESC, no hacer nada
            break;
        }
        case 1: mostrarRanking(); break;
        case 2: mostrarCreditos(); break;
        case 3: mostrarInstrucciones(); break;
        case 4: salidaJuego(); corriendo = 0; break;
        }
    }
    return 0;
}

//------------------- UTILIDADES -------------------

void gotoxy(int x, int y) {
    COORD coord; coord.X = x; coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
void gotoxyTablero(int x, int y) { gotoxy(x + offsetX, y + offsetY); }

void calcularOffset() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int anchoConsola = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int altoConsola = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int espacioArriba = 4;
    offsetX = (anchoConsola - dimX) / 2;
    offsetY = (altoConsola - dimY - espacioArriba) / 2 + espacioArriba;
    if (offsetX < 0) offsetX = 0;
    if (offsetY < espacioArriba) offsetY = espacioArriba;
}

void borrarCursor() {
    CONSOLE_CURSOR_INFO info = { 100, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

void maximizarConsola() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_MAXIMIZE);
    Sleep(100);
}

// Lee una tecla. Devuelve: 'U'=arriba, 'D'=abajo, 'L'=izq, 'R'=der, 'E'=enter, 27=esc, o el char
int leerTeclaMenu() {
    int k = _getch();
    if (k == 0 || k == 224) {
        int ext = _getch();
        switch (ext) {
        case TECLA_ARRIBA: return 'U';
        case TECLA_ABAJO:  return 'D';
        case TECLA_IZQ:    return 'L';
        case TECLA_DER:    return 'R';
        }
        return -1;
    }
    if (k == TECLA_ENTER) return 'E';
    if (k == TECLA_ESC)   return 27;
    return -1;
}

//------------------- DIBUJO MENU PRINCIPAL -------------------

void dibujarMenuPrincipal(int seleccion) {
    system("cls");
    int x = 25, y = 2, anchoCaja = 75, altoCaja = 22;

    gotoxy(x, y);
    printf("\033[92;1m%c", 201);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 187);

    for (int i = 1; i < altoCaja; i++) {
        gotoxy(x, y + i);
        printf("\033[92;1m%c\033[0m", 186);
        for (int j = 0; j < anchoCaja; j++) printf(" ");
        printf("\033[92;1m%c\033[0m", 186);
    }

    gotoxy(x, y + altoCaja);
    printf("\033[92;1m%c", 200);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 188);

    gotoxy(x + 32, y + 1);
    printf("\033[92;1mS N A K E\033[0m");

    // Serpiente ASCII decorativa
    gotoxy(x + 3, y + 3);  printf("\033[92m             /^\\/^\\\033[0m");
    gotoxy(x + 3, y + 4);  printf("\033[92m           _|_O|  O|\033[0m");
    gotoxy(x + 3, y + 5);  printf("\033[92m  \\/     /~     \\_/ \\\033[0m");
    gotoxy(x + 3, y + 6);  printf("\033[92m   \\____|__________/  \\\033[0m");
    gotoxy(x + 3, y + 7);  printf("\033[92m          \\_______      \\\033[0m");
    gotoxy(x + 3, y + 8);  printf("\033[92m                  `\\     \\\033[0m");
    gotoxy(x + 3, y + 9);  printf("\033[92m                    |     |\033[0m");
    gotoxy(x + 3, y + 10); printf("\033[92m                   /      /\033[0m");
    gotoxy(x + 3, y + 11); printf("\033[92m                  /     /\033[0m");
    gotoxy(x + 3, y + 12); printf("\033[92m                /     /\033[0m");
    gotoxy(x + 3, y + 13); printf("\033[92m               /    /\033[0m");
    gotoxy(x + 3, y + 14); printf("\033[92m             (     (\033[0m");
    gotoxy(x + 3, y + 15); printf("\033[92m              \\     ~-____-~\033[0m");
    gotoxy(x + 3, y + 16); printf("\033[92m                ~-_          ~-_\033[0m");
    gotoxy(x + 3, y + 17); printf("\033[92m                   ~--______-~\033[0m");

    // Opciones con navegacion
    const char* opciones[5] = { "Jugar", "Ver Ranking", "Creditos", "Instrucciones", "Salir" };

    for (int i = 0; i < 5; i++) {
        gotoxy(x + 42, y + 4 + i * 2);
        if (i == seleccion) {
            // Opcion seleccionada: resaltada con flecha y fondo
            printf("\033[93;7;1m > %s \033[0m", opciones[i]);
        }
        else {
            printf("\033[93;1m   \033[0m\033[97m%s\033[0m", opciones[i]);
        }
    }

    gotoxy(x + 42, y + 16); printf("\033[90m--------------------\033[0m");
    gotoxy(x + 42, y + 17); printf("\033[96mFLECHAS: Mover\033[0m");
    gotoxy(x + 42, y + 18); printf("\033[96mENTER: Confirmar\033[0m");

    gotoxy(x + 26, y + altoCaja + 1);
    printf("\033[96;1mUsa las flechas y ENTER para elegir\033[0m");
}

int menuPrincipal() {
    int seleccion = 0;

    while (1) {
        dibujarMenuPrincipal(seleccion);
        int k = leerTeclaMenu();

        if (k == 'U') seleccion = (seleccion + 4) % 5;
        else if (k == 'D') seleccion = (seleccion + 1) % 5;
        else if (k == 'E') return seleccion;
        else if (k == 27) return 4;  // ESC = Salir
    }
}

//------------------- DIBUJO SUBMENU MODO -------------------

void dibujarModoMenu(int seleccion) {
    system("cls");
    int x = 33, y = 8, anchoCaja = 44, altoCaja = 10;

    gotoxy(x, y);
    printf("\033[96;1m%c", 201);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 187);

    for (int i = 1; i < altoCaja; i++) {
        gotoxy(x, y + i);
        printf("\033[96;1m%c\033[0m", 186);
        for (int j = 0; j < anchoCaja; j++) printf(" ");
        printf("\033[96;1m%c\033[0m", 186);
    }

    gotoxy(x, y + altoCaja);
    printf("\033[96;1m%c", 200);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 188);

    gotoxy(x + 14, y + 1);
    printf("\033[96;1mMODO DE JUEGO\033[0m");

    const char* opciones[2] = { "1 Jugador", "2 Jugadores" };

    for (int i = 0; i < 2; i++) {
        gotoxy(x + 8, y + 4 + i * 2);
        if (i == seleccion) {
            printf("\033[93;7;1m > %s \033[0m", opciones[i]);
        }
        else {
            printf("\033[97m   %s\033[0m", opciones[i]);
        }
    }

    gotoxy(x + 8, y + 8);
    printf("\033[90mFLECHAS + ENTER | ESC: volver\033[0m");
}

// Retorna 1, 2 o 0 (ESC)
int seleccionarModo() {
    int seleccion = 0;

    while (1) {
        dibujarModoMenu(seleccion);
        int k = leerTeclaMenu();

        if (k == 'U') seleccion = (seleccion + 1) % 2;
        else if (k == 'D') seleccion = (seleccion + 1) % 2;
        else if (k == 'E') return seleccion + 1;
        else if (k == 27) return 0;
    }
}

//------------------- PEDIR NOMBRES -------------------

void pedirNombres(char nombre1[MAX_NOMBRE], char nombre2[MAX_NOMBRE]) {
    system("cls");
    borrarCursor();

    if (modo2Jugadores) {
        gotoxy(10, 5);
        printf("\033[92;1m=== JUGADOR 1 (VERDE - WASD) ===\033[0m");
        gotoxy(10, 7);
        printf("\033[92mIngrese su nombre: \033[0m");
        scanf_s("%s", nombre1, MAX_NOMBRE);

        system("cls");
        gotoxy(10, 5);
        printf("\033[94;1m=== JUGADOR 2 (AZUL - FLECHAS) ===\033[0m");
        gotoxy(10, 7);
        printf("\033[94mIngrese su nombre: \033[0m");
        scanf_s("%s", nombre2, MAX_NOMBRE);
    }
    else {
        gotoxy(10, 5);
        printf("\033[92;1m=== JUGADOR 1 ===\033[0m");
        gotoxy(10, 7);
        printf("\033[92mIngrese su nombre: \033[0m");
        scanf_s("%s", nombre1, MAX_NOMBRE);
    }
}

//------------------- DIBUJO MENU PAUSA -------------------

void dibujarMenuPausa(int seleccion) {
    int anchoCaja = 52, altoCaja = 13;
    int x = offsetX + (dimX - anchoCaja) / 2;
    int y = offsetY + (dimY - altoCaja) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    // Marco
    gotoxy(x, y);
    printf("\033[93;1m%c", 201);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 187);

    for (int i = 1; i < altoCaja; i++) {
        gotoxy(x, y + i);
        printf("\033[93;1m%c\033[0m", 186);
        for (int j = 0; j < anchoCaja; j++) printf(" ");
        printf("\033[93;1m%c\033[0m", 186);
    }

    gotoxy(x, y + altoCaja);
    printf("\033[93;1m%c", 200);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 188);

    // Titulo
    gotoxy(x + 5, y + 2);
    printf("\033[93;1mSEGURO QUE QUIERES SALIR DEL JUEGO?\033[0m");

    // Opciones con navegacion
    const char* opciones[3] = { "REANUDAR PARTIDA", "VOLVER AL MENU PRINCIPAL", "SALIR DEL JUEGO" };

    for (int i = 0; i < 3; i++) {
        gotoxy(x + 5, y + 5 + i * 2);
        if (i == seleccion) {
            printf("\033[93;7;1m > %s \033[0m", opciones[i]);
        }
        else {
            printf("\033[97m   %s\033[0m", opciones[i]);
        }
    }

    gotoxy(x + 5, y + 11);
    printf("\033[90mFLECHAS + ENTER | ESC: reanudar\033[0m");
}

// Retorna: 0=reanudar, 1=menu, 2=salir
int menuPausa() {
    int seleccion = 0;

    while (1) {
        dibujarMenuPausa(seleccion);
        int k = leerTeclaMenu();

        if (k == 'U') seleccion = (seleccion + 2) % 3;
        else if (k == 'D') seleccion = (seleccion + 1) % 3;
        else if (k == 'E') return seleccion;
        else if (k == 27) return 0;  // ESC = reanudar
    }
}

//------------------- JUEGO PRINCIPAL -------------------

void iniciarJuego() {
    int tamano = 0, tamano2 = 0, contador = 0;
    int puntos = 0, puntos2 = 0, velocidad = 130;
    char tablero[dimY][dimX];
    char nombreJugador[MAX_NOMBRE] = "J1";
    char nombreJugador2[MAX_NOMBRE] = "J2";

    pedirNombres(nombreJugador, nombreJugador2);
    system("cls");
    borrarCursor();

    calcularOffset();
    iniciarMapa(tablero);

    Serpiente* serpiente1 = NULL;
    Serpiente* serpiente2 = NULL;

    if (modo2Jugadores) {
        serpiente1 = crearSerpienteEn(8, dimY / 2);
        serpiente2 = crearSerpienteEn(dimX - 8, dimY / 2);
        dirActual = 'w';
        dirActual2 = 'w';
    }
    else {
        serpiente1 = crearSerpienteEn(dimX / 2, dimY / 2);
        dirActual = 'a';
    }

    Fruta fru;
    srand((unsigned)time(NULL));
    generarFruta(&fru, serpiente1, serpiente2);

    while (_kbhit()) _getch();
    pedidoPausa = 0;

    dibujarMarco();
    imprimirPuntos(puntos, puntos2);
    dibujarFruta(fru);
    dibujarSerpiente(serpiente1, 32, dirActual);
    if (modo2Jugadores) dibujarSerpiente(serpiente2, 34, dirActual2);

    int ganador = 0;

    while (1) {
        procesarEntrada();

        if (pedidoPausa) {
            pedidoPausa = 0;
            int r = menuPausa();

            if (r == 1) {
                vaciarLista(serpiente1);
                if (serpiente2) vaciarLista(serpiente2);
                return;
            }
            if (r == 2) {
                vaciarLista(serpiente1);
                if (serpiente2) vaciarLista(serpiente2);
                salidaJuego();
            }

            // r == 0: reanudar
            system("cls");
            calcularOffset();
            iniciarMapa(tablero);
            dibujarMarco();
            imprimirPuntos(puntos, puntos2);
            dibujarSerpiente(serpiente1, 32, dirActual);
            if (modo2Jugadores) dibujarSerpiente(serpiente2, 34, dirActual2);
            dibujarFruta(fru);
            continue;
        }

        dibujarFruta(fru);

        actualizarSerpiente(serpiente1, 32, dirActual);
        if (modo2Jugadores) {
            actualizarSerpiente(serpiente2, 34, dirActual2);
        }

        imprimirPuntos(puntos, puntos2);

        Sleep(velocidad);
        contador++;
        puntos = (int)((scoreFru * tamano) + (scoreMov * contador));

        if (comeFruta(fru, serpiente1)) {
            agrandarSerpiente(serpiente1, &tamano);
            generarFruta(&fru, serpiente1, serpiente2);
            dibujarFruta(fru);
            if (velocidad > 60) velocidad -= 3;
        }
        if (modo2Jugadores && comeFruta(fru, serpiente2)) {
            agrandarSerpiente(serpiente2, &tamano2);
            puntos2 += scoreFru;
            generarFruta(&fru, serpiente1, serpiente2);
            dibujarFruta(fru);
            if (velocidad > 60) velocidad -= 3;
        }

        bool muerto1 = colisionCuerpoPropio(serpiente1);
        bool muerto2 = modo2Jugadores ? colisionCuerpoPropio(serpiente2) : false;

        if (modo2Jugadores) {
            if (colisionConOtra(serpiente1, serpiente2)) muerto1 = true;
            if (colisionConOtra(serpiente2, serpiente1)) muerto2 = true;
            if (cabezasJuntas(serpiente1, serpiente2)) { muerto1 = true; muerto2 = true; }
        }

        if (muerto1 && muerto2) { ganador = 0; break; }
        if (muerto1) { ganador = 2; break; }
        if (muerto2) { ganador = 1; break; }
    }

    juegoTerminado(nombreJugador, puntos, nombreJugador2, puntos2, ganador, serpiente1, serpiente2);
}

//------------------- TABLERO -------------------

void generarTablero(char t[dimY][dimX]) {
    for (int i = 0; i < dimY; i++) {
        for (int j = 0; j < dimX; j++) {
            if (i == 0 && j == 0) t[i][j] = '\xC9';
            else if (i == 0 && j == dimX - 1) t[i][j] = '\xBB';
            else if (i == dimY - 1 && j == 0) t[i][j] = '\xC8';
            else if (i == dimY - 1 && j == dimX - 1) t[i][j] = '\xBC';
            else if (i == 0 || i == dimY - 1) t[i][j] = '\xCD';
            else if (j == 0 || j == dimX - 1) t[i][j] = '\xBA';
            else t[i][j] = ' ';
        }
    }
}
void mostrarTablero(char tablero[dimY][dimX]) {
    for (int i = 0; i < dimY; i++) {
        gotoxyTablero(0, i);
        for (int j = 0; j < dimX; j++) printf("%c", tablero[i][j]);
    }
}
void iniciarMapa(char tablero[dimY][dimX]) {
    generarTablero(tablero);
    mostrarTablero(tablero);
}
void dibujarMarco() {
    gotoxy(offsetX, offsetY - 3);
    printf("\033[90m");
    for (int i = 0; i < dimX; i++) printf("\xC4");
    printf("\033[0m");
}

//------------------- SERPIENTE -------------------

Serpiente* crearNodo(int x, int y) {
    Serpiente* nuevo = (Serpiente*)malloc(sizeof(Serpiente));
    if (!nuevo) { printf("\nError malloc\n"); return NULL; }
    nuevo->x = x; nuevo->y = y; nuevo->sig = NULL;
    return nuevo;
}

Serpiente* crearSerpienteEn(int cx, int cy) {
    Serpiente* cabeza = crearNodo(cx, cy);
    Serpiente* aux = cabeza;
    for (int i = 0; i < tamCuerpo; i++) {
        aux->sig = crearNodo(cx, cy + (i + 1));
        aux = aux->sig;
    }
    return cabeza;
}

void dibujarSerpiente(Serpiente* cabeza, int color, char dir) {
    if (cabeza == NULL) return;

    gotoxyTablero(cabeza->x, cabeza->y);
    printf("\033[%d;1m", color);
    switch (dir) {
    case 'w': printf("%c", 0x18); break;
    case 's': printf("%c", 0x19); break;
    case 'a': printf("%c", 0x11); break;
    case 'd': printf("%c", 0x10); break;
    default:  printf("%c", 0x10); break;
    }
    printf("\033[0m");

    Serpiente* actual = cabeza->sig;
    while (actual != NULL) {
        gotoxyTablero(actual->x, actual->y);
        if (actual->sig != NULL) printf("\033[%dm%c\033[0m", color, 0xDB);
        else printf("\033[2;%dm%c\033[0m", color, 0xB2);
        actual = actual->sig;
    }
}

void actualizarSerpiente(Serpiente* cabeza, int color, char dir) {
    if (cabeza == NULL) return;

    Serpiente* aux = cabeza;
    while (aux->sig != NULL) aux = aux->sig;
    int colaX = aux->x, colaY = aux->y;

    moverSerpiente(cabeza, dir);

    gotoxyTablero(colaX, colaY); printf(" ");

    dibujarSerpiente(cabeza, color, dir);
}

Serpiente* moverSerpiente(Serpiente* cabeza, char dir) {
    if (cabeza == NULL) return NULL;
    int prevX = cabeza->x, prevY = cabeza->y;

    switch (dir) {
    case 'w': cabeza->y--; if (cabeza->y < 1) cabeza->y = dimY - 2; break;
    case 's': cabeza->y++; if (cabeza->y > dimY - 2) cabeza->y = 1; break;
    case 'a': cabeza->x--; if (cabeza->x < 1) cabeza->x = dimX - 2; break;
    case 'd': cabeza->x++; if (cabeza->x > dimX - 2) cabeza->x = 1; break;
    }

    int tempX, tempY;
    Serpiente* aux = cabeza->sig;
    while (aux != NULL) {
        tempX = aux->x; tempY = aux->y;
        aux->x = prevX; aux->y = prevY;
        prevX = tempX; prevY = tempY;
        aux = aux->sig;
    }
    return cabeza;
}

//------------------- ENTRADA -------------------

void procesarEntrada() {
    while (_kbhit()) {
        int k = _getch();

        if (k == 0 || k == 224) {
            int ext = _getch();
            if (modo2Jugadores) {
                switch (ext) {
                case TECLA_ARRIBA: if (dirActual2 != 's') dirActual2 = 'w'; break;
                case TECLA_ABAJO:  if (dirActual2 != 'w') dirActual2 = 's'; break;
                case TECLA_IZQ:    if (dirActual2 != 'd') dirActual2 = 'a'; break;
                case TECLA_DER:    if (dirActual2 != 'a') dirActual2 = 'd'; break;
                }
            }
            continue;
        }

        if (k == TECLA_ESC) { pedidoPausa = 1; continue; }

        char c = (char)tolower(k);
        if (c == 'w' && dirActual != 's') dirActual = 'w';
        else if (c == 's' && dirActual != 'w') dirActual = 's';
        else if (c == 'a' && dirActual != 'd') dirActual = 'a';
        else if (c == 'd' && dirActual != 'a') dirActual = 'd';
        else if (c == 'p') pedidoPausa = 1;
    }
}

//------------------- FRUTA -------------------

void dibujarFruta(Fruta f) {
    gotoxyTablero(f.x, f.y);
    printf("\033[91;41;1m@\033[0m");
}

void generarFruta(Fruta* f, Serpiente* s1, Serpiente* s2) {
    int ok;
    do {
        ok = 1;
        f->x = 1 + rand() % (dimX - 2);
        f->y = 1 + rand() % (dimY - 2);

        for (Serpiente* cur = s1; cur != NULL; cur = cur->sig)
            if (f->x == cur->x && f->y == cur->y) ok = 0;

        if (s2) {
            for (Serpiente* cur = s2; cur != NULL; cur = cur->sig)
                if (f->x == cur->x && f->y == cur->y) ok = 0;
        }
    } while (!ok);
}

//------------------- CRECER / COMER -------------------

void agrandarSerpiente(Serpiente* cabeza, int* tamano) {
    Serpiente* aux = cabeza;
    while (aux->sig) aux = aux->sig;
    aux->sig = crearNodo(aux->x, aux->y);
    (*tamano)++;
    sonidoComerFruta();
}

bool comeFruta(Fruta f, Serpiente* cabeza) {
    return (cabeza->x == f.x && cabeza->y == f.y);
}

//------------------- COLISIONES -------------------

bool colisionCuerpoPropio(Serpiente* cabeza) {
    for (Serpiente* cur = cabeza->sig; cur; cur = cur->sig)
        if (cabeza->x == cur->x && cabeza->y == cur->y) return true;
    return false;
}

bool colisionConOtra(Serpiente* cabeza, Serpiente* otra) {
    for (Serpiente* cur = otra; cur; cur = cur->sig)
        if (cabeza->x == cur->x && cabeza->y == cur->y) return true;
    return false;
}

bool cabezasJuntas(Serpiente* a, Serpiente* b) {
    return (a->x == b->x && a->y == b->y);
}

//------------------- HUD -------------------

void imprimirPuntos(int p1, int p2) {
    gotoxy(offsetX, offsetY - 2);
    printf("\033[93;1mJ1: %-5d\033[0m", p1);

    if (modo2Jugadores) {
        gotoxy(offsetX + 15, offsetY - 2);
        printf("\033[94;1mJ2: %-5d\033[0m", p2);
    }

    gotoxy(offsetX + dimX - 32, offsetY - 2);
    if (modo2Jugadores)
        printf("\033[92mWASD\033[0m \033[90m|\033[0m \033[94mFLECHAS\033[0m \033[90m|\033[0m \033[96mESC: Pausa\033[0m");
    else
        printf("\033[96mWASD: Mover  \033[90m|\033[0m\033[96m  ESC: Pausa\033[0m");
}

//------------------- SONIDO -------------------

DWORD WINAPI reproducirSonido(LPVOID lpParam) { Beep(440, 200); return 0; }
void sonidoComerFruta() { CreateThread(NULL, 0, reproducirSonido, NULL, 0, NULL); }

//------------------- GAME OVER -------------------

void dibujarGameOver(const char* n1, int p1, const char* n2, int p2, int ganador, int invertido) {
    int anchoCaja = 56, x = 28, y = 5;
    if (invertido) printf("\033[91;7m"); else printf("\033[91;1m");

    gotoxy(x, y);
    printf("%c", 201);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c", 187);

    for (int i = 1; i < 12; i++) {
        gotoxy(x, y + i);
        printf("%c", 186);
        for (int j = 0; j < anchoCaja; j++) printf(" ");
        printf("%c", 186);
    }
    gotoxy(x, y + 12);
    printf("%c", 200);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c", 188);
    printf("\033[0m");

    gotoxy(x + 20, y + 2);
    if (invertido) printf("\033[91;7;1m  FIN DEL JUEGO  \033[0m");
    else printf("\033[91;1m  FIN DEL JUEGO  \033[0m");

    if (modo2Jugadores) {
        gotoxy(x + 4, y + 4);
        printf("\033[92;1m%s\033[0m  .........  \033[92;1m%d pts\033[0m", n1, p1);

        gotoxy(x + 4, y + 6);
        printf("\033[94;1m%s\033[0m  .........  \033[94;1m%d pts\033[0m", n2, p2);

        gotoxy(x + 18, y + 8);
        if (ganador == 0) printf("\033[93;1m*** EMPATE ***\033[0m");
        else if (ganador == 1) printf("\033[92;1mGANA %s!\033[0m", n1);
        else printf("\033[94;1mGANA %s!\033[0m", n2);
    }
    else {
        gotoxy(x + 6, y + 4);
        printf("\033[92;1mJugador:\033[0m \033[97m%s\033[0m", n1);

        gotoxy(x + 6, y + 6);
        printf("\033[93;1mPUNTAJE: %d\033[0m", p1);
    }

    gotoxy(x + 12, y + 10);
    if (invertido) printf("\033[96;7m Presiona una tecla para volver \033[0m");
    else printf("\033[96m Presiona una tecla para volver \033[0m");
}

void juegoTerminado(const char* n1, int p1, const char* n2, int p2,
    int ganador, Serpiente* s1, Serpiente* s2) {
    if (!modo2Jugadores) guardarPuntaje(n1, p1);
    vaciarLista(s1);
    if (s2) vaciarLista(s2);
    system("cls");

    for (int i = 0; i < 3; i++) {
        dibujarGameOver(n1, p1, n2, p2, ganador, 0); Sleep(300);
        system("cls");
        dibujarGameOver(n1, p1, n2, p2, ganador, 1); Sleep(300);
        system("cls");
    }
    dibujarGameOver(n1, p1, n2, p2, ganador, 0);

    while (_kbhit()) _getch();
    _getch();
    system("cls");
}

//------------------- CREDITOS -------------------

void textoCreditos() {
    gotoxy(10, 5);
    printf("\033[96;1m########################################\033[0m\n");
    gotoxy(10, 6);
    printf("\033[96;1m#\033[0m         \033[92mFernando Sarquis\033[0m             \033[96;1m#\033[0m\n");
    gotoxy(10, 7);
    printf("\033[96;1m#\033[0m         \033[92mJulian Tesoriero\033[0m             \033[96;1m#\033[0m\n");
    gotoxy(10, 8);
    printf("\033[96;1m#\033[0m         \033[92mMauro Perez\033[0m                  \033[96;1m#\033[0m\n");
    gotoxy(10, 9);
    printf("\033[96;1m########################################\033[0m\n");
}

void mostrarCreditos() {
    system("cls");
    while (!_kbhit()) {
        system("cls");
        gotoxy(10, 5);
        textoCreditos();
        gotoxy(10, 12);
        printf("\033[93mPresione una tecla para volver...\033[0m");
        Sleep(400);
        system("cls");
        printf("\033[7m");
        textoCreditos();
        printf("\033[0m");
        Sleep(400);
    }
    _getch();
    system("cls");
}

//------------------- RANKING -------------------

void vaciarLista(Serpiente* cabeza) {
    Serpiente* actual = cabeza;
    Serpiente* siguiente;
    while (actual != NULL) {
        siguiente = actual->sig;
        free(actual);
        actual = siguiente;
    }
}

void guardarPuntaje(const char nombre[MAX_NOMBRE], int puntaje) {
    FILE* archivo = fopen("puntuaciones.txt", "a");
    if (archivo != NULL) {
        fprintf(archivo, "%s %d\n", nombre, puntaje);
        fclose(archivo);
    }
}

void mostrarRanking() {
    FILE* archivo = fopen("puntuaciones.txt", "r");
    if (!archivo) {
        system("cls");
        gotoxy(10, 5);
        printf("\033[91mNo hay puntajes registrados.\033[0m\n");
        Sleep(1500);
        system("cls");
        return;
    }
    EntradaRanking entradas[MAX_PUNTAJES];
    int total = 0;
    while (fscanf_s(archivo, "%s %d", entradas[total].nombre, MAX_NOMBRE, &entradas[total].puntaje) == 2 && total < MAX_PUNTAJES) {
        total++;
    }
    fclose(archivo);
    for (int i = 0; i < total - 1; i++) {
        for (int j = i + 1; j < total; j++) {
            if (entradas[j].puntaje > entradas[i].puntaje) {
                EntradaRanking temp = entradas[i];
                entradas[i] = entradas[j];
                entradas[j] = temp;
            }
        }
    }
    system("cls");
    gotoxy(10, 3);
    printf("\033[93;1m========== TOP 4 RANKING ==========\033[0m\n\n");
    int mostrar = total < 4 ? total : 4;
    for (int i = 0; i < mostrar; i++) {
        gotoxy(10, 6 + i);
        printf("\033[92m %d.\033[0m \033[97m%-20s\033[0m \033[95;1m%5d pts\033[0m",
            i + 1, entradas[i].nombre, entradas[i].puntaje);
    }
    gotoxy(10, 12);
    printf("\n\033[96mPresione una tecla para volver al menu...\033[0m");
    _getch();
    system("cls");
}

//------------------- INSTRUCCIONES -------------------

void mostrarInstrucciones() {
    system("cls");
    borrarCursor();
    gotoxy(10, 3);
    printf("\033[96;1m########################################################\033[0m");
    gotoxy(10, 4);
    printf("\033[96;1m#                   INSTRUCCIONES DEL JUEGO            #\033[0m");
    gotoxy(10, 5);
    printf("\033[96;1m########################################################\033[0m");
    gotoxy(12, 7);
    printf("\033[93mUn jugador:\033[0m");
    gotoxy(15, 8);
    printf("\033[92mWASD = mover la serpiente VERDE\033[0m");
    gotoxy(12, 10);
    printf("\033[93mDos jugadores:\033[0m");
    gotoxy(15, 11);
    printf("\033[92mJugador 1 = WASD (serpiente VERDE)\033[0m");
    gotoxy(15, 12);
    printf("\033[94mJugador 2 = FLECHAS (serpiente AZUL)\033[0m");
    gotoxy(12, 14);
    printf("\033[93mESC = pausar  |  Los bordes TRASPASAN\033[0m");
    gotoxy(12, 16);
    printf("\033[93mPierde quien choque con su cuerpo o con la otra serpiente.\033[0m");
    gotoxy(12, 18);
    printf("\033[93mComer la manzana \033[91;41;1m@\033[93m da puntos y alarga la serpiente.\033[0m");
    gotoxy(10, 21);
    printf("\033[96;1m########################################################\033[0m");
    gotoxy(10, 22);
    printf("\033[96;1m#    Presiona una tecla para volver al menu principal  #\033[0m");
    gotoxy(10, 23);
    printf("\033[96;1m########################################################\033[0m");
    _getch();
    system("cls");
}

//------------------- SALIDA -------------------

void salidaJuego() {
    system("cls");
    borrarCursor();
    int anchoCaja = 54, x = 30, y = 8;

    gotoxy(x, y);
    printf("\033[92;1m%c", 201);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 187);

    for (int i = 1; i < 10; i++) {
        gotoxy(x, y + i);
        printf("\033[92;1m%c\033[0m", 186);
        for (int j = 0; j < anchoCaja; j++) printf(" ");
        printf("\033[92;1m%c\033[0m", 186);
    }

    gotoxy(x, y + 10);
    printf("\033[92;1m%c", 200);
    for (int i = 0; i < anchoCaja; i++) printf("%c", 205);
    printf("%c\033[0m", 188);

    gotoxy(x + 10, y + 2); printf("\033[92;1mG R A C I A S   P O R\033[0m");
    gotoxy(x + 16, y + 4); printf("\033[92;1mJ U G A R\033[0m");
    gotoxy(x + 12, y + 6); printf("\033[96mNos vemos pronto!\033[0m");
    gotoxy(x + 15, y + 8); printf("\033[90mCerrando el juego...\033[0m");

    Sleep(1500);
    exit(0);
}