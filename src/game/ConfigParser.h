#ifndef FALLOUT_GAME_CONFIG_PARSER_H_
#define FALLOUT_GAME_CONFIG_PARSER_H_

#include <string>
#include <map>
#include <vector>

namespace fallout {
namespace game {

// Stores INI data as: map<section_name, map<key, value>>
using IniData = std::map<std::string, std::map<std::string, std::string>>;

class ConfigParser {
public:
    ConfigParser();

    // Loads and parses an INI file.
    // Returns true on success, false on failure (e.g., file not found).
    bool LoadFile(const std::string& filePath);

    // Gets a string value from a specific section and key.
    // Returns the value if found, otherwise returns defaultValue.
    std::string GetString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;

    // Gets an integer value.
    int GetInteger(const std::string& section, const std::string& key, int defaultValue = 0) const;

    // Gets a float value.
    float GetFloat(const std::string& section, const std::string& key, float defaultValue = 0.0f) const;

    // Gets a boolean value.
    // True if value is "true", "yes", "on", "1". Case-insensitive.
    bool GetBoolean(const std::string& section, const std::string& key, bool defaultValue = false) const;

    // Checks if a section exists.
    bool HasSection(const std::string& section) const;

    // Checks if a key exists in a section.
    bool HasKey(const std::string& section, const std::string& key) const;

    // Returns all keys in a section.
    std::vector<std::string> GetSectionKeys(const std::string& section) const;

    const IniData& GetData() const { return data_; }

private:
    IniData data_;

    // Helper to trim whitespace
    static std::string Trim(const std::string& str);
    // Helper to convert string to lower case
    static std::string ToLower(const std::string& str);
};

} // namespace game
} // namespace fallout

#endif // FALLOUT_GAME_CONFIG_PARSER_H_
