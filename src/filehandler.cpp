// filehandler.cpp
// Implements file handling helpers.

#include "filehandler.h"
#include <fstream>
// fstream = File Stream — used here just to check if file can be opened

bool fileExists(const std::string& path) {
    std::ifstream file(path);
    // WHY: We try opening the file — if it opens successfully, it exists
    // HOW: ifstream constructor attempts to open the file immediately
    return file.is_open();
    // is_open() = returns true if file was successfully opened
    // WHY not use filesystem::exists(): Requires C++17 filesystem linking
    // This approach works cleanly across all GCC setups
}
