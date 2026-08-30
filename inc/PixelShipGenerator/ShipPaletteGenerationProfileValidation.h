#pragma once

#include "ShipPaletteGenerationProfile.h"
#include "Validation.h"

namespace PixelShipGenerator
{
    using ShipPaletteGenerationProfileValidationIssue = ValidationIssue;
    using ShipPaletteGenerationProfileValidationResult = ValidationResult;

    // Validates only deterministic/safe palette-generation contracts. Unusual,
    // monochrome, neon or low-contrast color languages are intentionally valid.
    ShipPaletteGenerationProfileValidationResult validateShipPaletteGenerationProfile(const ShipPaletteGenerationProfile& profile);
}
