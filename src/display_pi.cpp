#include "display_pi.h"
#include "basic_font.h"
#include <stdexcept>
#include <chrono>
#include <thread>
#include <iostream>


TFTDisplay::TFTDisplay(int channel, int reset_pin, int dc_pin, int width, int height)
    : spi(channel, 8000000),  // Initialize SPI with 8MHz clock
      reset_pin(reset_pin),
      dc_pin(dc_pin),
      width(width),
      height(height),
      rotation(DisplayRotation::ROTATION_0),
      current_color(COLOR_BLACK) {
    
    // Initialize current_font with zeros
    current_font = {nullptr, 0, 0, 0, 0};
    

}

TFTDisplay::~TFTDisplay() {
    // Cleanup
    try {
        // Turn off display
        writeCommand(CMD_DISPOFF);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        
        // Enter sleep mode
        writeCommand(CMD_SLPIN);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    } catch (...) {
        // Ignore any errors during cleanup
    }
}

bool TFTDisplay::init() {
    
    // Initialize SPI first
    if (!spi.init()) {
        return false;
    }
    
    const int init_retries = 3;
    for (int retry = 0; retry < init_retries; retry++) {
        try {
            // Hardware reset sequence with more conservative timing
            spi.setRST(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            spi.setRST(0);
            std::this_thread::sleep_for(std::chrono::milliseconds(120));  // Increased reset pulse width
            spi.setRST(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(120));  // Increased stabilization time
            
            // Software reset
            writeCommand(CMD_SWRESET);  // Use command constant instead of raw value
            std::this_thread::sleep_for(std::chrono::milliseconds(150));  // Increased post-reset delay
            
            // Power control settings with verification
            writeCommand(CMD_PWCTR1);  // Power control A
            writeData((uint8_t)0x39);
            writeData((uint8_t)0x2C);
            writeData((uint8_t)0x00);
            writeData((uint8_t)0x34);
            writeData((uint8_t)0x02);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            writeCommand(CMD_PWCTR2);  // Power control B
            writeData((uint8_t)0x00);
            writeData((uint8_t)0xC1);
            writeData((uint8_t)0x30);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Driver timing control
            writeCommand(CMD_DISSET5);
            writeData((uint8_t)0x85);
            writeData((uint8_t)0x00);
            writeData((uint8_t)0x78);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Power on sequence control
            writeCommand(CMD_PWCTR3);
            writeData((uint8_t)0x64);
            writeData((uint8_t)0x03);
            writeData((uint8_t)0x12);
            writeData((uint8_t)0x81);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            writeCommand(CMD_VMCTR1);  // Pump ratio control
            writeData((uint8_t)0x20);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            writeCommand(CMD_PWCTR1);  // Power Control 1
            writeData((uint8_t)0x23);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            writeCommand(CMD_PWCTR2);  // Power Control 2
            writeData((uint8_t)0x10);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // VCOM Control with more stable values
            writeCommand(CMD_VMCTR1);
            writeData((uint8_t)0x3E);  // Adjusted VCOM value
            writeData((uint8_t)0x28);  // Adjusted VCOM value
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Memory Access Control (MADCTL)
            writeCommand(CMD_MADCTL);
            writeData((uint8_t)0x48);  // Normal orientation
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Pixel Format Set
            writeCommand(CMD_COLMOD);
            writeData((uint8_t)0x55);  // 16-bit color
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Frame Rate Control
            writeCommand(CMD_FRMCTR1);
            writeData((uint8_t)0x00);
            writeData((uint8_t)0x18);  // 79Hz refresh rate for stability
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Display Function Control
            writeCommand(CMD_DISSET5);
            writeData((uint8_t)0x08);
            writeData((uint8_t)0x82);
            writeData((uint8_t)0x27);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Enable 3G (gamma control)
            writeCommand(CMD_GAMSET);
            writeData((uint8_t)0x00);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Gamma Set
            writeCommand(CMD_GAMSET);
            writeData((uint8_t)0x01);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Positive Gamma Correction
            writeCommand(CMD_GAMCTRP1);
            {
                uint8_t gamma_values[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                                        0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
                for (uint8_t value : gamma_values) {
                    writeData(value);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Negative Gamma Correction
            writeCommand(CMD_GAMCTRN1);
            {
                uint8_t gamma_values[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                                        0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
                for (uint8_t value : gamma_values) {
                    writeData(value);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Exit Sleep
            writeCommand(CMD_SLPOUT);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));  // Increased sleep-out delay
            
            // Display on
            writeCommand(CMD_DISPON);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));  // Increased stabilization delay
            
            // Clear screen to black
            clearScreen(COLOR_BLACK);
            
            return true;
            
        } catch (const std::exception& e) {
            
            if (retry < init_retries - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait before retry
                // Perform a hard reset before retrying
                spi.setRST(0);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                spi.setRST(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    return false;
}

void TFTDisplay::setRotation(DisplayRotation rotation) {
    this->rotation = rotation;
    writeCommand(CMD_MADCTL);

    uint8_t madctl = 0;
    switch (rotation) {
        case DisplayRotation::ROTATION_0:
            madctl = 0xA0;  // MY=1, MV=1 - Отражение по Y, смена осей
            width = ST7735_HEIGHT;
            height = ST7735_WIDTH;
            break;
        case DisplayRotation::ROTATION_90:
            madctl = 0x00;  // Нормальная ориентация
            width = ST7735_WIDTH;
            height = ST7735_HEIGHT;
            break;
        case DisplayRotation::ROTATION_180:
            madctl = 0x60;  // MX=1, MV=1 - Отражение по X, смена осей
            width = ST7735_HEIGHT;
            height = ST7735_WIDTH;
            break;
        case DisplayRotation::ROTATION_270:
            madctl = 0xC0;  // MX=1, MY=1 - Отражение по X и Y
            width = ST7735_WIDTH;
            height = ST7735_HEIGHT;
            break;
    }
    
    madctl |= 0x00;  // Порядок цветов RGB
    writeData(&madctl, 1);

    // Сброс окна адресации на полный размер дисплея
    setAddressWindow(0, 0, width - 1, height - 1);
}

void TFTDisplay::clearScreen(uint16_t color) {
    // Установка окна адресации на весь дисплей
    setAddressWindow(0, 0, width - 1, height - 1);
    
    // Команда записи в память
    writeCommand(CMD_RAMWR);
    
    // Подготовка байтов цвета
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    // Заполнение экрана цветом блоками
    const size_t chunk_size = 1024;  // Обработка
    std::vector<uint8_t> buffer(chunk_size);
    for (size_t i = 0; i < chunk_size; i += 2) {
        buffer[i] = hi;
        buffer[i + 1] = lo;
    }
    
    size_t total_pixels = width * height;
    size_t pixels_written = 0;
    
    while (pixels_written < total_pixels) {
        size_t pixels_to_write = std::min((chunk_size / 2), total_pixels - pixels_written);
        writeData(buffer.data(), pixels_to_write * 2);
        pixels_written += pixels_to_write;
    }
}

void TFTDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;  // Проверка границ
    }
    
    setAddressWindow(x, y, x, y);
    
    // Преобразование цвета в формат дисплея (RGB565)
    uint8_t color_bytes[2];
    color_bytes[0] = color >> 8;    // Старший байт
    color_bytes[1] = color & 0xFF;  // Младший байт
    
    writeData(color_bytes, 2);
}

void TFTDisplay::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    
    if (color == 0) {
        color = COLOR_RED;
    }

    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = ((dx > dy) ? dx : -dy) / 2;
    int16_t e2;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void TFTDisplay::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawLine(x, y, x + w - 1, y, color);         // Верхняя линия
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // Нижняя линия
    drawLine(x, y, x, y + h - 1, color);         // Левая линия
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Правая линия
}

void TFTDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    // Проверка границ
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return;
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;

    setAddressWindow(x, y, x + w - 1, y + h - 1);
    
    // Заполнение прямоугольника
    uint32_t num_pixels = w * h;
    std::vector<uint16_t> buffer(num_pixels, color);
    writeData(reinterpret_cast<uint8_t*>(buffer.data()), num_pixels * 2);
}

void TFTDisplay::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void TFTDisplay::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    drawLine(x0, y0 - r, x0, y0 - r + 2 * r + 1, color);
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawLine(x0 + x, y0 - y, x0 + x, y0 + y, color);
        drawLine(x0 - x, y0 - y, x0 - x, y0 + y, color);
        drawLine(x0 + y, y0 - x, x0 + y, y0 + x, color);
        drawLine(x0 - y, y0 - x, x0 - y, y0 + x, color);
    }
}

void TFTDisplay::setFont(const AppFont& font) {
    current_font = font;
    // std::cout << "Установлен шрифт: ширина=" << font.width 
    //           << ", высота=" << font.height 
    //           << ", первый символ=" << font.first_char 
    //           << ", последний символ=" << font.last_char << std::endl;
}

void TFTDisplay::drawChar(int16_t x, int16_t y, uint8_t character, uint16_t color) {
    // Выход, если шрифт не установлен или символ за пределами поддерживаемого диапазона
    if (current_font.data == nullptr || 
        character < current_font.first_char || 
        character > current_font.last_char) {
        return;
    }
    
    // Вычисляем смещение в массиве данных шрифта
    int offset = (character - current_font.first_char) * 5; // 5 байт на символ
    
    // Проверка границ экрана с учетом дополнительного пространства для выступающих элементов
    if (x < 0 || y < 0 || 
        x + current_font.width + 2 > width || 
        y + current_font.height + 2 > height) {
        return;
    }
    
    // Отрисовка основного символа
    for (int col = 0; col < current_font.width; col++) {
        uint8_t column_data = current_font.data[offset + col];
        
        for (int row = 0; row < current_font.height; row++) {
            if (column_data & (1 << row)) {
                drawPixel(x + col, y + row, color);
            }
        }
    }
    
    // Специальная обработка для русских букв с выступающими элементами
    uint8_t char_index = character - current_font.first_char;
    
    // Для буквы "ь" (код 0xD0 0xAC)
    if (char_index == 124) {
        // Рисуем правую часть буквы "ь"
        drawPixel(x + current_font.width, y + current_font.height - 4, color);
        drawPixel(x + current_font.width, y + current_font.height - 3, color);
        drawPixel(x + current_font.width - 1, y + current_font.height - 2, color);
    }
    // Для буквы "ы" (код 0xD1 0x8B)
    else if (char_index == 20) {
        for (int row = 1; row < current_font.height - 1; row++) {
            drawPixel(x + current_font.width + 1, y + row, color);
        }
    }
    // Для буквы "д" (код 0xD0 0xB4)
    else if (char_index == 6) {
        drawPixel(x - 1, y + current_font.height, color);
        drawPixel(x + current_font.width, y + current_font.height, color);
        for (int col = 0; col < current_font.width; col++) {
            drawPixel(x + col, y + current_font.height, color);
        }
    }
    // Для буквы "л" (код 0xD0 0xBB)
    else if (char_index == 11) {
        // Рисуем левую ножку буквы "л"
        drawPixel(x - 1, y + current_font.height - 1, color);
        drawPixel(x, y + current_font.height - 1, color);
        drawPixel(x + 1, y + current_font.height - 1, color);
        // Добавляем диагональную линию для формы буквы
        drawPixel(x + 1, y + current_font.height - 2, color);
        drawPixel(x + 2, y + current_font.height - 2, color);
    }
}

void TFTDisplay::drawText(int16_t x, int16_t y, const std::string& text, uint16_t color) {
    if (current_font.data == nullptr) {
        return;  // Шрифт не установлен
    }
    
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    
   
    for (size_t i = 0; i < text.length(); i++) {
        // Проверка на выход за границы экрана
        if (cursor_x + current_font.width > width) {
            // Переход на следующую строку
            cursor_x = x;
            cursor_y += current_font.height + 1;
            if (cursor_y + current_font.height > height) {
                break;  // Вышли за пределы экрана по вертикали
            }
        }
        
        // Проверяем, не является ли символ частью UTF-8 последовательности
        unsigned char c = text[i];
        if ((c & 0x80) == 0) {
            // ASCII символ
            drawChar(cursor_x, cursor_y, c, color);
            cursor_x += current_font.width + 1; // +1 для пробела между символами
        }
        // Пропускаем многобайтовые UTF-8 символы (их нужно обрабатывать отдельно)
    }
}

void TFTDisplay::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const std::vector<uint16_t>& image_data) {
    if (x >= width || y >= height) return;
    
    // Clip dimensions to display bounds
    if ((x + w) > width) w = width - x;
    if ((y + h) > height) h = height - y;
    if (w <= 0 || h <= 0) return;

    // Set the address window
    setAddressWindow(x, y, x + w - 1, y + h - 1);

    // Convert all pixels to display format
    std::vector<uint8_t> buffer(w * h * 2);
    for (int i = 0; i < w * h && i < image_data.size(); i++) {
        uint16_t pixel = image_data[i];
        buffer[i * 2] = pixel >> 8;        // High byte
        buffer[i * 2 + 1] = pixel & 0xFF;  // Low byte
    }

    // Send all data at once
    writeData(buffer.data(), buffer.size());
}

