#pragma once

#include <unordered_map>
#include <string>
#include <mutex>

namespace minimal_systems {
namespace init {

class PropertyManager {
public:
    // Singleton instance accessor
    static PropertyManager& instance();

    // Load properties from a file or initialize defaults
    void loadProperties(const std::string& propertyFile = "");

    // Save properties to a file
    void saveProperties(const std::string& propertyFile) const;

    // Get a property value by key, with an optional default
    std::string get(const std::string& key, const std::string& defaultValue = "") const;

    // Set a property value
    void set(const std::string& key, const std::string& value);

    // Retrieve all properties as a constant reference
    const std::unordered_map<std::string, std::string>& getAllProperties() const;

private:
    // Private constructor for Singleton pattern
    PropertyManager() = default;

    // Mutex to protect property map access
    mutable std::mutex property_mutex;

    // Property key-value map
    std::unordered_map<std::string, std::string> properties;
};

} // namespace init
} // namespace minimal_systems
