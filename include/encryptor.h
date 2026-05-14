// encryptor.h
#pragma once
#include <string>
#include <vector>
#include "decryptor.h"
// WHY include decryptor.h: DecryptOptions lives there — no duplicate definition

struct EncryptOptions {
    bool verbose     = false;
    std::string output = "";
};

void encryptFile(const std::string& inputPath, const std::string& password,
                 const EncryptOptions& opts = {});

void encryptFolder(const std::string& folderPath, const std::string& password,
                   const EncryptOptions& opts = {});

void encryptBatch(const std::vector<std::string>& files, const std::string& password,
                  const EncryptOptions& opts = {});
