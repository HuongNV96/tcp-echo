#ifndef _TCP_SERVER_
#define _TCP_SERVER_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <chrono>

#include "circ_buf.h"
#include "Message.hpp"

inline static constexpr size_t BUFFER_SIZE = 4096;
inline static constexpr size_t MAX_CLIENT = 5;
using namespace std::chrono_literals;

class TCPServer {
private :
    int port_;
    std::string host_;
    int server_socket_;
    std::atomic_bool exit_ {false};
    void run();
    std::thread run_thread_;
    
    class Client
    {
    private :
        circ_buf_t tx;
        circ_buf_t rx;
        std::atomic_bool exit_ {false};
        int client_;
        std::string user_, pass_;

        void run();
        std::thread run_thread_;
    public :
        Client(const int client);
        Client() {}
        ~Client();
        void sendRequest(Message  * message);

        friend class TCPServer;
    };

    std::mutex lock_;
    std::unordered_map<int, Client> clients_;

public :
    TCPServer(int port, std::string host) : port_(port), host_(host) {};
    ~TCPServer();
    bool init();
};


#endif