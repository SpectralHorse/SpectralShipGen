#pragma once

#include <cstdint>

#include <SpectralShipGen/ShipAnimationType.h>

namespace SpectralShipGen
{
    enum class AnimationSamplingMode : uint32_t
    {
        ADAPTIVE = 0u,
        EXACT_FRAME_COUNT,
        ANIMATION_SAMPLING_MODE_END
    };

    struct AnimationSamplingRequirements
    {
        ShipAnimationType Type = ShipAnimationType::IDLE;
        AnimationSamplingMode Mode = AnimationSamplingMode::ADAPTIVE;
        uint32_t DurationMilliseconds = 1500u;
        // Used only by EXACT_FRAME_COUNT. Adaptive sampling derives its count from the requirements below.
        uint32_t ExactFrameCount = 10u;
        uint32_t MinimumFrameCount = 10u;
        uint32_t MaximumFrameCount = 60u;
        uint32_t MaximumMechanicalTravelPixels = 0u;
        uint32_t MaximumExhaustTravelPixels = 0u;
        uint32_t ActiveAnimatedComponentCount = 0u;
        uint32_t IndependentPhaseGroupCount = 0u;
        uint32_t MaximumTemporalCyclesPerClip = 0u;
        uint32_t ScaleAnimationComplexity = 0u;
    };

    struct AnimationSamplingPlan
    {
        ShipAnimationType Type = ShipAnimationType::IDLE;
        AnimationSamplingMode Mode = AnimationSamplingMode::ADAPTIVE;
        uint32_t DurationMilliseconds = 0u;
        uint32_t ExactFrameCount = 0u;
        uint32_t MinimumFrameCount = 0u;
        uint32_t MaximumFrameCount = 0u;
        uint32_t ActualFrameCount = 0u;
        double ActualFrameDurationMilliseconds = 0.0;

        uint32_t MaximumMechanicalTravelPixels = 0u;
        uint32_t MaximumExhaustTravelPixels = 0u;
        uint32_t ActiveAnimatedComponentCount = 0u;
        uint32_t IndependentPhaseGroupCount = 0u;
        uint32_t MaximumTemporalCyclesPerClip = 0u;
        uint32_t ScaleAnimationComplexity = 0u;

        uint32_t TravelFrameRequirement = 0u;
        uint32_t ComponentFrameRequirement = 0u;
        uint32_t PhaseFrameRequirement = 0u;
        uint32_t TemporalFrameRequirement = 0u;
        uint32_t ScaleFrameRequirement = 0u;
    };

    class AnimationSamplingPlanner
    {
    public:
        AnimationSamplingPlan plan(const AnimationSamplingRequirements& requirements) const;
    };
}
