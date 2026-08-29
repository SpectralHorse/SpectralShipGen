#pragma once

#include "GeneratedShip.h"
#include "ShipMovementAnimation.h"

namespace PixelShipGenerator
{
    class ShipLateralMovementAnimator
    {
    public:
        ShipMovementAnimation generate(const GeneratedShip& ship, ShipAnimationType type, const ShipMovementAnimationSettings& settings = {}) const;
        Image evaluateFrameAtNormalizedTime(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings = {}) const;
    };
}
