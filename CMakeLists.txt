cmake_minimum_required(VERSION 3.12)
project(TcpSocketSelectExample)

set(CMAKE_CXX_STANDARD 14)

# Server
add_executable(echo_server)
target_sources(echo_server PRIVATE
    TcpServer.cpp
    echo_server.cpp
    circ_buf.cpp
    LoginMessage.cpp
    EchoMessage.cpp
)
target_compile_features(echo_server PRIVATE cxx_std_17)
target_link_libraries(echo_server pthread)

# Client
add_executable(echo_client)
target_sources(echo_client PRIVATE
    TcpClient.cpp
    echo_client.cpp
    circ_buf.cpp
    LoginMessage.cpp
    EchoMessage.cpp
)
target_compile_features(echo_client PRIVATE cxx_std_17)
target_link_libraries(echo_client pthread)
