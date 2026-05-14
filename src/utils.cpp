// utils.cpp
// Implements utility/helper functions.

#include "utils.h"
#include <windows.h>
// WHY windows.h: gives us access to SecureZeroMemory — a Windows API function
// that is GUARANTEED by the OS to never be optimized away by the compiler

bool isValidPath(const std::string& path) {
    // WHY: An empty string passed as file path would cause confusing crashes
    // HOW: empty() returns true if string has zero characters
    return !path.empty();
    // ! = NOT operator — flips true to false and vice versa
    // So: if path is empty → empty() = true → !true = false → invalid
    //     if path has text → empty() = false → !false = true → valid
}

void secureWipe(void* ptr, size_t size) {
    // WHY void*: accepts ANY type of pointer — char[], std::string buffer, etc.
    // WHY SecureZeroMemory: unlike memset(), the compiler CANNOT skip this call
    // HOW: Windows kernel writes zeros to every byte in the given memory range
    // FULL FORM: SecureZeroMemory = Windows API — Secure (safe) Zero (fill with 0) Memory (RAM block)
    SecureZeroMemory(ptr, size);
}

void secureWipeString(std::string& s) {
    // WHY &s[0]: gives us a raw pointer to the first character in the string's internal buffer
    // WHY s.size(): wipes exactly as many bytes as the password occupies — no more, no less
    // WHY s.clear() after: resets the string length to 0 so no code accidentally reads it again
    if (!s.empty()) {
        SecureZeroMemory(&s[0], s.size());
        s.clear();
    }
}
