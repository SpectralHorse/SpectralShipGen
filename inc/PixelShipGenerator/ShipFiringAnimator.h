#pragma once

#include <vector>

#include "GeneratedShip.h"
#include "ShipAnimationPose.h"
#include "ShipFiringAnimation.h"

namespace PixelShipGenerator
{
    class ShipFiringAnimator
    {
    public:
        std::vector<ShipFiringAnimationTarget> getAvailableTargets(const GeneratedShip& ship) const;
        ShipFiringAnimation generate(const GeneratedShip& ship, const ShipFiringAnimationTarget& target, const ShipFiringAnimationSettings& settings = {}) const;
        ShipFiringAnimation generate(const GeneratedShip& ship, const ShipAnimationPose& underlyingPose, const ShipFiringAnimationTarget& target, const ShipFiringAnimationSettings& settings = {}) const;
        Image evaluateFrameAtNormalizedTime(const GeneratedShip& ship, const ShipFiringAnimationTarget& target, double normalizedTime, const ShipFiringAnimationSettings& settings = {}) const;
        Image evaluateFrameAtNormalizedTime(const GeneratedShip& ship, const ShipAnimationPose& underlyingPose, const ShipFiringAnimationTarget& target, double normalizedTime, const ShipFiringAnimationSettings& settings = {}) const;
    };
}
