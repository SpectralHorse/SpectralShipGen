#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipVisualAnchorType : uint32_t
    {
        SILHOUETTE = 0u,
        COCKPIT,
        WINGS,
        ENGINES,
        WEAPONS,
        MAJOR_FEATURE,
        HULL_LAYERS,
        CENTRAL_CORE,
        MACRO_ASYMMETRY,
        NEGATIVE_SPACE,
        SHIP_VISUAL_ANCHOR_TYPE_END
    };

    const char* getShipVisualAnchorTypeName(ShipVisualAnchorType type);
}
