#pragma once

#include <string>

#include <PixelShipGenerator/Serialization/ShipGenerationRecipeJson.h>
#include <PixelShipGenerator/ShipFactionProfile.h>
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include <PixelShipGenerator/ShipPalette.h>
#include <PixelShipGenerator/ShipPaletteGenerationProfile.h>

namespace PixelShipGenerator::RecipeProfileSerialization
{
    RecipeJson::Value serialize(const ShipGenerationProfile& profile);
    RecipeJson::Value serialize(const ShipFactionProfile& profile);
    RecipeJson::Value serialize(const ShipPaletteGenerationProfile& profile);
    RecipeJson::Value serialize(const ShipPalette& palette);

    bool deserialize(const RecipeJson::Value& value, ShipGenerationProfile& profile, std::string& error, const std::string& path);
    bool deserialize(const RecipeJson::Value& value, ShipFactionProfile& profile, std::string& error, const std::string& path);
    bool deserialize(const RecipeJson::Value& value, ShipPaletteGenerationProfile& profile, std::string& error, const std::string& path);
    bool deserialize(const RecipeJson::Value& value, ShipPalette& palette, std::string& error, const std::string& path);
}
