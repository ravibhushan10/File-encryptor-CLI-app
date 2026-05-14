// main.cpp
// fenc — File Encryptor CLI

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "CLI11.hpp"
#include "encryptor.h"
#include "decryptor.h"
#include "utils.h"
#include "color.h"

#define VERSION "3.0.0"

void printHelp() {
    std::cout << "\n";
    std::cout << Color::BOLD_WHITE << "fenc" << Color::RESET
              << " v" << VERSION << "  File Encryptor - AES-256\n";
    std::cout << Color::DIM << "Build by Ravi Bhushan\n" << Color::RESET;
    std::cout << "\n";

    std::cout << Color::BOLD_WHITE << "USAGE\n" << Color::RESET;
    std::cout << "  fenc <command> -f <file> -p <password> [options]\n";
    std::cout << "\n";

    std::cout << Color::BOLD_WHITE << "ENCRYPT\n" << Color::RESET;
    std::cout << Color::GREEN << "  enc        " << Color::RESET
              << "  Encrypt a file      "
              << Color::DIM << "fenc enc -f secret.txt -p mypass\n" << Color::RESET;
    std::cout << Color::GREEN << "  enc-batch  " << Color::RESET
              << "  Encrypt multiple    "
              << Color::DIM << "fenc enc-batch -f a.txt b.txt -p mypass\n" << Color::RESET;
    std::cout << Color::GREEN << "  enc-folder " << Color::RESET
              << "  Encrypt a folder    "
              << Color::DIM << "fenc enc-folder -d ./docs -p mypass\n" << Color::RESET;
    std::cout << "\n";

    std::cout << Color::BOLD_WHITE << "DECRYPT\n" << Color::RESET;
    std::cout << Color::YELLOW << "  dec        " << Color::RESET
              << "  Decrypt a file      "
              << Color::DIM << "fenc dec -f secret.txt.enc -p mypass\n" << Color::RESET;
    std::cout << Color::YELLOW << "  dec-batch  " << Color::RESET
              << "  Decrypt multiple    "
              << Color::DIM << "fenc dec-batch -f a.enc b.enc -p mypass\n" << Color::RESET;
    std::cout << Color::YELLOW << "  dec-folder " << Color::RESET
              << "  Decrypt a folder    "
              << Color::DIM << "fenc dec-folder -d ./docs -p mypass\n" << Color::RESET;
    std::cout << "\n";

    std::cout << Color::BOLD_WHITE << "OPTIONS\n" << Color::RESET;
    std::cout << "  -o, --output    Custom output path\n";
    std::cout << "  -v, --verbose   Show detailed steps\n";
    std::cout << "  -V, --version   Show version\n";
    std::cout << "  -h, --help      Show this message\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {

    if (argc < 2) { printHelp(); return 0; }

    std::string first = argv[1];

    if (first == "--help" || first == "-h") { printHelp(); return 0; }

    if (first == "--version" || first == "-V") {
        std::cout << Color::BOLD_WHITE << "fenc" << Color::RESET
                  << " v" << VERSION << "\n";
        std::cout << Color::DIM
                  << "  algorithm  : AES-256-CBC\n"
                  << "  integrity  : HMAC-SHA256\n"
                  << "  kdf        : PBKDF2 (100,000 iterations)\n"
                  << Color::RESET;
        return 0;
    }

    CLI::App app{"fenc"};
    app.require_subcommand(1);
    app.set_help_flag("--cli-help");

    // ── enc ─────────────────────────────────────────────────────────
    auto enc = app.add_subcommand("enc", "Encrypt a single file");
    std::string enc_file, enc_pass, enc_out;
    bool enc_verbose = false;
    enc->add_option("-f,--file",     enc_file,    "Input file")->required()->check(CLI::ExistingFile);
    enc->add_option("-p,--password", enc_pass,    "Password")->required();
    enc->add_option("-o,--output",   enc_out,     "Output path");
    enc->add_flag ("-v,--verbose",   enc_verbose, "Verbose");
    enc->set_help_flag("--cli-help");
    enc->callback([&]() {
        EncryptOptions opts; opts.verbose = enc_verbose; opts.output = enc_out;
        encryptFile(enc_file, enc_pass, opts);
        secureWipeString(enc_pass);
    });

    // ── dec ─────────────────────────────────────────────────────────
    auto dec = app.add_subcommand("dec", "Decrypt a single .enc file");
    std::string dec_file, dec_pass, dec_out;
    bool dec_verbose = false;
    dec->add_option("-f,--file",     dec_file,    "Encrypted file")->required()->check(CLI::ExistingFile);
    dec->add_option("-p,--password", dec_pass,    "Password")->required();
    dec->add_option("-o,--output",   dec_out,     "Output path");
    dec->add_flag ("-v,--verbose",   dec_verbose, "Verbose");
    dec->set_help_flag("--cli-help");
    dec->callback([&]() {
        DecryptOptions opts; opts.verbose = dec_verbose; opts.output = dec_out;
        decryptFile(dec_file, dec_pass, opts);
        secureWipeString(dec_pass);
    });

    // ── enc-folder ──────────────────────────────────────────────────
    auto enc_folder = app.add_subcommand("enc-folder", "Encrypt all files in a folder");
    std::string ef_dir, ef_pass;
    bool ef_verbose = false;
    enc_folder->add_option("-d,--folder",   ef_dir,     "Folder path")->required()->check(CLI::ExistingDirectory);
    enc_folder->add_option("-p,--password", ef_pass,    "Password")->required();
    enc_folder->add_flag ("-v,--verbose",   ef_verbose, "Verbose");
    enc_folder->set_help_flag("--cli-help");
    enc_folder->callback([&]() {
        EncryptOptions opts; opts.verbose = ef_verbose;
        encryptFolder(ef_dir, ef_pass, opts);
        secureWipeString(ef_pass);
    });

    // ── dec-folder ──────────────────────────────────────────────────
    auto dec_folder = app.add_subcommand("dec-folder", "Decrypt all .enc files in a folder");
    std::string df_dir, df_pass;
    bool df_verbose = false;
    dec_folder->add_option("-d,--folder",   df_dir,     "Folder path")->required()->check(CLI::ExistingDirectory);
    dec_folder->add_option("-p,--password", df_pass,    "Password")->required();
    dec_folder->add_flag ("-v,--verbose",   df_verbose, "Verbose");
    dec_folder->set_help_flag("--cli-help");
    dec_folder->callback([&]() {
        DecryptOptions opts; opts.verbose = df_verbose;
        decryptFolder(df_dir, df_pass, opts);
        secureWipeString(df_pass);
    });

    // ── enc-batch ───────────────────────────────────────────────────
    auto enc_batch = app.add_subcommand("enc-batch", "Encrypt multiple files at once");
    std::vector<std::string> eb_files;
    std::string eb_pass;
    bool eb_verbose = false;
    enc_batch->add_option("-f,--files",    eb_files,   "Files to encrypt")->required()->expected(-1);
    enc_batch->add_option("-p,--password", eb_pass,    "Password")->required();
    enc_batch->add_flag ("-v,--verbose",   eb_verbose, "Verbose");
    enc_batch->set_help_flag("--cli-help");
    enc_batch->callback([&]() {
        EncryptOptions opts; opts.verbose = eb_verbose;
        encryptBatch(eb_files, eb_pass, opts);
        secureWipeString(eb_pass);
    });

    // ── dec-batch ───────────────────────────────────────────────────
    auto dec_batch = app.add_subcommand("dec-batch", "Decrypt multiple .enc files at once");
    std::vector<std::string> db_files;
    std::string db_pass;
    bool db_verbose = false;
    dec_batch->add_option("-f,--files",    db_files,   "Files to decrypt")->required()->expected(-1);
    dec_batch->add_option("-p,--password", db_pass,    "Password")->required();
    dec_batch->add_flag ("-v,--verbose",   db_verbose, "Verbose");
    dec_batch->set_help_flag("--cli-help");
    dec_batch->callback([&]() {
        DecryptOptions opts; opts.verbose = db_verbose;
        decryptBatch(db_files, db_pass, opts);
        secureWipeString(db_pass);
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}
