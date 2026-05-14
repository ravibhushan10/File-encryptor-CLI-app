// progress.h
// ASCII progress bar — avoids unicode rendering issues on Windows terminals

#pragma once
#include <iostream>
#include <string>

inline void printProgressBar(size_t current, size_t total, const std::string& label = "Progress") {
    const int BAR_WIDTH = 40;
    float ratio  = (total == 0) ? 1.0f : (float)current / (float)total;
    int filled   = (int)(ratio * BAR_WIDTH);
    int percent  = (int)(ratio * 100);

    std::cout << "\r\033[36m" << label << "\033[0m [";
    for (int i = 0; i < BAR_WIDTH; i++) {
        if (i < filled)
            std::cout << "\033[32m#\033[0m";  // # = filled
        else
            std::cout << "\033[90m-\033[0m";  // - = empty
    }
    std::cout << "] \033[1m" << percent << "%\033[0m";
    std::cout.flush();
    if (current >= total) std::cout << "\n";
}
