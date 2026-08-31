#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <SpectralShipGen/AnimationSamplingPlanner.h>
#include <SpectralShipGen/Image.h>
#include <SpectralShipGen/ShipAnimationType.h>

namespace SpectralShipGen
{
    struct ShipIdleAnimationSettings
    {
        uint32_t AnimationDurationMilliseconds = 1500u;

        // Authoritative only in EXACT_FRAME_COUNT mode. ADAPTIVE mode derives the actual
        // count from animation complexity within the configured bounds.
        uint32_t ExactFrameCount = 10u;
        uint32_t MinimumFrameCount = 10u;
        uint32_t MaximumFrameCount = 60u;
        AnimationSamplingMode SamplingMode = AnimationSamplingMode::ADAPTIVE;

        bool EngineFlicker = true;
        bool LightBlinking = true;
        bool MechanicalMicroMovement = true;
        bool HoverOffset = true;
        bool SmallDetailVariation = true;
        std::optional<uint64_t> Seed;
    };

    struct ShipIdleAnimation
    {
        ShipAnimationType Type = ShipAnimationType::IDLE;
        std::vector<Image> Frames;
        std::vector<double> NormalizedSampleTimes;
        uint32_t FrameWidth = 0u;
        uint32_t FrameHeight = 0u;
        uint64_t Seed = 0u;
        uint32_t DurationMilliseconds = 0u;
        double FrameDurationMilliseconds = 0.0;
        AnimationSamplingPlan Sampling;
    };
}
