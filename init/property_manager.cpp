#include "property_manager.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include "log_new.h"

namespace minimal_systems {
namespace init {

// Macro for conditional logging
#ifdef DEBUG_PROP
#define DEBUG_LOGD(...) LOGD(__VA_ARGS__)
#define DEBUG_LOGI(...) LOGI(__VA_ARGS__)
#define DEBUG_LOGE(...) LOGE(__VA_ARGS__)
#else
#define DEBUG_LOGD(...)  // No-op
#define DEBUG_LOGI(...)  // No-op
#define DEBUG_LOGE(...)  // No-op
#endif

// Singleton instance
PropertyManager& PropertyManager::instance() {
  static PropertyManager instance;
  return instance;
}

// Load properties from file into memory
void PropertyManager::loadProperties(const std::string& propertyFile) {
  std::lock_guard<std::mutex> lock(property_mutex);
  properties.clear();

  if (!propertyFile.empty()) {
    std::ifstream file(propertyFile);
    if (!file) {
      DEBUG_LOGE("Failed to open property file: %s", propertyFile.c_str());
      return;
    }

    std::string line;
    while (std::getline(file, line)) {
      std::istringstream lineStream(line);
      std::string key, value;
      if (std::getline(lineStream, key, '=') &&
          std::getline(lineStream, value)) {
        properties[key] = value;
        DEBUG_LOGD("Loaded property into memory: %s = %s", key.c_str(),
                   value.c_str());
      }
    }
  }
  DEBUG_LOGI("Properties loaded into memory successfully.");
}

// Save in-memory properties to file
void PropertyManager::saveProperties(const std::string& propertyFile) const {
  std::lock_guard<std::mutex> lock(property_mutex);

  if (propertyFile.empty()) {
    return;
  }

  std::ofstream file(propertyFile, std::ios::trunc);
  if (!file) {
    DEBUG_LOGE("Failed to open property file for writing: %s",
               propertyFile.c_str());
    return;
  }

  for (const auto& [key, value] : properties) {
    file << key << "=" << value << "\n";
  }
  DEBUG_LOGI("Properties saved to file successfully: %s", propertyFile.c_str());
}

// Get a property value from memory
std::string PropertyManager::get(const std::string& key,
                                 const std::string& defaultValue) const {
  std::lock_guard<std::mutex> lock(property_mutex);

  auto it = properties.find(key);
  if (it != properties.end()) {
    DEBUG_LOGD("Property found in memory: %s = %s", key.c_str(),
               it->second.c_str());
    return it->second;
  }

  DEBUG_LOGD("Property not found in memory. Returning default: %s = %s",
             key.c_str(), defaultValue.c_str());
  return defaultValue;
}

// Set a property value in memory
void PropertyManager::set(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(property_mutex);

  properties[key] = value;
  DEBUG_LOGD("Property set : %s = %s", key.c_str(), value.c_str());
}

// Synchronize properties with the file
void PropertyManager::syncToFile(const std::string& propertyFile) {
  DEBUG_LOGI("Synchronizing in-memory properties to file...");
  saveProperties(propertyFile);
}

// Get all properties from memory
const std::unordered_map<std::string, std::string>&
PropertyManager::getAllProperties() const {
  std::lock_guard<std::mutex> lock(property_mutex);

  return properties;
}

// Simplified setprop method using in-memory storage
void PropertyManager::setprop(const std::string& key,
                              const std::string& value) {
  set(key, value);
  DEBUG_LOGD("Property set via setprop: %s = %s", key.c_str(), value.c_str());
}

// Simplified getprop method using in-memory storage
std::string PropertyManager::getprop(const std::string& key) const {
  auto value = get(key, "");
  DEBUG_LOGD("Property get via getprop: %s = %s", key.c_str(), value.c_str());
  return value;
}

// Global getprop function (memory-based)
std::string getprop(const std::string& key) {
  return PropertyManager::instance().getprop(key);
}

// Global setprop function (memory-based)
void setprop(const std::string& key, const std::string& value) {
  PropertyManager::instance().setprop(key, value);
}

}  // namespace init
}  // namespace minimal_systems
