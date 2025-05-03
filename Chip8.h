#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <array>

class Chip8 {
public:
    static const int MEMORY_SIZE = 4096;
    static const int WIDTH_LOW = 64, HEIGHT_LOW = 32;
    static const int WIDTH_HIGH = 128, HEIGHT_HIGH = 64;
    uint8_t getSoundTimer() const { return soundTimer; }


    Chip8();
    void reset();
    bool loadROM(const char* filename);
    void emulateCycle();
    void updateTimers();
    void setKeyState(int key, bool pressed);

    // Graphics buffer (max 128x64), drawFlag indicates when to redraw
    std::array<uint8_t, WIDTH_HIGH * HEIGHT_HIGH> gfx;
    bool drawFlag;

private:
    uint16_t opcode;
    std::array<uint8_t, MEMORY_SIZE> memory;
    std::array<uint8_t, 16> V;
    uint16_t I, pc;
    std::array<uint16_t, 16> stack;
    uint8_t sp;
    uint8_t delayTimer, soundTimer;
    bool highRes;
    uint8_t keys[16];

    void executeOpcode();
};

#endif // CHIP8_H
