# C-Chip8 - Chip-8 Emulator

Un emulatore **Chip-8** scritto in C, costruito con **SDL3** per il rendering grafico, audio e input.

> **Nota**: Questo progetto è in fase di sviluppo (WIP).

## Descrizione

Chip-8 è un semplice linguaggio di programmazione virtuale sviluppato negli anni '70. Questo progetto implementa un emulatore che interpreta bytecode Chip-8 ed esegue i suoi programmi.

## Requisiti

- CMake >= 3.25
- Compilatore C23 (GCC 13+ o Clang 16+)
- SDL3 (inclusa come submodulo)

## Compilazione

```bash
# Clone del repository (con submoduli)
git clone --recursive https://github.com/Fazi-361/C-Chip8
cd C-Chip8

# Creazione directory build
mkdir build
cd build

# Configurazione e compilazione
cmake ..
cmake --build .\
```
## Avvio 
```bash
./C-Chip8  path\to\rom.ch8 
```
