#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>

class FileBrowser {
public:
    FileBrowser(SDL_Renderer* renderer, TTF_Font* font);
    // Opens a simple file selection UI. Returns true if a file was chosen.
    bool run(std::string& selected);
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    std::string currentDir;
    std::vector<std::string> entries;
    int selectedIndex;

    void listDirectory();
    void render();
};

#endif // FILEBROWSER_H
