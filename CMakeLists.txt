cmake_minimum_required(VERSION 3.15)
project(ChatApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/include/Coroutines
                    ${CMAKE_SOURCE_DIR}/include/Client
                    ${CMAKE_SOURCE_DIR}/include/Server
                    ${CMAKE_SOURCE_DIR}/include/Atomic_queue
                    ${CMAKE_SOURCE_DIR}/include/Polling
                    ${CMAKE_SOURCE_DIR}/include/Messages
                    )

add_executable(Client
            src/Client/Client_main.cpp
            src/Client/Client.cpp
            src/Polling/Epoll.cpp
)

add_executable(Server
            src/Server/Server_main.cpp
            src/Polling/Epoll.cpp
            )
