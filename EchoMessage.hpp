#ifndef _ECHO_MESSAGE_H
#define _ECHO_MESSAGE_H

#include <string>
#include <vector>

#include "Message.hpp"

class EchoMessageRequest : public Message {
private :
    uint32_t initial_key_;
    uint8_t cipher_key_[64];
    std::string msg_;
    uint16_t msg_len_;
    std::string user_, pass_;

    const uint8_t sumUserName() const noexcept{
        uint8_t sum = 0;
        for (int i = 0; i <  user_.length(); i++) {
            sum += user_[i];
        }
        return sum;
    }

    const uint8_t sumPassword() const noexcept{
        uint8_t sum = 0;
        for (int i = 0; i < pass_.length(); i++) {
            sum += pass_[i];
        }
        return sum;
    }

    const uint32_t initialKey(uint16_t message_sequence) const noexcept {
        return (message_sequence << 16) | (sumUserName() << 8) | sumPassword();
    }

public :
    EchoMessageRequest() = default;
    EchoMessageRequest(const uint8_t message_sequence, const std::string user, const std::string pass, std::string msg);
    EchoMessageRequest(const std::string user, const std::string pass);
    
    size_t size() override;
    int8_t * serialize() override;
    void decryption(uint8_t * buf, const uint16_t len);

    uint8_t * encryption(uint16_t &len);

    void deserialize(const int8_t * data, const size_t len) override;
    std::vector<uint8_t> cipherMessage(int8_t * data, const uint16_t len);

    const std::string msg() const noexcept {
        return msg_;
    }

};

class EchoMessageResponse : public Message {
private :
    std::string msg_;
    uint16_t msg_len_;
    std::vector<uint8_t> cipher_message_;

public :
    EchoMessageResponse() = default;
    EchoMessageResponse(const uint8_t message_sequence, std::vector<uint8_t> & cipher_message);
    EchoMessageResponse(const uint8_t message_sequence, std::string msg);
    size_t size() override;
    int8_t * serialize() override;
    void deserialize(const int8_t * data, const size_t len) override;
    const std::string msg() const noexcept {
        return msg_;
    }
};

#endif