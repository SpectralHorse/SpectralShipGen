#pragma once

#include <cstdint>
#include <optional>

#include <SpectralShipGen/ShipDimensions.h>
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipIdleAnimation.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGen
{
    struct ShipGenerationSettings;
    struct ShipGenerationConfiguration;
    struct ExplicitShipGenerationConfiguration;
    struct ShipResolvedGenerationConfiguration;

    // Portable Core generation state. Optional built-in preset identities are
    // provenance/compact serialization selectors only; custom sources embed the
    // complete semantic profiles directly.
    struct ShipGenerationRecipe
    {
        ShipGenerationSeeds Seeds;
        GenerationDomainSeedOverrides DomainSeedOverrides;
        ShipDimensions Dimensions;

        std::optional<ShipStyle> StructuralPreset = ShipStyle::FIGHTER;
        ShipGenerationProfile StructuralProfile;

        std::optional<ShipFactionType> FactionPreset = ShipFactionType::FRONTIER;
        ShipFactionProfile FactionProfile;

        ShipPaletteConfiguration PaletteConfiguration;

        uint32_t DetailDensity = 50u;
        uint32_t AsymmetricDetailChance = 10u;
        bool AttachmentsEnabled = true;
    };

    struct ShipGenerationRecipeDocument
    {
        ShipGenerationRecipe Recipe;
        std::optional<ShipIdleAnimationSettings> AnimationSettings;
    };

    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationSettings& settings);
    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile);
    ShipGenerationRecipe makeShipGenerationRecipe(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile);
    ShipGenerationRecipe makeShipGenerationRecipe(const ShipResolvedGenerationConfiguration& configuration);

    ValidationResult validateShipGenerationRecipe(const ShipGenerationRecipe& recipe);

    bool operator==(const ShipGenerationRecipe& first, const ShipGenerationRecipe& second);
    bool operator!=(const ShipGenerationRecipe& first, const ShipGenerationRecipe& second);
}
