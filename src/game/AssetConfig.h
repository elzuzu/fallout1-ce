#ifndef FALLOUT_GAME_ASSET_CONFIG_H_
#define FALLOUT_GAME_ASSET_CONFIG_H_

#include "ConfigParser.h"
#include <string>
#include <vector>

namespace fallout {
namespace game {

// Example structure for asset properties, could be more complex
struct AssetProperties {
    std::string path;
    std::string type; // e.g., "frm", "glb", "png"
    // Add other common properties: scale, default_animation, etc.
    std::map<std::string, std::string> additionalParams; // For arbitrary key-value params

    AssetProperties() = default;
};


class AssetConfig {
public:
    AssetConfig();

    // Loads asset configurations from the specified INI file.
    bool LoadConfig(const std::string& iniFilePath);

    // Gets the file path for a given asset name within a category.
    // Example: GetAssetPath("CritterModels", "Dog");
    std::string GetAssetPath(const std::string& category, const std::string& assetName) const;

    // Gets a specific string property for an asset.
    // Properties might be "path", "type", or custom ones.
    // Example: GetAssetProperty("ItemSprites", "Stimpack", "type");
    std::string GetAssetProperty(const std::string& category, const std::string& assetName, const std::string& propertyName, const std::string& defaultValue = "") const;

    // Gets all properties for a given asset.
    bool GetAssetProperties(const std::string& category, const std::string& assetName, AssetProperties& outProps) const;

    // Lists all asset names within a given category.
    std::vector<std::string> ListAssetsInCategory(const std::string& category) const;

private:
    ConfigParser parser_;

    // Helper to parse a value string that might contain multiple properties
    // e.g., "path:models/dog.glb, type:glb, scale:0.5"
    // For now, we'll assume simple path values in the INI and extend if needed.
    // AssetProperties ParseValueString(const std::string& valueString) const;
};

} // namespace game
} // namespace fallout

#endif // FALLOUT_GAME_ASSET_CONFIG_H_
