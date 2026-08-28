#pragma once

#include <cstdint>

#include "ShipDimensions.h"

namespace PixelShipGenerator::GenerationMath
{
    constexpr uint32_t ReferenceResolution = 64u;

    uint32_t scalePixelsFrom64(uint32_t value, uint32_t dimension);
    uint32_t scaleHorizontalPixelsFrom64(uint32_t value, const ShipDimensions& dimensions);
    uint32_t scaleVerticalPixelsFrom64(uint32_t value, const ShipDimensions& dimensions);
    uint32_t scaleScalarPixelsFrom64(uint32_t value, const ShipDimensions& dimensions);
    uint32_t scaleByPercent(uint32_t value, uint32_t percentage);
    uint32_t getPercentage(uint32_t value, uint32_t percentage);
    uint32_t getHalfWidthFromPercentage(uint32_t maximumHalfWidth, uint32_t percentage);
    uint32_t getDifference(uint32_t first, uint32_t second);
}
