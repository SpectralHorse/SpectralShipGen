#pragma once

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    // Validates safety and semantic contracts only. Unusual but safe custom
    // technological/material languages are intentionally accepted.
    ValidationResult validateShipFactionProfile(const ShipFactionProfile& profile);
}
