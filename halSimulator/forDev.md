# Guía de compilación: halSimulator

Este proyecto utiliza **CMake** como sistema de compilación y requiere la biblioteca **SDL2** para funcionar. A continuación, se detallan los requisitos previos y los diferentes métodos para compilar el proyecto.

## Prerrequisitos

Dado que el proyecto utiliza SDL2 mediante `find_package(SDL2 REQUIRED)`, debes asegurarte de tener instalada la biblioteca y sus archivos de desarrollo antes de compilar.

- **Linux (Ubuntu/Debian):**

```bash
sudo apt-get update
sudo apt-get install libsdl2-dev
```

- **macOS (Homebrew):**

```bash
brew install sdl2
```

- **Windows:** Se recomienda utilizar un gestor de paquetes como **vcpkg**, o descargar los archivos de desarrollo de SDL2 y configurarlos correctamente en tu entorno.

## Método 1: Compilación desde la terminal

Este es el método estándar y multiplataforma. Puedes utilizar una terminal como **Bash**, **CMD** o **PowerShell**, dependiendo de tu sistema operativo.

### 1. Abre una terminal en la raíz del proyecto

Debes situarte en la carpeta que contiene el archivo `CMakeLists.txt`.

### 2. Crea la carpeta de compilación

Ejecuta:

```bash
mkdir build
cd build
```

### 3. Configura el proyecto con CMake

Ejecuta:

```bash
cmake ..
```

Este comando configura el proyecto y genera los archivos necesarios para la compilación.

### 4. Compila el proyecto

Ejecuta:

```bash
cmake --build .
```

Una vez finalizada la compilación, el ejecutable `halSimulator` se encontrará dentro de la carpeta `build` o en una subcarpeta como `Debug` o `Release`, dependiendo del sistema operativo y del generador de CMake utilizado.

## Método 2: Compilación con Visual Studio

Visual Studio proporciona soporte nativo para proyectos basados en **CMake**, por lo que no es necesario crear manualmente la carpeta `build` ni ejecutar los comandos de CMake desde la terminal.

### 1. Abre el proyecto

Abre **Visual Studio** y selecciona:

> **Open a local folder**

Selecciona la carpeta raíz del proyecto, es decir, la carpeta que contiene el archivo `CMakeLists.txt`.

Visual Studio detectará automáticamente el archivo `CMakeLists.txt` y comenzará a configurar el proyecto en segundo plano. El progreso puede consultarse en la ventana **Output**.

### 2. Selecciona el ejecutable

Una vez finalizada la configuración:

1. Busca en la barra superior el menú desplegable **Startup Item**.
2. Selecciona:

```text
halSimulator.exe
```

### 3. Compila y ejecuta

Haz clic en el botón **Run/Debug** de Visual Studio o presiona:

```text
F5
```

Visual Studio compilará el proyecto y, si la compilación es exitosa, ejecutará `halSimulator.exe`.
