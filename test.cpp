#include <iostream>
#include <unordered_map>
#include <string>
#include <string.h>

class MyObject {
public:
    MyObject(const std::string value) : value_(value) {}

    // ... other member functions or data members

private:
    std::string value_;
};

uint8_t calculateChecksum(const std::string& username, uint8_t counter) {
    uint8_t sum = 0;

    // Sum the ASCII values of each character in the username
    for (char ch : username) {
        sum += static_cast<uint8_t>(ch);
    }

    // Add the counter value to the sum
    sum += counter;

    // Calculate the complement (bitwise NOT) of the sum
    uint8_t checksum = ~sum;

    return checksum;
}

uint32_t next_key(uint32_t key)
{
    return (key*1103515245 + 12345) % 0x7FFFFFFF;
}

int main() {
    char test[30] = "testuser";
    uint8_t sum = 0;
    for (int i = 0; i < 30; i++) {
        sum += test[i];
    }
    std::cout << (int)sum << std::endl;

    uint32_t initial_key = (87 << 16) | (sum << 8) | sum;
    uint32_t key;
    uint8_t cipher_key[64];

    for (int i = 0; i < 64; i++) {
        if (i == 0) {
            key = next_key(initial_key);
        } else {
            key = next_key(key);
        }
        cipher_key[i] = key % 256;
        std::cout << std::hex << (int)cipher_key[i] << " ";
    }

    const char* text = "helllo DKM";
    uint16_t buf_szie = ((strlen(text) / 64) + 1) * 64;
    char in_out[buf_szie];
    for (int i = 0; i < buf_szie; i++) {
        if (i < strlen(text)) {
            in_out[i] = text[i] ^ cipher_key[i % 64 ];
        } else {
            in_out[i] = 0 ^ cipher_key[i % 64];
        }
    }

    for (int i = 0; i < buf_szie; i++) {
        in_out[i] = in_out[i] ^ cipher_key[i % 64 ];
    }

    std::cout << in_out << std::endl;
}
