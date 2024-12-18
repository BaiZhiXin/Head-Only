#ifndef ENCODINGCONVERTER_H
#define ENCODINGCONVERTER_H

#include <uchardet/uchardet.h>
#include <unicode/ucnv.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <regex>
#include <filesystem>
#include <future>
#include <algorithm>
#include <fstream>
#include <iostream>

/**
 * @brief EncodingConverter 类
 *
 * EncodingConverter 提供自动检测文本文件编码并转换为目标编码的功能。
 * 支持通过正则表达式过滤文件和源编码。
 * 使用 uchardet 来检测编码，ICU (ucnv) 来执行编码转换。
 *
 * 通过重写 protected 的 logMessage 方法，可以实现自定义日志输出。
 */
class EncodingConverter {
public:
    /**
    * @brief 日志级别枚举
    */
    enum class LogLevel { INFO, WARN, ERROR };

    /**
     * @brief 构造函数
     */
    EncodingConverter() = default;

    /**
     * @brief 将指定路径（文件或目录）中的文件转换为指定编码（异步多线程处理）
     * @param path 要处理的文件或目录路径
     * @param toEncoding 目标编码
     * @param sourceEncodingFilter 源编码过滤器（正则表达式），为空则不过滤
     * @param fileFilter 文件过滤规则（正则表达式），为空则不过滤
     */
    inline void convert(
        const std::string& path,
        const std::string& toEncoding,
        const std::string& sourceEncodingFilter = "",
        const std::string& fileFilter = ""
        ) {
        namespace fs = std::filesystem;
        fs::path inputPath(path);
        std::vector<std::future<void>> futures;

        std::string mappedToEncoding = mapEncodingName(toEncoding);
        if (mappedToEncoding.empty()) {
            logMessage(LogLevel::ERROR, "Unsupported target encoding: " + toEncoding);
            throw std::runtime_error("Unsupported target encoding: " + toEncoding);
        } else {
            logMessage(LogLevel::INFO, "Target Encoding Mapped: " + mappedToEncoding);
        }

        if (fs::is_directory(inputPath)) {
            logMessage(LogLevel::INFO, "Processing directory: " + inputPath.string());
            for (const auto& entry : fs::recursive_directory_iterator(inputPath)) {
                if (entry.is_regular_file()) {
                    std::string fileName = entry.path().filename().string();
                    if (shouldProcessFile(fileName, fileFilter)) {
                        futures.emplace_back(std::async(std::launch::async, [this, entry, mappedToEncoding, sourceEncodingFilter]() {
                            try {
                                convertFile(entry.path().string(), mappedToEncoding, sourceEncodingFilter);
                            }
                            catch (const std::exception& e) {
                                logMessage(LogLevel::ERROR, "Error converting " + entry.path().string() + ": " + e.what());
                            }
                        }));
                    }
                } else {
                    logMessage(LogLevel::WARN, "Skipping non-regular file: " + entry.path().string());
                }
            }
        } else if (fs::is_regular_file(inputPath)) {
            std::string fileName = inputPath.filename().string();
            if (shouldProcessFile(fileName, fileFilter)) {
                logMessage(LogLevel::INFO, "Processing single file: " + inputPath.string());
                try {
                    convertFile(inputPath.string(), mappedToEncoding, sourceEncodingFilter);
                }
                catch (const std::exception& e) {
                    logMessage(LogLevel::ERROR, "Error converting " + inputPath.string() + ": " + e.what());
                }
            } else {
                logMessage(LogLevel::WARN, "File does not match filter and will be skipped: " + inputPath.string());
            }
        } else {
            logMessage(LogLevel::ERROR, "Invalid path: " + path);
            throw std::runtime_error("Invalid path: " + path);
        }

        for (auto& future : futures) {
            try {
                future.get();
            }
            catch (const std::exception& e) {
                logMessage(LogLevel::ERROR, "Asynchronous task failed: " + std::string(e.what()));
            }
        }

        // logMessage(LogLevel::INFO, "Conversion process completed.");
    }

