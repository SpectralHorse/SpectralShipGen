#pragma once

#include <string>
#include <vector>

#include "ShipGenerationProfile.h"

namespace PixelShipGenerator
{
    struct ShipGenerationProfileValidationIssue
    {
        std::string Field;
        std::string Message;
    };

    struct ShipGenerationProfileValidationResult
    {
        std::vector<ShipGenerationProfileValidationIssue> Errors;

        bool isValid() const noexcept { return Errors.empty(); }
    };

    // Validates structural safety/API contracts only. Unusual but mechanically
    // valid profiles are accepted; this is intentionally not an aesthetic
    // quality validator.
    ShipGenerationProfileValidationResult validateShipGenerationProfile(const ShipGenerationProfile& profile);
}
