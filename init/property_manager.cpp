#include "property_manager.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "log_new.h"

namespace minimal_systems {
namespace init {


#ifdef DEBUG_PROP
#define DEBUG_LOGD(...) LOGD(__VA_ARGS__)
#define DEBUG_LOGI(...) LOGI(__VA_ARGS__)
#define DEBUG_LOGE(...) LOGE(__VA_ARGS__)
#define DEBUG_LOGW(...) LOGW(__VA_ARGS__)
#else
#define DEBUG_LOGD(...)
#define DEBUG_LOGI(...)
#define DEBUG_LOGE(...)
#define DEBUG_LOGW(...)
#endif

// Singleton instance
PropertyManager& PropertyManager::instance() {
    static PropertyManager instance;
    return instance;
}

// Load properties from a file
void PropertyManager::loadProperties(const std::string& propertyFile) {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (propertyFile.empty()) {
        DEBUG_LOGW("Property file path is empty. Skipping.");
        return;
    }

    std::ifstream file(propertyFile);
    if (!file) {
        DEBUG_LOGE("Failed to open property file: %s", propertyFile.c_str());
        return;
    }

    DEBUG_LOGI("Loading properties from: %s", propertyFile.c_str());

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty or comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Trim whitespace (optional but recommended)
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        std::istringstream lineStream(line);
        std::string key, value;
        if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            properties[key] = value;  // overwrite if already exists
            DEBUG_LOGD("Loaded property: %s = %s", key.c_str(), value.c_str());
        }
    }

    DEBUG_LOGI("Finished loading properties from: %s", propertyFile.c_str());
}


// Save properties to a file
void PropertyManager::saveProperties(const std::string& propertyFile) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (propertyFile.empty()) {
        return;
    }

    std::ofstream file(propertyFile, std::ios::trunc);
    if (!file) {
        DEBUG_LOGE("Failed to open property file for writing: %s", propertyFile.c_str());
        return;
    }

    for (const auto& [key, value] : properties) {
        file << key << "=" << value << "\n";
    }
    DEBUG_LOGI("Properties saved successfully: %s", propertyFile.c_str());
}

// Reset a property (removes it from memory)
void PropertyManager::resetprop(const std::string& key) {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (properties.erase(key)) {
        DEBUG_LOGI("Property reset (removed from memory): %s", key.c_str());
    }

    if (persistentProperties.erase(key)) {
        persistentKeys.erase(key);
        syncPersistentProperties("./mnt/cache/property_persist.conf");
        DEBUG_LOGI("Persistent property reset (removed and synced): %s", key.c_str());
    }
}

// Load persistent properties
void PropertyManager::loadPersistentProperties(const std::string& persistentFile) {
    std::lock_guard<std::mutex> lock(property_mutex);
    persistentProperties.clear();
    persistentKeys.clear();

    if (!persistentFile.empty()) {
        std::ifstream file(persistentFile);
        if (!file) {
            DEBUG_LOGE("Failed to open persistent property file: %s", persistentFile.c_str());
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream lineStream(line);
            std::string key, value;
            if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
                persistentProperties[key] = value;
                persistentKeys.insert(key);
                DEBUG_LOGD("Loaded persistent property: %s = %s", key.c_str(), value.c_str());
            }
        }
    }
    DEBUG_LOGI("Persistent properties loaded successfully.");
}

// Save persistent properties to a file
void PropertyManager::savePersistentProperties(const std::string& persistentFile) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (persistentFile.empty()) {
        return;
    }

    std::ofstream file(persistentFile, std::ios::trunc);
    if (!file) {
        DEBUG_LOGE("Failed to open persistent property file for writing: %s",
                   persistentFile.c_str());
        return;
    }

    for (const auto& [key, value] : persistentProperties) {
        file << key << "=" << value << "\n";
    }
    DEBUG_LOGI("Persistent properties saved successfully.");
}

// Sync persistent properties to disk
void PropertyManager::syncPersistentProperties(const std::string& persistentFile) {
    DEBUG_LOGI("Syncing persistent properties...");
    savePersistentProperties(persistentFile);
}

// Get a property
std::string PropertyManager::get(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(property_mutex);

    if (persistentProperties.find(key) != persistentProperties.end()) {
        return persistentProperties.at(key);
    }

    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }

    return defaultValue;
}

// Set a property (also updates persistent properties if marked)
void PropertyManager::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(property_mutex);

    properties[key] = value;
    if (persistentKeys.find(key) != persistentKeys.end()) {
        persistentProperties[key] = value;
    }
}

// Global functions
std::string getprop(const std::string& key) {
    return PropertyManager::instance().getprop(key);
}

void setprop(const std::string& key, const std::string& value) {
    PropertyManager::instance().setprop(key, value);
}

void resetprop(const std::string& key) {
    PropertyManager::instance().resetprop(key);
}

const std::unordered_map<std::string, std::string>& PropertyManager::getAllProperties() const {
    return properties;
}

std::string PropertyManager::getprop(const std::string& key) const {
    return get(key, "");
}

void PropertyManager::setprop(const std::string& key, const std::string& value) {
    set(key, value);
}

}  // namespace init
}  // namespace minimal_systems
