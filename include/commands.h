#pragma once

#include <cstdint>

// команды дисплея
constexpr uint8_t CMD_NOP         = 0x00;
constexpr uint8_t CMD_SWRESET     = 0x01;
constexpr uint8_t CMD_RDDID       = 0x04;
constexpr uint8_t CMD_RDDST       = 0x09;
constexpr uint8_t CMD_SLPIN       = 0x10;
constexpr uint8_t CMD_SLPOUT      = 0x11;
constexpr uint8_t CMD_PTLON       = 0x12;
constexpr uint8_t CMD_NORON       = 0x13;
constexpr uint8_t CMD_INVOFF      = 0x20;
constexpr uint8_t CMD_INVON       = 0x21;
constexpr uint8_t CMD_DISPOFF     = 0x28;
constexpr uint8_t CMD_DISPON      = 0x29;
constexpr uint8_t CMD_CASET       = 0x2A;
constexpr uint8_t CMD_RASET       = 0x2B;
constexpr uint8_t CMD_RAMWR       = 0x2C;
constexpr uint8_t CMD_RAMRD       = 0x2E;
constexpr uint8_t CMD_PTLAR       = 0x30;
constexpr uint8_t CMD_COLMOD      = 0x3A;
constexpr uint8_t CMD_MADCTL      = 0x36;
constexpr uint8_t CMD_FRMCTR1     = 0xB1;
constexpr uint8_t CMD_FRMCTR2     = 0xB2;
constexpr uint8_t CMD_FRMCTR3     = 0xB3;
constexpr uint8_t CMD_INVCTR      = 0xB4;
constexpr uint8_t CMD_DISSET5     = 0xB6;
constexpr uint8_t CMD_PWCTR1      = 0xC0;
constexpr uint8_t CMD_PWCTR2      = 0xC1;
constexpr uint8_t CMD_PWCTR3      = 0xC2;
constexpr uint8_t CMD_PWCTR4      = 0xC3;
constexpr uint8_t CMD_PWCTR5      = 0xC4;
constexpr uint8_t CMD_VMCTR1      = 0xC5;
constexpr uint8_t CMD_VMOFCTR     = 0xC7;
constexpr uint8_t CMD_WRID2       = 0xD1;
constexpr uint8_t CMD_WRID3       = 0xD2;
constexpr uint8_t CMD_NVCTR1      = 0xD9;
constexpr uint8_t CMD_NVCTR2      = 0xDE;
constexpr uint8_t CMD_NVCTR3      = 0xDF;
constexpr uint8_t CMD_GAMCTRP1    = 0xE0;
constexpr uint8_t CMD_GAMCTRN1    = 0xE1;

// команды для рисования
constexpr uint8_t CMD_CLEAR_SCREEN = 0x40;
constexpr uint8_t CMD_DRAW_PIXEL   = 0x41;
constexpr uint8_t CMD_DRAW_LINE    = 0x42;
constexpr uint8_t CMD_DRAW_RECT    = 0x43;
constexpr uint8_t CMD_FILL_RECT    = 0x44;
constexpr uint8_t CMD_DRAW_CIRCLE  = 0x45;
constexpr uint8_t CMD_FILL_CIRCLE  = 0x46;
constexpr uint8_t CMD_DRAW_TEXT    = 0x47;
constexpr uint8_t CMD_DRAW_IMAGE   = 0x48;
constexpr uint8_t CMD_SET_ROTATION = 0x49;
constexpr uint8_t CMD_SET_FONT     = 0x4A;

// ST7735 Commands
#define CMD_GAMSET    0x26  // Gamma Set
#define CMD_VSCRDEF   0x33  // Vertical Scrolling Definition
#define CMD_TEOFF     0x34  // Tearing Effect Line OFF
#define CMD_TEON      0x35  // Tearing Effect Line ON
#define CMD_VSCRSADD  0x37  // Vertical Scrolling Start Address
#define CMD_IDMOFF    0x38  // Idle Mode Off
#define CMD_IDMON     0x39  // Idle Mode On
#define CMD_RAMWRC    0x3C  // Memory Write Continue
#define CMD_RAMRDC    0x3E  // Memory Read Continue
#define CMD_TESCAN    0x44  // Set Tear Scanline
#define CMD_RDTESCAN  0x45  // Get Tear Scanline
#define CMD_WRDISBV   0x51  // Write Display Brightness
#define CMD_RDDISBV   0x52  // Read Display Brightness Value
#define CMD_WRCTRLD   0x53  // Write CTRL Display
#define CMD_RDCTRLD   0x54  // Read CTRL Display
#define CMD_WRCACE    0x55  // Write Content Adaptive Brightness Control and Color Enhancement
#define CMD_RDCABC    0x56  // Read Content Adaptive Brightness Control
#define CMD_WRCABCMB  0x5E  // Write CABC Minimum Brightness
#define CMD_RDCABCMB  0x5F  // Read CABC Minimum Brightness
#define CMD_RDID1     0xDA  // Read ID1
#define CMD_RDID2     0xDB  // Read ID2
#define CMD_RDID3     0xDC  // Read ID3

// MADCTL bits
#define MADCTL_MY     0x80  // Row Address Order
#define MADCTL_MX     0x40  // Column Address Order
#define MADCTL_MV     0x20  // Row/Column Exchange
#define MADCTL_ML     0x10  // Vertical Refresh Order
#define MADCTL_RGB    0x00  // RGB Order
#define MADCTL_BGR    0x08  // BGR Order
#define MADCTL_MH     0x04  // Horizontal Refresh Order 