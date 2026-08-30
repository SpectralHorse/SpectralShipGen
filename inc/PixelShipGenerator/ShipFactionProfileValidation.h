#pragma once

#include "ShipFactionProfile.h"
#include "Validation.h"

namespace PixelShipGenerator
{
    using ShipFactionProfileValidationIssue = ValidationIssue;
    using ShipFactionProfileValidationResult = ValidationResult;

    // Validates safety and semantic contracts only. Unusual but safe custom
    // technological/material languages are intentionally accepted.
    ShipFactionProfileValidationResult validateShipFactionProfile(const ShipFactionProfile& profile);
}
