#pragma once

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    using ShipFactionProfileValidationIssue = ValidationIssue;
    using ShipFactionProfileValidationResult = ValidationResult;

    // Validates safety and semantic contracts only. Unusual but safe custom
    // technological/material languages are intentionally accepted.
    ShipFactionProfileValidationResult validateShipFactionProfile(const ShipFactionProfile& profile);
}
