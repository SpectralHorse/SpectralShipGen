#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipMaterialZoneType : uint32_t
    {
        WING_SURFACE = 0u,
        SHOULDER_SURFACE,
        AXIAL_BAND,
        REAR_MECHANICAL,
        COCKPIT_COLLAR,
        HARDPOINT_SURROUND,
        SHIP_MATERIAL_ZONE_TYPE_END
    };

    const char* getShipMaterialZoneTypeName(ShipMaterialZoneType type);
}
