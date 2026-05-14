cmake_minimum_required(VERSION 3.15)
project(FileEncryptor VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

add_executable(fenc
    src/main.cpp
    src/encryptor.cpp
    src/decryptor.cpp
    src/filehandler.cpp
    src/utils.cpp
)

target_include_directories(fenc PRIVATE include)

target_link_libraries(fenc
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    stdc++fs
)
