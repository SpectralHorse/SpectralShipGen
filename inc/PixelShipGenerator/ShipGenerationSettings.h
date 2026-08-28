#pragma once

#include "ShipDimensions.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipPalette.h"

#include <cstdint>

namespace PixelShipGenerator
{
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
} // namespace PixelShipGenerator
