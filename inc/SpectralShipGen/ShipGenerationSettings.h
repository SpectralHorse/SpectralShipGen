#pragma once

#include <SpectralShipGen/ShipDimensions.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipPalette.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>

#include <cstdint>

namespace SpectralShipGen
{
    // Built-in-preset convenience settings. Resolved/custom integrations should
    // prefer ShipResolvedGenerationConfiguration as the canonical semantic input.
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
    };

    // Style- and faction-independent common generation inputs used by explicit
    // structural + faction profile paths. Resolved structural/faction behavior
    // is carried by ShipResolvedGenerationConfiguration.
    struct ExplicitShipGenerationConfiguration
    {
        uint64_t Seed = 0;
        ShipDimensions Dimensions;

        uint32_t DetailDensity = 50;
        uint32_t AsymmetricDetailChance = 10;

        bool AttachmentsEnabled = true;
        ShipGenerationSeedOverrides SeedOverrides;
        GenerationDomainSeedOverrides DomainSeedOverrides;

        // Palette source is independent from structural/faction configuration.
        // The default preserves faction-profile generated palette behavior.
        ShipPaletteConfiguration PaletteConfiguration;
    };

    // Convenience inputs for an explicit structural profile combined with a
    // built-in faction selector. Fully resolved callers should prefer
    // ShipResolvedGenerationConfiguration.
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

        ShipPaletteConfiguration PaletteConfiguration;
    };
} // namespace SpectralShipGen
