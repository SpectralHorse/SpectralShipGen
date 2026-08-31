#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipFactionType : uint32_t
    {
        FRONTIER = 0,
        MILITARY,
        ASCENDANT,
        XENO,
        CORPORATE,
        RELIC,

        SHIP_FACTION_TYPE_END
    };
}