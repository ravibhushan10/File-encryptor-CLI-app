// hmac.h
// HMAC-SHA256 integrity verification for .enc files.
// WHY: Detects if encrypted file was tampered with — real security feature
// HMAC = Hash-based Message Authentication Code
// SHA256 = Secure Hash Algorithm 256-bit

#pragma once
#include <vector>
#include <string>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// HMAC_SIZE = 32 bytes = 256 bits — SHA256 always produces exactly 32 bytes
const int HMAC_SIZE = 32;

// computeHMAC()
// WHY: Generates a 32-byte authentication tag from data + key
// HOW: HMAC-SHA256 runs SHA256 twice internally with the key mixed in
//      This makes it impossible to forge without knowing the key
inline std::vector<unsigned char> computeHMAC(
    const unsigned char* data, size_t dataLen,
    const unsigned char* key, size_t keyLen)
{
    std::vector<unsigned char> result(HMAC_SIZE);
    unsigned int outLen = HMAC_SIZE;

    // HMAC() = OpenSSL function
    // EVP_sha256() = use SHA256 as the hash function
    HMAC(EVP_sha256(), key, (int)keyLen, data, dataLen, result.data(), &outLen);
    return result;
}
