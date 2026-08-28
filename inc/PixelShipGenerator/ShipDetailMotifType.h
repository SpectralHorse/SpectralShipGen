#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class ShipDetailMotifType : uint32_t
    {
        PAIRED_VENTS = 0u,
        TRIPLE_VENT_BANK,
        PAIRED_LIGHTS,
        THREE_NODE_LIGHTS,
        PARALLEL_SEAMS,
        REPEATED_DASHES,
        RECESSED_SLOT,

        SHIP_DETAIL_MOTIF_TYPE_END
    };

    const char* getShipDetailMotifTypeName(ShipDetailMotifType type);
}
