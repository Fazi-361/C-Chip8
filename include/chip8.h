#ifndef CHIP_8_EMULATOR_INCLUDE_H
#define CHIP_8_EMULATOR_INCLUDE_H

#include <stdbool.h>
#include <stddef.h>

#define MEMORY_SIZE 4096
#define SCREEN_W 64
#define SCREEN_H 32

typedef char byte_t;
typedef unsigned char ubyte_t;

typedef struct {
    short lo;
    short hi;
} range;

typedef struct {
    // Memoria RAM
    ubyte_t memory[MEMORY_SIZE];        // RAM da 4096 byte (indirizzi 0x000 - 0xFFF)

    // Registri della CPU
    ubyte_t V[16];                      // Registri generali V0-VF
    unsigned short I;                   // Registro di memoria I (16-bit, punta agli indirizzi in RAM)
    ubyte_t *pc;                        // Program Counter (punta all'istruzione corrente)

    //  Stack
    // Utilizzato per salvare il Program Counter prima dei salti a subroutine
    ubyte_t *stack[16];                 // Gestisce fino a 16 livelli di chiamate annidate
    unsigned short sp;                  // Stack Pointer (punta all'elemento corrente dello stack)

    // Timer (decrementati automaticamente a 60 Hz)
    ubyte_t delay_timer;                // Timer di ritardo per il timing dei giochi
    ubyte_t sound_timer;                // Timer audio: finché è > 0 viene riprodotto un beep

    // Grafica
    ubyte_t gfx[SCREEN_W * SCREEN_H];

    // Tastiera
    bool keys[16];                      // Stato dei 16 tasti della tastiera esadecimale (0x0 - 0xF)
    bool prevKeys[16];   // <-- nuovo: stato al ciclo precedente

    // Opcode attuale
    unsigned short opcode;

    // Flag usata per controllare se bisogna disegnare a schermo
    bool drawFlag;
    bool audioFlag;

} Chip8;

/**
 * @brief Crea e inizializza una nuova istanza dell'emulatore Chip8 azzerando memoria, registri ecc..
 * @return Struttura Chip8 inizializzata.
 */
Chip8 newChip8(void);

/**
 * @brief Carica un file ROM (.ch8) nella memoria dell'emulatore.
 *
 * @param chip8   Puntatore alla struttura dell'emulatore.
 * @param romPath Percorso relativo o assoluto del file ROM.
 */
void loadRom(Chip8* chip8, char* romPath);

/**
 * @brief Esegue il ciclo Fetch -> Decode -> Execute per interpretare i byte della ram
 *
 * @param chip8 Puntatore alla struttura dell'emulatore.
 */
void emulateCycle(Chip8* chip8);

/**
 * @brief Esegue il Fetch dell'opcode e aggiorna il pc
 *
 * @param chip8 Puntatore alla struttura dell'emulatore.
 */
void fetch(Chip8 *chip8);

/**
 * @brief Svuota l'array gfx
 *
 * @param chip8 Puntatore alla struttura dell'emulatore.
 */
void clearGraphics(Chip8 *chip8);

/**
 * @brief Aggiorna l'array keys del chip8
 *
 * @param chip8 Puntatore alla struttura dell'emulatore.
 * @param keysPressed Risultato di SDL_GetKeyboardState(NULL)
 */
void updateKeys(Chip8 *chip8, const bool *keysPressed);

void tickTimers(Chip8 *chip8);

#endif //CHIP_8_EMULATOR_INCLUDE_H