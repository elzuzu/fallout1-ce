#include "ConfigParser.h"
#include <fstream>
#include <algorithm> // For std::transform
#include <cctype>    // For std::isspace, std::tolower

namespace fallout {
namespace game {

ConfigParser::ConfigParser() = default;

std::string ConfigParser::Trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // String is all whitespace
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string ConfigParser::ToLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lowerStr;
}

bool ConfigParser::LoadFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        // std::cerr << "Error: Could not open INI file: " << filePath << std::endl;
        return false;
    }

    data_.clear();
    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        line = Trim(line);

        if (line.empty() || line[0] == ';' || line[0] == '#') { // Skip empty lines and comments
            continue;
        }

        if (line[0] == '[' && line.back() == ']') { // Section header
            currentSection = Trim(line.substr(1, line.length() - 2));
            if (!currentSection.empty()) {
                data_[currentSection] = std::map<std::string, std::string>(); // Initialize section
            }
        } else if (!currentSection.empty()) { // Key-value pair
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = Trim(line.substr(0, equalsPos));
                std::string value = Trim(line.substr(equalsPos + 1));
                if (!key.empty()) {
                    data_[currentSection][key] = value;
                }
            }
        }
    }
    file.close();
    return true;
}

std::string ConfigParser::GetString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int ConfigParser::GetInteger(const std::string& section, const std::string& key, int defaultValue) const {
    std::string valueStr = GetString(section, key);
    if (!valueStr.empty()) {
        try {
            return std::stoi(valueStr);
        } catch (const std::invalid_argument& ia) {
            // std::cerr << "Warning: Invalid integer format for " << section << "." << key << ": " << valueStr << std::endl;
        } catch (const std::out_of_range& oor) {
            // std::cerr << "Warning: Integer out of range for " << section << "." << key << ": " << valueStr << std::endl;
        }
    }
    return defaultValue;
}

float ConfigParser::GetFloat(const std::string& section, const std::string& key, float defaultValue) const {
    std::string valueStr = GetString(section, key);
    if (!valueStr.empty()) {
        try {
            return std::stof(valueStr);
        } catch (const std::invalid_argument& ia) {
            // std::cerr << "Warning: Invalid float format for " << section << "." << key << ": " << valueStr << std::endl;
        } catch (const std::out_of_range& oor) {
            // std::cerr << "Warning: Float out of range for " << section << "." << key << ": " << valueStr << std::endl;
        }
    }
    return defaultValue;
}

bool ConfigParser::GetBoolean(const std::string& section, const std::string& key, bool defaultValue) const {
    std::string valueStr = ToLower(GetString(section, key));
    if (valueStr == "true" || valueStr == "yes" || valueStr == "on" || valueStr == "1") {
        return true;
    }
    if (valueStr == "false" || valueStr == "no" || valueStr == "off" || valueStr == "0") {
        return false;
    }
    return defaultValue;
}

bool ConfigParser::HasSection(const std::string& section) const {
    return data_.count(section) > 0;
}

bool ConfigParser::HasKey(const std::string& section, const std::string& key) const {
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        return sectionIt->second.count(key) > 0;
    }
    return false;
}

std::vector<std::string> ConfigParser::GetSectionKeys(const std::string& section) const {
    std::vector<std::string> keys;
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        for (const auto& pair : sectionIt->second) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

} // namespace game
} // namespace fallout
