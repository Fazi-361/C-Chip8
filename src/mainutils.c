//
// Created by Utente on 11/09/2026.
//

#include "../include/mainutils.h"
#include "../include/chip8.h"
#include <stdio.h>


static const SDL_DialogFileFilter filters[] = {
    { "Chip-8 ROM",  "ch8" }
};

void beep(SDL_AudioStream *stream, int *current_sine_sample) {
    const int minimum_audio = (100 * sizeof (float));
    if (SDL_GetAudioStreamQueued(stream) < minimum_audio) {
        static float samples[512];  /* this will feed 512 samples each frame until we get to our maximum. */
        int i;

        for (i = 0; i < SDL_arraysize(samples); i++) {
            const int freq = 880;
            const float phase = *current_sine_sample * freq / 8000.0f;
            samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
            current_sine_sample++;
        }

        *current_sine_sample %= 8000;

        SDL_PutAudioStreamData(stream, samples, sizeof (samples));
    }
}

void updateScreen(SDL_Renderer *renderer, Chip8 *chip8) {
    SDL_RenderClear(renderer);
    int x = 0, y = 0;
    for (int i = 0; i < sizeof(chip8->gfx) / sizeof(chip8->gfx[0]); ++i) {
        x = i % SCREEN_W;
        y = i / SCREEN_W;
        if (chip8->gfx[i] != 0)
            SDL_SetRenderDrawColor(renderer, COLOR_ON, SDL_ALPHA_OPAQUE);
        else
            SDL_SetRenderDrawColor(renderer, COLOR_OFF, SDL_ALPHA_OPAQUE);
        SDL_RenderPoint(renderer, x, y);
    }
    SDL_RenderPresent(renderer);
}

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
    if (filelist && filelist[0]) {
        printf("%s\n", *filelist);
        loadRom(&chip8, *filelist);
    }
    else {
        printf("Rom not loaded. Closing program.");
        chip8.crashFlag = true;
    }
}

void getFileFromUser(SDL_Window *window) {
    SDL_ShowOpenFileDialog(callback, NULL, window, filters, SDL_arraysize(filters), NULL, true);
}