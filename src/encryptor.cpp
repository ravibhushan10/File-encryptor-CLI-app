// encryptor.cpp
// AES-256-CBC encryption with:
// ✅ Colored output    ✅ Progress bar     ✅ HMAC integrity
// ✅ Folder encrypt    ✅ Verbose mode     ✅ --output flag
// ✅ Metadata header   ✅ Batch mode       ✅ File size display

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
const size_t CHUNK          = 65536; // 64KB per read/encrypt chunk

// ── Metadata Header Layout ───────────────────────────────────────────────────
// We store metadata INSIDE the encrypted payload so it's protected too.
// Layout of .enc file:
// [ Salt(32) | IV(16) | HMAC(32) | Ciphertext ]
//   where Ciphertext = AES( MetadataBlock + OriginalFileBytes )
//
// MetadataBlock (plaintext before encryption):
// [ MAGIC(8) | version(1) | algo(1) | timestamp(8) | nameLen(2) | filename(nameLen) ]
//
// WHY magic bytes: lets us detect valid .enc files on decrypt
// WHY version byte: future-proofs the format -- v1 files always readable
// WHY algo byte: 0x01 = AES-256-CBC, room for ChaCha20 later
// WHY timestamp: shows when file was encrypted -- useful metadata
// WHY filename: lets us restore original filename on decrypt

const uint8_t  META_VERSION  = 0x01;
const uint8_t  ALGO_AES256   = 0x01;
const char     MAGIC[8]      = {'F','E','N','C','R','Y','P','T'};
// MAGIC = "FENCRYPT" -- File ENCRYPTor -- our signature bytes

// buildMetadata()
// WHY: packages filename + timestamp + algo into a byte block prepended to plaintext
// HOW: writes fixed fields then variable-length filename
static std::vector<unsigned char> buildMetadata(const std::string& originalFilename) {
    // Get current Unix timestamp
    // WHY time_t: standard C type for seconds since Jan 1 1970 (Unix epoch)
    uint64_t ts = (uint64_t)std::time(nullptr);

    // Extract just the filename, not the full path
    // WHY: we store "secret.txt" not "C:\Users\ravib\secret.txt"
    std::string fname = fs::path(originalFilename).filename().string();
    uint16_t nameLen  = (uint16_t)fname.size();

    std::vector<unsigned char> meta;

    // MAGIC (8 bytes)
    meta.insert(meta.end(), MAGIC, MAGIC + 8);

    // Version (1 byte)
    meta.push_back(META_VERSION);

    // Algorithm (1 byte)
    meta.push_back(ALGO_AES256);

    // Timestamp (8 bytes, little-endian)
    // WHY little-endian: x86/x64 CPUs store lowest byte first
    for (int i = 0; i < 8; i++)
        meta.push_back((ts >> (i * 8)) & 0xFF);

    // Filename length (2 bytes, little-endian)
    meta.push_back(nameLen & 0xFF);
    meta.push_back((nameLen >> 8) & 0xFF);

    // Filename bytes
    meta.insert(meta.end(), fname.begin(), fname.end());

    return meta;
}

