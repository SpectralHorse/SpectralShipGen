#pragma once

#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    // Validates structural safety/API contracts only. Unusual but mechanically
    // valid profiles are accepted; this is intentionally not an aesthetic
    // quality validator.
    ValidationResult validateShipGenerationProfile(const ShipGenerationProfile& profile);
}
