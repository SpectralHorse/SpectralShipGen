#pragma once

#include <cstdint>
#include <vector>

#include "Image.h"
#include "PixelMask.h"
#include "ShipAttachment.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipIdleAnimationMetadata.h"
#include "ShipPalette.h"

namespace PixelShipGenerator
{
    struct GeneratedShip
    {
        uint64_t Seed = 0;
        ShipGenerationSeeds Seeds;
        GenerationDomainSeeds DomainSeeds;
        ShipPalette Palette;
        ShipStyle Style = ShipStyle::FIGHTER;
        ShipFactionType Faction = ShipFactionType::FRONTIER;

        PixelMask HullMask;
        PixelMask CockpitMask;
        PixelMask EngineMask;
        PixelMask EngineExhaustMask;

        PixelMask AttachmentMask;
        std::vector<ShipAttachmentPlacement> AttachmentPlacements;

        // Panels, accents, vents and small details
        PixelMask AccentMask;
        PixelMask MechanicalDetailMask;
        PixelMask LightMask;

        ShipIdleAnimationMetadata IdleAnimationMetadata;

        Image FinalImage;

        void reset(uint32_t width, uint32_t height, const ShipGenerationSeeds& seeds);
        void clear();
    };
} // namespace PixelShipGenerator
