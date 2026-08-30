#pragma once

#include <string>
#include <vector>

#include "ShipFactionProfile.h"

namespace PixelShipGenerator
{
    struct ShipFactionProfileValidationIssue
    {
        std::string Field;
        std::string Message;
    };

    struct ShipFactionProfileValidationResult
    {
        std::vector<ShipFactionProfileValidationIssue> Errors;

        bool isValid() const noexcept { return Errors.empty(); }
    };

    // Validates safety and semantic contracts only. Unusual but safe custom
    // technological/material languages are intentionally accepted.
    ShipFactionProfileValidationResult validateShipFactionProfile(const ShipFactionProfile& profile);
}
