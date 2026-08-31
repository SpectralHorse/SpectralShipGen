#pragma once

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipIdleAnimation.h>

namespace SpectralShipGen
{
    class ShipIdleAnimator
    {
    public:
        ShipIdleAnimation generate(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings = {}) const;
        Image evaluateFrameAtNormalizedTime(const GeneratedShip& ship, double normalizedTime, const ShipIdleAnimationSettings& settings = {}) const;
    };
}
