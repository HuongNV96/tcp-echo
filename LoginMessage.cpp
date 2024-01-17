#include "LoginMessage.hpp"
#include <string.h>
#include <iostream>

LoginMessageRequest::LoginMessageRequest() {
    message_size_ = sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(username_) + sizeof(password_);
    message_type_ = 0;
    message_sequence_ = 0;

    memset((void *)username_, 0, 32);
    memset((void *)password_, 0, 32);
}

LoginMessageRequest::LoginMessageRequest(std::string username, std::string password) : LoginMessageRequest() {
    memcpy(username_, username.c_str(), username.length() > 32 ? 32 : username.length());
    memcpy(password_, password.c_str(), password.length() > 32 ? 32 : password.length());
}

size_t LoginMessageRequest::size() {
    return (message_size_ + 4);
}
int8_t * LoginMessageRequest::serialize() {
    int8_t * ret = (int8_t *)malloc(message_size_ + 4);//1 byte for start frame, 1 byte for stop frame
    ret[0] = 0xFF;
    ret[1] = 0xEE;
    ret[2] = 0xDD;
    ret[3] = 0xCC;

    memcpy(ret + 4,&message_size_, sizeof(message_size_));
    memcpy(ret + 4 + sizeof(message_size_),&message_type_, sizeof(message_type_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_),&message_sequence_, sizeof(message_sequence_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), username_, sizeof(username_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(username_),password_, sizeof(password_));
    return ret;
}


void LoginMessageRequest::deserialize(const int8_t * data, const size_t len) {
    if (len != size()) {
        std::cerr << "Len is missing : "<< size() << std::endl;
        return;
    }
    memcpy(&message_size_, data + 4, sizeof(message_size_));
    memcpy(&message_type_, data + 4 + sizeof(message_size_), sizeof(message_type_));
    memcpy(&message_sequence_, data + 4 + sizeof(message_size_) + sizeof(message_type_), sizeof(message_sequence_));
    memcpy(username_, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), sizeof(username_));
    memcpy(password_, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(username_), sizeof(password_));
}

LoginMessageResponse::LoginMessageResponse() {
    message_size_ = sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_) + sizeof(status_code_);
    message_type_ = 1;
    message_sequence_ = 0;
}

LoginMessageResponse::LoginMessageResponse(const uint16_t status_code) : LoginMessageResponse() {
    status_code_ = status_code;
}

size_t LoginMessageResponse::size() {
    return (message_size_ + 4);
}

int8_t * LoginMessageResponse::serialize() {
    int8_t * ret = (int8_t *)malloc(message_size_ + 4);//1 byte for start frame, 1 byte for stop frame
    ret[0] = 0xFF;
    ret[1] = 0xEE;
    ret[2] = 0xDD;
    ret[3] = 0xCC;

    memcpy(ret + 4,&message_size_, sizeof(message_size_));
    memcpy(ret + 4 + sizeof(message_size_),&message_type_, sizeof(message_type_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_),&message_sequence_, sizeof(message_sequence_));
    memcpy(ret + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), &status_code_, sizeof(status_code_));
    return ret;
}

void LoginMessageResponse::deserialize(const int8_t * data, const size_t len) {
    if (len != size()) {
        std::cerr << "Len is missing : "<< size() << std::endl;
        return;
    }
    memcpy(&message_size_, data + 4, sizeof(message_size_));
    memcpy(&message_type_, data + 4 + sizeof(message_size_), sizeof(message_type_));
    memcpy(&message_sequence_, data + 4 + sizeof(message_size_) + sizeof(message_type_), sizeof(message_sequence_));
    memcpy(&status_code_, data + 4 + sizeof(message_size_) + sizeof(message_type_) + sizeof(message_sequence_), sizeof(status_code_));
}