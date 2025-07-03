#pragma once

#include <cstdint>
#include <string>
#include <gpiod.h>
#include <linux/spi/spidev.h>
#include <memory>

class SPIDevice {
private:
    int spi_fd;                    // File descriptor для SPI
    int spi_channel;
    uint32_t spi_speed;            // Изменил тип на uint32_t для совместимости с SPI
    const char* chip_name;         // Имя GPIO чипа (обычно "gpiochip0")
    struct gpiod_chip *chip;       // GPIO чип
    struct gpiod_line *dc_line;    // Линия DC
    struct gpiod_line *rst_line;   // Линия RST
    int dc_pin;                    // Номер пина DC
    int rst_pin;                   // Номер пина RST
    
public:
    SPIDevice(int channel = 0, uint32_t speed = 8000000);
    ~SPIDevice();
    
    bool init();
    void write(uint8_t* data, size_t length);
    void setDC(bool state);
    void setRST(bool state);
    void delay(uint32_t ms);
    void waitForTransferComplete();
}; 