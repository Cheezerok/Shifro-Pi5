#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "spi_pi.h"
#include "colors.h"
#include "commands.h"

// Размеры дисплея
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160

// Смещения 
#define ST7735_XSTART 0
#define ST7735_YSTART 0

// Настройки поворота дисплея
enum class DisplayRotation {
    ROTATION_0   = 0,  // Нормальная ориентация
    ROTATION_90  = 1,  // Поворот на 90 градусов
    ROTATION_180 = 2,  // Поворот на 180 градусов
    ROTATION_270 = 3   // Поворот на 270 градусов
};

// Структура шрифта
struct AppFont {
    const uint8_t* data;      // Данные шрифта
    int width;                // Ширина символа
    int height;               // Высота символа
    int first_char;          // Первый символ в наборе
    int last_char;           // Последний символ в наборе
};

// Основной класс для работы с TFT дисплеем
class TFTDisplay {
public:
    // Конструктор с параметрами подключения
    TFTDisplay(int channel = 0, int reset_pin = 25, int dc_pin = 24, 
               int width = ST7735_WIDTH, int height = ST7735_HEIGHT);
    ~TFTDisplay();

    // Основные функции управления дисплеем
    bool init();                    // Инициализация дисплея
    void setRotation(DisplayRotation rotation);  // Установка ориентации
    void clearScreen(uint16_t color = COLOR_BLACK);  // Очистка экрана

    // Функции рисования
    void drawPixel(int16_t x, int16_t y, uint16_t color);  // Рисование точки
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);  // Линия
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);  // Прямоугольник
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);  // Закрашенный прямоугольник
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);  // Окружность
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);  // Закрашенный круг

    // Работа с текстом
    void setFont(const AppFont& font);  // Установка шрифта
    void drawChar(int16_t x, int16_t y, uint8_t character, uint16_t color);  // Вывод одного символа
    void drawText(int16_t x, int16_t y, const std::string& text, uint16_t color);  // Вывод текста

    // Работа с изображениями
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const std::vector<uint16_t>& image_data);

    // Управление цветом
    void setColor(uint16_t color);  // Установка текущего цвета
    uint16_t getColor() const;      // Получение текущего цвета

    // Получение размеров дисплея
    int getWidth() const { return width; }    // Получить ширину
    int getHeight() const { return height; }   // Получить высоту

private:
    // Внутренние функции для работы с дисплеем
    void writeCommand(uint8_t cmd);   // Отправка команды
    void writeData(const uint8_t* data, size_t length);  // Отправка данных
    void writeData(uint8_t data);  // Отправка одного байта данных
    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);  // Установка области рисования

    // Параметры подключения и состояния
    SPIDevice spi;              // SPI интерфейс
    int reset_pin;              // Пин сброса
    int dc_pin;                 // Пин выбора данные/команда
    int width;                  // Текущая ширина
    int height;                 // Текущая высота
    DisplayRotation rotation;   // Текущий поворот
    uint16_t current_color;     // Текущий цвет
    AppFont current_font;       // Текущий шрифт
}; 