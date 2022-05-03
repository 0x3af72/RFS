#include <iostream>
#include "csprng.hpp"

#pragma once

duthomhas::csprng rng;
int randInt(int min, int max){
    return float(rng()) / rng.max() * max + min;
}