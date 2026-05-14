#include "decryptor.h"
#include "color.h"
#include "progress.h"
#include "hmac.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <ctime>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace fs = std::filesystem;

const int KEY_SIZE          = 32;
const int IV_SIZE           = 16;
const int SALT_SIZE         = 32;
const int PBKDF2_ITERATIONS = 100000;
const int HEADER_SIZE       = SALT_SIZE + IV_SIZE + HMAC_SIZE;
const size_t CHUNK          = 65536;

const char MAGIC[8] = {'F','E','N','C','R','Y','P','T'};

static std::string parseMetadata(const std::vector<unsigned char>& plaintext,
                                  size_t& metaSize, bool verbose)
{
    metaSize = 0;
    if (plaintext.size() < 20) return "";

    if (std::memcmp(plaintext.data(), MAGIC, 8) != 0) {
        Color::printWarning("No metadata header found -- old format file.");
        return "";
    }

    uint8_t version = plaintext[8];
    uint8_t algo    = plaintext[9];

    uint64_t ts = 0;
    for (int i = 0; i < 8; i++)
        ts |= ((uint64_t)plaintext[10 + i]) << (i * 8);

    uint16_t nameLen = plaintext[18] | ((uint16_t)plaintext[19] << 8);
    if (plaintext.size() < (size_t)(20 + nameLen)) return "";

    std::string fname(plaintext.begin() + 20, plaintext.begin() + 20 + nameLen);
    metaSize = 20 + nameLen;

    if (verbose) {
        time_t t = (time_t)ts;
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        Color::printInfo("  version   : " + std::to_string(version));
        Color::printInfo("  algorithm : " + std::string(algo == 0x01 ? "AES-256-CBC" : "Unknown"));
        Color::printInfo("  encrypted : " + std::string(timeBuf));
        Color::printInfo("  filename  : " + fname);
    }

    return fname;
}

