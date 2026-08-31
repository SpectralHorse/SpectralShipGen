#pragma once

#include <SpectralShipGen/ShipPaletteGenerationProfile.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    using ShipPaletteGenerationProfileValidationIssue = ValidationIssue;
    using ShipPaletteGenerationProfileValidationResult = ValidationResult;

    // Validates only deterministic/safe palette-generation contracts. Unusual,
    // monochrome, neon or low-contrast color languages are intentionally valid.
    ShipPaletteGenerationProfileValidationResult validateShipPaletteGenerationProfile(const ShipPaletteGenerationProfile& profile);
}
