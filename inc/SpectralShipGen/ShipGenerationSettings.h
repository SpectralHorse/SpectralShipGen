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
    // Backward-compatible built-in-preset convenience settings. The member order
    // is kept unchanged so existing C++17 aggregate initialization remains valid.
    // New integrations that already own resolved/custom profiles should prefer
    // ShipResolvedGenerationConfiguration as the unambiguous canonical input.
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
        GenerationRandomStreamMode RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;

        // Palette source is independent from structural/faction configuration.
        // The default preserves faction-profile generated palette behavior.
        ShipPaletteConfiguration PaletteConfiguration;
    };

    // Style-independent compatibility inputs for an explicit structural profile
    // combined with a built-in faction convenience selector. Faction is
    // deliberately absent from ExplicitShipGenerationConfiguration above.
    // Prefer ShipResolvedGenerationConfiguration for new fully resolved code.
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
} // namespace SpectralShipGen
