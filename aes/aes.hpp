#include <iostream>
#include <vector>

#include "rijndael.h"
#include "../rand/cs_random.hpp"

#pragma once

// encrypt result object
class EncryptRes{
    public:
        std::string encrypted, iv;
        int rPadsize;
        EncryptRes(std::string encrypted, std::string iv, int rPadsize) : encrypted(encrypted), iv(iv), rPadsize(rPadsize){};
        EncryptRes() = default;
};

// pad to number
int padTo(std::string& str, int to){
    if (str.size() < to){
        int diff = to - str.size();
        std::string pad(to - str.size(), 'P');
        str += pad;
        return diff;
    }
    return 0;
}

// round to nearest mul
int roundTo(int n, int mul){
    return ((n + mul - 1) / mul) * mul;
}

// encrypt string
EncryptRes encrypt(std::string raw, std::string key, std::string iv = ""){

    // set key
    padTo(key, 32);
    unsigned char aKey[32];
    for (int i = 0; i != key.size(); i++) aKey[i] = key[i];
    rijn_context ctx;
    rijn_set_key(&ctx, aKey, 256, 256);

    // pad raw
    int pSize = padTo(raw, roundTo(raw.size(), 32));

    // iv
    unsigned char aIv[32];
    if (iv.size()){
        for (int i = 0; i != iv.size(); i++) aIv[i] = iv[i];
    } else {
        for (int i = 0; i != 32; i++){
            aIv[i] = randInt(0, 255);
            iv += aIv[i];
        }
    }
    
    // chunk up raw
    std::string enc;
    for (int m = 0; m != raw.size(); m += 32){
        unsigned char aCh[32], res[32];
        std::string sCh = raw.substr(m, 32);
        for (int i = 0; i != sCh.size(); i++) aCh[i] = sCh[i];
        rijn_cbc_encrypt(&ctx, aIv, aCh, res, 32);
        for (int i = 0; i != sizeof(res); i++) enc += res[i];
    }
    return EncryptRes(enc, iv, pSize);
}

// decrypt string
std::string decrypt(std::string eStr, std::string key, std::string iv, int pSize){

    // set key
    padTo(key, 32);
    unsigned char aKey[32];
    for (int i = 0; i != key.size(); i++) aKey[i] = key[i];
    rijn_context ctx;
    rijn_set_key(&ctx, aKey, 256, 256);

    // iv
    unsigned char aIv[32];
    for (int i = 0; i != iv.size(); i++) aIv[i] = iv[i];

    // chunk up encrypted
    std::string dec;
    for (int m = 0; m != eStr.size(); m += 32){
        unsigned char aCh[32], res[32];
        std::string sCh = eStr.substr(m, 32);
        for (int i = 0; i != sCh.size(); i++) aCh[i] = sCh[i];
        rijn_cbc_decrypt(&ctx, aIv, aCh, res, 32);
        for (int i = 0; i != sizeof(res); i++) dec += res[i];
    }
    return dec.substr(0, dec.size() - pSize);
}