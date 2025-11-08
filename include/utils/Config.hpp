// include/utils/Config.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace utils {

class Config {
public:
    Config() = default;
    explicit Config(const std::string& configPath);
    
    void load(const std::string& configPath);
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;
    
    template<typename T>
    std::vector<T> getVector(const std::string& key, const std::vector<T>& defaultValue = {}) const;
    
    bool has(const std::string& key) const;
    
private:
    YAML::Node root_;
    
    template<typename T>
    T getImpl(const std::string& key, const T& defaultValue) const;
};

} // namespace utils