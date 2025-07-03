#include "counter_mode.h"
#include <random>
#include <chrono>

namespace counter_mode {

std::vector<uint8_t> generate_iv() {
    std::vector<uint8_t> iv(IV_SIZE);
    
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    
    for(size_t i = 0; i < IV_SIZE - 8; ++i) {
        iv[i] = static_cast<uint8_t>(dis(gen));
    }
    
    
    for(size_t i = 0; i < 8; ++i) {
        iv[IV_SIZE - 1 - i] = static_cast<uint8_t>(timestamp & 0xFF);
        timestamp >>= 8;
    }
    
    return iv;
}

void generate_gamma(uint8_t* gamma, Counter& ctr, const uint8_t* roundKeys) {
    kuznechik_encrypt(ctr.value, gamma, const_cast<uint8_t*>(roundKeys));
    ctr.increment();
}

void apply_gamma(uint8_t* data, const uint8_t* gamma, size_t length) {
    for(size_t i = 0; i < length; ++i) {
        data[i] ^= gamma[i];
    }
}

}