
#include "TcpClient.hpp"
#include "LoginMessage.hpp"
#include "EchoMessage.hpp"

TCPClient::~TCPClient() {
    exit_ = true;
    run_thread_.join();
    close(client_socket_);

    del_circ_buf(&tx);
    del_circ_buf(&rx);
}

TCPClient::TCPClient(int port, std::string host, std::function<void(Message * msg)> callback) : port_(port), host_(host), callback_(callback) {
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
}


bool TCPClient::reconnect() {
    // Create client socket
    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_ == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    //config socket 
    socklen_t reuse = 1;
    if (setsockopt(client_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
        return false;
    }
    if (setsockopt(client_socket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt(SO_REUSEPORT) failed" << std::endl;
        return false;
    }
    struct timeval timeout;      
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(client_socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof(timeout)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return false;
    }

    if (setsockopt(client_socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof(timeout)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return false;
    }
    
    server_address_.sin_family = AF_INET;
    server_address_.sin_port = htons(port_);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, host_.c_str(), &server_address_.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        close(client_socket_);
        return false;
    }

    // Connect to the server
    if (connect(client_socket_, reinterpret_cast<struct sockaddr*>(&server_address_), sizeof(server_address_)) == -1) {
        std::cerr << "Error connecting to server" << std::endl;
        close(client_socket_);
        return false;
    }
        std::cout << "Connected to server" << std::endl;
    return true;
}

void TCPClient::init() {
    reconnect();
    run_thread_ = std::thread(&TCPClient::run, this);
    recv_thread_ = std::thread(&TCPClient::receive, this);
}

ssize_t TCPClient::sendBlock(void * data, size_t len) {

    return send(client_socket_, data , len, 0); 
}

ssize_t TCPClient::recvBlock(void * data, size_t len) {
    return recv(client_socket_, data, len,  0); 
}

void TCPClient::receive() {
    while(!exit_) {
        int8_t buffer[BUFFER_SIZE]; 
        ssize_t len = recvBlock(buffer,BUFFER_SIZE);
        if (len > 0) {
            rx.lock(&rx);
            rx.write_bytes(&rx,buffer,len);
            rx.unlock(&rx);
        }
    }
}

void TCPClient::run() {
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

                    rx.lock(&rx);
                    size_t aviable_size = rx.size(&rx);
                    if (aviable_size >= (frame_len - 4)) {
                        rx.read_bytes(&rx,buffer + index,frame_len - 4);
                        rx.unlock(&rx);

                        //recive a freame, will call callback
                        if (msg_type == 1) {
                            LoginMessageResponse response;
                            response.deserialize(buffer + start_index, frame_len + 4);
                            if (response.statusCode() == 0) {
                                reconnect();
                            }
                            callback_(&response);
                        } 
                        else if (msg_type == 3) {
                            EchoMessageResponse response;
                            response.deserialize(buffer + start_index, frame_len + 4);
                            std::cout << response.msg() << std::endl;
                        } /*else if (msg_type == 2) {
                            EchoMessageRequest request("string","string");
                            request.deserialize(buffer + start_index, frame_len + 4);
                            std::cout << request.msg() << " ggf" << std::endl;
                        }*/

                        /*else if (msg_type == 0) {
                            LoginMessageRequest request;
                            request.deserialize(buffer + start_index, frame_len + 4);
                        }*/

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
                sendBlock(buffer,len);
            }
        }
        if (!active) {
            std::this_thread::sleep_for(100ms);
        }
    }
}

void TCPClient::sendRequest(Message * message) {
    int8_t * data = message->serialize();
    tx.lock(&tx);
    tx.write_bytes(&tx,data,message->size());
    tx.unlock(&tx);
    free(data);
}

void TCPClient::login(std::string username, std::string password) {
    LoginMessageRequest request(username, password);
    sendRequest(&request);
}
