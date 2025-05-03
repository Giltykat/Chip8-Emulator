#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "Chip8.h"
#include "FileBrowser.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 320;

// Audio callback function
void audioCallback(void* userdata, Uint8* stream, int len) {
    static float phase = 0.0f;
    Chip8* emu = static_cast<Chip8*>(userdata);
    float* buf = reinterpret_cast<float*>(stream);
    int count = len / sizeof(float);
    for (int i = 0; i < count; ++i) {
        // Play square wave if sound timer > 0
        buf[i] = (emu->getSoundTimer() > 0 && phase < 0.5f) ? 0.25f : -0.25f;
        phase += 440.0f / 44100.0f;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL video + audio
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << "\n";
        return 1;
    }
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "TTF Init Error: " << TTF_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    // Load font for UI
    TTF_Font* font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << "\n";
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // File browser to select a .ch8 ROM
    FileBrowser browser(renderer, font);
    std::string romPath;
    if (!browser.run(romPath)) {
        std::cout << "No ROM selected.\n";
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    // Initialize CHIP-8 and load ROM
    Chip8 chip8;
    if (!chip8.loadROM(romPath.c_str())) {
        std::cerr << "Failed to load ROM: " << romPath << "\n";
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Set up audio spec and open device
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = 44100;
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = 2048;
    want.callback = audioCallback;
    want.userdata = &chip8;
    if (SDL_OpenAudio(&want, &have) < 0) {
        std::cerr << "Audio Error: " << SDL_GetError() << "\n";
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_PauseAudio(0);

    // Create texture for low-res 64×32 display
    SDL_Texture* screenTex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        Chip8::WIDTH_LOW,
        Chip8::HEIGHT_LOW
    );

    // Declare main-loop helpers
    bool quit = false;
    SDL_Event e;
    Uint32 lastTimer = SDL_GetTicks();

    // Main emulation loop
    while (!quit) {
        // Emulate one cycle
        chip8.emulateCycle();

        // Poll events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                bool pressed = (e.type == SDL_KEYDOWN);
                switch (e.key.keysym.sym) {
                    case SDLK_1: chip8.setKeyState(0x1, pressed); break;
                    case SDLK_2: chip8.setKeyState(0x2, pressed); break;
                    case SDLK_3: chip8.setKeyState(0x3, pressed); break;
                    case SDLK_4: chip8.setKeyState(0xC, pressed); break;
                    case SDLK_q: chip8.setKeyState(0x4, pressed); break;
                    case SDLK_w: chip8.setKeyState(0x5, pressed); break;
                    case SDLK_e: chip8.setKeyState(0x6, pressed); break;
                    case SDLK_r: chip8.setKeyState(0xD, pressed); break;
                    case SDLK_a: chip8.setKeyState(0x7, pressed); break;
                    case SDLK_s: chip8.setKeyState(0x8, pressed); break;
                    case SDLK_d: chip8.setKeyState(0x9, pressed); break;
                    case SDLK_f: chip8.setKeyState(0xE, pressed); break;
                    case SDLK_z: chip8.setKeyState(0xA, pressed); break;
                    case SDLK_x: chip8.setKeyState(0x0, pressed); break;
                    case SDLK_c: chip8.setKeyState(0xB, pressed); break;
                    case SDLK_v: chip8.setKeyState(0xF, pressed); break;
                }
            }
        }

        // Update timers at ~60Hz
        if (SDL_GetTicks() - lastTimer > 16) {
            chip8.updateTimers();
            lastTimer = SDL_GetTicks();
        }

        // Redraw if needed
        if (chip8.drawFlag) {
            uint32_t pixels[Chip8::WIDTH_LOW * Chip8::HEIGHT_LOW] = {0};
            for (int y = 0; y < Chip8::HEIGHT_LOW; ++y) {
                for (int x = 0; x < Chip8::WIDTH_LOW; ++x) {
                    if (chip8.gfx[y * Chip8::WIDTH_HIGH + x]) {
                        pixels[y * Chip8::WIDTH_LOW + x] = 0xFFFFFFFF;
                    }
                }
            }
            SDL_UpdateTexture(screenTex, nullptr, pixels, Chip8::WIDTH_LOW * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, screenTex, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            chip8.drawFlag = false;
        }
    }

    // Cleanup
    SDL_DestroyTexture(screenTex);
    SDL_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
