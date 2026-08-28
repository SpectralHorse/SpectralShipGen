#pragma once

#include "GeneratedShip.h"
#include "ShipIdleAnimation.h"

namespace PixelShipGenerator
{
    class ShipIdleAnimator
    {
    public:
        ShipIdleAnimation generate(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings = {}) const;
    };
}
