#include "cmac.h"
#include "kuznechik.h"

namespace cmac {

// Сдвиг влево на 1 бит для 128-битного блока
void leftShift(uint8_t* data) {
    uint8_t carry = 0;
    for (int i = BLOCK_SIZE - 1; i >= 0; i--) {
        uint8_t next_carry = (data[i] & 0x80) ? 1 : 0;
        data[i] = (data[i] << 1) | carry;
        carry = next_carry;
    }
}

// XOR двух 128-битных блоков
void xorBlocks(uint8_t* dst, const uint8_t* src) {
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        dst[i] ^= src[i];
    }
}

// Генерация подключей K1 и K2 для CMAC
CMACKeys generateSubkeys(const uint8_t* key) {
    CMACKeys subkeys;
    uint8_t L[BLOCK_SIZE] = {0};
    uint8_t Z[BLOCK_SIZE] = {0};
    uint8_t temp_key[160];  // Временный буфер для копии ключа
    
    // Копируем ключ во временный буфер
    std::memcpy(temp_key, key, 160);
    
    // L = CIPH_K(0^n)
    kuznechik_encrypt(Z, L, temp_key);
    
    // K1 = CIPH_K(0^n)
    std::memcpy(subkeys.K1, L, BLOCK_SIZE);
    leftShift(subkeys.K1);
    if (L[0] & 0x80) {
        subkeys.K1[BLOCK_SIZE - 1] ^= R;
    }
    
    // K2 = CIPH_K(K1)
    std::memcpy(subkeys.K2, subkeys.K1, BLOCK_SIZE);
    leftShift(subkeys.K2);
    if (subkeys.K1[0] & 0x80) {
        subkeys.K2[BLOCK_SIZE - 1] ^= R;
    }
    
    return subkeys;
}

// вычисление CMAC
std::vector<uint8_t> calculateCMAC(const std::vector<char>& data, 
                                 const uint8_t* key, 
                                 const uint8_t* roundKeys) {
    std::vector<uint8_t> mac(BLOCK_SIZE, 0);
    uint8_t temp_keys[160]; 
    
    // раундовые ключи
    std::memcpy(temp_keys, roundKeys, 160);
    
    CMACKeys subkeys = generateSubkeys(roundKeys);
    
    // количество полных блоков
    size_t n = (data.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
    bool is_complete = (data.size() % BLOCK_SIZE == 0);
    
    // все блоки кроме последнего
    uint8_t block[BLOCK_SIZE] = {0};
    for (size_t i = 0; i < n - 1; i++) {
        std::memcpy(block, &data[i * BLOCK_SIZE], BLOCK_SIZE);
        xorBlocks(mac.data(), block);
        kuznechik_encrypt(mac.data(), mac.data(), temp_keys);
    }
    
    // последний блок
    size_t last_block_size = data.size() - (n - 1) * BLOCK_SIZE;
    std::memset(block, 0, BLOCK_SIZE);
    std::memcpy(block, &data[(n - 1) * BLOCK_SIZE], last_block_size);
    
    if (!is_complete) {
        // padding
        block[last_block_size] = 0x80;
        xorBlocks(block, subkeys.K2);
    } else {
        xorBlocks(block, subkeys.K1);
    }
    
    xorBlocks(mac.data(), block);
    kuznechik_encrypt(mac.data(), mac.data(), temp_keys);
    
    return mac;
}

} // namespace cmac 