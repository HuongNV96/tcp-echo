#ifndef _TCP_CLIENT_
#define _TCP_CLIENT_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <functional>

#include "circ_buf.h"
#include "Message.hpp"

inline static constexpr size_t BUFFER_SIZE = 4096;
using namespace std::chrono_literals;

class TCPClient {
private :
    int port_;
    std::string host_;
    int client_socket_;
    std::atomic_bool exit_ {false};
    sockaddr_in server_address_;

    std::thread run_thread_;
    std::thread recv_thread_;
    circ_buf_t tx;
    circ_buf_t rx;

    void run();
    void receive();
    ssize_t sendBlock(void * data, size_t len);
    ssize_t recvBlock(void * data, size_t len);
    std::function<void(Message * msg)> callback_;
public :
    TCPClient(int port, std::string host, std::function<void(Message * msg)> callback);
    ~TCPClient();
    void init();
    void login(std::string username, std::string password);
    void sendRequest(Message  * message);
    bool reconnect();
};

#endif