#pragma once

#include <cstdint>

// RGB565 color definitions
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_PURPLE  0x8010
#define COLOR_PINK    0xFE19
#define COLOR_BROWN   0xA145
#define COLOR_GRAY    0x8410

// Color conversion macros
#define RGB565_CONVERT(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
#define RGB888_TO_RGB565(color) RGB565_CONVERT((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF)
#define RGB565_TO_RGB888(color) ((((color >> 11) & 0x1F) << 19) | (((color >> 5) & 0x3F) << 10) | ((color & 0x1F) << 3))

// Helper function for creating RGB565 color
constexpr uint16_t make_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
} 