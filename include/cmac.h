#ifndef CMAC_H
#define CMAC_H

#include <vector>
#include <cstdint>
#include <cstring>

namespace cmac {

// Константы
constexpr uint8_t R = 0x87;  // Константа для генерации подключей (x^7 + x^6 + x^2 + x^1 + 1)
constexpr size_t BLOCK_SIZE = 16;  


// Структура для хранения подключей CMAC
struct CMACKeys {
    uint8_t K1[BLOCK_SIZE];  // Первый подключ
    uint8_t K2[BLOCK_SIZE];  // Второй подключ
};

// Сдвиг влево
void leftShift(uint8_t* data);

// XOR двух блоков
void xorBlocks(uint8_t* dst, const uint8_t* src);

// Генерирует подключи для CMAC
CMACKeys generateSubkeys(const uint8_t* key);

// Вычисляет CMAC для заданных данных
std::vector<uint8_t> calculateCMAC(const std::vector<char>& data, 
                                 const uint8_t* key, 
                                 const uint8_t* roundKeys);

} 

#endif