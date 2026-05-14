cmake_minimum_required(VERSION 3.15)
project(FileEncryptor VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

add_executable(fileencryptor
    src/main.cpp
    src/encryptor.cpp
    src/decryptor.cpp
    src/filehandler.cpp
    src/utils.cpp
)

target_include_directories(fileencryptor PRIVATE include)

target_link_libraries(fileencryptor
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    stdc++fs
)
# WHY stdc++fs: links std::filesystem — required for folder encryption/decryption
# on MinGW/GCC. Without this, recursive_directory_iterator won't link.
