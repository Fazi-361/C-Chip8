#include <stdio.h>
#include "../include/chip8.h"

range interpreterRange = {0x000, 0x1FF};
range fontSetRange     = {0x050, 0x0A0};
range workRange        = {0x200, 0xFFF};

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
    chip8.sp = nullptr;

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

    return chip8;
}

void loadRom(Chip8 *chip8, char *romPath) {
    FILE *rom = fopen(romPath, "rb");
    if (!rom) {
        printf("Errore: Impossibile aprire la ROM %s\n", romPath);
        perror("Error");
        return;
    }

    ubyte_t *workRangePtr = chip8->memory + workRange.lo;
    fread(workRangePtr, 1, workRange.hi - workRange.lo, rom);
    fclose(rom);

    chip8->pc = chip8->memory + workRange.lo;

    // for (int i = 0; i < MEMORY_SIZE; ++i) {
    //     bool newLine = (i % 16 == 15);
    //     printf("%02x ", chip8->memory[i]);
    //     if (newLine) {
    //         printf("\n");
    //     }
    // }
}

void emulateCycle(Chip8* chip8) {

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
                    chip8->pc+=2;
                    break;
                }
                default: {
                    printf("Opcode sconosciuto: [0x0000]: 0x%X\n", opcode);
                }
            }
            break;
        }

        case 0x1000: {
            // Jump
            // Imposta il program counter alla parte dopo l'1
            chip8->pc = chip8->memory + (opcode & 0x0FFF);
            break;
        }

        case 0x6000: {
            // Set Register VX

            // Operazione bitwise per prendere la seconda esadecimale piu' significativa dell'opcode
            ubyte_t vx = (opcode & 0x0F00) >> 8;
            unsigned short value = opcode & 0x00FF;

            // Assegnazione del valore a VX
            chip8->V[vx] = value;

            chip8->pc+=2;
            break;
        }

        case 0xA000: {
            // Load index register I
            unsigned short value = opcode & 0x0FFF;
            chip8->I = value;

            chip8->pc+=2;
            break;
        }

        case 0xD000: {
            // 0xDXYN
            // Disegna uno sprite alle coordinate x, y con larghezza 8 e lunghezza N
            unsigned short x = chip8->V[(opcode & 0x0F00) >> 8];
            unsigned short y = chip8->V[(opcode & 0x00F0) >> 4];
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

        default: {
            printf("Opcode sconsociuto: 0x%X\n", opcode);
        }

    }

    // Execute
}

void fetch(Chip8 *chip8) {
    if (chip8->pc + 1 == NULL) {
        perror("End Of File!");
        return;
    }

    // Aggiorna l'opcode
    chip8->opcode = *chip8->pc << 8 | *(chip8->pc + 1);

    // printf("%04x ", chip8->opcode);
}

void clearGraphics(Chip8 *chip8) {
    for (int i = 0; i < sizeof(chip8->gfx) / sizeof(
        chip8->gfx[0]); ++i) {
        chip8->gfx[i] = 0;
    }
}