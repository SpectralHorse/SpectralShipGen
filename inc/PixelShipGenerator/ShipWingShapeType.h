#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class WingShapeType : uint32_t
    {
        NONE = 0u,
        SMALL,
        SWEPT,
        BROAD,
        WING_SHAPE_TYPE_END
    };
}
