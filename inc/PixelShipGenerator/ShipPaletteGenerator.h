#pragma once

#include <cstdint>

#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipPalette.h"

namespace PixelShipGenerator
{
    class ShipPaletteGenerator
    {
    public:
        static ShipPalette generate(uint64_t paletteSeed, ShipFactionType faction, const ShipGenerationProfile& styleProfile, bool enhancedMaterialContrast = true);
    };
}