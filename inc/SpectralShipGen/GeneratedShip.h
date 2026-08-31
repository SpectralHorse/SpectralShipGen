#pragma once

#include <cstdint>
#include <vector>

#include <SpectralShipGen/Image.h>
#include <SpectralShipGen/PixelMask.h>
#include <SpectralShipGen/ShipAnimationTraits.h>
#include <SpectralShipGen/ShipAttachment.h>
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProvenance.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipIdleAnimationMetadata.h>
#include <SpectralShipGen/ShipPalette.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>

namespace SpectralShipGen
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
        ShipGenerationProvenance Provenance;
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
} // namespace SpectralShipGen
