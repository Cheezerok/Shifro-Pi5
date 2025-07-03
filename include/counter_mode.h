#ifndef COUNTER_MODE_H
#define COUNTER_MODE_H

#include <cstdint>
#include <vector>
#include <random>
#include <cstring>
#include "kuznechik.h"

namespace counter_mode {

// Размер блока и синхропосылки в байтах
constexpr size_t BLOCK_SIZE = 16;
constexpr size_t IV_SIZE = 16;

// Структура для хранения состояния счетчика
struct Counter {
    uint8_t value[BLOCK_SIZE];
    
    // Инкрементация счетчика
    void increment() {
        for(int i = BLOCK_SIZE - 1; i >= 0; --i) {
            if(++value[i] != 0) break;
        }
    }
    
    // Установка значения счетчика
    void setValue(const uint8_t* iv) {
        memcpy(value, iv, BLOCK_SIZE);
    }
};

// Генерация случайной синхропосылки
std::vector<uint8_t> generate_iv();

// Генерация гаммы для блока данных
void generate_gamma(uint8_t* gamma, Counter& ctr, const uint8_t* roundKeys);

// Наложение гаммы на блок данных
void apply_gamma(uint8_t* data, const uint8_t* gamma, size_t length);

} // namespace counter_mode

#endif 