    /**
     * @brief 以单线程方式处理指定路径文件的转换（不使用异步）
     * @param path 路径（文件或目录）
     * @param toEncoding 目标编码
     * @param sourceEncodingFilter 源编码过滤器（可选）
     * @param fileFilter 文件过滤（可选）
     */
    inline void convert_single(
        const std::string& path,
        const std::string& toEncoding,
        const std::string& sourceEncodingFilter = "",
        const std::string& fileFilter = ""
        ) {
        namespace fs = std::filesystem;
        fs::path inputPath(path);

        std::string mappedToEncoding = mapEncodingName(toEncoding);
        if (mappedToEncoding.empty()) {
            logMessage(LogLevel::ERROR, "Unsupported target encoding: " + toEncoding);
            throw std::runtime_error("Unsupported target encoding: " + toEncoding);
        } else {
            logMessage(LogLevel::INFO, "Target Encoding Mapped: " + mappedToEncoding);
        }

        if (fs::is_directory(inputPath)) {
            logMessage(LogLevel::INFO, "Processing directory: " + inputPath.string());
            for (const auto& entry : fs::recursive_directory_iterator(inputPath)) {
                if (entry.is_regular_file()) {
                    std::string fileName = entry.path().filename().string();
                    if (shouldProcessFile(fileName, fileFilter)) {
                        try {
                            convertFile(entry.path().string(), mappedToEncoding, sourceEncodingFilter);
                        }
                        catch (const std::exception& e) {
                            logMessage(LogLevel::ERROR, "Error converting " + entry.path().string() + ": " + e.what());
                        }
                    }
                } else {
                    logMessage(LogLevel::WARN, "Skipping non-regular file: " + entry.path().string());
                }
            }
        } else if (fs::is_regular_file(inputPath)) {
            std::string fileName = inputPath.filename().string();
            if (shouldProcessFile(fileName, fileFilter)) {
                logMessage(LogLevel::INFO, "Processing single file: " + inputPath.string());
                try {
                    convertFile(inputPath.string(), mappedToEncoding, sourceEncodingFilter);
                }
                catch (const std::exception& e) {
                    logMessage(LogLevel::ERROR, "Error converting " + inputPath.string() + ": " + e.what());
                }
            } else {
                logMessage(LogLevel::WARN, "File does not match filter and will be skipped: " + inputPath.string());
            }
        } else {
            logMessage(LogLevel::ERROR, "Invalid path: " + path);
            throw std::runtime_error("Invalid path: " + path);
        }

        // logMessage(LogLevel::INFO, "Conversion process completed.");
    }

protected:
    /**
     * @brief 可重写的日志函数，子类可重写此函数实现自定义日志输出
     * @param level 日志级别
     * @param message 日志信息
     */
    virtual void logMessage(LogLevel level, const std::string& message) {
        std::string prefix;
        switch (level) {
        case LogLevel::INFO:
            prefix = "[INFO] ";
            break;
        case LogLevel::WARN:
            prefix = "[WARN] ";
            break;
        case LogLevel::ERROR:
            prefix = "[ERROR] ";
            break;
        }

        std::cerr << prefix << message << std::endl;
    }

private:
    inline void convertFile(const std::string& filePath, const std::string& toEncoding, const std::string& sourceEncodingFilter)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            logMessage(LogLevel::ERROR, "Failed to open file: " + filePath);
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        std::string fileContent(buffer.begin(), buffer.end());
        std::string detectedEncoding = detectEncoding(fileContent, sourceEncodingFilter);

        if (detectedEncoding == "UNKNOWN" || detectedEncoding == "MISMATCH") {
            logMessage(LogLevel::WARN, "Skipping file due to encoding issues: " + filePath);
            return;
        }

        std::string mappedDetectedEncoding = mapEncodingName(detectedEncoding);
        if (mappedDetectedEncoding.empty()) {
            logMessage(LogLevel::WARN, "Unsupported detected encoding '" + detectedEncoding + "' for file: " + filePath);
            return;
        }

        std::string convertedContent;
        try {
            convertEncoding(fileContent, mappedDetectedEncoding, toEncoding, convertedContent);
        }
        catch (const std::exception& e) {
            logMessage(LogLevel::ERROR, "Conversion failed for file: " + filePath + " | Error: " + e.what());
            throw;
        }

