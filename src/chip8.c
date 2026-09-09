#include <stdio.h>
#include <SDL3/SDL.h>
#include "../include/chip8.h"

range interpreterRange = {0x000, 0x1FF};
range fontSetRange     = {0x050, 0x0A0};
range workRange        = {0x200, 0xFFF};

const SDL_Scancode keyMap[16] = {
    SDL_SCANCODE_X, // 0
    SDL_SCANCODE_1, // 1
    SDL_SCANCODE_2, // 2
    SDL_SCANCODE_3, // 3
    SDL_SCANCODE_Q, // 4
    SDL_SCANCODE_W, // 5
    SDL_SCANCODE_E, // 6
    SDL_SCANCODE_A, // 7
    SDL_SCANCODE_S, // 8
    SDL_SCANCODE_D, // 9
    SDL_SCANCODE_Z, // A
    SDL_SCANCODE_C, // B
    SDL_SCANCODE_4, // C
    SDL_SCANCODE_R, // D
    SDL_SCANCODE_F, // E
    SDL_SCANCODE_V  // F
};


ubyte_t font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void printReleasedKeys(Chip8 const *chip8) {
    for (int i=0; i<16; ++i) {
        if (chip8->keys[i] && ! chip8->prevKeys[i])
            printf("%0x  ", i);
    }
}

Chip8 newChip8(void) {
    Chip8 chip8;

    // Inizializza Memoria
    for (size_t i = 0; i < sizeof(chip8.memory); ++i) {
        chip8.memory[i] = 0x00;
    }

    // Inizializza i registri
    for (size_t i = 0; i < sizeof(chip8.V) / sizeof(chip8.V[0]); ++i) {
        chip8.V[i] = 0x00;
    }
    chip8.I  = 0;
    chip8.pc = nullptr;

    // Inizializza la stack
    for (size_t i = 0; i < sizeof(chip8.stack) / sizeof(chip8.stack[0]); ++i) {
        chip8.stack[i] = 0x0000;
    }
    chip8.sp = 0;

    // Inizializza i timer
    chip8.delay_timer = 0x00;
    chip8.sound_timer = 0x00;

    // Inizializza la grafica
    for (size_t i = 0; i < sizeof(chip8.gfx) / sizeof(chip8.gfx[0]); ++i) {
        chip8.gfx[i] = 0x00;
    }

    // Inizializza lo stato delle key
    for (size_t i = 0; i < sizeof(chip8.keys) / sizeof(chip8.keys[0]); ++i) {
        chip8.keys[i] = false;
    }

    // Carica il font nella memoria
    for (size_t i = 0; i < sizeof(font) / sizeof(font[0]); ++i) {
        chip8.memory[i] = font[i];
    }

    // Impostiamo l'opcode attuale
    chip8.opcode = 0x00;

    // Inizializza la draw flag
    chip8.drawFlag = false;
    chip8.audioFlag = false;
    chip8.crashFlag = false;

    return chip8;
}

void loadRom(Chip8 *chip8, char *romPath) {
    FILE *rom = fopen(romPath, "rb");
    if (!rom) {
        printf("Errore: Impossibile aprire la ROM %s\n", romPath);
        perror("Error");
        return;
    }

    fseek(rom, 0, SEEK_END);
    long fileSize = ftell(rom);
    rewind(rom);

    if (fileSize > MEMORY_SIZE - workRange.lo) {
        printf("Errore: ROM troppo grande (%ld byte)\n", fileSize);
        fclose(rom);
        return;
    }

    ubyte_t *workRangePtr = chip8->memory + workRange.lo;
    fread(workRangePtr, 1, workRange.hi - workRange.lo, rom);
    fclose(rom);

    chip8->pc = chip8->memory + workRange.lo;
}

