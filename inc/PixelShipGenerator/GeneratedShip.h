#pragma once

#include <cstdint>
#include <vector>

#include "Image.h"
#include "PixelMask.h"
#include "ShipAnimationTraits.h"
#include "ShipAttachment.h"
#include "ShipFactionProfile.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipIdleAnimationMetadata.h"
#include "ShipPalette.h"
#include "ShipPaletteConfiguration.h"

namespace PixelShipGenerator
{
    struct GeneratedShip
    {
        uint64_t Seed = 0;
        ShipGenerationSeeds Seeds;
        GenerationDomainSeeds DomainSeeds;
        ShipPalette Palette;
        ShipPaletteSourceMode PaletteSourceMode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;
        ShipAnimationTraits AnimationTraits;
        ShipFactionAnimationProfile FactionAnimationProfile;
        ShipFactionPaintColorRole FactionWeaponMuzzleRole = ShipFactionPaintColorRole::ENGINE_HIGHLIGHT;
        ShipStyle Style = ShipStyle::SHIP_STYLE_END; // Built-in preset provenance; SHIP_STYLE_END means custom/no built-in origin.
        ShipFactionType Faction = ShipFactionType::SHIP_FACTION_TYPE_END; // Built-in preset provenance; END means custom/no built-in origin.

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