        std::ofstream outputFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outputFile.is_open()) {
            logMessage(LogLevel::ERROR, "Failed to open file for writing: " + filePath);
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        }
        outputFile.write(convertedContent.data(), convertedContent.size());
        outputFile.close();

        logMessage(LogLevel::INFO, filePath + " | " + mappedDetectedEncoding + " -> " + toEncoding);
    }

    inline std::string detectEncoding(const std::string& data, const std::string& encodingFilter = "")
    {
        uchardet_t ud = uchardet_new();
        if (ud == nullptr) {
            logMessage(LogLevel::ERROR, "Failed to initialize uchardet.");
            throw std::runtime_error("Failed to initialize uchardet.");
        }

        if (uchardet_handle_data(ud, data.data(), data.size()) != 0) {
            logMessage(LogLevel::ERROR, "Failed to handle data with uchardet.");
            uchardet_delete(ud);
            throw std::runtime_error("Failed to handle data with uchardet.");
        }

        uchardet_data_end(ud);
        const char* detectedCharset = uchardet_get_charset(ud);
        std::string result = detectedCharset ? detectedCharset : "UNKNOWN";
        uchardet_delete(ud);

        if (!encodingFilter.empty() && result != "UNKNOWN") {
            try {
                std::regex re("^(" + encodingFilter + ")$", std::regex_constants::ECMAScript | std::regex_constants::icase);
                if (!std::regex_match(result, re)) {
                    logMessage(LogLevel::WARN, "Detected encoding '" + result + "' does not match filter: '" + encodingFilter + "'.");
                    return "MISMATCH";
                }
            }
            catch (const std::regex_error& e) {
                logMessage(LogLevel::ERROR, "Invalid regex filter: " + encodingFilter + " | Error: " + e.what());
                throw std::runtime_error("Invalid regex filter: " + encodingFilter);
            }
        }

        return result;
    }

    inline void convertEncoding(const std::string& input, const std::string& fromEncoding, const std::string& toEncoding, std::string& output)
    {
        UErrorCode status = U_ZERO_ERROR;
        int32_t targetCapacity = static_cast<int32_t>(input.size()) * 4 + 1;
        std::vector<char> targetBuffer(targetCapacity);

        int32_t convertedSize = ucnv_convert(
            toEncoding.c_str(),
            fromEncoding.c_str(),
            targetBuffer.data(),
            targetCapacity,
            input.c_str(),
            static_cast<int32_t>(input.size()),
            &status
            );

        if (U_FAILURE(status)) {
            logMessage(LogLevel::ERROR, "ICU conversion failed: " + std::string(u_errorName(status)));
            throw std::runtime_error("ICU conversion failed: " + std::string(u_errorName(status)));
        }

        output.assign(targetBuffer.data(), convertedSize);
    }

    inline bool shouldProcessFile(const std::string& fileName, const std::string& filter)
    {
        if (filter.empty()) {
            return true;
        }

        try {
            std::string regexPattern = "^.*\\.(" + filter + ")$";
            std::regex re(regexPattern, std::regex_constants::ECMAScript | std::regex_constants::icase);
            bool isMatch = std::regex_match(fileName, re);

            if (!isMatch) {
                logMessage(LogLevel::WARN, "File MisMatch: " + fileName + " does not match filter: " + filter);
                return false;
            }
            return true;
        }
        catch (const std::regex_error& e) {
            logMessage(LogLevel::ERROR, "Invalid regex filter: " + filter + " | Error: " + e.what());
            throw std::runtime_error("Invalid regex filter: " + filter);
        }
    }

    inline std::string mapEncodingName(const std::string& encoding)
    {
        if (encoding.empty()) return encoding;

        std::string lowerEncoding = encoding;
        std::transform(lowerEncoding.begin(), lowerEncoding.end(), lowerEncoding.begin(), ::tolower);

        if (lowerEncoding == "windows-1252" || lowerEncoding == "windows1252") return "windows-1252";
        if (lowerEncoding == "iso-8859-1" || lowerEncoding == "iso8859-1") return "ISO-8859-1";
        if (lowerEncoding == "utf-8" || lowerEncoding == "utf8") return "UTF-8";
        if (lowerEncoding == "gbk") return "GBK";
        if (lowerEncoding == "gb18030") return "GB18030";
        if (lowerEncoding == "ascii") return "ASCII";
        if (lowerEncoding == "big5") return "Big5";
        if (lowerEncoding == "utf-16le" || lowerEncoding == "utf16le") return "UTF-16LE";
        if (lowerEncoding == "utf-16be" || lowerEncoding == "utf16be") return "UTF-16BE";

        logMessage(LogLevel::WARN, "Unknown encoding name encountered: " + encoding);
        return encoding;
    }
};

#endif // ENCODINGCONVERTER_H
