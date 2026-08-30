#pragma once

#include "ShipFactionProfile.h"

namespace PixelShipGenerator
{
    // Generic aliases for the existing faction-authored palette range/relationship
    // value types. These aliases preserve the Task-83 public faction API while
    // making the same semantic data usable independently of faction provenance.
    using ShipPaletteGenerationRanges = ShipFactionPaletteProfile;
    using ShipPaletteGenerationBehaviorProfile = ShipFactionPaletteBehaviorProfile;

    // Complete semantic language used to deterministically resolve a ShipPalette.
    // Ranges select base/role HSV values; Behavior defines relationships between
    // those values. Structural ShipGenerationProfile palette modifiers are applied
    // separately by ShipPaletteGenerator and are intentionally not stored here.
    struct ShipPaletteGenerationProfile
    {
        ShipPaletteGenerationRanges Ranges;
        ShipPaletteGenerationBehaviorProfile Behavior;
    };

    // Copies the palette-generation language represented by a faction profile.
    // The returned value is independent and may be modified by the caller.
    ShipPaletteGenerationProfile getShipPaletteGenerationProfile(const ShipFactionProfile& factionProfile);

    // Convenience built-in preset copy. The returned profile is not process-global
    // mutable state and can safely be edited before explicit generation.
    ShipPaletteGenerationProfile getShipPaletteGenerationProfile(ShipFactionType faction);
}
