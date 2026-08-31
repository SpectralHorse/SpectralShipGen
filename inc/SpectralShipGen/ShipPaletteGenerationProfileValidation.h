#pragma once

#include <SpectralShipGen/ShipPaletteGenerationProfile.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    // Validates only deterministic/safe palette-generation contracts. Unusual,
    // monochrome, neon or low-contrast color languages are intentionally valid.
    ValidationResult validateShipPaletteGenerationProfile(const ShipPaletteGenerationProfile& profile);
}
