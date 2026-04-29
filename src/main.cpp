#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "markdown_translator.hpp"
#include "file_uploader.hpp"
#include "rclone_uploader.hpp"

bool parseArguments(int argc, char* argv[], std::unordered_map<std::string, std::string>& params, std::string& inputFile) {
    if (argc < 2) {
        std::cerr << "Usage: COMMAND <input_file> [-o <output_file>] [-css <css_file_path>] [--encode <passphrase>] [other options]\n";
        return false;
    }
    params["-o"] = "index.html";
    params["-css"] = "styles/ffxiv-style.css";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            if (i + 1 < argc) {
                params[arg] = argv[i + 1];
                ++i;
            } else {
                std::cerr << "Error: Missing value for option " << arg << "\n";
                return false;
            }
        } else {
            if (inputFile.empty()) {
                inputFile = arg;
            } else {
                std::cerr << "Error: Multiple input files specified: " << inputFile << " and " << arg << "\n";
                return false;
            }
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        return false;
    }

    return true;
}

// Encrypt using AES-256-GCM with PBKDF2-HMAC-SHA256 key derivation.
static std::string base64Encode(const std::vector<unsigned char>& data) {
    if (data.empty()) return std::string();
    size_t out_len = 4 * ((data.size() + 2) / 3) + 1;
    std::vector<unsigned char> out(out_len);
    int written = EVP_EncodeBlock(out.data(), data.data(), (int)data.size());
    return std::string(reinterpret_cast<char*>(out.data()), written);
}

static std::vector<unsigned char> encryptAESGCM(const std::string& plaintext, const std::string& passphrase) {
    const int SALT_LEN = 16;
    const int IV_LEN = 12;
    const int TAG_LEN = 16;
    const int KEY_LEN = 32;
    const int PBKDF2_ITER = 100000;

    std::vector<unsigned char> salt(SALT_LEN);
    if (RAND_bytes(salt.data(), SALT_LEN) != 1) {
        throw std::runtime_error("RAND_bytes for salt failed");
    }

    std::vector<unsigned char> iv(IV_LEN);
    if (RAND_bytes(iv.data(), IV_LEN) != 1) {
        throw std::runtime_error("RAND_bytes for iv failed");
    }

    std::vector<unsigned char> key(KEY_LEN);
    if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), (int)passphrase.size(), salt.data(), SALT_LEN, PBKDF2_ITER, EVP_sha256(), KEY_LEN, key.data()) != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    int rc = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    if (rc != 1) { EVP_CIPHER_CTX_free(ctx); throw std::runtime_error("EVP_EncryptInit_ex failed"); }
    rc = EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), iv.data());
    if (rc != 1) { EVP_CIPHER_CTX_free(ctx); throw std::runtime_error("EVP_EncryptInit_ex set key/iv failed"); }

    std::vector<unsigned char> ciphertext(plaintext.size());
    int outlen = 0;
    rc = EVP_EncryptUpdate(ctx, ciphertext.data(), &outlen, reinterpret_cast<const unsigned char*>(plaintext.data()), (int)plaintext.size());
    if (rc != 1) { EVP_CIPHER_CTX_free(ctx); throw std::runtime_error("EVP_EncryptUpdate failed"); }
    int tmplen = 0;
    rc = EVP_EncryptFinal_ex(ctx, ciphertext.data() + outlen, &tmplen);
    if (rc != 1) { EVP_CIPHER_CTX_free(ctx); throw std::runtime_error("EVP_EncryptFinal_ex failed"); }
    int cipher_len = outlen + tmplen;
    ciphertext.resize(cipher_len);

    std::vector<unsigned char> tag(TAG_LEN);
    rc = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag.data());
    if (rc != 1) { EVP_CIPHER_CTX_free(ctx); throw std::runtime_error("EVP_CIPHER_CTX_ctrl GET_TAG failed"); }

    EVP_CIPHER_CTX_free(ctx);
    std::vector<unsigned char> out;
    out.reserve(salt.size() + iv.size() + ciphertext.size() + tag.size());
    out.insert(out.end(), salt.begin(), salt.end());
    out.insert(out.end(), iv.begin(), iv.end());
    out.insert(out.end(), ciphertext.begin(), ciphertext.end());
    out.insert(out.end(), tag.begin(), tag.end());
    return out;
}

