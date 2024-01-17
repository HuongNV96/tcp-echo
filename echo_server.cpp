#include <csignal>

#include "TcpServer.hpp"

std::atomic<bool> keepRunning(true);

void handleSignal(int signum) {
    keepRunning = false;
}

int main() {
    //signal(SIGINT, handleSignal);

    TCPServer server(12345, "127.0.0.1");
    server.init();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
