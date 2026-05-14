// color.h
// Git-style clean terminal colors
#pragma once
#include <string>
#include <iostream>

namespace Color {
    const std::string RESET       = "\033[0m";
    const std::string RED         = "\033[31m";
    const std::string GREEN       = "\033[32m";
    const std::string YELLOW      = "\033[33m";
    const std::string WHITE       = "\033[97m";
    const std::string BOLD        = "\033[1m";
    const std::string BOLD_WHITE  = "\033[1;97m";
    const std::string BOLD_GREEN  = "\033[1;32m";
    const std::string BOLD_RED    = "\033[1;31m";
    const std::string BOLD_YELLOW = "\033[1;33m";
    const std::string DIM         = "\033[2m";

    // Git style: "error: message"
    inline void printSuccess(const std::string& msg) {
        std::cout << GREEN << msg << RESET << "\n";
    }
    inline void printError(const std::string& msg) {
        std::cerr << BOLD_RED << "error: " << RESET << msg << "\n";
    }
    inline void printWarning(const std::string& msg) {
        std::cout << YELLOW << "warning: " << RESET << msg << "\n";
    }
    inline void printInfo(const std::string& msg) {
        std::cout << DIM << msg << RESET << "\n";
    }
    inline void printHeader(const std::string& msg) {
        std::cout << "\n" << BOLD_WHITE << msg << RESET << "\n";
    }
    inline void printDone(const std::string& from, const std::string& to) {
        std::cout << GREEN << "  done  " << RESET
                  << from << " -> " << BOLD_WHITE << to << RESET << "\n";
    }
}
