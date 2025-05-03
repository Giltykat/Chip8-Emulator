#include "FileBrowser.h"
#include <filesystem>
#include <algorithm>

FileBrowser::FileBrowser(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font), currentDir("."), selectedIndex(0) {
    listDirectory();
}

bool FileBrowser::run(std::string& selected) {
    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return false;
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        if (selectedIndex > 0) selectedIndex--;
                        break;
                    case SDLK_DOWN:
                        if (selectedIndex + 1 < entries.size()) selectedIndex++;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER: {
                        std::string name = entries[selectedIndex];
                        if (name == "..") {
                            currentDir = std::filesystem::path(currentDir).parent_path().string();
                            if (currentDir.empty()) currentDir = ".";
                        } else {
                            std::string path = currentDir + "/" + name;
                            if (std::filesystem::is_directory(path)) {
                                currentDir = path;
                            } else {
                                selected = path;
                                return true;
                            }
                        }
                        selectedIndex = 0;
                        listDirectory();
                        break;
                    }
                    case SDLK_BACKSPACE:
                        if (currentDir != ".") {
                            currentDir = std::filesystem::path(currentDir).parent_path().string();
                            if (currentDir.empty()) currentDir = ".";
                            selectedIndex = 0;
                            listDirectory();
                        }
                        break;
                }
            }
        }
        render();
        SDL_Delay(16); // ~60 FPS
    }
    return false;
}

void FileBrowser::listDirectory() {
    entries.clear();
    if (currentDir != ".") {
        entries.push_back("..");
    }
    for (auto& p : std::filesystem::directory_iterator(currentDir)) {
        std::string name = p.path().filename().string();
        if (p.is_directory() ||
            (name.size() > 4 &&
             (name.substr(name.size()-4) == ".ch8" || name.substr(name.size()-4) == ".CH8"))) {
            entries.push_back(name);
        }
    }
    std::sort(entries.begin(), entries.end());
}

void FileBrowser::render() {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    int y = 20;
    for (int i = 0; i < entries.size(); ++i) {
        SDL_Color color = (i == selectedIndex) ? SDL_Color{255,255,255,255}
                                              : SDL_Color{200,200,200,255};
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, entries[i].c_str(), color);
        SDL_Texture* textTex = SDL_CreateTextureFromSurface(renderer, textSurface);
        int w, h;
        SDL_QueryTexture(textTex, NULL, NULL, &w, &h);
        SDL_Rect dst = { 20, y, w, h };
        SDL_RenderCopy(renderer, textTex, NULL, &dst);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTex);
        y += h + 5;
    }
    SDL_RenderPresent(renderer);
}
