@echo off
g++ src\main.cpp src\Chip8.cpp src\FileBrowser.cpp -std=c++17 -Iinclude -Llib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -o chip8.exe
pause