void TFTDisplay::writeCommand(uint8_t cmd) {
    try {
        spi.setDC(false);  // Режим команды
        spi.write(&cmd, 1);
    } catch (const std::exception& e) {
        
    }
}

void TFTDisplay::writeData(const uint8_t* data, size_t length) {
    try {
        spi.setDC(true);  // Режим данных
        spi.write(const_cast<uint8_t*>(data), length);
    } catch (const std::exception& e) {
        
    }
}

void TFTDisplay::writeData(uint8_t data) {
    try {
        spi.setDC(true);  // Режим данных
        spi.write(&data, 1);
    } catch (const std::exception& e) {
        
    }
}

void TFTDisplay::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Установка диапазона столбцов
    writeCommand(CMD_CASET);
    uint8_t caset[] = {0x00, (uint8_t)x0, 0x00, (uint8_t)x1};
    writeData(caset, sizeof(caset));

    // Установка диапазона строк
    writeCommand(CMD_RASET);
    uint8_t raset[] = {0x00, (uint8_t)y0, 0x00, (uint8_t)y1};
    writeData(raset, sizeof(raset));

    // Команда записи в память
    writeCommand(CMD_RAMWR);
}

void TFTDisplay::setColor(uint16_t color) {
    // Преобразование из формата RGB565
    current_color = ((color & 0xFF00) >> 8) | ((color & 0x00FF) << 8);
}

uint16_t TFTDisplay::getColor() const {
    return current_color;
} 