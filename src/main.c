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
static SDL_AudioStream *stream = NULL;
static int current_sine_sample = 0;

static Chip8 chip8;

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
        puts("Per avviare l'emulatore, fornire il percorso ad una rom.");
        puts("Uso: ./C-Chip8 path/per/la/rom.ch8");
        return SDL_APP_FAILURE;
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
    currentTime = SDL_GetTicks();

    if (chip8.crashFlag) {
        return SDL_APP_FAILURE;
    }

    if (chip8.audioFlag) {
        /* see if we need to feed the audio stream more data yet.
       We're being lazy here, but if there's less than half a second queued, generate more.
       A sine wave is unchanging audio--easy to stream--but for video games, you'll want
       to generate significantly _less_ audio ahead of time! */
        const int minimum_audio = (100 * sizeof (float));  /* 8000 float samples per second. Half of that. */
        if (SDL_GetAudioStreamQueued(stream) < minimum_audio) {
            static float samples[512];  /* this will feed 512 samples each frame until we get to our maximum. */
            int i;

            /* generate a 440Hz pure tone */
            for (i = 0; i < SDL_arraysize(samples); i++) {
                const int freq = 880;
                const float phase = current_sine_sample * freq / 8000.0f;
                samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
                current_sine_sample++;
            }

            /* wrapping around to avoid floating-point errors */
            current_sine_sample %= 8000;

            /* feed the new data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
            SDL_PutAudioStreamData(stream, samples, sizeof (samples));
        }
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