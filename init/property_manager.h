#ifndef PROPERTY_MANAGER_H
#define PROPERTY_MANAGER_H

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace minimal_systems {
namespace init {

class PropertyManager {
  public:
    static PropertyManager& instance();

    void loadProperties(const std::string& propertyFile);
    void saveProperties(const std::string& propertyFile) const;
    void syncToFile(const std::string& propertyFile);

    void loadPersistentProperties(const std::string& persistentFile);
    void savePersistentProperties(const std::string& persistentFile) const;
    void syncPersistentProperties(const std::string& persistentFile);

    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    void set(const std::string& key, const std::string& value);
    void markPersistent(const std::string& key);
    void resetprop(const std::string& key);  // New resetprop method

    std::string getprop(const std::string& key) const;
    void setprop(const std::string& key, const std::string& value);

    const std::unordered_map<std::string, std::string>& getAllProperties() const;

  private:
    PropertyManager() = default;

    mutable std::mutex property_mutex;
    std::unordered_map<std::string, std::string> properties;
    std::unordered_map<std::string, std::string> persistentProperties;
    std::unordered_set<std::string> persistentKeys;
};

std::string getprop(const std::string& key);
void setprop(const std::string& key, const std::string& value);
void resetprop(const std::string& key);  // Global resetprop function

}  // namespace init
}  // namespace minimal_systems

#endif  // PROPERTY_MANAGER_H
