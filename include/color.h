// color.h
// ANSI color codes for terminal output.

#pragma once
#include <string>
#include <iostream>

namespace Color {
    const std::string RESET       = "\033[0m";
    const std::string RED         = "\033[31m";
    const std::string GREEN       = "\033[32m";
    const std::string YELLOW      = "\033[33m";
    const std::string BLUE        = "\033[34m";
    const std::string CYAN        = "\033[36m";
    const std::string BOLD_RED    = "\033[1;31m";
    const std::string BOLD_GREEN  = "\033[1;32m";
    const std::string BOLD_YELLOW = "\033[1;33m";
    const std::string BOLD_CYAN   = "\033[1;36m";
    const std::string BOLD        = "\033[1m";

    inline void printSuccess(const std::string& msg) {
        std::cout << BOLD_GREEN << "[OK] " << RESET << GREEN << msg << RESET << "\n";
    }
    inline void printError(const std::string& msg) {
        std::cerr << BOLD_RED << "[ERR] " << RESET << RED << msg << RESET << "\n";
    }
    inline void printWarning(const std::string& msg) {
        std::cout << BOLD_YELLOW << "[!] " << RESET << YELLOW << msg << RESET << "\n";
    }
    inline void printInfo(const std::string& msg) {
        std::cout << BOLD_CYAN << "[i] " << RESET << CYAN << msg << RESET << "\n";
    }
    inline void printHeader(const std::string& msg) {
        std::cout << "\n" << BOLD_CYAN << "=== " << msg << " ===" << RESET << "\n\n";
    }
}
