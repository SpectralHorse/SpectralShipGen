#pragma once

#include <vector>

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipAnimationPose.h>
#include <SpectralShipGen/ShipFiringAnimation.h>

namespace SpectralShipGen
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