void encryptFile(const std::string& inputPath, const std::string& password,
                 const EncryptOptions& opts)
{
    Color::printHeader("Encrypting: " + fs::path(inputPath).filename().string());

    // ── Read input file ─────────────────────────────────────────────
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        Color::printError("Cannot open input file: " + inputPath);
        return;
    }

    inputFile.seekg(0, std::ios::end);
    size_t fileSize = (size_t)inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (opts.verbose)
        Color::printInfo("File size: " + std::to_string(fileSize) + " bytes");

    std::vector<unsigned char> plaintext(fileSize);
    size_t bytesRead = 0;
    while (bytesRead < fileSize) {
        size_t toRead = std::min(CHUNK, fileSize - bytesRead);
        inputFile.read(reinterpret_cast<char*>(plaintext.data() + bytesRead), toRead);
        bytesRead += toRead;
        printProgressBar(bytesRead, fileSize, "Reading  ");
    }
    inputFile.close();

    // ── Build metadata block ────────────────────────────────────────
    auto meta = buildMetadata(inputPath);
    if (opts.verbose)
        Color::printInfo("Metadata built -- filename: " +
            fs::path(inputPath).filename().string() + ", algo: AES-256-CBC");

    // Prepend metadata to plaintext
    // WHY prepend: metadata encrypted alongside data -- tamper-proof
    std::vector<unsigned char> payload;
    payload.insert(payload.end(), meta.begin(), meta.end());
    payload.insert(payload.end(), plaintext.begin(), plaintext.end());

    // ── Generate Salt + IV ──────────────────────────────────────────
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];

    if (RAND_bytes(salt, SALT_SIZE) != 1) { Color::printError("Failed to generate salt."); return; }
    if (RAND_bytes(iv,   IV_SIZE)   != 1) { Color::printError("Failed to generate IV.");   return; }

    if (opts.verbose) {
        Color::printInfo("Salt generated  -- " + std::to_string(SALT_SIZE) + " random bytes");
        Color::printInfo("IV generated    -- " + std::to_string(IV_SIZE)   + " random bytes");
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
        Color::printInfo("Key derived via PBKDF2-HMAC-SHA256 (" +
            std::to_string(PBKDF2_ITERATIONS) + " iterations)");

    // ── AES-256-CBC Encryption ──────────────────────────────────────
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { Color::printError("Failed to create cipher context."); return; }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        Color::printError("Failed to initialize AES encryption.");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    if (opts.verbose)
        Color::printInfo("AES-256-CBC cipher initialized");

    std::vector<unsigned char> ciphertext(payload.size() + 16);
    int bytesWritten = 0, totalBytes = 0;
    size_t encPos = 0;

    while (encPos < payload.size()) {
        size_t toEnc = std::min(CHUNK, payload.size() - encPos);
        if (EVP_EncryptUpdate(ctx,
                ciphertext.data() + totalBytes, &bytesWritten,
                payload.data() + encPos, (int)toEnc) != 1) {
            Color::printError("Encryption failed.");
            EVP_CIPHER_CTX_free(ctx);
            return;
        }
        totalBytes += bytesWritten;
        encPos     += toEnc;
        printProgressBar(encPos, payload.size(), "Encrypting");
    }

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + totalBytes, &bytesWritten) != 1) {
        Color::printError("Encryption finalization failed.");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    totalBytes += bytesWritten;
    EVP_CIPHER_CTX_free(ctx);

    if (opts.verbose)
        Color::printInfo("Encryption complete -- " + std::to_string(totalBytes) + " bytes ciphertext");

    // ── HMAC over ciphertext ────────────────────────────────────────
    auto hmacTag = computeHMAC(ciphertext.data(), totalBytes, key, KEY_SIZE);
    if (opts.verbose)
        Color::printInfo("HMAC-SHA256 tag computed -- 32 bytes");

    // ── Write output file ───────────────────────────────────────────
    // Determine output path: --output flag overrides default
    std::string outputPath = opts.output.empty() ? (inputPath + ".enc") : opts.output;

    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        Color::printError("Cannot create output file: " + outputPath);
        return;
    }

    outputFile.write(reinterpret_cast<char*>(salt),              SALT_SIZE);
    outputFile.write(reinterpret_cast<char*>(iv),                IV_SIZE);
    outputFile.write(reinterpret_cast<char*>(hmacTag.data()),    HMAC_SIZE);
    outputFile.write(reinterpret_cast<char*>(ciphertext.data()), totalBytes);
    outputFile.close();

    if (opts.verbose)
        Color::printInfo("Written: Salt + IV + HMAC + Ciphertext -> " + outputPath);

    // File size display
    double inMB  = (double)fileSize   / (1024.0 * 1024.0);
    double outMB = (double)totalBytes / (1024.0 * 1024.0);
    // Format to 2 decimal places manually (no printf needed)
    auto fmt = [](double v) {
        int whole = (int)v;
        int frac  = (int)((v - whole) * 100);
        return std::to_string(whole) + "." + (frac < 10 ? "0" : "") + std::to_string(frac);
    };
    Color::printSuccess("Done: " + inputPath + " -> " + outputPath +
        "  [" + fmt(inMB) + " MB -> " + fmt(outMB) + " MB]");
}

// ── Batch Encryption ─────────────────────────────────────────────────────────
// WHY: encrypts multiple files in one command -- fileencryptor encrypt-batch --files a.txt b.txt
void encryptBatch(const std::vector<std::string>& files, const std::string& password,
                  const EncryptOptions& opts)
{
    Color::printHeader("Batch Encryption -- " + std::to_string(files.size()) + " files");
    int success = 0, failed = 0;
    for (const auto& f : files) {
        if (!fs::exists(f)) {
            Color::printWarning("File not found, skipping: " + f);
            failed++;
            continue;
        }
        encryptFile(f, password, opts);
        success++;
    }
    Color::printSuccess("Batch done -- " + std::to_string(success) + " encrypted, " +
        std::to_string(failed) + " failed.");
}

// ── Folder Encryption ─────────────────────────────────────────────────────────
void encryptFolder(const std::string& folderPath, const std::string& password,
                   const EncryptOptions& opts)
{
    Color::printHeader("Folder Encryption: " + folderPath);

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        Color::printError("Not a valid directory: " + folderPath);
        return;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (path.size() > 4 && path.substr(path.size() - 4) == ".enc") {
            Color::printWarning("Skipping already encrypted: " + path);
            continue;
        }
        files.push_back(path);
    }

    int success = 0;
    for (const auto& f : files) {
        encryptFile(f, password, opts);
        success++;
    }

    Color::printSuccess("Folder done -- " + std::to_string(success) + " files encrypted.");
}
