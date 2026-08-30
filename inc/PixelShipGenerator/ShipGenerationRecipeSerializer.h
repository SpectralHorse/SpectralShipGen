#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "ShipGenerationRecipe.h"

namespace PixelShipGenerator
{
    inline constexpr uint32_t ShipGenerationRecipeFormatVersion = 5u;

    struct ShipGenerationRecipeLoadResult
    {
        bool Success = false;
        ShipGenerationRecipeDocument Document;
        std::string Error;
    };

    std::string shipStyleToRecipeString(ShipStyle style);
    std::string shipFactionToRecipeString(ShipFactionType faction);
    bool shipStyleFromRecipeString(const std::string& value, ShipStyle& style);
    bool shipFactionFromRecipeString(const std::string& value, ShipFactionType& faction);

    std::string serializeShipGenerationRecipe(const ShipGenerationRecipeDocument& document);
    ShipGenerationRecipeLoadResult deserializeShipGenerationRecipe(const std::string& jsonText);
    bool saveShipGenerationRecipe(const ShipGenerationRecipeDocument& document, const std::filesystem::path& path, std::string& error);
    ShipGenerationRecipeLoadResult loadShipGenerationRecipe(const std::filesystem::path& path);
}
