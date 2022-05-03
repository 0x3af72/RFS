#include <iostream>
#include <random>
#include <vector>

#include "_aes.hpp"
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

std::vector<unsigned char> strToVec(std::string str){
    std::vector<unsigned char> res(str.c_str(), str.c_str() + str.size());
    return res;
}

int roundUp(int num, int multiple){
    return ((num + multiple - 1) / multiple) * multiple;
}

EncryptRes encrypt(std::string raw, std::string key, std::string iv = ""){

    std::vector<unsigned char> vRaw, vKey, vIv;
    AES aes(AESKeyLength::AES_256);

    // get new raw
    int rPadsize = roundUp(raw.size(), 16) - raw.size();
    if (rPadsize){std::string pad(rPadsize, 'P'); raw += pad;}
    vRaw = strToVec(raw);

    // pad key
    if (key.size() < 32){std::string pad(32 - key.size(), 'P'); key += pad;}
    vKey = strToVec(key);

    // generate iv
    if (!iv.size()){
        for (int i = 0; i != 16; i++){
            vIv.push_back(randInt(0, 255));
        }
    } else {vIv = strToVec(iv);}
    std::string fIv(vIv.begin(), vIv.end());

    std::vector<unsigned char> veStr = aes.EncryptCBC(vRaw, vKey, vIv);
    std::string eStr(veStr.begin(), veStr.end());
    return EncryptRes(eStr, fIv, rPadsize);
}

std::string decrypt(std::string eStr, std::string key, std::string iv, int rPadsize){

    std::vector<unsigned char> veStr, vKey, vIv, vdStr;
    AES aes(AESKeyLength::AES_256);

    veStr = strToVec(eStr);
    if (key.size() < 32){std::string pad(32 - key.size(), 'P'); key += pad;} // pad key
    vKey = strToVec(key);
    vIv = strToVec(iv);

    vdStr = aes.DecryptCBC(veStr, vKey, vIv);
    std::string dStr(vdStr.begin(), vdStr.end());
    return dStr.substr(0, dStr.size() - rPadsize);
}