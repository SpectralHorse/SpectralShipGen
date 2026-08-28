#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class ShipStructuralNegativeSpaceType : uint32_t
    {
        WING_CHANNEL = 0u,
        REAR_FORK,
        SHOULDER_GAP,
        OPEN_FRAME_BAY,
        NACELLE_CHANNEL,
        SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END
    };

    const char* getShipStructuralNegativeSpaceTypeName(ShipStructuralNegativeSpaceType type);
}
