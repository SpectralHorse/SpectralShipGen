#pragma once

#include <cstdint>
#include <optional>

#include "ShipDimensions.h"
#include "ShipFactionProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipIdleAnimation.h"
#include "ShipPaletteConfiguration.h"

namespace PixelShipGenerator
{
    struct ShipGenerationSettings;
    struct ShipGenerationConfiguration;
    struct ExplicitShipGenerationConfiguration;

    enum class ShipGenerationRecipeProfileSource : uint32_t
    {
        BUILT_IN_PRESET = 0u,
        EMBEDDED_CUSTOM,
        SHIP_GENERATION_RECIPE_PROFILE_SOURCE_END
    };

    // Portable Core generation state. Built-in sources remain compact through
    // Style/Faction preset references; custom sources embed complete public
    // profiles. Style/Faction are ignored as behavior sources when the matching
    // source is EMBEDDED_CUSTOM and should normally be *_END in that case.
    struct ShipGenerationRecipe
    {
        ShipGenerationSeeds Seeds;
        GenerationDomainSeedOverrides DomainSeedOverrides;
        GenerationRandomStreamMode RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        ShipDimensions Dimensions;

        ShipGenerationRecipeProfileSource StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        ShipStyle Style = ShipStyle::FIGHTER;
        ShipGenerationProfile StructuralProfile;

        ShipGenerationRecipeProfileSource FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        ShipFactionType Faction = ShipFactionType::FRONTIER;
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

    // Convenience conversion helpers. Recipe seed fields store the resolved
    // per-channel seeds so exported documents remain self-contained even when
    // the source configuration used seed overrides.
    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationSettings& settings);
    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile);
    ShipGenerationRecipe makeShipGenerationRecipe(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile);

    bool operator==(const ShipGenerationRecipe& first, const ShipGenerationRecipe& second);
    bool operator!=(const ShipGenerationRecipe& first, const ShipGenerationRecipe& second);
}
