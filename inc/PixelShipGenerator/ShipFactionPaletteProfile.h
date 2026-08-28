#pragma once

#include <cstdint>

#include "ShipFactionType.h"

namespace PixelShipGenerator
{
    struct PaletteUIntRange
    {
        uint32_t Min = 0u;
        uint32_t Max = 0u;
    };

    struct PaletteIntRange
    {
        int32_t Min = 0;
        int32_t Max = 0;
    };

    struct PaletteRoleProfile
    {
        PaletteIntRange HueOffset;
        PaletteUIntRange Saturation;
        PaletteUIntRange Value;
    };

    struct ShipFactionPaletteProfile
    {
        PaletteUIntRange HullHue;
        PaletteUIntRange HullSaturation;
        PaletteUIntRange HullValue;

        PaletteRoleProfile Accent;
        PaletteRoleProfile Cockpit;
        PaletteRoleProfile Light;
        PaletteRoleProfile Exhaust;

        PaletteUIntRange MechanicalSaturation;
        PaletteUIntRange MechanicalValue;
    };

    const ShipFactionPaletteProfile& getShipFactionPaletteProfile(ShipFactionType faction);
}