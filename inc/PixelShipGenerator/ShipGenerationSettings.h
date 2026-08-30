#pragma once

#include "ShipDimensions.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipPalette.h"
#include "ShipPaletteConfiguration.h"

#include <cstdint>

namespace PixelShipGenerator
{
    // Backward-compatible built-in-preset settings. The member order is kept
    // unchanged so existing C++17 aggregate initialization remains valid.
    struct ShipGenerationSettings
    {
        uint64_t Seed = 0;
        ShipDimensions Dimensions;
        ShipStyle Style = ShipStyle::FIGHTER;
        ShipFactionType Faction = ShipFactionType::FRONTIER;

        uint32_t DetailDensity = 50;
        uint32_t AsymmetricDetailChance = 10;

        bool AttachmentsEnabled = true;
        ShipGenerationSeedOverrides SeedOverrides;
        GenerationDomainSeedOverrides DomainSeedOverrides;
        GenerationRandomStreamMode RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
    };

    // Style- and faction-independent generation inputs used by the fully
    // explicit structural + faction profile path. This is the canonical
    // behavior configuration once built-in preset selectors have been resolved.
    struct ExplicitShipGenerationConfiguration
    {
        uint64_t Seed = 0;
        ShipDimensions Dimensions;

        uint32_t DetailDensity = 50;
        uint32_t AsymmetricDetailChance = 10;

        bool AttachmentsEnabled = true;
        ShipGenerationSeedOverrides SeedOverrides;
        GenerationDomainSeedOverrides DomainSeedOverrides;
        GenerationRandomStreamMode RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;

        // Palette source is independent from structural/faction configuration.
        // The default preserves faction-profile generated palette behavior.
        ShipPaletteConfiguration PaletteConfiguration;
    };

    // Style-independent generation inputs used by the Task-82 explicit
    // ShipGenerationProfile + built-in-faction compatibility path. Faction is
    // deliberately absent from ExplicitShipGenerationConfiguration above.
    struct ShipGenerationConfiguration
    {
        uint64_t Seed = 0;
        ShipDimensions Dimensions;
        ShipFactionType Faction = ShipFactionType::FRONTIER;

        uint32_t DetailDensity = 50;
        uint32_t AsymmetricDetailChance = 10;

        bool AttachmentsEnabled = true;
        ShipGenerationSeedOverrides SeedOverrides;
        GenerationDomainSeedOverrides DomainSeedOverrides;
        GenerationRandomStreamMode RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;

        ShipPaletteConfiguration PaletteConfiguration;
    };
} // namespace PixelShipGenerator
