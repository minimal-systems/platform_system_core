#include "property_manager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>

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
            std::cerr << "Failed to open property file: " << propertyFile << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream lineStream(line);
            std::string key, value;
            if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
                properties[key] = value;
            }
        }
    }

    // Add default properties if not overridden
    properties["ro.boot.hardware"] = "generic";
    properties["ro.boot.mode"] = "normal";
}

// Save properties to file
void PropertyManager::saveProperties(const std::string& propertyFile) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (propertyFile.empty()) {
        return;
    }

    std::ofstream file(propertyFile, std::ios::trunc);
    if (!file) {
        std::cerr << "Failed to open property file for writing: " << propertyFile << std::endl;
        return;
    }

    for (const auto& [key, value] : properties) {
        file << key << "=" << value << "\n";
    }
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

// Simplified setprop method
void PropertyManager::setprop(const std::string& key, const std::string& value) {
    set(key, value);
    std::cout << "Property set: " << key << " = " << value << std::endl;
}

// Simplified getprop method
std::string PropertyManager::getprop(const std::string& key) const {
    auto value = get(key, "");
    std::cout << "Property get: " << key << " = " << value << std::endl;
    return value;
}

} // namespace init
} // namespace minimal_systems
