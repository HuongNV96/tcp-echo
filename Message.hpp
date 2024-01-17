#ifndef _MESSAGE_HPP_
#define _MESSAGE_HPP_

#include <stdint.h>
#include <stdlib.h>

class Message {
protected :
    uint16_t message_size_;
    uint8_t message_type_;
    uint8_t message_sequence_;
public :
    virtual int8_t * serialize() = 0;
    virtual void deserialize(const int8_t * data, const size_t len) = 0;
    virtual size_t size() = 0;
    const uint8_t messageType() const noexcept {
        return message_type_;
    }
    const uint8_t messageSequence() const noexcept {
        return message_sequence_;
    }
};

#endif