//
// Created by Utente on 11/09/2026.
//

#ifndef CHIP_8_EMULATOR_MAINUTILS_H
#define CHIP_8_EMULATOR_MAINUTILS_H

#include <SDL3/SDL.h>
#include "chip8.h"
#define SCREEN_W 64
#define SCREEN_H 32
#define COLOR_ON  0xFF, 0x99, 0x00
#define COLOR_OFF 0, 0, 0

static bool rom_loaded = false;
extern Chip8 chip8;

void beep(SDL_AudioStream *stream, int *current_sine_sample);
void updateScreen(SDL_Renderer *render, Chip8 *chip8);

// static void SDLCALL callback(void* userdata, const char* const* filelist, int filter);
void getFileFromUser(SDL_Window *window);

#endif //CHIP_8_EMULATOR_MAINUTILS_H