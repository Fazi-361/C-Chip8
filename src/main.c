#define SDL_MAIN_USE_CALLBACKS 1
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "../include/chip8.h"

#define COLOR_ON  0xFF, 0x99, 0x00
#define COLOR_OFF 0, 0, 0
#define TICK_SPEED 700

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static Chip8 chip8;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (argc < 2) {
        puts("Per avviare l'emulatore, fornire il percorso ad una rom.");
        puts("Uso: ./C-Chip8 path/per/la/rom.ch8");
        return SDL_APP_FAILURE;
    }

    srand(time(NULL));
    SDL_SetAppMetadata("C-Chip8", "1.0", "C-Chip8");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("C-Chip8", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_W, SCREEN_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Inizializzazione Emulatore
    chip8 = newChip8();
    loadRom(&chip8, argv[1]);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

unsigned int lastTickTime = 0, lastTimerTime = 0, currentTime;

SDL_AppResult SDL_AppIterate(void *appstate) {
    currentTime = SDL_GetTicks();

    if (chip8.crashFlag) {
        return SDL_APP_FAILURE;
    }

    // Tickiamo i timer
    if (currentTime - lastTimerTime > 1000 / 60){
        tickTimers(&chip8);
        lastTimerTime = currentTime;
    }

    if (currentTime - lastTickTime > (float) 1000 / TICK_SPEED) {
        emulateCycle(&chip8);

        if (chip8.drawFlag) {
            SDL_RenderClear(renderer);
            int x = 0, y = 0;
            for (int i = 0; i < sizeof(chip8.gfx) / sizeof(chip8.gfx[0]); ++i) {
                x = i % SCREEN_W;
                y = i / SCREEN_W;
                if (chip8.gfx[i] != 0)
                    SDL_SetRenderDrawColor(renderer, COLOR_ON, SDL_ALPHA_OPAQUE);
                else
                    SDL_SetRenderDrawColor(renderer, COLOR_OFF, SDL_ALPHA_OPAQUE);
                SDL_RenderPoint(renderer, x, y);
            }
            SDL_RenderPresent(renderer);
        }

        lastTickTime = currentTime;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}