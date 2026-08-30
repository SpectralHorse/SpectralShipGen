#pragma once

#include "ShipFactionProfile.h"
#include "ShipGenerationProvenance.h"
#include "ShipGenerationSettings.h"
#include "Validation.h"

namespace PixelShipGenerator
{
    struct ShipGenerationRecipe;

    // Canonical public resolved configuration. This is the stable path for
    // callers that already know the exact structural/faction behavior they want.
    // Built-in APIs simply resolve preset profiles and populate optional
    // provenance before delegating to this same representation.
    struct ShipResolvedGenerationConfiguration
    {
        ExplicitShipGenerationConfiguration Generation;
        ShipGenerationProfile StructuralProfile;
        ShipFactionProfile FactionProfile;
        ShipGenerationProvenance Provenance;
    };

    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ShipGenerationSettings& settings);
    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& structuralProfile);
    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& structuralProfile, const ShipFactionProfile& factionProfile);
    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ShipGenerationRecipe& recipe);

    // Validates the combined resolved configuration and all nested public
    // profiles. This intentionally validates technical contracts, not aesthetics.
    ValidationResult validateShipGenerationConfiguration(const ShipResolvedGenerationConfiguration& configuration);
}
