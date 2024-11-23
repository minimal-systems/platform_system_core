#ifndef PROPERTY_MANAGER_H
#define PROPERTY_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>

namespace minimal_systems {
namespace init {

class PropertyManager {
public:
    static PropertyManager& instance();

    void loadProperties(const std::string& propertyFile);
    void saveProperties(const std::string& propertyFile) const;

    std::string get(const std::string& key, const std::string& defaultValue) const;
    void set(const std::string& key, const std::string& value);
    const std::unordered_map<std::string, std::string>& getAllProperties() const;

    void setprop(const std::string& key, const std::string& value);
    std::string getprop(const std::string& key) const;

private:
    mutable std::mutex property_mutex;
    std::unordered_map<std::string, std::string> properties;

    PropertyManager() = default;
    ~PropertyManager() = default;

    // Disallow copy and assign
    PropertyManager(const PropertyManager&) = delete;
    PropertyManager& operator=(const PropertyManager&) = delete;
};

// Global free functions for simplified usage
void setprop(const std::string& key, const std::string& value);
std::string getprop(const std::string& key);

} // namespace init
} // namespace minimal_systems

#endif // PROPERTY_MANAGER_H
