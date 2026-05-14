#include "encryptor.h"
#include "color.h"
#include "progress.h"
#include "hmac.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <ctime>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace fs = std::filesystem;

const int KEY_SIZE          = 32;
const int IV_SIZE           = 16;
const int SALT_SIZE         = 32;
const int PBKDF2_ITERATIONS = 100000;
const size_t CHUNK          = 65536;

const uint8_t META_VERSION = 0x01;
const uint8_t ALGO_AES256  = 0x01;
const char    MAGIC[8]     = {'F','E','N','C','R','Y','P','T'};

static std::vector<unsigned char> buildMetadata(const std::string& originalFilename) {
    uint64_t ts = (uint64_t)std::time(nullptr);
    std::string fname = fs::path(originalFilename).filename().string();
    uint16_t nameLen  = (uint16_t)fname.size();

    std::vector<unsigned char> meta;
    meta.insert(meta.end(), MAGIC, MAGIC + 8);
    meta.push_back(META_VERSION);
    meta.push_back(ALGO_AES256);
    for (int i = 0; i < 8; i++)
        meta.push_back((ts >> (i * 8)) & 0xFF);
    meta.push_back(nameLen & 0xFF);
    meta.push_back((nameLen >> 8) & 0xFF);
    meta.insert(meta.end(), fname.begin(), fname.end());
    return meta;
}

void encryptFile(const std::string& inputPath, const std::string& password,
                 const EncryptOptions& opts)
{
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        Color::printError("cannot open file: " + inputPath);
        return;
    }

    inputFile.seekg(0, std::ios::end);
    size_t fileSize = (size_t)inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (opts.verbose) {
        Color::printInfo("file     : " + inputPath);
        Color::printInfo("size     : " + std::to_string(fileSize) + " bytes");
    }

    std::vector<unsigned char> plaintext(fileSize);
    inputFile.read(reinterpret_cast<char*>(plaintext.data()), fileSize);
    inputFile.close();

    auto meta = buildMetadata(inputPath);
    if (opts.verbose)
        Color::printInfo("algo     : AES-256-CBC");

    std::vector<unsigned char> payload;
    payload.insert(payload.end(), meta.begin(), meta.end());
    payload.insert(payload.end(), plaintext.begin(), plaintext.end());

    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    if (RAND_bytes(salt, SALT_SIZE) != 1) { Color::printError("failed to generate salt"); return; }
    if (RAND_bytes(iv,   IV_SIZE)   != 1) { Color::printError("failed to generate IV");   return; }

    if (opts.verbose) {
        Color::printInfo("salt     : " + std::to_string(SALT_SIZE) + " bytes (random)");
        Color::printInfo("iv       : " + std::to_string(IV_SIZE)   + " bytes (random)");
    }

    unsigned char key[KEY_SIZE];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, SALT_SIZE, PBKDF2_ITERATIONS,
                           EVP_sha256(), KEY_SIZE, key) != 1) {
        Color::printError("key derivation failed");
        return;
    }
    if (opts.verbose)
        Color::printInfo("kdf      : PBKDF2-HMAC-SHA256 (" + std::to_string(PBKDF2_ITERATIONS) + " iterations)");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { Color::printError("failed to create cipher context"); return; }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        Color::printError("failed to initialize encryption");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    std::vector<unsigned char> ciphertext(payload.size() + 16);
    int bytesWritten = 0, totalBytes = 0;
    size_t encPos = 0;

    while (encPos < payload.size()) {
        size_t toEnc = std::min(CHUNK, payload.size() - encPos);
        if (EVP_EncryptUpdate(ctx,
                ciphertext.data() + totalBytes, &bytesWritten,
                payload.data() + encPos, (int)toEnc) != 1) {
            Color::printError("encryption failed");
            EVP_CIPHER_CTX_free(ctx);
            return;
        }
        totalBytes += bytesWritten;
        encPos     += toEnc;
        // only show progress bar for files > 1MB
        if (fileSize > 1048576)
            printProgressBar(encPos, payload.size(), "encrypting");
    }

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + totalBytes, &bytesWritten) != 1) {
        Color::printError("encryption finalization failed");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    totalBytes += bytesWritten;
    EVP_CIPHER_CTX_free(ctx);

    auto hmacTag = computeHMAC(ciphertext.data(), totalBytes, key, KEY_SIZE);
    if (opts.verbose)
        Color::printInfo("hmac     : SHA256 tag computed");

    std::string outputPath = opts.output.empty() ? (inputPath + ".enc") : opts.output;

    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        Color::printError("cannot create output file: " + outputPath);
        return;
    }

    outputFile.write(reinterpret_cast<char*>(salt),              SALT_SIZE);
    outputFile.write(reinterpret_cast<char*>(iv),                IV_SIZE);
    outputFile.write(reinterpret_cast<char*>(hmacTag.data()),    HMAC_SIZE);
    outputFile.write(reinterpret_cast<char*>(ciphertext.data()), totalBytes);
    outputFile.close();

    if (opts.verbose)
        Color::printInfo("output   : " + outputPath);

    Color::printDone(fs::path(inputPath).filename().string(),
                     fs::path(outputPath).filename().string());
}

void encryptBatch(const std::vector<std::string>& files, const std::string& password,
                  const EncryptOptions& opts)
{
    int success = 0, failed = 0;
    for (const auto& f : files) {
        if (!fs::exists(f)) {
            Color::printWarning("not found, skipping: " + f);
            failed++;
            continue;
        }
        encryptFile(f, password, opts);
        success++;
    }
    std::cout << "\n";
    Color::printSuccess(std::to_string(success) + " encrypted, " +
                        std::to_string(failed) + " failed");
}

void encryptFolder(const std::string& folderPath, const std::string& password,
                   const EncryptOptions& opts)
{
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        Color::printError("not a valid directory: " + folderPath);
        return;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (path.size() > 4 && path.substr(path.size() - 4) == ".enc") {
            Color::printWarning("skipping already encrypted: " +
                                fs::path(path).filename().string());
            continue;
        }
        files.push_back(path);
    }

    int success = 0;
    for (const auto& f : files) {
        encryptFile(f, password, opts);
        success++;
    }

    std::cout << "\n";
    Color::printSuccess(std::to_string(success) + " files encrypted");
}
