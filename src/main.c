#define SDL_MAIN_USE_CALLBACKS 1
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "../include/chip8.h"
#include "../include/mainutils.h"

#define COLOR_ON  0xFF, 0x99, 0x00
#define COLOR_OFF 0, 0, 0
#define TICK_SPEED 700

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;
static int current_sine_sample = 0;

Chip8 chip8;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    srand(time(NULL));

    SDL_AudioSpec spec;
    SDL_SetAppMetadata("C-Chip8", "1.0", "C-Chip8");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("C-Chip8", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_W, SCREEN_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Settiamo l'audio
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = 8000;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
    SDL_ResumeAudioStreamDevice(stream);

    // Inizializzazione Emulatore
    chip8 = newChip8();

    if (argc < 2) {
        getFileFromUser(window);
        return SDL_APP_CONTINUE;
    }
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

    if (chip8.crashFlag) {
        return SDL_APP_FAILURE;
    }

    if (!chip8.romLoaded) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
        SDL_RenderClear(renderer);  /* start with a blank canvas. */

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
        SDL_RenderDebugText(renderer, 10, 5, "LOAD A");
        SDL_RenderDebugText(renderer, 20, 15, "ROM");
        SDL_RenderPresent(renderer);
        return SDL_APP_CONTINUE;
    }

    currentTime = SDL_GetTicks();

    // Facciamo il beep se necessario
    if (chip8.audioFlag) {
        const int minimum_audio = (100 * sizeof (float));  /* 8000 float samples per second. Half of that. */
        if (SDL_GetAudioStreamQueued(stream) < minimum_audio) {
            static float samples[512];  /* this will feed 512 samples each frame until we get to our maximum. */
            int i;

            for (i = 0; i < SDL_arraysize(samples); i++) {
                const int freq = 880;
                const float phase = current_sine_sample * freq / 8000.0f;
                samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
                current_sine_sample++;
            }

            current_sine_sample %= 8000;

            SDL_PutAudioStreamData(stream, samples, sizeof (samples));
        }
    }

    // Tickiamo i timer
    if (currentTime - lastTimerTime > 1000 / 60){
        tickTimers(&chip8);
        lastTimerTime = currentTime;
    }

    // Emuliamo un tick se e' giunto il momento di farlo
    if (currentTime - lastTickTime > (float) 1000 / TICK_SPEED) {
        emulateCycle(&chip8);

        if (chip8.drawFlag) {
            updateScreen(renderer, &chip8);
        }

        lastTickTime = currentTime;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}