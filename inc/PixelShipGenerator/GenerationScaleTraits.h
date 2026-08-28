#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>

#include "ShipDimensions.h"

namespace PixelShipGenerator
{
    enum class GenerationScaleTier : uint32_t
    {
        TINY = 0u,
        SMALL,
        MEDIUM,
        LARGE,
        GENERATION_SCALE_TIER_END
    };

    struct GenerationScaleTraits
    {
        ShipDimensions Dimensions;
        uint32_t MinimumDimension = 0u;
        uint32_t MaximumDimension = 0u;
        uint64_t CanvasArea = 0u;
        double AspectRatio = 1.0;

        uint32_t HorizontalCapacity = 0u;
        uint32_t LongitudinalCapacity = 0u;
        uint32_t SmallFeatureCapacity = 0u;
        uint32_t MajorFeatureCapacity = 0u;
        uint32_t DetailComplexity = 0u;
        uint32_t ShadingComplexity = 0u;
        uint32_t AttachmentComplexity = 0u;
        uint32_t AnimationComplexity = 0u;

        GenerationScaleTier Tier = GenerationScaleTier::TINY;

        static GenerationScaleTraits fromDimensions(const ShipDimensions& dimensions)
        {
            GenerationScaleTraits traits;
            traits.Dimensions = dimensions;
            traits.MinimumDimension = std::min(dimensions.Width, dimensions.Height);
            traits.MaximumDimension = std::max(dimensions.Width, dimensions.Height);
            traits.CanvasArea = static_cast<uint64_t>(dimensions.Width) * static_cast<uint64_t>(dimensions.Height);
            traits.AspectRatio = dimensions.Height == 0u ? 0.0 : static_cast<double>(dimensions.Width) / static_cast<double>(dimensions.Height);
            traits.HorizontalCapacity = calculateAxisCapacity(dimensions.Width);
            traits.LongitudinalCapacity = calculateAxisCapacity(dimensions.Height);

            const uint32_t minimumAxisCapacity = std::min(traits.HorizontalCapacity, traits.LongitudinalCapacity);
            const uint32_t maximumAxisCapacity = std::max(traits.HorizontalCapacity, traits.LongitudinalCapacity);
            traits.SmallFeatureCapacity = minimumAxisCapacity;
            traits.MajorFeatureCapacity = (minimumAxisCapacity * 2u + maximumAxisCapacity + 1u) / 3u;
            traits.DetailComplexity = (minimumAxisCapacity * 3u + maximumAxisCapacity + 2u) / 4u;
            traits.ShadingComplexity = minimumAxisCapacity;
            traits.AttachmentComplexity = (minimumAxisCapacity + maximumAxisCapacity + 1u) / 2u;
            traits.AnimationComplexity = minimumAxisCapacity;

            const uint32_t tierCapacity = (traits.SmallFeatureCapacity + traits.MajorFeatureCapacity + 1u) / 2u;
            if (tierCapacity < 15u) { traits.Tier = GenerationScaleTier::TINY; }
            else if (tierCapacity < 40u) { traits.Tier = GenerationScaleTier::SMALL; }
            else if (tierCapacity < 75u) { traits.Tier = GenerationScaleTier::MEDIUM; }
            else { traits.Tier = GenerationScaleTier::LARGE; }

            return traits;
        }

    private:
        static uint32_t calculateAxisCapacity(uint32_t dimension)
        {
            struct CapacityPoint
            {
                uint32_t Dimension;
                uint32_t Capacity;
            };

            constexpr std::array<CapacityPoint, 6u> Points =
            { {
                { 24u, 0u },
                { 32u, 20u },
                { 44u, 40u },
                { 64u, 60u },
                { 96u, 80u },
                { 160u, 100u }
            } };

            if (dimension <= Points.front().Dimension) { return Points.front().Capacity; }
            if (dimension >= Points.back().Dimension) { return Points.back().Capacity; }

            for (std::size_t index = 0u; index + 1u < Points.size(); ++index)
            {
                const CapacityPoint& lower = Points[index];
                const CapacityPoint& upper = Points[index + 1u];
                if (dimension > upper.Dimension) { continue; }

                const uint32_t dimensionRange = upper.Dimension - lower.Dimension;
                const uint32_t capacityRange = upper.Capacity - lower.Capacity;
                const uint32_t offset = dimension - lower.Dimension;
                return lower.Capacity + static_cast<uint32_t>((static_cast<uint64_t>(capacityRange) * offset + dimensionRange / 2u) / dimensionRange);
            }

            return Points.back().Capacity;
        }
    };

    inline const char* getGenerationScaleTierName(GenerationScaleTier tier)
    {
        switch (tier)
        {
        case GenerationScaleTier::TINY: return "TINY";
        case GenerationScaleTier::SMALL: return "SMALL";
        case GenerationScaleTier::MEDIUM: return "MEDIUM";
        case GenerationScaleTier::LARGE: return "LARGE";
        default: return "UNKNOWN";
        }
    }
}
