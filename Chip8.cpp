#include "Chip8.h"
#include <fstream>
#include <cstring> // For memset
#include <cstdlib> // For exit (though maybe better to handle termination differently)
#include <iostream> // Added for potential debugging, can be removed

Chip8::Chip8() {
    reset();
}

void Chip8::reset() {
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0;     // Reset current opcode
    I = 0;          // Reset index register
    sp = 0;         // Reset stack pointer
    delayTimer = 0; // Reset delay timer
    soundTimer = 0; // Reset sound timer
    highRes = false; // Start in low-res mode
    drawFlag = true; // Set draw flag initially to clear screen

    // Clear display, stack, keys, and registers
    gfx.fill(0);
    stack.fill(0);
    V.fill(0);
    memset(keys, 0, sizeof(keys));

    // Clear memory from 0x200 upwards (leave fontset area untouched for now)
    // Although loading a ROM will overwrite this anyway.
    // memory.fill(0); // Maybe better to only fill memory where ROM doesn't load

    // Load built-in fontset into memory starting at 0x50
    static uint8_t fontset[80] = {
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
    for (int i = 0; i < 80; ++i) {
        memory[0x50 + i] = fontset[i];
    }
} // <--- THIS WAS THE MISSING CLOSING BRACE

bool Chip8::loadROM(const char* filename) {
    reset(); // Reset the system before loading a new ROM

    std::ifstream rom(filename, std::ios::binary | std::ios::ate);
    if (!rom) {
        std::cerr << "Error opening ROM file: " << filename << std::endl;
        return false;
    }

    std::streampos size = rom.tellg();
    if (size > (MEMORY_SIZE - 0x200)) {
        std::cerr << "Error: ROM file too large! Size: " << size << std::endl;
        return false;
    }

    rom.seekg(0, std::ios::beg);
    // Read directly into the memory array starting at 0x200
    if (!rom.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
         std::cerr << "Error reading ROM file into memory." << std::endl;
         return false;
    }

    std::cout << "Loaded ROM: " << filename << " (" << size << " bytes)" << std::endl;
    pc = 0x200; // Ensure PC is set correctly after loading
    return true;
}

void Chip8::emulateCycle() {
    // Fetch Opcode
    // Ensure PC is within memory bounds (basic check)
    if (pc + 1 >= MEMORY_SIZE) {
         std::cerr << "Error: Program Counter out of bounds! PC=" << std::hex << pc << std::endl;
         // Handle error appropriately, maybe stop emulation
         exit(1); // Or set a 'halted' flag
    }
    opcode = (memory[pc] << 8) | memory[pc+1];

    // For debugging:
    // std::cout << "PC: " << std::hex << pc << " Opcode: " << opcode << std::endl;

    // Decode and Execute Opcode
    executeOpcode();

    // Update timers (this happens in main loop now based on SDL ticks)
    // updateTimers(); // Moved to main loop for ~60Hz timing
}

void Chip8::executeOpcode() {
    // Extract common opcode parts
    uint16_t nnn = opcode & 0x0FFF;
    uint8_t n = opcode & 0x000F;
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t kk = opcode & 0x00FF;

    // Process opcode (increment PC at the end of most cases)
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) { // Use 'opcode' directly for 0x00E0, 0x00EE etc.
                case 0x00E0: // 00E0: CLS - Clear the display
                    gfx.fill(0);
                    drawFlag = true;
                    pc += 2;
                    break;
                case 0x00EE: // 00EE: RET - Return from subroutine
                    if (sp == 0) {
                         std::cerr << "Error: Stack underflow on RET!" << std::endl;
                         exit(1); // Or handle differently
                    }
                    sp--;           // Decrement stack pointer
                    pc = stack[sp]; // Set PC to address from stack
                    pc += 2;        // Increment PC past the return address storage
                    break;

                // --- SuperChip / SCHIP Opcodes (Optional) ---
                // case 0x00FB: // SCRR: scroll right 4
                //     {
                //         int w = highRes ? WIDTH_HIGH : WIDTH_LOW;
                //         int h = highRes ? HEIGHT_HIGH : HEIGHT_LOW;
                //         for(int r=0; r<h; ++r) {
                //             for(int c=w-1; c>=4; --c) {
                //                 gfx[r*WIDTH_HIGH + c] = gfx[r*WIDTH_HIGH + c-4];
                //             }
                //             for(int c=0; c<4; ++c) gfx[r*WIDTH_HIGH + c] = 0;
                //         }
                //         drawFlag = true;
                //         pc += 2;
                //     }
                //     break;
                // case 0x00FC: // SCRL: scroll left 4
                //     {
                //         int w = highRes ? WIDTH_HIGH : WIDTH_LOW;
                //         int h = highRes ? HEIGHT_HIGH : HEIGHT_LOW;
                //         for(int r=0; r<h; ++r) {
                //             for(int c=0; c<w-4; ++c) {
                //                 gfx[r*WIDTH_HIGH + c] = gfx[r*WIDTH_HIGH + c+4];
                //             }
                //             for(int c=w-4; c<w; ++c) gfx[r*WIDTH_HIGH + c] = 0;
                //         }
                //         drawFlag = true;
                //         pc += 2;
                //     }
                //     break;
                // case 0x00FD: // EXIT (non-standard opcode to terminate)
                //     exit(0);
                //     break; // technically unreachable
                // case 0x00FE: // Disable high-resolution mode
                //     highRes = false;
                //     // Maybe clear screen? Behavior varies.
                //     pc += 2;
                //     break;
                // case 0x00FF: // Enable high-resolution mode
                //     highRes = true;
                //     // Maybe clear screen? Behavior varies.
                //     pc += 2;
                //     break;

                default:
                    if ((opcode & 0xFFF0) == 0x00C0) { // Check for 00CN pattern
                        // 00CN: scroll down N lines (SCHIP)
                        // int scroll_n = opcode & 0x000F;
                        // int w = highRes ? WIDTH_HIGH : WIDTH_LOW;
                        // int h = highRes ? HEIGHT_HIGH : HEIGHT_LOW;
                        // // Create temporary buffer for shifted data
                        // std::array<uint8_t, WIDTH_HIGH * HEIGHT_HIGH> temp_gfx = gfx;
                        // gfx.fill(0); // Clear original gfx first

                        // for(int r=0; r < h - scroll_n; ++r) {
                        //     for(int c=0; c<w; ++c) {
                        //         gfx[(r+scroll_n)*WIDTH_HIGH + c] = temp_gfx[r*WIDTH_HIGH + c];
                        //     }
                        // }
                        // drawFlag = true;
                        pc += 2; // Still increment PC even if ignored
                    } else {
                         std::cerr << "Unknown opcode [0x0000]: " << std::hex << opcode << std::endl;
                         pc += 2; // Skip unknown 0x0XXX opcodes
                    }
                    break;
            }
            break; // End of 0x0000 case

        case 0x1000: // 1NNN: JP addr - Jump to location NNN
            pc = nnn; // Set PC directly, do not increment later
            break;

        case 0x2000: // 2NNN: CALL addr - Call subroutine at NNN
             if (sp >= stack.size()) {
                 std::cerr << "Error: Stack overflow on CALL!" << std::endl;
                 exit(1); // Or handle differently
             }
            stack[sp] = pc; // Store current PC on stack
            sp++;           // Increment stack pointer
            pc = nnn;       // Set PC to subroutine address
            break;

        case 0x3000: // 3XKK: SE Vx, byte - Skip next instruction if V[x] == KK
            if (V[x] == kk) {
                pc += 4; // Skip next instruction (2 bytes)
            } else {
                pc += 2;
            }
            break;

        case 0x4000: // 4XKK: SNE Vx, byte - Skip next instruction if V[x] != KK
             if (V[x] != kk) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x5000: // 5XY0: SE Vx, Vy - Skip next instruction if V[x] == V[y]
            if (n != 0) { // Ensure last nibble is 0
                 std::cerr << "Unknown opcode [0x5000]: " << std::hex << opcode << std::endl;
                 pc += 2;
                 break;
            }
            if (V[x] == V[y]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x6000: // 6XKK: LD Vx, byte - Set V[x] = KK
            V[x] = kk;
            pc += 2;
            break;

        case 0x7000: // 7XKK: ADD Vx, byte - Set V[x] = V[x] + KK
            V[x] += kk; // Overflow behavior is intentional (wraps around)
            pc += 2;
            break;

        case 0x8000: // 8XYN: Bitwise/Math operations
            switch (n) { // Check the last nibble 'N'
                case 0x0: // 8XY0: LD Vx, Vy - Set V[x] = V[y]
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x1: // 8XY1: OR Vx, Vy - Set V[x] = V[x] OR V[y]
                    V[x] |= V[y];
                    pc += 2;
                    break;
                case 0x2: // 8XY2: AND Vx, Vy - Set V[x] = V[x] AND V[y]
                    V[x] &= V[y];
                    pc += 2;
                    break;
                case 0x3: // 8XY3: XOR Vx, Vy - Set V[x] = V[x] XOR V[y]
                    V[x] ^= V[y];
                    pc += 2;
                    break;
                case 0x4: // 8XY4: ADD Vx, Vy - Set V[x] = V[x] + V[y], set VF = carry
                    {
                        uint16_t sum = V[x] + V[y];
                        V[0xF] = (sum > 0xFF) ? 1 : 0; // Set carry flag
                        V[x] = static_cast<uint8_t>(sum); // Store lower 8 bits
                        pc += 2;
                    }
                    break;
                case 0x5: // 8XY5: SUB Vx, Vy - Set V[x] = V[x] - V[y], set VF = NOT borrow
                    V[0xF] = (V[x] > V[y]) ? 1 : 0; // Set NOT borrow flag
                    V[x] -= V[y];
                    pc += 2;
                    break;
                case 0x6: // 8XY6: SHR Vx {, Vy} - Set V[x] = V[x] SHR 1
                    // Optional: V[x] = V[y] SHR 1 on some interpreters
                    V[0xF] = V[x] & 0x1; // Store least significant bit in VF
                    V[x] >>= 1;
                    pc += 2;
                    break;
                case 0x7: // 8XY7: SUBN Vx, Vy - Set V[x] = V[y] - V[x], set VF = NOT borrow
                    V[0xF] = (V[y] > V[x]) ? 1 : 0; // Set NOT borrow flag
                    V[x] = V[y] - V[x];
                    pc += 2;
                    break;
                case 0xE: // 8XYE: SHL Vx {, Vy} - Set V[x] = V[x] SHL 1
                    // Optional: V[x] = V[y] SHL 1 on some interpreters
                    V[0xF] = (V[x] & 0x80) ? 1 : 0; // Store most significant bit in VF
                    V[x] <<= 1;
                    pc += 2;
                    break;
                default:
                     std::cerr << "Unknown opcode [0x8000]: " << std::hex << opcode << std::endl;
                     pc += 2;
                     break;
            }
            break; // End of 0x8000 case

        case 0x9000: // 9XY0: SNE Vx, Vy - Skip next instruction if V[x] != V[y]
            if (n != 0) { // Ensure last nibble is 0
                 std::cerr << "Unknown opcode [0x9000]: " << std::hex << opcode << std::endl;
                 pc += 2;
                 break;
            }
            if (V[x] != V[y]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0xA000: // ANNN: LD I, addr - Set I = NNN
            I = nnn;
            pc += 2;
            break;

        case 0xB000: // BNNN: JP V0, addr - Jump to location NNN + V[0]
            pc = nnn + V[0];
            break;

        case 0xC000: // CXKK: RND Vx, byte - Set V[x] = random byte AND KK
            // Use a better random number generator if available
            V[x] = (rand() % 256) & kk;
            pc += 2;
            break;

        case 0xD000: // DXYN: DRW Vx, Vy, nibble - Display n-byte sprite starting at memory[I] at (Vx, Vy), set VF = collision
            {
                uint8_t coordX = V[x];
                uint8_t coordY = V[y];
                uint8_t height = n;
                V[0xF] = 0; // Reset collision flag

                int current_width = highRes ? WIDTH_HIGH : WIDTH_LOW;
                int current_height = highRes ? HEIGHT_HIGH : HEIGHT_LOW;

                for (int row = 0; row < height; ++row) {
                     if (I + row >= MEMORY_SIZE) {
                         std::cerr << "Warning: Sprite draw out of memory bounds (I=" << I << ", row=" << row << ")" << std::endl;
                         continue; // Skip this row or handle error
                     }
                    uint8_t spriteByte = memory[I + row];
                    for (int col = 0; col < 8; ++col) {
                        // Check if the current sprite pixel is set
                        if ((spriteByte & (0x80 >> col)) != 0) {
                            // Calculate screen coordinates with wrap-around
                            int screenX = (coordX + col) % current_width;
                            int screenY = (coordY + row) % current_height;

                            // Calculate the index in the 1D gfx array
                            // IMPORTANT: Use WIDTH_HIGH for indexing gfx, even in low-res mode
                            int index = screenY * WIDTH_HIGH + screenX;

                            // Check for collision: if screen pixel is already set
                            if (gfx[index] == 1) {
                                V[0xF] = 1; // Set collision flag
                            }
                            // XOR the pixel onto the screen
                            gfx[index] ^= 1;
                        }
                    }
                }
                drawFlag = true; // Set flag to indicate screen needs redraw
                pc += 2;
            }
            break; // End of 0xD000 case

        case 0xE000: // EX9E, EXA1: Skip based on key press
            switch (kk) {
                case 0x9E: // EX9E: SKP Vx - Skip next instruction if key with the value of V[x] is pressed
                    if (keys[V[x] & 0xF]) { // Check key state corresponding to value in V[x]
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;
                case 0xA1: // EXA1: SKNP Vx - Skip next instruction if key with the value of V[x] is NOT pressed
                    if (!keys[V[x] & 0xF]) { // Check key state
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;
                default:
                     std::cerr << "Unknown opcode [0xE000]: " << std::hex << opcode << std::endl;
                     pc += 2;
                     break;
            }
            break; // End of 0xE000 case

        case 0xF000: // FXnn: Miscellaneous operations
            switch (kk) {
                case 0x07: // FX07: LD Vx, DT - Set V[x] = delay timer value
                    V[x] = delayTimer;
                    pc += 2;
                    break;
                case 0x0A: // FX0A: LD Vx, K - Wait for a key press, store the value of the key in V[x]
                    {
                        bool keyPressed = false;
                        for (int i = 0; i < 16; ++i) {
                            if (keys[i]) {
                                V[x] = i; // Store the key index
                                keyPressed = true;
                                break; // Found a pressed key
                            }
                        }
                        // If no key was pressed, *do not* increment PC - effectively halting until a key is pressed
                        if (!keyPressed) {
                            return; // Exit emulateCycle without incrementing PC
                        }
                        pc += 2; // Key was pressed, continue
                    }
                    break;
                case 0x15: // FX15: LD DT, Vx - Set delay timer = V[x]
                    delayTimer = V[x];
                    pc += 2;
                    break;
                case 0x18: // FX18: LD ST, Vx - Set sound timer = V[x]
                    soundTimer = V[x];
                    pc += 2;
                    break;
                case 0x1E: // FX1E: ADD I, Vx - Set I = I + V[x]
                    // Check for undocumented overflow behavior if needed (VF flag setting varies)
                    I += V[x];
                    pc += 2;
                    break;
                case 0x29: // FX29: LD F, Vx - Set I = location of sprite for digit V[x]
                    // Sprites are 5 bytes high, fontset starts at 0x50
                    I = 0x50 + ((V[x] & 0x0F) * 5);
                    pc += 2;
                    break;
                case 0x33: // FX33: LD B, Vx - Store BCD representation of V[x] in memory locations I, I+1, and I+2
                    {
                         if (I + 2 >= MEMORY_SIZE) {
                             std::cerr << "Warning: BCD store out of memory bounds (I=" << I << ")" << std::endl;
                             // Handle error or proceed cautiously
                         }
                        uint8_t value = V[x];
                        memory[I]     = value / 100;       // Hundreds digit
                        memory[I + 1] = (value / 10) % 10; // Tens digit
                        memory[I + 2] = value % 10;        // Ones digit
                        pc += 2;
                    }
                    break;
                case 0x55: // FX55: LD [I], Vx - Store registers V0 through V[x] in memory starting at location I
                    {
                         if (I + x >= MEMORY_SIZE) {
                             std::cerr << "Warning: Register dump (FX55) out of memory bounds (I=" << I << ", x=" << (int)x << ")" << std::endl;
                         }
                        for (int i = 0; i <= x; ++i) {
                            memory[I + i] = V[i];
                        }
                        // Optional: On some original interpreters, I = I + X + 1 after this operation.
                        // Modern interpretation usually leaves I unchanged. Let's leave it unchanged for now.
                        // I += x + 1;
                        pc += 2;
                    }
                    break;
                case 0x65: // FX65: LD Vx, [I] - Read registers V0 through V[x] from memory starting at location I
                     {
                         if (I + x >= MEMORY_SIZE) {
                             std::cerr << "Warning: Register load (FX65) out of memory bounds (I=" << I << ", x=" << (int)x << ")" << std::endl;
                         }
                        for (int i = 0; i <= x; ++i) {
                            V[i] = memory[I + i];
                        }
                        // Optional: On some original interpreters, I = I + X + 1 after this operation.
                        // Modern interpretation usually leaves I unchanged. Let's leave it unchanged for now.
                        // I += x + 1;
                        pc += 2;
                    }
                    break;
                default:
                     std::cerr << "Unknown opcode [0xF000]: " << std::hex << opcode << std::endl;
                     pc += 2;
                     break;
            }
            break; // End of 0xF000 case

        default:
            std::cerr << "Unknown major opcode: " << std::hex << opcode << std::endl;
            pc += 2; // Skip unknown opcode
    }
}


void Chip8::updateTimers() {
    // Called roughly 60 times per second by the main loop
    if (delayTimer > 0) {
        --delayTimer;
    }
    if (soundTimer > 0) {
        --soundTimer;
        // Sound should play if soundTimer > 0, handled by audio callback
    }
}

void Chip8::setKeyState(int key, bool pressed) {
    if (key >= 0 && key < 16) {
        keys[key] = pressed ? 1 : 0;
        // Optional: Add logging here if needed
        // std::cout << "Key " << std::hex << key << (pressed ? " pressed" : " released") << std::endl;
    } else {
        std::cerr << "Warning: Invalid key index: " << key << std::endl;
    }
}