void decryptFile(const std::string& inputPath, const std::string& password,
                 const DecryptOptions& opts)
{
    // no header

    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        Color::printError("cannot open file: " + inputPath);
        return;
    }

    inputFile.seekg(0, std::ios::end);
    size_t fileSize = (size_t)inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (fileSize <= (size_t)HEADER_SIZE) {
        Color::printError("file too small -- not a valid .enc file");
        return;
    }

    if (opts.verbose)
        Color::printInfo("file size: " + std::to_string(fileSize) + " bytes");

    std::vector<unsigned char> fileData(fileSize);
    size_t bytesRead = 0;
    while (bytesRead < fileSize) {
        size_t toRead = std::min(CHUNK, fileSize - bytesRead);
        inputFile.read(reinterpret_cast<char*>(fileData.data() + bytesRead), toRead);
        bytesRead += toRead;
        if (fileSize > 1048576) printProgressBar(bytesRead, fileSize, "reading  ");
    }
    inputFile.close();

    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char storedHMAC[HMAC_SIZE];

    std::copy(fileData.begin(),                       fileData.begin() + SALT_SIZE,           salt);
    std::copy(fileData.begin() + SALT_SIZE,           fileData.begin() + SALT_SIZE + IV_SIZE, iv);
    std::copy(fileData.begin() + SALT_SIZE + IV_SIZE, fileData.begin() + HEADER_SIZE,         storedHMAC);

    if (opts.verbose) {
        Color::printInfo("salt extracted -- " + std::to_string(SALT_SIZE) + " bytes");
        Color::printInfo("iv extracted   -- " + std::to_string(IV_SIZE)   + " bytes");
        Color::printInfo("hmac extracted -- " + std::to_string(HMAC_SIZE) + " bytes");
    }

    unsigned char key[KEY_SIZE];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, SALT_SIZE, PBKDF2_ITERATIONS,
                           EVP_sha256(), KEY_SIZE, key) != 1) {
        Color::printError("key derivation failed");
        return;
    }
    if (opts.verbose)
        Color::printInfo("key derived via PBKDF2-HMAC-SHA256");

    size_t cipherLen   = fileSize - HEADER_SIZE;
    auto computedHMAC  = computeHMAC(fileData.data() + HEADER_SIZE, cipherLen, key, KEY_SIZE);
    auto storedHMACVec = std::vector<unsigned char>(storedHMAC, storedHMAC + HMAC_SIZE);

    if (computedHMAC != storedHMACVec) {
        Color::printError("integrity check failed -- wrong password or file tampered");
        return;
    }
    Color::printInfo("integrity check passed");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { Color::printError("failed to create cipher context"); return; }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        Color::printError("failed to initialize AES decryption");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    std::vector<unsigned char> payload(cipherLen + 16);
    int bytesWritten = 0, totalBytes = 0;
    size_t decPos = 0;

    while (decPos < cipherLen) {
        size_t toDec = std::min(CHUNK, cipherLen - decPos);
        if (EVP_DecryptUpdate(ctx,
                payload.data() + totalBytes, &bytesWritten,
                fileData.data() + HEADER_SIZE + decPos, (int)toDec) != 1) {
            Color::printError("decryption failed");
            EVP_CIPHER_CTX_free(ctx);
            return;
        }
        totalBytes += bytesWritten;
        decPos     += toDec;
        if (cipherLen > 1048576) printProgressBar(decPos, cipherLen, "decrypting");
    }

    if (EVP_DecryptFinal_ex(ctx, payload.data() + totalBytes, &bytesWritten) != 1) {
        Color::printError("decryption finalization failed -- wrong password?");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    totalBytes += bytesWritten;
    EVP_CIPHER_CTX_free(ctx);
    payload.resize(totalBytes);

    if (opts.verbose)
        Color::printInfo("decrypted -- " + std::to_string(totalBytes) + " bytes");

    size_t metaSize = 0;
    std::string originalFilename = parseMetadata(payload, metaSize, opts.verbose);

    size_t dataStart = metaSize;
    size_t dataSize  = totalBytes - metaSize;

    std::string outputPath;
    if (!opts.output.empty()) {
        outputPath = opts.output;
    } else if (!originalFilename.empty()) {
        std::string parent = fs::path(inputPath).parent_path().string();
        outputPath = parent.empty() ? originalFilename : parent + "/" + originalFilename;
        if (opts.verbose)
            Color::printInfo("restoring filename: " + originalFilename);
    } else {
        outputPath = inputPath;
        if (outputPath.size() > 4 && outputPath.substr(outputPath.size() - 4) == ".enc")
            outputPath = outputPath.substr(0, outputPath.size() - 4);
        else
            outputPath += ".dec";
    }

    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        Color::printError("cannot create output file: " + outputPath);
        return;
    }

    outputFile.write(reinterpret_cast<char*>(payload.data() + dataStart), dataSize);
    outputFile.close();

    Color::printDone(fs::path(inputPath).filename().string(),
                     fs::path(outputPath).filename().string());
}

void decryptBatch(const std::vector<std::string>& files, const std::string& password,
                  const DecryptOptions& opts)
{
    // no header
    int success = 0, failed = 0;
    for (const auto& f : files) {
        if (!fs::exists(f)) {
            Color::printWarning("not found, skipping: " + f);
            failed++;
            continue;
        }
        decryptFile(f, password, opts);
        success++;
    }
    std::cout << "\n";
    Color::printSuccess(std::to_string(success) + " decrypted, " +
                        std::to_string(failed) + " failed");
}

void decryptFolder(const std::string& folderPath, const std::string& password,
                   const DecryptOptions& opts)
{
    // no header

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        Color::printError("not a valid directory: " + folderPath);
        return;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (path.size() <= 4 || path.substr(path.size() - 4) != ".enc") continue;
        files.push_back(path);
    }

    int success = 0;
    for (const auto& f : files) {
        decryptFile(f, password, opts);
        success++;
    }

    std::cout << "\n";
    Color::printSuccess(std::to_string(success) + " files decrypted");
}
