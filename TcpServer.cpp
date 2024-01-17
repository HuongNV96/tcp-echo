#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "TcpServer.hpp"
#include "Message.hpp"
#include "LoginMessage.hpp"
#include "EchoMessage.hpp"

TCPServer::Client::Client(const int client) {
    client_ = client;
    //init buf
    rx.init = init_circ_buf;
    rx.del = del_circ_buf;
    rx.write_bytes = write_bytes_to_circ_buf;
    rx.write_byte = write_byte_to_circ_buf;
    rx.read_bytes = read_bytes_from_circ_buf;
    rx.read_byte = read_byte_form_circ_buf;
    rx.size = circ_buf_num_items;
    rx.lock = lock_circ_buf;
    rx.unlock = unlock_circ_buf;
    if (!rx.init(&rx,4 * BUFFER_SIZE)) {
        std::cerr << "init rx buf fail" << std::endl;
    }

    tx.init = init_circ_buf;
    tx.del = del_circ_buf;
    tx.write_bytes = write_bytes_to_circ_buf;
    tx.write_byte = write_byte_to_circ_buf;
    tx.read_bytes = read_bytes_from_circ_buf;
    tx.read_byte = read_byte_form_circ_buf;
    tx.size = circ_buf_num_items;
    tx.lock = lock_circ_buf;
    tx.unlock = unlock_circ_buf;
    if (!tx.init(&tx,4 * BUFFER_SIZE)) {
        std::cerr << "init tx buf fail" << std::endl;
    }

    run_thread_ = std::thread(&TCPServer::Client::run, this);
}

TCPServer::Client::~Client() {
    exit_ = true;
    run_thread_.join();
    close(client_);
    
    del_circ_buf(&tx);
    del_circ_buf(&rx);
}

TCPServer::~TCPServer() {
    exit_ = true;
    run_thread_.join();
    close(server_socket_);
}

bool TCPServer::init() {
    // Create server socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    //config socket 
    socklen_t reuse = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
        return false;
    }
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt(SO_REUSEPORT) failed" << std::endl;
        return false;
    }
    struct timeval timeout;      
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof(timeout)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return false;
    }

    if (setsockopt(server_socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof(timeout)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return false;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(host_.c_str());
    server_address.sin_port = htons(port_);

    // Bind the socket
    if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        close(server_socket_);
        return false;
    }

    // Listen for incoming connections
    if (listen(server_socket_, MAX_CLIENT) == -1) {
        std::cerr << "Error listening for connections" << std::endl;
        close(server_socket_);
        return false;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    run_thread_ = std::thread(&TCPServer::run, this);

    return true;
}
void TCPServer::run() {
    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(server_socket_, &master);
    int fd_max = server_socket_;

    while (!exit_) {
        read_fds = master;  // Copy the master set before calling select

        if (select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
            std::cerr << "Error in select" << std::endl;
            close(server_socket_);
            return;
        }

        // Iterate through the existing connections looking for data to read
        for (int i = 0; i <= fd_max; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket_) {
                    // Accept a new connection
                    sockaddr_in client_address{};
                    socklen_t client_len = sizeof(client_address);
                    int client_socket = accept(server_socket_, reinterpret_cast<struct sockaddr*>(&client_address), &client_len);

                    if (client_socket == -1) {
                        std::cerr << "Error accepting connection" << std::endl;
                    } else {
                        FD_SET(client_socket, &master);  // Add to master set
                        {
                            std::lock_guard<std::mutex> lock(lock_);
                            clients_.try_emplace(client_socket, client_socket);

                        }
                        if (client_socket > fd_max) {
                            fd_max = client_socket;
                        }

                        std::cout << "New connection from " << inet_ntoa(client_address.sin_addr) << std::endl;
                    }
                } else {
                    // Receive data from a client
                    char buffer[BUFFER_SIZE];
                    ssize_t bytes_received = recv(i, buffer, sizeof(buffer), 0);
                    if (bytes_received <= 0) {
                        if (bytes_received == 0) {
                            // Connection closed by client
                            std::cout << "Connection closed by client on socket " << i << std::endl;
                        } else {
                            std::cerr << "Error receiving data" << std::endl;
                        }

                        // Close the socket and remove from the master set
                        close(i);
                        FD_CLR(i, &master);
                        {
                            std::lock_guard<std::mutex> lock(lock_);
                            clients_.erase(i);
                        }
                    } else {
                        // Echo the received data back to the client
                        //send(i, buffer, static_cast<size_t>(bytes_received), 0);
                        std::lock_guard<std::mutex> lock(lock_);
                        clients_[i].rx.lock(&clients_[i].rx);
                        clients_[i].rx.write_bytes(&clients_[i].rx,buffer,bytes_received);
                        clients_[i].rx.unlock(&clients_[i].rx);
                    }
                }
            }
        }
        {
        std::lock_guard<std::mutex> lock(lock_);
            for (const auto& [client_id, client] : clients_) {
                if (clients_[client_id].exit_ == true) {
                    close(client_id);
                    FD_CLR(client_id, &master);
                    clients_.erase(client_id);
                }
            }
        }
    }
}

