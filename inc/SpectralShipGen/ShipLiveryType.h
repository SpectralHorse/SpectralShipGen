#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipLiveryType : uint32_t
    {
        CENTER_STRIPE = 0u,
        DOUBLE_CENTER_STRIPE,
        WING_BAND,
        SHOULDER_BLOCK,
        NOSE_BAND,
        CHEVRON,
        ID_PANEL,
        GEOMETRIC_INSIGNIA,
        SHIP_LIVERY_TYPE_END
    };

    const char* getShipLiveryTypeName(ShipLiveryType type);
}
