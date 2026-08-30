#pragma once

#include <cstdint>

#include "ShipPalette.h"
#include "ShipPaletteGenerationProfile.h"

namespace PixelShipGenerator
{
    enum class ShipPaletteSourceMode : uint32_t
    {
        // Generate from the ShipFactionProfile supplied to the generation call.
        // This preserves all existing built-in/custom faction behavior by default.
        FACTION_PROFILE_GENERATED = 0u,

        // Generate from an explicitly supplied palette-generation language.
        EXPLICIT_GENERATED,

        // Use the caller-provided resolved semantic ShipPalette exactly. Structural
        // palette-generation modifiers and Palette-domain seed/reroll changes do
        // not alter the supplied color values in this mode.
        FIXED,

        SHIP_PALETTE_SOURCE_MODE_END
    };

    struct ShipPaletteConfiguration
    {
        ShipPaletteSourceMode Mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;
        ShipPaletteGenerationProfile Generated;
        ShipPalette Fixed;
    };
}