void emulateCycle(Chip8* chip8) {

    updateKeys(chip8, SDL_GetKeyboardState(NULL));
    printReleasedKeys(chip8);

    chip8->drawFlag = false;

    // Fetch dell'opcode
    fetch(chip8);

    // Decode ed Execute
    unsigned short opcode = chip8->opcode;

    // Controlliamo la prima cifra
    switch (opcode & 0xF000) {
        case 0x0000: {
            switch (opcode & 0x000F) {
                case 0x0000: { // 0x00E0: Clears the screen
                    clearGraphics(chip8);
                    chip8->pc+=2;
                    break;
                }
                case 0x000E: { // 0x00EE: Ritorna dalla subroutine
                    chip8->pc = chip8->stack[chip8->sp-- - 1];
                    chip8->pc += 2;
                    break;
                }
                default: {
                    // 0NNN. Non necessario.
                    chip8->pc += 2;
                    break;
                }
            }
            break;
        }

        case 0x1000: {
            // 1NNN
            // Imposta il program counter a NNN
            chip8->pc = chip8->memory + (opcode & 0x0FFF);
            break;
        }

        case 0x2000: {
            // 2nnn - Chiama la subroutine in posizione NNN
            unsigned short nnn = opcode & 0x0FFF;

            chip8->stack[chip8->sp++] = chip8->pc;
            chip8->pc = chip8->memory + nnn;

            break;
        }

        case 0x3000: {
            // 3XNN: salta il prossimo opcode se NN e' uguale a *VX
            ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
            unsigned short nn = opcode & 0x00FF;
            if (*vx == nn)
                chip8->pc += 4;
            else
                chip8->pc += 2;
            break;
        }

        case 0x4000: {
            // 4XNN: salta il prossimo opcode se NN e' diverso da *VX
            ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
            unsigned short nn = opcode & 0x00FF;
            if (*vx != nn)
                chip8->pc += 4;
            else
                chip8->pc += 2;
            break;
        }

        case 0x5000: {
            // 5XY0: salta il prossimo opcode se *VX e' uguale a *VY
            ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
            ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];
            if (*vx == * vy)
                chip8->pc += 4;
            else
                chip8->pc += 2;
            break;
        }

        case 0x6000: {
            // Set Register VX

            // Operazione bitwise per prendere la seconda cifra esadecimale piu' significativa dell'opcode
            ubyte_t vx = (opcode & 0x0F00) >> 8;
            unsigned short value = opcode & 0x00FF;

            // Assegnazione del valore a VX
            chip8->V[vx] = value;

            chip8->pc+=2;
            break;
        }

        case 0x7000: {
            // 7XNN: Add NN to VX
            ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
            unsigned short nn = opcode & 0x00FF;
            // printf("Opcode %04x. Adding %d to register %x\n", opcode, nn, ((opcode & 0x0F00) >> 8));
            *vx += nn;
            chip8->pc += 2;
            break;
        }

        case 0x8000: {
            switch (opcode & 0x000F) {

                case 0x0: {
                    // 8XY0: VX = VY
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    *vx = *vy;
                    chip8->pc += 2;
                    break;
                }

                case 0x1: {
                    // 8XY1: VX = VX | VY
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    *vx |= *vy;
                    chip8->pc += 2;
                    break;
                }

                case 0x2: {
                    // 8XY2: VX = VX & VY
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    *vx &= *vy;
                    chip8->pc += 2;
                    break;
                }

                case 0x3: {
                    // 8XY3: VX = VX ^ VY
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    *vx ^= *vy;
                    chip8->pc += 2;
                    break;
                }

                case 0x4: {
                    // 8XY4: VX = VX + VY
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    chip8->V[0xF] = ((int)*vx + (int)*vy > 255);

                    *vx += *vy;
                    chip8->pc += 2;
                    break;
                }

                case 0x5: {
                    // 8XY5: VX = VX - VY
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    chip8->V[0xF] = (*vx >= *vy);
                    *vx -= *vy;

                    chip8->pc += 2;
                    break;
                }

                case 0x6: {
                    // 8XY6: VX = VY >> 1
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    chip8->V[0xF] = *vy & 0x01;
                    *vx = *vy >> 1;

                    chip8->pc += 2;
                    break;
                }

                case 0x7: {
                    // 8XY7: VX = VY - VX
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    chip8->V[0xF] = (*vy >= *vx);
                    *vx = *vy - *vx;

                    chip8->pc += 2;
                    break;
                }

                case 0xE: {
                    // 8XYE: VX = VY << 1
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];

                    chip8->V[0xF] = (*vy & 0x80) != 0;
                    *vx = *vy << 1;

                    chip8->pc += 2;
                    break;
                }

                default:
                    printf("Opcode sconosciuto: 0x%04X\n", opcode);
                    break;
            }

            break;
        }

        case 0x9000: {
            // 9XY0: salta il prossimo opcode se *VX e' diverso da *VY
            ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
            ubyte_t *vy = &chip8->V[(opcode & 0x00F0) >> 4];
            if (*vx != * vy)
                chip8->pc += 4;
            else
                chip8->pc += 2;
            break;
        }

        case 0xA000: {
            // Load index register I
            unsigned short value = opcode & 0x0FFF;
            chip8->I = value;

            chip8->pc+=2;
            break;
        }

        case 0xB000: {
            // BNNN: Imposta il PC a NNN + *V0
            unsigned short nnn = opcode & 0x0FFF;
            chip8->pc = chip8->memory + nnn + chip8->V[0];
            break;
        }

        case 0xC000: {
            // CXNN
            // Genera un numero casuale, Fa l'operazione & con NN e mette il risultato in VX
            ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
            ubyte_t nn = opcode & 0x00FF;
            ubyte_t random = rand() % 0x00FF;

            *vx = nn & random;
            chip8->pc += 2;
            break;
        }

        case 0xD000: {
            // 0xDXYN
            // Disegna uno sprite alle coordinate x, y con larghezza 8 e lunghezza N
            unsigned short x = chip8->V[(opcode & 0x0F00) >> 8] % 64;
            unsigned short y = chip8->V[(opcode & 0x00F0) >> 4] % 32;
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;
            chip8->V[0xF]  = 0;

            for (int yline = 0; yline < height; yline++) {
                pixel = chip8->memory[chip8->I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        if (chip8->gfx[(x + xline + ((y + yline) * 64))] == 1)
                            chip8->V[0xF] = 1;
                        chip8->gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }

            chip8->drawFlag = true;
            chip8->pc+=2;
            break;
        }

        case 0xE000: {
            switch (opcode & 0x00FF) {
                case 0x009E: {
                    // EX9E: Salta la prossima istruzione se il tasto in VX e' premuto
                    ubyte_t vx = (opcode & 0x0F00) >> 8;
                    vx &= 0x000F;
                    if (chip8->keys[chip8->V[vx]])
                        chip8->pc += 2; // Salta l' istruzione successiva

                    chip8 -> pc += 2;
                    break;
                }

                case 0x00A1: {
                    // EX9E: Salta la prossima istruzione se il tasto in VX NON e' premuto
                    ubyte_t vx = (opcode & 0x0F00) >> 8;
                    vx &= 0x000F;
                    if (!chip8->keys[chip8->V[vx]])
                        chip8->pc += 2; // Salta l' istruzione successiva

                    chip8 -> pc += 2;
                    break;
                }

                default: {
                    printf("Opcode sconsociuto: 0x%X\n", opcode);
                }
            }
            break;
        }

        case 0xF000: {
            switch (opcode & 0x00FF) {

                case 0x0007: {
                    // FX07 sets VX to the current value of the delay timer
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    *vx = chip8->delay_timer;
                    chip8->pc += 2;
                    break;
                }

                case 0x000A: {
                    // A key press is awaited, and then stored in VX
                    // (blocking operation, all instruction halted until next key event,
                    // delay and sound timers should continue processing).

                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];

                    for (int i = 0; i < 16; ++i) {
                        if (chip8->keys[i] && !chip8->prevKeys[i]) {
                            *vx = i;
                            chip8->pc += 2;
                            break;
                        }
                    }

                    break;
                }
                case 0x0015: {
                    // FX15 sets the delay timer to the value in VX
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];

                    chip8 -> delay_timer = *vx;
                    chip8->pc += 2;

                    break;
                }

                case 0x0018: {
                    //FX18 sets the sound timer to the value in VX
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];

                    chip8 -> sound_timer = *vx;
                    chip8->pc += 2;

                    break;
                }

                case 0x001E: {
                    // FX1E: Aggiungi il valore di VX a I
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];

                    chip8->V[0xF] = (*vx + chip8->I > 0xFFF); // Controlla il carry
                    chip8->I += *vx;

                    chip8->pc += 2;
                    break;
                }

                case 0x0029: {
                    // FX29: Il registro I viene impostato all'indirizzo del carattere esadecimale puntato da VX.
                    // Per il font.
                    ubyte_t *vx = &chip8->V[(opcode & 0x0F00) >> 8];
                    *vx &= 0x000F;
                    chip8->I = *vx * 5 ;
                    // printf("VX: %d.  ", *vx);
                    // printf("Put I in position %d\n", chip8->I);
                    chip8->pc += 2;
                    break;
                }

                case 0x0033: {
                    // Fx33 - store binary-coded decimal representation of vX to memory at i, i + 1 and i + 2
                    chip8->memory[chip8->I]     = chip8->V[(opcode & 0x0F00) >> 8] / 100;
                    chip8->memory[chip8->I + 1] = (chip8->V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    chip8->memory[chip8->I + 2] = (chip8->V[(opcode & 0x0F00) >> 8] % 100) % 10;
                    chip8->pc += 2;
                    break;
                }

                case 0x0055: {
                    // FX5E: save registers v0 - vX to memory starting at i
                    ubyte_t vx = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= vx; ++i) {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }

                    chip8->pc += 2;
                    break;
                }
                case 0x0065: {
                    // FX5E: load registers v0 - vX to memory starting at i
                    ubyte_t vx = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= vx; ++i) {
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    }

                    chip8->pc += 2;
                    break;
                }

                default:{
                    printf("Opcode sconsociuto: 0x%X\n", opcode);
                    break;
                }
            }

            break;
        }

        default: {
            printf("Opcode sconsociuto: 0x%X\n", opcode);
        }

    }

}

