#ifndef _LOGIN_MESSAGE_HPP_
#define _LOGIN_MESSAGE_HPP_

#include "Message.hpp"
#include <string>

class LoginMessageRequest : public Message
{
private :
    uint8_t username_[32];
    uint8_t password_[32];
public :
    LoginMessageRequest();
    LoginMessageRequest(std::string username, std::string password);
    size_t size() override;
    int8_t * serialize() override;
    void deserialize(const int8_t * data, const size_t len) override;
    
    const std::string username() const {
        return std::string((char *)username_);
    } 
    const std::string password() const {
        return std::string((char *)password_);
    }
};

class LoginMessageResponse: public Message
{
private :
    uint16_t status_code_;
public :
    LoginMessageResponse(const uint16_t status_code);
    LoginMessageResponse();
    size_t size() override;
    virtual int8_t * serialize() override;
    virtual void deserialize(const int8_t * data, const size_t len) override;
    const uint16_t statusCode() const {
        return status_code_;
    }
};

#endif