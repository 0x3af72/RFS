#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

#include "SDL2/include/SDL2/SDL.h"
#include "SDL2/include/SDL2/SDL_image.h"
#include "SDL2/include/SDL2/SDL_ttf.h"

#pragma once

std::string _default_chars = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()?.,[]{};:'\"/<>+=-_";
TTF_Font* font;

// Color class.
class Color{
    public:
        int r, g, b, a;
        Color(int r, int g, int b, int a);
        Color() = default;
};

Color::Color(int r, int g, int b, int a = 255){
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
    if (r == 0 && g == 0 && b == 0){
        this->b = 1;
    }
}

// Character class for managing characters and their respective textures.
class Character{

    public:

        // dimensions
        int width;
        int height;

        // texture
        SDL_Texture* texture;
        
        // render function
        int render(SDL_Renderer* renderer, int x, int y, Color color);

        // constructor
        Character(SDL_Renderer* renderer, SDL_Surface* character_surf);
        Character() = default;

};

Character::Character(SDL_Renderer* renderer, SDL_Surface* character_surf){

    // store dimensions
    width = character_surf->w;
    height = character_surf->h;

    // convert surface to texture
    texture = SDL_CreateTextureFromSurface(renderer, character_surf);

    // set blendmode
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    // free surface
    SDL_FreeSurface(character_surf);

}

int Character::render(SDL_Renderer* renderer, int x, int y, Color color){

    // get rect
    SDL_Rect rect = {x, y, width, height};

    // set color
    SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(texture, color.a);

    // render
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // return width
    return width;

}

// Map of colors and maps of characters and their corresponding textures.
std::unordered_map<std::string, std::unordered_map<int, std::unordered_map<std::string, Character>>> character_map;
std::unordered_map<std::string, std::vector<int>> added_sizes;

// Load a font file and edit the character map accordingly.
void loadFont(
    SDL_Renderer* renderer,
    std::string font_file,
    int size,
    std::string characters
){

    // load font
    font = TTF_OpenFont(font_file.c_str(), size);

    // render text in black
    SDL_Color color = {255, 255, 255};
    
    // store each character
    for (char& character: characters){
        
        std::string char_str(1, character);
        SDL_Surface* thing = TTF_RenderText_Blended(font, char_str.c_str(), color);
        character_map[font_file][size][char_str] = Character(renderer, thing);
        
    }
}

// Render text.
void renderText(
    SDL_Renderer* renderer,
    Color color,
    std::string text,
    int x, int y,
    int size,
    std::string font = "fonts/hlgt.ttf",
    std::string load_characters = _default_chars
){

    if (size <= 1){
        return;
    }

    // load font with that size if not loaded
    if (std::find(added_sizes[font].begin(), added_sizes[font].end(), size) == added_sizes[font].end()){
        added_sizes[font].push_back(size);
        loadFont(renderer, font, size, load_characters);
    }

    // render characters
    for (char& character: text){
        std::string char_str(1, character);
        x += character_map[font][size][char_str].render(renderer, x, y, color);
    }

}

// Get width of text to be rendered.
float textWidth(
    SDL_Renderer* renderer, std::string text, int size,
    std::string font = "fonts/hlgt.ttf", float max = 10000,
    std::string load_characters = _default_chars
){

    if (!text.size()){return 0;}

    if (std::find(added_sizes[font].begin(), added_sizes[font].end(), size) == added_sizes[font].end()){
        added_sizes[font].push_back(size);
        loadFont(renderer, font, size, load_characters);
    }

    float width = 0;

    // go through widths
    for (char& character: text){

        std::string char_str(1, character);
        width += character_map[font][size][char_str].width;

        // check if width exceeded max
        if (width >= max){
            return width;
        }

    }

    return width;

}

// Get last character before text wrap.
int textWrapIndex(std::string text, int size, std::string font = "fonts/hlgt.ttf", float max = 10000, bool wrap_by_words = false){

    if (!text.size()){return 0;}

    float width = 0;

    // last space (for wrapping by words)
    int last_space_index = -1;

    // go through widths
    int index = 0;
    for (char& character: text){

        std::string char_str(1, character);
        width += character_map[font][size][char_str].width;

        // check if is a space
        if (char_str == " "){
            last_space_index = index;
        }

        // check if is newline
        if (char_str == "\n"){
            return index + 1;
        }

        // check if width exceeded max
        if (width >= max){
            return (wrap_by_words) ? ((last_space_index == -1) ? -1 : last_space_index + 1) : index;
        }

        index += 1;

    }

    return -1;

}

// Get height of text to be rendered.
float textHeight(
    SDL_Renderer* renderer,
    std::string text, int size,
    std::string font = "fonts/hlgt.ttf",
    std::string load_characters = _default_chars
){

    if (!text.size()){return 0;}

    if (std::find(added_sizes[font].begin(), added_sizes[font].end(), size) == added_sizes[font].end()){
        added_sizes[font].push_back(size);
        loadFont(renderer, font, size, load_characters);
    }

    // all characters have the same height
    return character_map[font][size][text.substr(0, 1)].height;

}