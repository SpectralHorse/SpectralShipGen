#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class ShipMajorFeatureType : uint32_t
    {
        CENTRAL_SPINE = 0u,
        ARMOR_PLATE,
        RECESSED_BAY,
        VENT_BANK,
        WING_PLATE,
        TECH_CORE,
        SHIP_MAJOR_FEATURE_TYPE_END
    };
}
