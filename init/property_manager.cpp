#include "property_manager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include "log_new.h"

namespace minimal_systems {
namespace init {

// Singleton instance
PropertyManager& PropertyManager::instance() {
    static PropertyManager instance;
    return instance;
}

// Load properties from file
void PropertyManager::loadProperties(const std::string& propertyFile) {
    std::lock_guard<std::mutex> lock(property_mutex);
    properties.clear();

    if (!propertyFile.empty()) {
        std::ifstream file(propertyFile);
        if (!file) {
            LOGE("Failed to open property file: %s", propertyFile.c_str());
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream lineStream(line);
            std::string key, value;
            if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
                properties[key] = value;
                LOGD("Loaded property from file: %s = %s", key.c_str(), value.c_str());
            }
        }
    }
}

// Save properties to file
void PropertyManager::saveProperties(const std::string& propertyFile) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (propertyFile.empty()) {
        return;
    }

    std::ofstream file(propertyFile, std::ios::trunc);
    if (!file) {
        LOGE("Failed to open property file for writing: %s", propertyFile.c_str());
        return;
    }

    for (const auto& [key, value] : properties) {
        file << key << "=" << value << "\n";
    }
    LOGI("Properties saved successfully to %s", propertyFile.c_str());
}

// Get a property value
std::string PropertyManager::get(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }
    return defaultValue;
}

// Set a property value
void PropertyManager::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(property_mutex);

    properties[key] = value;
}

// Get all properties
const std::unordered_map<std::string, std::string>& PropertyManager::getAllProperties() const {
    std::lock_guard<std::mutex> lock(property_mutex);

    return properties;
}

// Simplified setprop method using set
void PropertyManager::setprop(const std::string& key, const std::string& value) {
    set(key, value);
    LOGD("Property set: %s = %s", key.c_str(), value.c_str());
}

// Simplified getprop method using get
std::string PropertyManager::getprop(const std::string& key) const {
    auto value = get(key, "");
    LOGD("Property get: %s = %s", key.c_str(), value.c_str());
    return value;
}

// Global getprop function
std::string getprop(const std::string& key) {
    return PropertyManager::instance().getprop(key);
}

// Global setprop function
void setprop(const std::string& key, const std::string& value) {
    PropertyManager::instance().setprop(key, value);
}

} // namespace init
} // namespace minimal_systems
