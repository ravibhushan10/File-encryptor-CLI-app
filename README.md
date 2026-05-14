# fenc — File Encryptor CLI

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
| [fencSetup.exe](https://github.com/ravibhushan10/File-encryptor-CLI-app/releases/download/v3.0.0/fenc-v3.0.0-windows.exe) | ~3.5 MB | Windows installer — includes all required DLLs |

**Install:** Double-click `fencSetup.exe` → follow wizard → `fenc` is ready in any terminal.

> After install, open CMD, PowerShell, Git Bash, or Windows Terminal and type `fenc --help`. No restart needed.

---

## Features

- **AES-256-CBC Encryption** — industry-standard symmetric encryption
- **PBKDF2-HMAC-SHA256 Key Derivation** — 100,000 iterations, unique salt per file
- **HMAC-SHA256 Integrity Verification** — detects tampering before decryption
- **Metadata Header** — stores original filename, timestamp, and algorithm inside the encrypted file
- **Batch Mode** — encrypt/decrypt multiple files in one command
- **Folder Mode** — recursively encrypt/decrypt entire directories
- **Progress Bar** — shown only for large files (>1MB), no noise for small files
- **Verbose Mode** — prints every internal step (salt, IV, key derivation, HMAC)
- **Custom Output Path** — `-o` flag for full control over output location
- **Secure Memory Wiping** — passwords zeroed from RAM immediately after use
- **Windows Installer** — built with NSIS, adds `fenc` to system PATH automatically

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
```
On decrypt: salt and IV are read first, key is re-derived, HMAC is verified — then decryption begins.

**Metadata (encrypted inside ciphertext):**
```
[ MAGIC (8) | Version (1) | Algorithm (1) | Timestamp (8) | FilenameLen (2) | Filename ]
```
Original filename and timestamp are restored automatically on decrypt.

---

## Usage

```
fenc <command> -f <file> -p <password> [options]
```

### Encrypt a file
```cmd
fenc enc -f secret.txt -p mypassword
```

### Decrypt a file
```cmd
fenc dec -f secret.txt.enc -p mypassword
```

### Encrypt with verbose output
```cmd
fenc enc -f secret.txt -p mypassword -v
```

### Custom output path
```cmd
fenc enc -f secret.txt -p mypassword -o out\secret.enc
fenc dec -f out\secret.enc -p mypassword -o restored.txt
```

### Batch encrypt multiple files
```cmd
fenc enc-batch -f a.txt b.txt c.txt -p mypassword
```

### Batch decrypt multiple files
```cmd
fenc dec-batch -f a.txt.enc b.txt.enc c.txt.enc -p mypassword
```

### Encrypt entire folder
```cmd
fenc enc-folder -d C:\SensitiveData -p mypassword
```

### Decrypt entire folder
```cmd
fenc dec-folder -d C:\SensitiveData -p mypassword
```

### Version info
```cmd
fenc --version
```

---

## Demo

```
$ fenc --help

fenc v3.0.0  File Encryptor - AES-256
by Ravi Bhushan

USAGE
  fenc <command> -f <file> -p <password> [options]

ENCRYPT
  enc          Encrypt a file      fenc enc -f secret.txt -p mypass
  enc-batch    Encrypt multiple    fenc enc-batch -f a.txt b.txt -p mypass
  enc-folder   Encrypt a folder    fenc enc-folder -d ./docs -p mypass

DECRYPT
  dec          Decrypt a file      fenc dec -f secret.txt.enc -p mypass
  dec-batch    Decrypt multiple    fenc dec-batch -f a.enc b.enc -p mypass
  dec-folder   Decrypt a folder    fenc dec-folder -d ./docs -p mypass

OPTIONS
  -o, --output    Custom output path
  -v, --verbose   Show detailed steps
  -V, --version   Show version
  -h, --help      Show this message


$ fenc enc -f secret.txt -p mypass
  done  secret.txt -> secret.txt.enc

$ fenc dec -f secret.txt.enc -p mypass
integrity check passed
  done  secret.txt.enc -> secret.txt

$ fenc dec -f secret.txt.enc -p wrongpass
error: integrity check failed -- wrong password or file tampered
```

---

## Build From Source

**Requirements:**
- MSYS2 UCRT64
- GCC 16+ (`mingw-w64-ucrt-x86_64-gcc`)
- CMake 3.15+ (`mingw-w64-ucrt-x86_64-cmake`)
- OpenSSL 3.x (`mingw-w64-ucrt-x86_64-openssl`)
- NSIS 3.x — for installer only

```bash
# Clone
git clone https://github.com/ravibhushan10/File-encryptor-CLI-app.git
cd File-encryptor-CLI-app

# Build
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=/ucrt64/bin/mingw32-make
cmake --build .

# Run
./fenc.exe --help
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
│   ├── color.h             # Git-style colored terminal output
│   ├── progress.h          # CLI progress bar (large files only)
│   ├── hmac.h              # HMAC-SHA256 integrity verification
│   ├── filehandler.h       # File utility helpers
│   ├── utils.h             # Secure memory wipe + path validation
│   └── CLI11.hpp           # Command line argument parser
└── src/
    ├── main.cpp            # Entry point, CLI routing, help screen
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
| CMake | Build system |
| NSIS | Windows installer with PATH registration |
| std::filesystem | Recursive folder traversal |
| SecureZeroMemory | Password wiping from RAM |

---

## Security Design

| Threat | Mitigation |
|---|---|
| Brute force password attack | PBKDF2 with 100,000 iterations — 100,000x slower per attempt |
| Same password produces same key | Random 32-byte salt per file — every encryption is unique |
| File tampering or corruption | HMAC-SHA256 verified before decryption — rejects modified files |
| Password left in RAM | SecureZeroMemory wipes password bytes immediately after use |
| IV reuse | Random 16-byte IV per file — prevents pattern analysis |

---

## Author

**Ravi Bhushan**  
B.Tech CSE — CT Institute of Engineering Management and Technology, Punjab  
GitHub: [@ravibhushan10](https://github.com/ravibhushan10)

---

## License

MIT License — free to use, modify, and distribute.
