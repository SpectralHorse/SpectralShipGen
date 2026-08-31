#pragma once

#include <cstdint>

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipPalette.h>
#include <SpectralShipGen/ShipPaletteGenerationProfile.h>

namespace SpectralShipGen
{
    class ShipPaletteGenerator
    {
    public:
        // Canonical generated-palette path. The palette language is independent
        // of faction provenance; structural palette modifiers remain separate.
        static ShipPalette generate(uint64_t paletteSeed, const ShipPaletteGenerationProfile& paletteProfile, const ShipGenerationProfile& styleProfile, bool enhancedMaterialContrast = true);

        // Compatibility/profile-composition convenience.
        static ShipPalette generate(uint64_t paletteSeed, const ShipFactionProfile& factionProfile, const ShipGenerationProfile& styleProfile, bool enhancedMaterialContrast = true);
        // Backward-compatible built-in faction convenience.
        static ShipPalette generate(uint64_t paletteSeed, ShipFactionType faction, const ShipGenerationProfile& styleProfile, bool enhancedMaterialContrast = true);
    };
}