int main(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> params;
    std::string inputFile;

    // FileUploader* fileupload;
    // fileupload = new RcloneUploader("", "r2");
    // std::cout << fileupload->testConnection() << std::endl;

    if (!parseArguments(argc, argv, params, inputFile)) {
        return 1;
    }

    std::ifstream file(inputFile);
    if (!file) {
        std::cerr << "Error: Could not open file " << inputFile << "\n";
        return 1;
    }

    // Read entire file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string markdownContent = buffer.str();
    file.close();

    // Create translator and convert markdown to HTML
    MarkdownTranslator translator;

    // Get CSS path from parameters or use default
    std::string cssPath = "";
    if (params.find("-css") != params.end()) {
        cssPath = params["-css"];
    }

    std::string htmlOutput = translator.translate(markdownContent);

    if (params.find("--encode") != params.end() && !params["--encode"].empty()) {
        try {
            std::string startMarker = "    <div class=\"nav-sidebar\">";
            size_t startPos = htmlOutput.find(startMarker);
            if (startPos != std::string::npos) {
                const std::string endMarker = "<!-- WIKI_CONTENT_END -->";
                size_t endPos = htmlOutput.find(endMarker, startPos);
                if (endPos != std::string::npos) {
                    std::string contentBlock = htmlOutput.substr(startPos, endPos - startPos);

                    std::vector<unsigned char> encrypted = encryptAESGCM(contentBlock, params["--encode"]);
                    std::string b64 = base64Encode(encrypted);

                    std::stringstream repl;
                    repl << "<div id=\"lock-screen\">\n";
                    repl << "  <div id=\"lock-box\">\n";
                    repl << "    <div class=\"lock-icon\">&#128274;</div>\n";
                    repl << "    <h2>Encoded Content</h2>\n";
                    repl << "    <p>Enter the passphrase to decode this post.</p>\n";
                    repl << "    <input id=\"enc-input\" type=\"password\" placeholder=\"Enter passphrase...\"/>\n";
                    repl << "  </div>\n";
                    repl << "</div>\n";
                    repl << "<div id=\"encrypted-content\" data-enc=\"" << b64 << "\"></div>\n";
                    repl << "<script>\n";
                    repl << "(function(){\n";
                    repl << "function b64ToArr(b64){var bin=atob(b64);var len=bin.length;var arr=new Uint8Array(len);for(var i=0;i<len;i++)arr[i]=bin.charCodeAt(i);return arr;}\n";
                    repl << "async function tryDecrypt(pass){try{var c=document.getElementById('encrypted-content');if(!c)return;var data=b64ToArr(c.dataset.enc);var salt=data.slice(0,16);var iv=data.slice(16,28);var tag=data.slice(data.length-16);var ct=data.slice(28,data.length-16);var ek=new TextEncoder().encode(pass);var km=await crypto.subtle.importKey('raw',ek,{name:'PBKDF2'},false,['deriveKey']);var key=await crypto.subtle.deriveKey({name:'PBKDF2',salt:salt,iterations:100000,hash:'SHA-256'},km,{name:'AES-GCM',length:256},false,['decrypt']);var full=new Uint8Array(ct.length+tag.length);full.set(ct,0);full.set(tag,ct.length);var plain=await crypto.subtle.decrypt({name:'AES-GCM',iv:iv},key,full);var decoded=new TextDecoder().decode(plain);var ls=document.getElementById('lock-screen');if(ls)ls.remove();c.outerHTML=decoded;if(window.Prism)Prism.highlightAll();}catch(e){}}\n";
                    repl << "document.addEventListener('DOMContentLoaded',function(){var inp=document.getElementById('enc-input');if(inp)inp.addEventListener('input',function(e){tryDecrypt(e.target.value);});});\n";
                    repl << "})();\n";
                    repl << "</script>\n";
                    htmlOutput = htmlOutput.substr(0, startPos) + repl.str()
                                 + htmlOutput.substr(endPos + endMarker.length());
                }
            }
        } catch (const std::exception& ex) {
        }
    }

    // Always strip the WIKI_CONTENT_END marker when not encrypted
    {
        const std::string marker = "<!-- WIKI_CONTENT_END -->";
        size_t pos = htmlOutput.find(marker);
        if (pos != std::string::npos)
            htmlOutput.erase(pos, marker.length());
    }

    // Write to output file
    std::string outputFile = params["-o"];
    std::ofstream outFile(outputFile);
    if (!outFile) {
        std::cerr << "Error: Could not open output file " << outputFile << "\n";
        return 1;
    }

    outFile << htmlOutput;
    outFile.close();

    std::cout << "HTML output written to " << outputFile << std::endl;
    return 0;
}
