#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class ShipAnimationType : uint32_t
    {
        IDLE = 0u,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_UP,
        MOVE_DOWN,
        FIRE,
        SHIP_ANIMATION_TYPE_END
    };
}
