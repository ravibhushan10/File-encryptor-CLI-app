// utils.h
// Declares utility/helper functions used across the project.

#pragma once
#include <string>

// WHY: Validates that a file path is not empty before we try to open it
bool isValidPath(const std::string& path);
// bool = Boolean — returns either true or false



// Securely wipes sensitive data from RAM
// WHY: prevents password recovery from memory dumps
void secureWipe(void* ptr, size_t size);
void secureWipeString(std::string& s);
