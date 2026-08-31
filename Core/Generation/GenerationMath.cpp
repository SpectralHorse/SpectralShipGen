#include "GenerationMath.h"

#include <algorithm>
#include <cstdint>

namespace SpectralShipGen::GenerationMath
{
    uint32_t scalePixelsFrom64(uint32_t value, uint32_t dimension)
    {
        if (value == 0u)
        {
            return 0u;
        }

        return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(value) * dimension + ReferenceResolution / 2u) / ReferenceResolution));
    }

    uint32_t scaleHorizontalPixelsFrom64(uint32_t value, const ShipDimensions& dimensions)
    {
        return scalePixelsFrom64(value, dimensions.Width);
    }

    uint32_t scaleVerticalPixelsFrom64(uint32_t value, const ShipDimensions& dimensions)
    {
        return scalePixelsFrom64(value, dimensions.Height);
    }

    uint32_t scaleScalarPixelsFrom64(uint32_t value, const ShipDimensions& dimensions)
    {
        return scalePixelsFrom64(value, std::min(dimensions.Width, dimensions.Height));
    }

    uint32_t scaleByPercent(uint32_t value, uint32_t percentage)
    {
        if (value == 0u || percentage == 0u)
        {
            return 0u;
        }

        return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(value) * percentage + 50u) / 100u));
    }

    uint32_t getPercentage(uint32_t value, uint32_t percentage)
    {
        return static_cast<uint32_t>((static_cast<uint64_t>(value) * percentage) / 100u);
    }

    uint32_t getHalfWidthFromPercentage(uint32_t maximumHalfWidth, uint32_t percentage)
    {
        return std::max(1u, getPercentage(maximumHalfWidth, percentage));
    }

    uint32_t getDifference(uint32_t first, uint32_t second)
    {
        return first > second ? first - second : second - first;
    }
}
