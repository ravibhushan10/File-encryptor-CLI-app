# FileEncryptor CLI

> AES-256-CBC file encryption tool for Windows with HMAC-SHA256 integrity verification, batch mode, folder encryption, and a professional installer.

[![Release](https://img.shields.io/badge/Download-v3.0.0-brightgreen?style=for-the-badge&logo=github)](https://github.com/ravibhushan10/File-encryptor-CLI-app/releases/tag/v3.0.0)
[![Language](https://img.shields.io/badge/C++-17-blue?style=for-the-badge&logo=cplusplus)](https://isocpp.org/)
[![OpenSSL](https://img.shields.io/badge/OpenSSL-3.x-red?style=for-the-badge&logo=openssl)](https://www.openssl.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)](LICENSE)

---

## Download

| File | Size | Description |
|---|---|---|
| [FileEncryptorSetup.exe](https://github.com/ravibhushan10/File-encryptor-CLI-app/releases/download/v3.0.0/FileEncryptorSetup.exe) | 3.51 MB | Windows installer — includes all required DLLs |

**Install:** Double-click → follow wizard → use from any Command Prompt.

---

## Features

- **AES-256-CBC Encryption** — industry-standard symmetric encryption
- **PBKDF2-HMAC-SHA256 Key Derivation** — 100,000 iterations, unique salt per file
- **HMAC-SHA256 Integrity Verification** — detects tampering before decryption
- **Metadata Header** — stores original filename, timestamp, and algorithm inside encrypted file
- **Batch Mode** — encrypt/decrypt multiple files in one command
- **Folder Mode** — recursively encrypt/decrypt entire directories
- **Colored Terminal Output** — green for success, red for errors, cyan for info
- **Progress Bar** — real-time progress for large files
- **Verbose Mode** — prints every internal step (salt, IV, key derivation, HMAC)
- **Custom Output Path** — `--output` flag for full control
- **Secure Memory Wiping** — passwords zeroed from RAM immediately after use
- **Windows Installer** — built with NSIS, registers in Add/Remove Programs

---

## How It Works

```
Password + Salt  →  PBKDF2-HMAC-SHA256 (100,000 iterations)  →  AES-256 Key
Key + IV         →  AES-256-CBC                               →  Ciphertext
Ciphertext       →  HMAC-SHA256                               →  Integrity Tag
```

**.enc file layout:**
```
[ Salt (32 bytes) | IV (16 bytes) | HMAC (32 bytes) | Ciphertext ]
         ↑ header read first on decrypt, key re-derived, HMAC verified before decryption
```

**Metadata (encrypted inside ciphertext):**
```
[ MAGIC (8) | Version (1) | Algorithm (1) | Timestamp (8) | FilenameLen (2) | Filename ]
```

---

## Usage

### Encrypt a file
```cmd
fileencryptor encrypt --file secret.txt --password mypassword
```

### Decrypt a file
```cmd
fileencryptor decrypt --file secret.txt.enc --password mypassword
```

### Encrypt with verbose output
```cmd
fileencryptor encrypt --file secret.txt --password mypassword --verbose
```

### Custom output path
```cmd
fileencryptor encrypt --file secret.txt --password mypassword --output out\secret.enc
fileencryptor decrypt --file out\secret.enc --password mypassword --output restored.txt
```

### Batch encrypt multiple files
```cmd
fileencryptor encrypt-batch --files a.txt b.txt c.txt --password mypassword
```

### Batch decrypt multiple files
```cmd
fileencryptor decrypt-batch --files a.txt.enc b.txt.enc c.txt.enc --password mypassword
```

### Encrypt entire folder
```cmd
fileencryptor encrypt-folder --folder C:\SensitiveData --password mypassword
```

### Decrypt entire folder
```cmd
fileencryptor decrypt-folder --folder C:\SensitiveData --password mypassword
```

### Check version
```cmd
fileencryptor --version
```

---

## Demo

```
+======================================+
|     FileEncryptor  v3.0              |
|     AES-256-CBC + HMAC-SHA256        |
|     by Ravi Bhushan                  |
+======================================+

=== Encrypting: secret.txt ===
[i] File size: 1048576 bytes
Reading    [########################################] 100%
[i] Metadata built -- filename: secret.txt, algo: AES-256-CBC
[i] Salt generated  -- 32 random bytes
[i] IV generated    -- 16 random bytes
[i] Key derived via PBKDF2-HMAC-SHA256 (100000 iterations)
[i] AES-256-CBC cipher initialized
Encrypting [########################################] 100%
[i] HMAC-SHA256 tag computed -- 32 bytes
[OK] Done: secret.txt -> secret.txt.enc  [1.00 MB -> 1.00 MB]

=== Decrypting: secret.txt.enc ===
Reading    [########################################] 100%
[i] HMAC verified -- file integrity confirmed
Decrypting [########################################] 100%
[i]   Version   : 1
[i]   Algorithm : AES-256-CBC
[i]   Encrypted : 2026-05-15 01:46:27
[i]   Filename  : secret.txt
[OK] Done: secret.txt.enc -> secret.txt  [1.00 MB -> 1.00 MB]
```

---

## Build From Source

**Requirements:**
- MSYS2 UCRT64
- GCC 16+ (`mingw-w64-ucrt-x86_64-gcc`)
- CMake 3.15+ (`mingw-w64-ucrt-x86_64-cmake`)
- OpenSSL 3.x (`mingw-w64-ucrt-x86_64-openssl`)
- NSIS 3.x (`mingw-w64-ucrt-x86_64-nsis`) — for installer only

```bash
# Clone
git clone https://github.com/ravibhushan10/File-encryptor-CLI-app.git
cd File-encryptor-CLI-app

# Build
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=/ucrt64/bin/mingw32-make
cmake --build .

# Run
./fileencryptor.exe --help
```

**Build installer:**
```bash
cd ..
cp /ucrt64/bin/libcrypto-3-x64.dll build/
cp /ucrt64/bin/libgcc_s_seh-1.dll build/
cp /ucrt64/bin/libstdc++-6.dll build/
cp /ucrt64/bin/libwinpthread-1.dll build/
makensis installer.nsi
```

---

## Project Structure

```
File-encryptor-CLI-app/
├── CMakeLists.txt          # Build configuration
├── installer.nsi           # NSIS Windows installer script
├── include/
│   ├── encryptor.h         # Encryption function declarations
│   ├── decryptor.h         # Decryption function declarations
│   ├── color.h             # ANSI colored terminal output
│   ├── progress.h          # CLI progress bar
│   ├── hmac.h              # HMAC-SHA256 integrity verification
│   ├── filehandler.h       # File utility helpers
│   ├── utils.h             # Secure memory wipe + path validation
│   └── CLI11.hpp           # Command line argument parser
└── src/
    ├── main.cpp            # Entry point, CLI routing
    ├── encryptor.cpp       # AES-256-CBC encryption implementation
    ├── decryptor.cpp       # AES-256-CBC decryption implementation
    ├── filehandler.cpp     # File existence checking
    └── utils.cpp           # SecureZeroMemory, path validation
```

---

## Tech Stack

| Technology | Purpose |
|---|---|
| C++17 | Core language |
| OpenSSL 3.x | AES-256-CBC, PBKDF2, HMAC-SHA256 |
| CLI11 | Command line argument parsing |
| CMake | Cross-platform build system |
| NSIS | Windows installer generation |
| std::filesystem | Recursive folder traversal |
| SecureZeroMemory | Password wiping from RAM |

---

## Security Design

| Threat | Mitigation |
|---|---|
| Brute force password attack | PBKDF2 with 100,000 iterations — 100,000x slower per attempt |
| Same password produces same key | Random 32-byte salt per file — every encryption is unique |
| File tampering / corruption | HMAC-SHA256 verified before decryption — rejects modified files |
| Password in RAM after use | SecureZeroMemory wipes password bytes immediately after use |
| IV reuse | Random 16-byte IV per file — prevents pattern analysis |

---

## Author

**Ravi Bhushan**
B.Tech CSE — CT Institute of Engineering Management and Technology, Punjab
GitHub: [@ravibhushan10](https://github.com/ravibhushan10)

---

## License

MIT License — free to use, modify, and distribute.
