#ifndef SIGN_VERIFY
#define SIGN_VERIFY


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>

// 错误处理辅助函数
void handleOpenSSLErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

// 计算 SHA-256 哈希
std::vector<unsigned char> computeSHA256(const std::string& data) {
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    if (!SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash.data())) {
        std::cerr << "SHA256 computation failed." << std::endl;
        handleOpenSSLErrors();
    }
    return hash;
}

// 使用私钥签名哈希
std::vector<unsigned char> signHash(const std::vector<unsigned char>& hash, const std::string& privateKeyPath) {
    // 打开私钥文件
    std::ifstream keyFile(privateKeyPath, std::ios::binary);
    if (!keyFile.is_open()) {
        std::cerr << "Unable to open private key file: " << privateKeyPath << std::endl;
        exit(EXIT_FAILURE);
    }

    // 读取私钥
    std::stringstream keyStream;
    keyStream << keyFile.rdbuf();
    std::string keyStr = keyStream.str();
    BIO* bio = BIO_new_mem_buf(keyStr.c_str(), -1);
    EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    keyFile.close();

    if (!privateKey) {
        std::cerr << "Error reading private key." << std::endl;
        handleOpenSSLErrors();
    }

    // 创建签名上下文
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) handleOpenSSLErrors();

    if (EVP_SignInit(ctx, EVP_sha256()) != 1) handleOpenSSLErrors();
    if (EVP_SignUpdate(ctx, hash.data(), hash.size()) != 1) handleOpenSSLErrors();

    // 分配足够的内存存储签名
    unsigned int sigLen = EVP_PKEY_size(privateKey);
    std::vector<unsigned char> signature(sigLen);

    if (EVP_SignFinal(ctx, signature.data(), &sigLen, privateKey) != 1) handleOpenSSLErrors();
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(privateKey);

    // 调整签名大小
    signature.resize(sigLen);
    return signature;
}

// 使用公钥验证签名
bool verifySignature(const std::vector<unsigned char>& hash, const std::vector<unsigned char>& signature, const std::string& publicKeyPath) {
    // 打开公钥文件
    std::ifstream keyFile(publicKeyPath, std::ios::binary);
    if (!keyFile.is_open()) {
        std::cerr << "Unable to open public key file: " << publicKeyPath << std::endl;
        exit(EXIT_FAILURE);
    }

    // 读取公钥
    std::stringstream keyStream;
    keyStream << keyFile.rdbuf();
    std::string keyStr = keyStream.str();
    BIO* bio = BIO_new_mem_buf(keyStr.c_str(), -1);
    EVP_PKEY* publicKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    keyFile.close();

    if (!publicKey) {
        std::cerr << "Error reading public key." << std::endl;
        handleOpenSSLErrors();
    }

    // 创建验证上下文
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) handleOpenSSLErrors();

    if (EVP_VerifyInit(ctx, EVP_sha256()) != 1) handleOpenSSLErrors();
    if (EVP_VerifyUpdate(ctx, hash.data(), hash.size()) != 1) handleOpenSSLErrors();

    // 验证签名
    int result = EVP_VerifyFinal(ctx, signature.data(), signature.size(), publicKey);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(publicKey);

    if (result == 1) {
        std::cout << "Signature is valid." << std::endl;
        return true;
    } else if (result == 0) {
        std::cerr << "Signature verification failed." << std::endl;
        return false;
    } else {
        std::cerr << "Error during signature verification." << std::endl;
        handleOpenSSLErrors();
        return false;
    }
}


#endif
