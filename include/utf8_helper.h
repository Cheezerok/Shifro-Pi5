#pragma once

#include <iostream>
#include <map>
#include <string>



namespace utf8 {

uint8_t russian_char_to_index(unsigned char c1, unsigned char c2) {


    static const std::map<uint16_t, uint8_t> utf8_to_cyrillic = {
        // Заглавные буквы
        { 0xD090, 128 }, // А
        { 0xD091, 129 }, // Б
        { 0xD092, 130 }, // В
        { 0xD093, 131 }, // Г
        { 0xD094, 132 }, // Д
        { 0xD095, 133 }, // Е
        { 0xD096, 134 }, // Ж
        { 0xD097, 135 }, // З
        { 0xD098, 136 }, // И
        { 0xD099, 137 }, // Й
        { 0xD09A, 138 }, // К
        { 0xD09B, 139 }, // Л
        { 0xD09C, 140 }, // М
        { 0xD09D, 141 }, // Н
        { 0xD09E, 142 }, // О
        { 0xD09F, 143 }, // П
        { 0xD0A0, 144 }, // Р
        { 0xD0A1, 145 }, // С
        { 0xD0A2, 146 }, // Т
        { 0xD0A3, 147 }, // У
        { 0xD0A4, 148 }, // Ф
        { 0xD0A5, 149 }, // Х
        { 0xD0A6, 150 }, // Ц
        { 0xD0A7, 151 }, // Ч
        { 0xD0A8, 152 }, // Ш
        { 0xD0A9, 153 }, // Щ
        { 0xD0AA, 154 }, // Ъ
        { 0xD0AB, 155 }, // Ы
        { 0xD0AC, 156 }, // Ь
        { 0xD0AD, 157 }, // Э
        { 0xD0AE, 158 }, // Ю
        { 0xD0AF, 159 }, // Я
        
        // Строчные буквы
        { 0xD0B0, 160 }, // а
        { 0xD0B1, 161 }, // б
        { 0xD0B2, 162 }, // в
        { 0xD0B3, 163 }, // г
        { 0xD0B4, 164 }, // д
        { 0xD0B5, 165 }, // е
        { 0xD0B6, 166 }, // ж
        { 0xD0B7, 167 }, // з
        { 0xD0B8, 168 }, // и
        { 0xD0B9, 169 }, // й
        { 0xD0BA, 170 }, // к
        { 0xD0BB, 171 }, // л
        { 0xD0BC, 172 }, // м
        { 0xD0BD, 173 }, // н
        { 0xD0BE, 174 }, // о
        { 0xD0BF, 175 }, // п
        { 0xD180, 176 }, // р
        { 0xD181, 177 }, // с
        { 0xD182, 178 }, // т
        { 0xD183, 179 }, // у
        { 0xD184, 180 }, // ф
        { 0xD185, 181 }, // х
        { 0xD186, 182 }, // ц
        { 0xD187, 183 }, // ч
        { 0xD188, 184 }, // ш
        { 0xD189, 185 }, // щ
        { 0xD18A, 186 }, // ъ
        { 0xD18B, 187 }, // ы
        { 0xD18C, 188 }, // ь
        { 0xD18D, 189 }, // э
        { 0xD18E, 190 }, // ю
        { 0xD18F, 191 }  // я
    };
    

    uint16_t key = ((uint16_t)c1 << 8) | c2;
    

    auto it = utf8_to_cyrillic.find(key);
    if (it != utf8_to_cyrillic.end()) {
        std::cout << "  Найдено соответствие: индекс шрифта " << (int)it->second << std::endl;
        return it->second;
    }
    

    if (c1 < 128) {
        return c1;
    }

    std::cout << "  Символ не найден в таблице!" << std::endl;
    return 0; // Пробел
}

} // namespace utf8 