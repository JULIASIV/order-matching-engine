// src/utils/Config.cpp
#include "Config.hpp"
#include "Logger.hpp"
#include <fstream>

namespace utils {

Config::Config(const std::string& configPath) {
    load(configPath);
}

void Config::load(const std::string& configPath) {
    try {
        root_ = YAML::LoadFile(configPath);
        LOG_INFO("Configuration loaded from: {}", configPath);
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to load configuration from {}: {}", configPath, e.what());
        throw;
    }
}

template<typename T>
T Config::get(const std::string& key, const T& defaultValue) const {
    return getImpl<T>(key, defaultValue);
}

template<typename T>
std::vector<T> Config::getVector(const std::string& key, const std::vector<T>& defaultValue) const {
    try {
        if (!root_[key]) {
            return defaultValue;
        }
        
        std::vector<T> result;
        for (const auto& node : root_[key]) {
            result.push_back(node.as<T>());
        }
        return result;
    } catch (const YAML::Exception& e) {
        LOG_WARNING("Failed to parse vector config key '{}': {}", key, e.what());
        return defaultValue;
    }
}

bool Config::has(const std::string& key) const {
    return root_[key].IsDefined();
}

// Template specializations
template<>
std::string Config::getImpl<std::string>(const std::string& key, const std::string& defaultValue) const {
    try {
        if (!root_[key]) {
            return defaultValue;
        }
        return root_[key].as<std::string>();
    } catch (const YAML::Exception& e) {
        LOG_WARNING("Failed to parse config key '{}': {}", key, e.what());
        return defaultValue;
    }
}

template<>
int Config::getImpl<int>(const std::string& key, const int& defaultValue) const {
    try {
        if (!root_[key]) {
            return defaultValue;
        }
        return root_[key].as<int>();
    } catch (const YAML::Exception& e) {
        LOG_WARNING("Failed to parse config key '{}': {}", key, e.what());
        return defaultValue;
    }
}

template<>
double Config::getImpl<double>(const std::string& key, const double& defaultValue) const {
    try {
        if (!root_[key]) {
            return defaultValue;
        }
        return root_[key].as<double>();
    } catch (const YAML::Exception& e) {
        LOG_WARNING("Failed to parse config key '{}': {}", key, e.what());
        return defaultValue;
    }
}

template<>
bool Config::getImpl<bool>(const std::string& key, const bool& defaultValue) const {
    try {
        if (!root_[key]) {
            return defaultValue;
        }
        return root_[key].as<bool>();
    } catch (const YAML::Exception& e) {
        LOG_WARNING("Failed to parse config key '{}': {}", key, e.what());
        return defaultValue;
    }
}

// Explicit template instantiations
template std::string Config::get<std::string>(const std::string&, const std::string&) const;
template int Config::get<int>(const std::string&, const int&) const;
template double Config::get<double>(const std::string&, const double&) const;
template bool Config::get<bool>(const std::string&, const bool&) const;

template std::vector<std::string> Config::getVector<std::string>(const std::string&, const std::vector<std::string>&) const;
template std::vector<int> Config::getVector<int>(const std::string&, const std::vector<int>&) const;

} // namespace utils