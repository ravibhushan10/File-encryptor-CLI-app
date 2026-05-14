// main.cpp
// v3.0 — all features: verbose, version, output, metadata, batch mode

#include <iostream>
#include "CLI11.hpp"
#include "encryptor.h"
#include "decryptor.h"
#include "utils.h"
#include "color.h"

#define VERSION "3.0.0"

int main(int argc, char* argv[]) {

    // ── --version flag (check BEFORE app runs) ──────────────────────
    // WHY: CLI11 requires a subcommand, so --version must be caught early
    // HOW: manually scan argv[] before CLI11 processes anything
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--version" || std::string(argv[i]) == "-V") {
            std::cout << Color::BOLD_CYAN << "FileEncryptor v" << VERSION << Color::RESET << "\n";
            std::cout << "  Algorithm : AES-256-CBC\n";
            std::cout << "  Integrity : HMAC-SHA256\n";
            std::cout << "  KDF       : PBKDF2 (100,000 iterations)\n";
            return 0;
        }
    }

    // ── Banner (ASCII only — avoids encoding issues on Windows terminals) ──
    // WHY ASCII: box-drawing unicode chars like ╔ render as garbage in MSYS2
    std::cout << Color::BOLD_CYAN
              << "\n+======================================+\n"
              << "|     FileEncryptor  v3.0              |\n"
              << "|     AES-256-CBC + HMAC-SHA256        |\n"
              << "|     by Ravi Bhushan                  |\n"
              << "+======================================+\n"
              << Color::RESET << "\n";

    CLI::App app{"FileEncryptor - AES-256 File Encryption Tool", "fileencryptor"};
    app.require_subcommand(1);

    // ── encrypt (single file) ───────────────────────────────────────
    auto enc = app.add_subcommand("encrypt", "Encrypt a single file using AES-256");
    std::string enc_file, enc_pass, enc_out;
    bool enc_verbose = false;
    enc->add_option("--file,-f",     enc_file,    "Input file path")->required()->check(CLI::ExistingFile);
    enc->add_option("--password,-p", enc_pass,    "Encryption password")->required();
    enc->add_option("--output,-o",   enc_out,     "Output file path (default: input.enc)");
    enc->add_flag ("--verbose,-v",   enc_verbose, "Print each internal step");
    enc->callback([&]() {
        EncryptOptions opts;
        opts.verbose = enc_verbose;
        opts.output  = enc_out;
        encryptFile(enc_file, enc_pass, opts);
        secureWipeString(enc_pass);
    });

    // ── decrypt (single file) ───────────────────────────────────────
    auto dec = app.add_subcommand("decrypt", "Decrypt a single .enc file");
    std::string dec_file, dec_pass, dec_out;
    bool dec_verbose = false;
    dec->add_option("--file,-f",     dec_file,    "Encrypted .enc file path")->required()->check(CLI::ExistingFile);
    dec->add_option("--password,-p", dec_pass,    "Decryption password")->required();
    dec->add_option("--output,-o",   dec_out,     "Output file path (default: restored from metadata)");
    dec->add_flag ("--verbose,-v",   dec_verbose, "Print each internal step");
    dec->callback([&]() {
        DecryptOptions opts;
        opts.verbose = dec_verbose;
        opts.output  = dec_out;
        decryptFile(dec_file, dec_pass, opts);
        secureWipeString(dec_pass);
    });

    // ── encrypt-folder ──────────────────────────────────────────────
    auto enc_folder = app.add_subcommand("encrypt-folder", "Encrypt all files in a folder");
    std::string ef_folder, ef_pass;
    bool ef_verbose = false;
    enc_folder->add_option("--folder,-d",   ef_folder,  "Folder path")->required()->check(CLI::ExistingDirectory);
    enc_folder->add_option("--password,-p", ef_pass,    "Encryption password")->required();
    enc_folder->add_flag ("--verbose,-v",   ef_verbose, "Print each internal step");
    enc_folder->callback([&]() {
        EncryptOptions opts;
        opts.verbose = ef_verbose;
        encryptFolder(ef_folder, ef_pass, opts);
        secureWipeString(ef_pass);
    });

    // ── decrypt-folder ──────────────────────────────────────────────
    auto dec_folder = app.add_subcommand("decrypt-folder", "Decrypt all .enc files in a folder");
    std::string df_folder, df_pass;
    bool df_verbose = false;
    dec_folder->add_option("--folder,-d",   df_folder,  "Folder path")->required()->check(CLI::ExistingDirectory);
    dec_folder->add_option("--password,-p", df_pass,    "Decryption password")->required();
    dec_folder->add_flag ("--verbose,-v",   df_verbose, "Print each internal step");
    dec_folder->callback([&]() {
        DecryptOptions opts;
        opts.verbose = df_verbose;
        decryptFolder(df_folder, df_pass, opts);
        secureWipeString(df_pass);
    });

    // ── encrypt-batch ───────────────────────────────────────────────
    auto enc_batch = app.add_subcommand("encrypt-batch", "Encrypt multiple files at once");
    std::vector<std::string> eb_files;
    std::string eb_pass;
    bool eb_verbose = false;
    enc_batch->add_option("--files,-f",    eb_files,   "List of files to encrypt")->required()->expected(-1);
    enc_batch->add_option("--password,-p", eb_pass,    "Encryption password")->required();
    enc_batch->add_flag ("--verbose,-v",   eb_verbose, "Print each internal step");
    enc_batch->callback([&]() {
        EncryptOptions opts;
        opts.verbose = eb_verbose;
        encryptBatch(eb_files, eb_pass, opts);
        secureWipeString(eb_pass);
    });

    // ── decrypt-batch ───────────────────────────────────────────────
    auto dec_batch = app.add_subcommand("decrypt-batch", "Decrypt multiple .enc files at once");
    std::vector<std::string> db_files;
    std::string db_pass;
    bool db_verbose = false;
    dec_batch->add_option("--files,-f",    db_files,   "List of .enc files to decrypt")->required()->expected(-1);
    dec_batch->add_option("--password,-p", db_pass,    "Decryption password")->required();
    dec_batch->add_flag ("--verbose,-v",   db_verbose, "Print each internal step");
    dec_batch->callback([&]() {
        DecryptOptions opts;
        opts.verbose = db_verbose;
        decryptBatch(db_files, db_pass, opts);
        secureWipeString(db_pass);
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}
