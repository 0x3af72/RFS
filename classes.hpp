#include <iostream>
#include <unordered_map>

#include "font_renderer.hpp"

#include <SDL2/include/SDL2/SDL.h>
#include <SDL2/include/SDL2/SDL_image.h>

#pragma once

std::unordered_map<std::string, SDL_Texture*> _cached_textures;

// texture loading function i keep reusing
SDL_Texture* loadTexture(SDL_Renderer* renderer, std::string path){

    if (_cached_textures.count(path) != 0){
        return _cached_textures[path];
    }

    SDL_Surface* temp_surface = IMG_Load(path.c_str());

    if (!temp_surface){
        std::cout << "Unable to load image: " << SDL_GetError() << "\n";
    }

    SDL_Texture* loaded_texture = SDL_CreateTextureFromSurface(renderer, temp_surface);
    SDL_FreeSurface(temp_surface);

    // cache texture
    _cached_textures[path] = loaded_texture;

    // return texture
    return loaded_texture;

}

// file class
class DisFile{
    public:
        SDL_Texture* tex;
        SDL_Rect rect;
        std::string name, disName;
        DisFile(SDL_Renderer* ren, std::string name);
        void render(SDL_Renderer* ren, int y);
        bool collides(SDL_Rect other, int y, SDL_Rect& sRect);
};

DisFile::DisFile(SDL_Renderer* ren, std::string name) : name(name){
    tex = loadTexture(ren, "images/file.png");
    rect = {50, 0, 400, 36};
    disName = name;
    if (textWidth(ren, disName, 20) >= 400 - textWidth(ren, "...", 20) - 30 - 22){
        while (textWidth(ren, disName, 20) >= 400 - textWidth(ren, "...", 20) - 30 - 22){
            disName = disName.substr(0, disName.size() - 1);
        }
        disName += "...";
    }
}

bool DisFile::collides(SDL_Rect other, int y, SDL_Rect& sRect){
    SDL_Rect nr = rect;
    nr.y = y;
    sRect = nr;
    return SDL_HasIntersection(&other, &nr);
}

void DisFile::render(SDL_Renderer* ren, int y){
    SDL_Rect nr = rect;
    nr.y = y;

    // icon
    SDL_Rect icRect = {nr.x, nr.y, 30, 36};
    SDL_RenderCopy(ren, tex, NULL, &icRect);

    // name
    renderText(
        ren, Color(0, 255, 0),
        disName, nr.x + icRect.w + 22, nr.y, 20
    );
}

// folder class
class DisFolder{
    public:
        SDL_Texture* tex;
        SDL_Rect rect;
        std::string name, disName;
        DisFolder(SDL_Renderer* ren, std::string name);
        void render(SDL_Renderer* ren, int y);
        bool collides(SDL_Rect other, int y, SDL_Rect& sRect);
};

DisFolder::DisFolder(SDL_Renderer* ren, std::string name) : name(name){
    tex = loadTexture(ren, "images/folder.png");
    rect = {50, 0, 400, 36};
    disName = name;
    if (textWidth(ren, disName, 20) >= 400 - textWidth(ren, "...", 20) - 42 - 10){
        while (textWidth(ren, disName, 20) >= 400 - textWidth(ren, "...", 20) - 42 - 10){
            disName = disName.substr(0, disName.size() - 1);
        }
        disName += "...";
    }
}

bool DisFolder::collides(SDL_Rect other, int y, SDL_Rect& sRect){
    SDL_Rect nr = rect;
    nr.y = y;
    sRect = nr;
    return SDL_HasIntersection(&other, &nr);
}

void DisFolder::render(SDL_Renderer* ren, int y){
    SDL_Rect nr = rect;
    nr.y = y;

    // icon
    SDL_Rect icRect = {nr.x, nr.y, 42, 36};
    SDL_RenderCopy(ren, tex, NULL, &icRect);

    // name
    renderText(
        ren, Color(0, 255, 0),
        disName, nr.x + icRect.w + 10, nr.y + 5, 20
    );
}