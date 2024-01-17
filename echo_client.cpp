#include "TcpClient.hpp"
#include "LoginMessage.hpp"
#include "EchoMessage.hpp"

int main() {
    std::atomic_bool login_success = false;
    std::atomic_bool login_done_status = false;
    std::string user, pass;
    TCPClient client(12345, "127.0.0.1",[&login_success, &login_done_status](Message * msg){
        if (msg->messageType() == 1) {
            LoginMessageResponse * response = static_cast<LoginMessageResponse *>(msg);
            //std::cout << response->statusCode() << std::endl;

            login_success = response->statusCode();
            login_done_status = true;
        }
    });
    client.init();
    
    user = "string111";
    pass = "string111";
    client.login(user, pass);
    while(!login_done_status) {std::this_thread::sleep_for(std::chrono::seconds(1));}

    if (!login_success) {
        std::cout << "login failure, will login again" << std::endl;
        login_done_status = false;
        user = "string";
        pass = "string";
        client.login(user, pass);
        while(!login_done_status) {std::this_thread::sleep_for(std::chrono::seconds(1));}
    }

    if (!login_success) {
        std::cerr << "login failure" << std::endl;
        exit(0);
    }

    std::cout << "login is ok" << std::endl;
    for (int i = 0; i < 100; i++) {
        EchoMessageRequest request(i, user, pass, "Helloworld" + std::to_string(i));
        client.sendRequest(&request);
        std::this_thread::sleep_for(1ms);
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
