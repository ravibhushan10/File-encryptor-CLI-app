// decryptor.cpp
// AES-256-CBC decryption with:
// ✅ Colored output    ✅ Progress bar     ✅ HMAC verification
// ✅ Folder decrypt    ✅ Verbose mode     ✅ --output flag
// ✅ Metadata parsing  ✅ Batch mode       ✅ File size display

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

// parseMetadata()
// WHY: reads the metadata block prepended to plaintext after decryption
// HOW: reads fixed fields sequentially, then variable-length filename
// RETURNS: original filename stored at encrypt time
static std::string parseMetadata(const std::vector<unsigned char>& plaintext,
                                  size_t& metaSize,
                                  bool verbose)
{
    metaSize = 0;

    // Minimum metadata size: 8(magic)+1(ver)+1(algo)+8(ts)+2(nameLen) = 20 bytes
    if (plaintext.size() < 20) return "";

    // Verify magic bytes
    // WHY: if magic doesn't match, this isn't a valid FileEncryptor file
    if (std::memcmp(plaintext.data(), MAGIC, 8) != 0) {
        Color::printWarning("No metadata header found -- old format file.");
        return "";
    }

    uint8_t version = plaintext[8];
    uint8_t algo    = plaintext[9];

    // Read timestamp (8 bytes little-endian)
    uint64_t ts = 0;
    for (int i = 0; i < 8; i++)
        ts |= ((uint64_t)plaintext[10 + i]) << (i * 8);

    // Read filename length (2 bytes little-endian)
    uint16_t nameLen = plaintext[18] | ((uint16_t)plaintext[19] << 8);

    if (plaintext.size() < (size_t)(20 + nameLen)) return "";

    // Read filename
    std::string fname(plaintext.begin() + 20, plaintext.begin() + 20 + nameLen);

    metaSize = 20 + nameLen;

    if (verbose) {
        // Convert timestamp to readable string
        time_t t = (time_t)ts;
        char timeBuf[64];
        // strftime = String Format Time -- formats time_t into human-readable string
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

        Color::printInfo("Metadata found:");
        Color::printInfo("  Version   : " + std::to_string(version));
        Color::printInfo("  Algorithm : " + std::string(algo == 0x01 ? "AES-256-CBC" : "Unknown"));
        Color::printInfo("  Encrypted : " + std::string(timeBuf));
        Color::printInfo("  Filename  : " + fname);
    }

    return fname;
}

