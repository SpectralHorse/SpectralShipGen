#pragma once

#include "ShipDimensions.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipPalette.h"

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

    // Style-independent generation inputs used by the first-class explicit
    // ShipGenerationProfile path. Structural behavior is supplied separately
    // through the profile argument passed to ShipGenerator.
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
    };
} // namespace PixelShipGenerator
