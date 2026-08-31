#pragma once

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipAnimationPose.h>
#include <SpectralShipGen/ShipMovementAnimation.h>

namespace SpectralShipGen
{
    class ShipLongitudinalMovementAnimator
    {
    public:
        ShipMovementAnimation generate(const GeneratedShip& ship, ShipAnimationType type, const ShipMovementAnimationSettings& settings = {}) const;
        Image evaluateFrameAtNormalizedTime(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings = {}) const;
        ShipAnimationPose evaluatePoseAtNormalizedTime(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings = {}) const;
    };
}
