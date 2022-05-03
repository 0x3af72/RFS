#include <iostream>
#include <vector>

#include "font_renderer.hpp"
#include "rand/cs_random.hpp"

#include <SDL2/include/SDL2/SDL.h>
#include <SDL2/include/SDL2/SDL_image.h>

std::string symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

class MatRainParticle{
    public:
        int x, y, alpha = 255, size;
        bool dead = false;
        std::string cont;
        MatRainParticle(int x, int y, int size, std::string cont) : x(x), y(y), size(size), cont(cont){};
        void update(SDL_Renderer* ren, Color color);
};
void MatRainParticle::update(SDL_Renderer* ren, Color color){
    if (dead) return;
    color.a = alpha;
    renderText(
        ren,
        color, cont,
        x, y, size,
        "fonts/hlgt.ttf",
        symbols
    );
    alpha -= 3;
    dead = alpha <= 3;
}

class MatRainStreak{
    public:
        int x, y = 0, step, maxSpawn, spawned = 0, nextSpawn = 0, spInterval;
        std::vector<MatRainParticle> particles;
        MatRainStreak(int x) : x(x){
            maxSpawn = 50;
            spInterval = randInt(5, 15);
        };
        bool update(SDL_Renderer* ren);
};

bool MatRainStreak::update(SDL_Renderer* ren){
    
    step = textHeight(ren, "L", 20);

    // add particles
    if (nextSpawn <= 0 && spawned != maxSpawn){
        std::string newCont(1, symbols[randInt(0, symbols.size())]);
        particles.push_back(MatRainParticle(x, y, 20, newCont));
        y += step;
        nextSpawn = spInterval;
        spawned += 1;
    }
    nextSpawn -= 1;

    // render particles
    for (int i = 0; i != int(particles.size() - 1); i++){
        particles[i].update(ren, Color(0, 255, 0));
    }
    particles[particles.size() - 1].update(ren, Color(255, 255, 255));
    return (spawned == maxSpawn && particles[particles.size() - 1].dead);

}