#define SDL_MAIN_USE_CALLBACKS 1
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "../include/chip8.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static Chip8 chip8;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Chip-8", "1.0", "Chip-8");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Chip-8", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_W, SCREEN_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Inizializzazione Emulatore
    chip8 = newChip8();
    loadRom(&chip8, argv[1]);
    for (int i=0; i<40; ++i) {
        fetch(&chip8);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    emulateCycle(&chip8);

    if (chip8.drawFlag) {
        SDL_RenderClear(renderer);
        int x = 0, y = 0;
        for (int i = 0; i < sizeof(chip8.gfx) / sizeof(chip8.gfx[0]); ++i) {
            x = i % SCREEN_W;
            y = i / SCREEN_W;
            if (chip8.gfx[i] != 0)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            else
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderPoint(renderer, x, y);
        }
        SDL_RenderPresent(renderer);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}