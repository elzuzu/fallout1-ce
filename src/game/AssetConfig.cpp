#include "AssetConfig.h"
#include <sstream> // For potential future parsing of complex value strings

namespace fallout {
namespace game {

AssetConfig::AssetConfig() = default;

bool AssetConfig::LoadConfig(const std::string& iniFilePath) {
    return parser_.LoadFile(iniFilePath);
}

std::string AssetConfig::GetAssetPath(const std::string& category, const std::string& assetName) const {
    // Assuming the INI value for an asset key is directly its path.
    // If properties are stored like "path:actual/path.glb, type:glb", this needs more parsing.
    // For now, direct value is path.
    return parser_.GetString(category, assetName, "");
}

std::string AssetConfig::GetAssetProperty(const std::string& category, const std::string& assetName, const std::string& propertyName, const std::string& defaultValue) const {
    // This basic implementation assumes the INI value itself might be what's requested if propertyName is e.g. "path"
    // Or, it assumes a more complex structure in the INI or a fixed interpretation.
    // For a simple INI where "AssetName = path_to_asset", if propertyName is "path", return the value.
    // This will be expanded if the INI stores more structured data per asset.

    std::string rawValue = parser_.GetString(category, assetName, "");
    if (rawValue.empty()) {
        return defaultValue;
    }

    // Simple case: if propertyName is "path", return the whole raw value.
    if (propertyName == "path") {
        return rawValue;
    }

    // Placeholder for more complex property parsing from rawValue if needed.
    // Example: if rawValue is "type:frm, offset:10"
    // if (propertyName == "type") { /* parse and return type */ }
    // For now, only "path" is directly supported this way.

    // If we expect properties to be separate keys like AssetName_type, AssetName_scale under the category:
    // return parser_.GetString(category, assetName + "_" + propertyName, defaultValue);

    return defaultValue; // Property not found or not supported by this simple logic
}


bool AssetConfig::GetAssetProperties(const std::string& category, const std::string& assetName, AssetProperties& outProps) const {
    std::string pathValue = parser_.GetString(category, assetName, "");
    if (pathValue.empty() && !parser_.HasKey(category,assetName) ) { // check HasKey in case value is legitimately empty string
        return false; // Asset not found under this category/name
    }

    // Current simple assumption: the value IS the path.
    // Type needs to be inferred or defined elsewhere, or by convention in category name.
    outProps.path = pathValue;

    // Infer type from path extension (basic example)
    size_t dotPos = pathValue.rfind('.');
    if (dotPos != std::string::npos && dotPos + 1 < pathValue.length()) {
        outProps.type = parser_.ToLower(pathValue.substr(dotPos + 1));
    } else {
        outProps.type = "unknown";
    }

    // Example: If INI could store more, like:
    // AssetName = path:actual/path.glb, type:glb, other_prop:value
    // Then, we'd parse 'pathValue' here.
    // For now, additionalParams remains empty.
    outProps.additionalParams.clear();

    // Example: If the INI has separate keys for properties:
    // AssetName_type = glb
    // AssetName_scale = 1.0
    // outProps.type = parser_.GetString(category, assetName + "_type", outProps.type);
    // std::string scale_str = parser_.GetString(category, assetName + "_scale", "");
    // if (!scale_str.empty()) outProps.additionalParams["scale"] = scale_str;

    return true;
}

std::vector<std::string> AssetConfig::ListAssetsInCategory(const std::string& category) const {
    return parser_.GetSectionKeys(category);
}


} // namespace game
} // namespace fallout
