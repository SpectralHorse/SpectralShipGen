#pragma once

#include <string>
#include <vector>

#include "ShipPaletteGenerationProfile.h"

namespace PixelShipGenerator
{
    struct ShipPaletteGenerationProfileValidationIssue
    {
        std::string Field;
        std::string Message;
    };

    struct ShipPaletteGenerationProfileValidationResult
    {
        std::vector<ShipPaletteGenerationProfileValidationIssue> Errors;

        bool isValid() const noexcept { return Errors.empty(); }
    };

    // Validates only deterministic/safe palette-generation contracts. Unusual,
    // monochrome, neon or low-contrast color languages are intentionally valid.
    ShipPaletteGenerationProfileValidationResult validateShipPaletteGenerationProfile(const ShipPaletteGenerationProfile& profile);
}
