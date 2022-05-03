#include <iostream>
#include "_sha256.hpp"

#pragma once

std::string _sha256(SHA256& shObj, std::string raw){
    shObj.update(raw);
    unsigned char* digest = shObj.digest();
    std::string res = SHA256::toString(digest);
    delete[] digest;
    return res;
}

std::string sha256(std::string raw, int iter){
    SHA256 shObj;
    std::string hashed = raw;
    for (int i = 0; i != iter; i++){
        hashed = _sha256(shObj, hashed);
    }
    return hashed;
}