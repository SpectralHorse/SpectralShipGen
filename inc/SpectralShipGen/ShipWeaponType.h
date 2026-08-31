#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipWeaponType : uint32_t
    {
        SINGLE_CANNON = 0u,
        TWIN_CANNON,
        COMPACT_TURRET,
        RAIL_WEAPON,
        WEAPON_POD,
        SHIP_WEAPON_TYPE_END
    };

    enum class ShipWeaponHardpointRegion : uint32_t
    {
        CENTRAL_NOSE = 0u,
        FORWARD_FUSELAGE_SIDE,
        WING_ROOT,
        OUTER_WING,
        FORWARD_SHOULDER,
        CENTRAL_BODY,
        SHIP_WEAPON_HARDPOINT_REGION_END
    };
}
