#pragma once

#include <cstdint>
#include <iostream>
#include "display_pi.h"

// Базовый шрифт 5x7 пикселей
// Содержит ASCII символы и русские буквы
const uint8_t basic_font[] = {
    // Пробел (32)
    0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x00, 0x00, 0x5F, 0x00, 0x00,
    // - (45)
    0x08, 0x08, 0x08, 0x08, 0x08,
    // 0 (48)
    0x3E, 0x51, 0x49, 0x45, 0x3E,
    // 1 (49)
    0x00, 0x42, 0x7F, 0x40, 0x00,
    // 2 (50)
    0x42, 0x61, 0x51, 0x49, 0x46,
    // Д (кириллическая)
    0x3C, 0x24, 0x24, 0x24, 0x7F,
    // е (кириллическая)
    0x38, 0x54, 0x54, 0x54, 0x18,
    // и (кириллическая)
    0x7C, 0x10, 0x10, 0x10, 0x7C,
    // ф (кириллическая)
    0x38, 0x54, 0xFE, 0x54, 0x38,
    // р (кириллическая)
    0x7C, 0x14, 0x14, 0x14, 0x08,
    // о (кириллическая)
    0x38, 0x44, 0x44, 0x44, 0x38,
    // в (кириллическая)
    0x7C, 0x54, 0x54, 0x54, 0x28,
    // а (кириллическая)
    0x20, 0x54, 0x54, 0x54, 0x78,
    // т (кириллическая)
    0x04, 0x04, 0x7C, 0x04, 0x04,
    // ь (кириллическая)
    0x7C, 0x44, 0x44, 0x44, 0x44,
    // З (кириллическая)
    0x42, 0x42, 0x5A, 0x56, 0x5A,
    // ш (кириллическая)
    0x7C, 0x40, 0x7C, 0x40, 0x7C,
    // с (кириллическая)
    0x38, 0x44, 0x44, 0x44, 0x00,
    // з (кириллическая)
    0x44, 0x54, 0x54, 0x54, 0x38,
    // ы (кириллическая)
    0x7C, 0x40, 0x7C, 0x44, 0x7C
};

// Определяем индексы для русских букв
#define RUS_DE 6    // Д
#define RUS_JE 7    // е
#define RUS_I  8    // и
#define RUS_EF 9    // ф
#define RUS_ER 10   // р
#define RUS_O  11   // о
#define RUS_VE 12   // в
#define RUS_A  13   // а
#define RUS_TE 14   // т
#define RUS_SOFT 15 // ь
#define RUS_ZE 16   // З
#define RUS_SHA 17  // ш
#define RUS_ES 18   // с
#define RUS_ZE_SMALL 19 // з
#define RUS_YERU 20 // ы

// Функция для преобразования UTF-8 в индекс для базового шрифта
uint8_t basic_utf8_to_index(unsigned char c1, unsigned char c2) {
    // Специальные символы (кириллические)
    if (c1 == 0xD0) {
        switch (c2) {
            case 0x94: return RUS_DE;     // Д
            case 0xB5: return RUS_JE;     // е
            case 0xB8: return RUS_I;      // и
            case 0xA4: case 0xB4: return RUS_EF; // Ф/ф
            case 0xA0: case 0x80: return RUS_ER; // Р/р
            case 0x9E: case 0xBE: return RUS_O;  // О/о
            case 0x92: case 0xB2: return RUS_VE; // В/в
            case 0x90: case 0xB0: return RUS_A;  // А/а
            case 0xA2: case 0x82: return RUS_TE; // Т/т
            case 0x97: return RUS_ZE;     // З
            case 0xB7: return RUS_ZE_SMALL; // з
            case 0xA1: case 0xB1: return RUS_ES; // С/с
            case 0xAB: case 0xBB: return RUS_YERU; // Ы/ы
            case 0xAC: case 0xBC: return RUS_SOFT; // Ь/ь
        }
    } else if (c1 == 0xD1) {
        switch (c2) {
            case 0x84: return RUS_EF;     // ф
            case 0x80: return RUS_ER;     // р
            case 0x82: return RUS_TE;     // т
            case 0x88: return RUS_SHA;    // ш
            case 0x8C: return RUS_SOFT;   // ь
            case 0x81: return RUS_ES;     // с
            case 0x8B: return RUS_YERU;   // ы
        }
    }
    
    // ASCII символы
    if (c1 >= 32 && c1 <= 50) {
        return c1 - 32;
    }
    
    // Если не найдено соответствие, выводим код символа для отладки
    std::cout << "Неизвестный символ UTF-8: " << std::hex << (int)c1 << " " << (int)c2 << std::dec << std::endl;
    return 0; // Вернуть пробел для неизвестных символов
}

// Определение шрифта для использования в TFTDisplay
const struct AppFont BASIC_FONT = {
    basic_font,     // Данные шрифта
    5,              // Ширина символа
    7,              // Высота символа
    32,             // Первый символ (пробел)
    50              // Последний символ (цифра 2)
}; 