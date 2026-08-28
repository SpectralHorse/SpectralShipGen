#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class ShipCoreTreatmentType : uint32_t
    {
        CENTRAL_SPINE = 0u,
        COCKPIT_SURROUND,
        RAISED_CORE_PLATE,
        LATERAL_RECESSES,
        LONGITUDINAL_ARMOR_BAND,
        CORE_CHANNEL,
        SHIP_CORE_TREATMENT_TYPE_END
    };

    const char* getShipCoreTreatmentTypeName(ShipCoreTreatmentType type);
}
