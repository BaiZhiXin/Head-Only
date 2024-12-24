#include <iostream>
#include <fstream>
#include <mutex>
#include <yaml-cpp/yaml.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class YAMLConfig {
public:
    // 构造函数，指定配置文件路径
    YAMLConfig(const std::string& filename) : filename_(filename) {
        load();
    }

    // 读取配置文件中的指定键值，返回类型为 T
    template <typename T>
    T read(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config_[key]) {
            throw std::runtime_error("Key '" + key + "' not found in config file.");
        }
        return config_[key].as<T>();
    }

    // 写入配置文件中的指定键值
    template <typename T>
    void write(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value;
        save();
    }

    // 读取整个配置文件并打印（调试用）
    void print() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << config_ << std::endl;
    }

private:
    // 加载 YAML 配置文件
    void load() {
        try {
            config_ = YAML::LoadFile(filename_);
        } catch (const YAML::Exception& e) {
            std::cerr << "Error loading YAML file: " << e.what() << std::endl;
            config_ = YAML::Node();
        }
    }

    // 保存当前配置到 YAML 文件
    void save() {
        std::ofstream fout(filename_);
        if (!fout.is_open()) {
            throw std::runtime_error("Error opening file for writing: " + filename_);
        }
        fout << config_;
    }

private:
    std::string filename_;      // 配置文件路径
    YAML::Node config_;         // 存储 YAML 配置内容
    std::mutex mutex_;          // 用于同步文件访问
};