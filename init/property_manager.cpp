#include "property_manager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>

namespace minimal_systems {
namespace init {

PropertyManager& PropertyManager::instance() {
    static PropertyManager instance;
    return instance;
}

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

std::string PropertyManager::get(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }
    return defaultValue;
}

void PropertyManager::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(property_mutex);

    properties[key] = value;
}

const std::unordered_map<std::string, std::string>& PropertyManager::getAllProperties() const {
    std::lock_guard<std::mutex> lock(property_mutex);

    return properties;
}

} // namespace init
} // namespace minimal_systems
