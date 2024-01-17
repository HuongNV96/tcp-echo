#include <iostream>
#include <string.h>
#include <string>

#include "EchoMessage.hpp"

static uint32_t next_key(uint32_t key)
{
    return (key*1103515245 + 12345) % 0x7FFFFFFF;
}

EchoMessageRequest::EchoMessageRequest(const std::string user, const std::string pass) {
    user_ = user;
    pass_ = pass; 
}

EchoMessageRequest::EchoMessageRequest(const uint8_t message_sequence, const std::string user, const std::string pass, std::string msg) {
    message_type_ = 2;
    message_sequence_ = message_sequence;
    user_ = user;
    pass_ = pass;
    msg_ = msg;

    uint32_t key = initialKey(message_sequence_);

    for (int i = 0; i < sizeof(cipher_key_); i++) {
        key = next_key(key);
        cipher_key_[i] = key % 256;
    }
}
uint8_t * EchoMessageRequest::encryption(uint16_t &len) {
    len = ((msg_.length() / 64) + 1) * 64;

    uint8_t * out = (uint8_t *)malloc(len);
    for (int i = 0; i < len; i++) {
        if (i < msg_.length()) {
            out[i] = msg_[i] ^ cipher_key_[i % 64 ];
        } else {
            out[i] = 0 ^ cipher_key_[i % 64];
        }
    }
    return out;
}

void EchoMessageRequest::decryption(uint8_t * buf, const uint16_t len) {
    if (len % 64) {
        std::cerr << "Len is missing : "<< len << std::endl;
    }
    for (int i = 0; i < len; i++) {
        buf[i] = buf[i] ^ cipher_key_[i % 64 ];
    }
}


int8_t * EchoMessageRequest::serialize() {
    uint8_t * buf = encryption(msg_len_);

    message_size_ = sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_) + msg_len_;
    int8_t * ret = (int8_t *)malloc(message_size_ + 4);//1 byte for start frame, 1 byte for stop frame
    ret[0] = 0xFF;
    ret[1] = 0xEE;
    ret[2] = 0xDD;
    ret[3] = 0xCC;

    memcpy(ret + 4,&message_size_, sizeof(message_size_));
    memcpy(ret + 4 + sizeof(message_size_),&message_type_, sizeof(message_type_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_),&message_sequence_, sizeof(message_sequence_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), &msg_len_, sizeof(msg_len_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_),buf, msg_len_);

    free(buf);
    return ret;
}

size_t EchoMessageRequest::size() {
    return (message_size_ + 4);
}

std::vector<uint8_t> EchoMessageRequest::cipherMessage(int8_t * data, const uint16_t len) {
    if (len <= 10) {
        std::cerr << "Len is missing : "<< len << std::endl;
        return std::vector<uint8_t>();
    }

    memcpy(&message_size_, data + 4, sizeof(message_size_));
    memcpy(&message_type_, data + 4 + sizeof(message_size_), sizeof(message_type_));
    memcpy(&message_sequence_, data + 4 + sizeof(message_size_) + sizeof(message_type_), sizeof(message_sequence_));
    memcpy(&msg_len_, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), sizeof(msg_len_));

    uint8_t * buf = (uint8_t*)malloc(msg_len_);

    memcpy(buf, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_), msg_len_);

    std::vector<uint8_t> ret(buf, buf + msg_len_);
    free(buf);

    return std::move(ret);
}

void EchoMessageRequest::deserialize(const int8_t * data, const size_t len) {
    if (len <= 10) {
        std::cerr << "Len is missing : "<< len << std::endl;
        return;
    }

    memcpy(&message_size_, data + 4, sizeof(message_size_));
    memcpy(&message_type_, data + 4 + sizeof(message_size_), sizeof(message_type_));
    memcpy(&message_sequence_, data + 4 + sizeof(message_size_) + sizeof(message_type_), sizeof(message_sequence_));
    memcpy(&msg_len_, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), sizeof(msg_len_));

    uint8_t * buf = (uint8_t*)malloc(msg_len_);

    memcpy(buf, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_), msg_len_);

    uint32_t key = initialKey(message_sequence_);

    for (int i = 0; i < sizeof(cipher_key_); i++) {
        key = next_key(key);
        cipher_key_[i] = key % 256;
    }

    decryption(buf, msg_len_);

    msg_ = std::string((char*)buf);
    free(buf);
}

EchoMessageResponse::EchoMessageResponse(const uint8_t message_sequence, std::vector<uint8_t> &cipher_message) {
    message_type_ = 3;
    message_sequence_ = message_sequence;
    cipher_message_ = std::move(cipher_message);

    msg_len_ = cipher_message_.size();
    message_size_ = sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_) + msg_len_;
}

EchoMessageResponse::EchoMessageResponse(const uint8_t message_sequence, std::string msg) {
    message_type_ = 3;
    msg_ = msg;
    msg_len_ = msg.length();
    message_size_ = sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_) + msg_len_;
}

int8_t * EchoMessageResponse::serialize() {
    int8_t * ret = (int8_t *)malloc(message_size_ + 4);//1 byte for start frame, 1 byte for stop frame
    ret[0] = 0xFF;
    ret[1] = 0xEE;
    ret[2] = 0xDD;
    ret[3] = 0xCC;

    memcpy(ret + 4,&message_size_, sizeof(message_size_));
    memcpy(ret + 4 + sizeof(message_size_),&message_type_, sizeof(message_type_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_),&message_sequence_, sizeof(message_sequence_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), &msg_len_, sizeof(msg_len_));

    if (!cipher_message_.empty()) {
        memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_), cipher_message_.data(), msg_len_);
    } else {
        memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_), msg_.c_str(), msg_len_);
    }

    return ret;
}

size_t EchoMessageResponse::size() {
    return (message_size_ + 4);
}

void EchoMessageResponse::deserialize(const int8_t * data, const size_t len) {
    if (len <= 10) {
        std::cerr << "Len is missing : "<< len << std::endl;
        return;
    }

    memcpy(&message_size_, data + 4, sizeof(message_size_));
    memcpy(&message_type_, data + 4 + sizeof(message_size_), sizeof(message_type_));
    memcpy(&message_sequence_, data + 4 + sizeof(message_size_) + sizeof(message_type_), sizeof(message_sequence_));
    memcpy(&msg_len_, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), sizeof(msg_len_));


    uint8_t * buf = (uint8_t*)malloc(msg_len_);

    memcpy(buf, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(msg_len_), msg_len_);
    msg_ = std::string((char*)buf);
    free(buf);
}