void decryptFile(const std::string& inputPath, const std::string& password,
                 const DecryptOptions& opts)
{
    Color::printHeader("Decrypting: " + fs::path(inputPath).filename().string());

    // ── Read encrypted file ─────────────────────────────────────────
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        Color::printError("Cannot open encrypted file: " + inputPath);
        return;
    }

    inputFile.seekg(0, std::ios::end);
    size_t fileSize = (size_t)inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (fileSize <= (size_t)HEADER_SIZE) {
        Color::printError("File too small -- not a valid .enc file.");
        return;
    }

    if (opts.verbose)
        Color::printInfo("Encrypted file size: " + std::to_string(fileSize) + " bytes");

    std::vector<unsigned char> fileData(fileSize);
    size_t bytesRead = 0;
    while (bytesRead < fileSize) {
        size_t toRead = std::min(CHUNK, fileSize - bytesRead);
        inputFile.read(reinterpret_cast<char*>(fileData.data() + bytesRead), toRead);
        bytesRead += toRead;
        printProgressBar(bytesRead, fileSize, "Reading  ");
    }
    inputFile.close();

    // ── Extract Salt, IV, HMAC ──────────────────────────────────────
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char storedHMAC[HMAC_SIZE];

    std::copy(fileData.begin(),                       fileData.begin() + SALT_SIZE,           salt);
    std::copy(fileData.begin() + SALT_SIZE,           fileData.begin() + SALT_SIZE + IV_SIZE, iv);
    std::copy(fileData.begin() + SALT_SIZE + IV_SIZE, fileData.begin() + HEADER_SIZE,         storedHMAC);

    if (opts.verbose) {
        Color::printInfo("Salt extracted -- " + std::to_string(SALT_SIZE) + " bytes");
        Color::printInfo("IV extracted   -- " + std::to_string(IV_SIZE)   + " bytes");
        Color::printInfo("HMAC extracted -- " + std::to_string(HMAC_SIZE) + " bytes");
    }

    // ── PBKDF2 Key Derivation ───────────────────────────────────────
    unsigned char key[KEY_SIZE];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, SALT_SIZE, PBKDF2_ITERATIONS,
                           EVP_sha256(), KEY_SIZE, key) != 1) {
        Color::printError("PBKDF2 key derivation failed.");
        return;
    }
    if (opts.verbose)
        Color::printInfo("Key re-derived via PBKDF2-HMAC-SHA256");

    // ── HMAC Verification ───────────────────────────────────────────
    size_t cipherLen    = fileSize - HEADER_SIZE;
    auto computedHMAC   = computeHMAC(fileData.data() + HEADER_SIZE, cipherLen, key, KEY_SIZE);
    auto storedHMACVec  = std::vector<unsigned char>(storedHMAC, storedHMAC + HMAC_SIZE);

    if (computedHMAC != storedHMACVec) {
        Color::printError("HMAC verification FAILED -- file tampered or wrong password!");
        return;
    }
    if (opts.verbose)
        Color::printInfo("HMAC verified -- file integrity confirmed");
    else
        Color::printInfo("Integrity check passed -- integrity confirmed"

    // ── AES-256-CBC Decryption ──────────────────────────────────────
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { Color::printError("Failed to create cipher context."); return; }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        Color::printError("Failed to initialize AES decryption.");
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
            Color::printError("Decryption failed.");
            EVP_CIPHER_CTX_free(ctx);
            return;
        }
        totalBytes += bytesWritten;
        decPos     += toDec;
        printProgressBar(decPos, cipherLen, "Decrypting");
    }

    if (EVP_DecryptFinal_ex(ctx, payload.data() + totalBytes, &bytesWritten) != 1) {
        Color::printError("Decryption finalization failed -- wrong password?");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    totalBytes += bytesWritten;
    EVP_CIPHER_CTX_free(ctx);

    payload.resize(totalBytes);

    if (opts.verbose)
        Color::printInfo("Decryption complete -- " + std::to_string(totalBytes) + " bytes plaintext");

    // ── Parse metadata ──────────────────────────────────────────────
    size_t metaSize = 0;
    std::string originalFilename = parseMetadata(payload, metaSize, opts.verbose);

    // Actual file data starts after metadata block
    size_t dataStart = metaSize;
    size_t dataSize  = totalBytes - metaSize;

    // ── Determine output path ───────────────────────────────────────
    std::string outputPath;
    if (!opts.output.empty()) {
        // --output flag takes highest priority
        outputPath = opts.output;
    } else if (!originalFilename.empty()) {
        // Use original filename from metadata -- stored at encrypt time
        outputPath = fs::path(inputPath).parent_path().string().empty() ? originalFilename : fs::path(inputPath).parent_path().string() + "/" + originalFilename;
        if (opts.verbose)
            Color::printInfo("Restoring original filename: " + originalFilename);
    } else {
        // Fallback: strip .enc extension
        outputPath = inputPath;
        if (outputPath.size() > 4 && outputPath.substr(outputPath.size() - 4) == ".enc")
            outputPath = outputPath.substr(0, outputPath.size() - 4);
        else
            outputPath += ".dec";
    }

    // ── Write output file ───────────────────────────────────────────
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        Color::printError("Cannot create output file: " + outputPath);
        return;
    }

    outputFile.write(reinterpret_cast<char*>(payload.data() + dataStart), dataSize);
    outputFile.close();

    double inMB  = (double)fileSize / (1024.0 * 1024.0);
    double outMB = (double)dataSize  / (1024.0 * 1024.0);
    auto fmt = [](double v) {
        int whole = (int)v;
        int frac  = (int)((v - whole) * 100);
        return std::to_string(whole) + "." + (frac < 10 ? "0" : "") + std::to_string(frac);
    };

    Color::printSuccess("Done: " + inputPath + " -> " + outputPath +
        "  [" + fmt(inMB) + " MB -> " + fmt(outMB) + " MB]");
}

// ── Batch Decryption ──────────────────────────────────────────────────────────
void decryptBatch(const std::vector<std::string>& files, const std::string& password,
                  const DecryptOptions& opts)
{
    Color::printHeader("Batch Decryption -- " + std::to_string(files.size()) + " files");
    int success = 0, failed = 0;
    for (const auto& f : files) {
        if (!fs::exists(f)) {
            Color::printWarning("File not found, skipping: " + f);
            failed++;
            continue;
        }
        decryptFile(f, password, opts);
        success++;
    }
    Color::printSuccess("Batch done -- " + std::to_string(success) + " decrypted, " +
        std::to_string(failed) + " failed.");
}

// ── Folder Decryption ─────────────────────────────────────────────────────────
void decryptFolder(const std::string& folderPath, const std::string& password,
                   const DecryptOptions& opts)
{
    Color::printHeader("Folder Decryption: " + folderPath);

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        Color::printError("Not a valid directory: " + folderPath);
        return;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (path.size() <= 4 || path.substr(path.size() - 4) != ".enc") {
            continue;
        }
        files.push_back(path);
    }

    int success = 0;
    for (const auto& f : files) {
        decryptFile(f, password, opts);
        success++;
    }

    Color::printSuccess("Folder done -- " + std::to_string(success) + " files decrypted.");
}
