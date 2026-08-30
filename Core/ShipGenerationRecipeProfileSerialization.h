#pragma once

#include <string>

#include "ShipGenerationRecipeJson.h"
#include "ShipFactionProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipPalette.h"
#include "ShipPaletteGenerationProfile.h"

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