void TCPServer::Client::sendRequest(Message * message) {
    int8_t * data = message->serialize();
    tx.lock(&tx);
    tx.write_bytes(&tx,data,message->size());
    tx.unlock(&tx);
    free(data);
}

void TCPServer::Client::run() {
    int8_t buffer[BUFFER_SIZE] ; 
    int16_t index = 0,start_index = -1;
    uint16_t frame_len;
    uint8_t msg_type;

    while(!exit_) {
        bool active = false;
        //RX
        {
            rx.lock(&rx);
            if(rx.read_byte(&rx,&buffer[index])) {
                if (index >= 7) {
                    if ((uint8_t)buffer[index - 4] == 0xCC
                    && (uint8_t)buffer[index - 5] == 0xDD
                    && (uint8_t)buffer[index - 6] == 0xEE
                    && (uint8_t)buffer[index - 7] == 0xFF) {//start frame
                        frame_len = (uint8_t)buffer[index - 2] >> 8 | (uint8_t)buffer[index - 3];
                        msg_type = (uint8_t)buffer[index - 1];
                        if (start_index == -1) {
                            start_index = index - 7;
                        }
                    }
                } 

                index ++;
                if (index == BUFFER_SIZE) {
                    start_index = -1;
                    index = 0;
                }
                active = true;
            }
            rx.unlock(&rx);

            if (active) {
                if (start_index >= 0) {
                    //recive a freame, will call callback
                    rx.lock(&rx);
                    size_t aviable_size = rx.size(&rx);
                    if (aviable_size >= (frame_len - 4)) {
                        rx.read_bytes(&rx,buffer + index,frame_len - 4);
                        rx.unlock(&rx);

                        if (msg_type == 0) {
                            LoginMessageRequest request;
                            request.deserialize(buffer + start_index, frame_len + 4);
                            if (request.username() == "string" &&  request.password() == "string") {
                                user_ = request.username();
                                pass_ = request.password();
                                LoginMessageResponse response(1);
                                sendRequest(&response);
                            } else {
                                //close socket
                                LoginMessageResponse response(0);
                                sendRequest(&response);
                                exit_ = true;
                            }
                        } else if (msg_type == 2) {
                            EchoMessageRequest request(user_,pass_);
                            request.deserialize(buffer + start_index, frame_len + 4);
                            if (request.messageSequence() == 50) {
                                //omitting
                                auto cipher_essage = request.cipherMessage(buffer + start_index, frame_len + 4);
                                EchoMessageResponse response(request.messageSequence(),cipher_essage);

                                sendRequest(&response);
                            } else {
                                EchoMessageResponse response(request.messageSequence(), request.msg());
                                sendRequest(&response);
                            }
                        }

                        start_index = -1;
                        index = 0;
                    }
                    rx.unlock(&rx);
                }

            }
        }
        //TX
        {
            int8_t buffer[BUFFER_SIZE]; 
            tx.lock(&tx);
            size_t len = tx.read_bytes(&tx,buffer,BUFFER_SIZE);
            tx.unlock(&tx);
            if (len > 0) {
                active = true;
                send(client_, buffer , len, 0);
            }
        }
        if (!active) {
            std::this_thread::sleep_for(100ms);
        }
    }
    
}
