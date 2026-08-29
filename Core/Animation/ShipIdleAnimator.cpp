#include "ShipIdleAnimator.h"

#include "AnimationSamplingPlanner.h"
#include "ShipIdleAnimationInternal.h"

namespace PixelShipGenerator
{
    ShipIdleAnimation ShipIdleAnimator::generate(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings) const
    {
        ShipIdleAnimation animation;
        animation.Type = ShipAnimationType::IDLE;
        animation.FrameWidth = ship.HullMask.getWidth();
        animation.FrameHeight = ship.HullMask.getHeight();
        animation.Seed = IdleAnimationInternal::resolveIdleAnimationSeed(ship, settings);

        const IdleAnimationInternal::IdleAnimationPlan idlePlan = IdleAnimationInternal::createIdleAnimationPlan(ship, settings, animation.Seed);
        AnimationSamplingPlanner samplingPlanner;
        animation.Sampling = samplingPlanner.plan(idlePlan.SamplingRequirements);
        animation.DurationMilliseconds = animation.Sampling.DurationMilliseconds;
        animation.FrameDurationMilliseconds = animation.Sampling.ActualFrameDurationMilliseconds;
        animation.Frames.reserve(animation.Sampling.ActualFrameCount);
        animation.NormalizedSampleTimes.reserve(animation.Sampling.ActualFrameCount);

        for (uint32_t frameIndex = 0u; frameIndex < animation.Sampling.ActualFrameCount; ++frameIndex)
        {
            const double normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(animation.Sampling.ActualFrameCount);
            animation.NormalizedSampleTimes.push_back(normalizedTime);
            animation.Frames.push_back(IdleAnimationInternal::evaluateIdleFrame(ship, settings, normalizedTime, idlePlan));
        }

        return animation;
    }

    Image ShipIdleAnimator::evaluateFrameAtNormalizedTime(const GeneratedShip& ship, double normalizedTime, const ShipIdleAnimationSettings& settings) const
    {
        const uint64_t seed = IdleAnimationInternal::resolveIdleAnimationSeed(ship, settings);
        const IdleAnimationInternal::IdleAnimationPlan idlePlan = IdleAnimationInternal::createIdleAnimationPlan(ship, settings, seed);
        return IdleAnimationInternal::evaluateIdleFrame(ship, settings, normalizedTime, idlePlan);
    }
}
