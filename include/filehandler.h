// filehandler.h
// Declares file handling helper functions.

#pragma once
#include <string>

// WHY: Check if file exists before passing it to encryptor/decryptor
bool fileExists(const std::string& path);
