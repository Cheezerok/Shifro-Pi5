#include "spi_pi.h"
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


#define DC_PIN 24
#define RST_PIN 25

SPIDevice::SPIDevice(int channel, uint32_t speed) 
    : spi_channel(channel), spi_speed(speed), spi_fd(-1),
      chip_name("gpiochip0"), chip(nullptr), 
      dc_line(nullptr), rst_line(nullptr),
      dc_pin(DC_PIN), rst_pin(RST_PIN) {
}

SPIDevice::~SPIDevice() {
    
    
    if (dc_line) {
        gpiod_line_release(dc_line);
    }
    if (rst_line) {
        gpiod_line_release(rst_line);
    }
    
    
    if (chip) {
        gpiod_chip_close(chip);
    }
    
   
    if (spi_fd >= 0) {
        close(spi_fd);
    }
}

bool SPIDevice::init() {
    
    const int max_retries = 3;
    int retry_count = 0;
    
    while (retry_count < max_retries) {
        
        chip = gpiod_chip_open_by_name(chip_name);
        if (!chip) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
            continue;
        }
        
       
        dc_line = gpiod_chip_get_line(chip, dc_pin);
        rst_line = gpiod_chip_get_line(chip, rst_pin);
        
        if (!dc_line || !rst_line) {
            gpiod_chip_close(chip);
            chip = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
            continue;
        }
        
        
        if (gpiod_line_request_output(dc_line, "spi_dc", 0) < 0 ||
            gpiod_line_request_output(rst_line, "spi_rst", 1) < 0) {
            gpiod_chip_close(chip);
            chip = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
            continue;
        }
        
        
        char spi_device[32];
        snprintf(spi_device, sizeof(spi_device), "/dev/spidev0.%d", spi_channel);
        spi_fd = open(spi_device, O_RDWR);
        
        if (spi_fd < 0) {
            gpiod_chip_close(chip);
            chip = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
            continue;
        }
        
       
        uint8_t mode = SPI_MODE_0;
        uint8_t bits = 8;
        uint32_t conservative_speed = 4000000;  
        
        if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
            ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
            ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &conservative_speed) < 0) {
            close(spi_fd);
            spi_fd = -1;
            gpiod_chip_close(chip);
            chip = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
            continue;
        }
        
        
        return true;
    }
    
    return false;
}

void SPIDevice::write(uint8_t* data, size_t length) {
    if (spi_fd < 0) {
        throw std::runtime_error("SPI device not initialized");
    }
    
    const int max_retries = 3;
    int retry_count = 0;
    
    while (retry_count < max_retries) {
        struct spi_ioc_transfer tr = {};  
        tr.tx_buf = (unsigned long)data;
        tr.rx_buf = 0;
        tr.len = static_cast<__u32>(length);
        tr.speed_hz = spi_speed;
        tr.delay_usecs = 1; 
        tr.bits_per_word = 8;
        tr.cs_change = 0;    
        
        if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) >= 0) {
            // Successful transfer
            std::this_thread::sleep_for(std::chrono::microseconds(10));  
            return;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));  
        retry_count++;
    }
    
    throw std::runtime_error("SPI write failed after all retries");
}

void SPIDevice::setDC(bool state) {
    int retries = 3;
    while (retries > 0) {
        if (gpiod_line_set_value(dc_line, state ? 1 : 0) == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        retries--;
    }
}

void SPIDevice::setRST(bool state) {
    int retries = 3;
    while (retries > 0) {
        if (gpiod_line_set_value(rst_line, state ? 1 : 0) == 0) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        retries--;
    }
}

void SPIDevice::delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void SPIDevice::waitForTransferComplete() {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
} 
