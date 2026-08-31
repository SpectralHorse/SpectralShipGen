#include <PixelShipGenerator/AnimationSamplingPlanner.h>

#include <algorithm>
#include <cstdint>

namespace PixelShipGenerator
{
    AnimationSamplingPlan AnimationSamplingPlanner::plan(const AnimationSamplingRequirements& requirements) const
    {
        AnimationSamplingPlan result;
        result.Type = requirements.Type;
        result.Mode = requirements.Mode;
        result.DurationMilliseconds = std::max(1u, requirements.DurationMilliseconds);
        result.ExactFrameCount = std::max(1u, requirements.ExactFrameCount);
        result.MinimumFrameCount = std::max(1u, requirements.MinimumFrameCount);
        result.MaximumFrameCount = std::max(result.MinimumFrameCount, requirements.MaximumFrameCount);
        result.MaximumMechanicalTravelPixels = requirements.MaximumMechanicalTravelPixels;
        result.MaximumExhaustTravelPixels = requirements.MaximumExhaustTravelPixels;
        result.ActiveAnimatedComponentCount = requirements.ActiveAnimatedComponentCount;
        result.IndependentPhaseGroupCount = requirements.IndependentPhaseGroupCount;
        result.MaximumTemporalCyclesPerClip = requirements.MaximumTemporalCyclesPerClip;
        result.ScaleAnimationComplexity = requirements.ScaleAnimationComplexity;

        const uint32_t maximumTravel = std::max(requirements.MaximumMechanicalTravelPixels, requirements.MaximumExhaustTravelPixels);
        result.TravelFrameRequirement = maximumTravel == 0u ? 0u : 8u + maximumTravel * 4u;
        result.ComponentFrameRequirement = requirements.ActiveAnimatedComponentCount == 0u ? 0u : 8u + requirements.ActiveAnimatedComponentCount * 2u;
        result.PhaseFrameRequirement = requirements.IndependentPhaseGroupCount == 0u ? 0u : 8u + requirements.IndependentPhaseGroupCount * 3u;
        result.TemporalFrameRequirement = requirements.MaximumTemporalCyclesPerClip <= 1u ? 0u : 8u + (requirements.MaximumTemporalCyclesPerClip - 1u) * 6u;

        // Tiny-scale geometry has too few raster positions to benefit from phase/component-driven frame inflation.
        // Use the existing scale trait together with actual travel rather than a resolution lookup table.
        if (requirements.ScaleAnimationComplexity < 20u && maximumTravel <= 1u)
        {
            const uint32_t lowComplexityRequirementCap = std::max(10u, result.MinimumFrameCount);
            result.TravelFrameRequirement = std::min(result.TravelFrameRequirement, lowComplexityRequirementCap);
            result.ComponentFrameRequirement = std::min(result.ComponentFrameRequirement, lowComplexityRequirementCap);
            result.PhaseFrameRequirement = std::min(result.PhaseFrameRequirement, lowComplexityRequirementCap);
            result.TemporalFrameRequirement = std::min(result.TemporalFrameRequirement, lowComplexityRequirementCap);
        }

        if (maximumTravel >= 2u && requirements.ActiveAnimatedComponentCount >= 3u)
        {
            const uint32_t contributingComponents = std::min(8u, requirements.ActiveAnimatedComponentCount);
            result.ScaleFrameRequirement = 8u + static_cast<uint32_t>((static_cast<uint64_t>(requirements.ScaleAnimationComplexity) * contributingComponents + 39u) / 40u);
        }

        if (requirements.Mode == AnimationSamplingMode::EXACT_FRAME_COUNT)
        {
            result.ActualFrameCount = result.ExactFrameCount;
        }
        else
        {
            uint32_t actualFrameCount = result.MinimumFrameCount;
            actualFrameCount = std::max(actualFrameCount, result.TravelFrameRequirement);
            actualFrameCount = std::max(actualFrameCount, result.ComponentFrameRequirement);
            actualFrameCount = std::max(actualFrameCount, result.PhaseFrameRequirement);
            actualFrameCount = std::max(actualFrameCount, result.TemporalFrameRequirement);
            actualFrameCount = std::max(actualFrameCount, result.ScaleFrameRequirement);
            result.ActualFrameCount = std::clamp(actualFrameCount, result.MinimumFrameCount, result.MaximumFrameCount);
        }

        result.ActualFrameCount = std::max(1u, result.ActualFrameCount);
        result.ActualFrameDurationMilliseconds = static_cast<double>(result.DurationMilliseconds) / static_cast<double>(result.ActualFrameCount);
        return result;
    }
}
