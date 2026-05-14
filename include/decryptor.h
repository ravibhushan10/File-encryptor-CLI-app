// decryptor.h
#pragma once
#include <string>
#include <vector>

struct DecryptOptions {
    bool verbose    = false;
    std::string output = "";
};

void decryptFile(const std::string& inputPath, const std::string& password,
                 const DecryptOptions& opts = {});

void decryptFolder(const std::string& folderPath, const std::string& password,
                   const DecryptOptions& opts = {});

void decryptBatch(const std::vector<std::string>& files, const std::string& password,
                  const DecryptOptions& opts = {});
