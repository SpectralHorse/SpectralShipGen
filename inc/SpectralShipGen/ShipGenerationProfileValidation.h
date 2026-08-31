#pragma once

#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    using ShipGenerationProfileValidationIssue = ValidationIssue;
    using ShipGenerationProfileValidationResult = ValidationResult;

    // Validates structural safety/API contracts only. Unusual but mechanically
    // valid profiles are accepted; this is intentionally not an aesthetic
    // quality validator.
    ShipGenerationProfileValidationResult validateShipGenerationProfile(const ShipGenerationProfile& profile);
}