void fetch(Chip8 *chip8) {
    // Controlla se siamo alla fine del file
    ubyte_t *memory_end = chip8->memory + MEMORY_SIZE;
    if (chip8->pc < chip8->memory || chip8->pc + 1 >= memory_end) {
        printf("Errore: Program Counter (0x%tX) fuori dai limiti della memoria\n",
                chip8->pc - chip8->memory);
        chip8->crashFlag = true;
        return;
    }

    // Aggiorna l'opcode
    chip8->opcode = *chip8->pc << 8 | *(chip8->pc + 1);
}

void clearGraphics(Chip8 *chip8) {
    for (int i = 0; i < (int) sizeof(chip8->gfx) / (int) sizeof(chip8->gfx[0]); ++i) {
        chip8->gfx[i] = 0;
    }
}

void updateKeys(Chip8 *chip8, const bool *keysPressed) {

    for (int i = 0; i < 16; ++i)
        chip8->prevKeys[i] = chip8->keys[i];

    for (int i = 0; i < 16; i++) {
        chip8->keys[i] = keysPressed[keyMap[i]];
    }
}

void tickTimers(Chip8 *chip8) {
    if (chip8->delay_timer > 0)
        chip8->delay_timer -= 1;
    if (chip8->sound_timer > 0)
        chip8->sound_timer -= 1;
}