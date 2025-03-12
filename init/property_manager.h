#ifndef PROPERTY_MANAGER_H
#define PROPERTY_MANAGER_H

#include <mutex>
#include <string>
#include <unordered_map>

namespace minimal_systems
{
namespace init
{

class PropertyManager {
    public:
	// Access the singleton instance
	static PropertyManager &instance();

	// Load properties from a file into memory
	void loadProperties(const std::string &propertyFile);

	// Save in-memory properties to a file
	void saveProperties(const std::string &propertyFile) const;

	// Get a property value with a default fallback
	std::string get(const std::string &key,
			const std::string &defaultValue) const;

	// Set a property value in memory
	void set(const std::string &key, const std::string &value);

	// Retrieve all properties stored in memory
	const std::unordered_map<std::string, std::string> &
	getAllProperties() const;

	// Synchronize in-memory properties to a file
	void syncToFile(const std::string &propertyFile);

	// Simplified property accessors (setprop/getprop)
	void setprop(const std::string &key, const std::string &value);
	std::string getprop(const std::string &key) const;

    private:
	// Mutex for thread-safe property access
	mutable std::mutex property_mutex;

	// In-memory property storage
	std::unordered_map<std::string, std::string> properties;

	// Private constructor/destructor for singleton
	PropertyManager() = default;
	~PropertyManager() = default;

	// Prevent copy and assignment
	PropertyManager(const PropertyManager &) = delete;
	PropertyManager &operator=(const PropertyManager &) = delete;
};

// Global free functions for simplified property access
void setprop(const std::string &key, const std::string &value);
std::string getprop(const std::string &key);

} // namespace init
} // namespace minimal_systems

#endif // PROPERTY_MANAGER_H
