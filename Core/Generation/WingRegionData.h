#pragma once

#include <cstdint>
#include <vector>

#include "PixelMask.h"
#include "ShipWingShapeType.h"

namespace PixelShipGenerator
{
    struct WingRegionData
    {
        void reset(uint32_t width, uint32_t height)
        {
            Shape = WingShapeType::NONE;
            WingMask = PixelMask(width, height, false);
            WingRootMask = PixelMask(width, height, false);
            OuterWingMask = PixelMask(width, height, false);
            FuselageHalfWidths.assign(height, 0u);
            StartY = 0u;
            PeakY = 0u;
            EndY = 0u;
            MaximumSpan = 0u;
            MaximumExtension = 0u;
            RootThickness = 0u;
        }

        bool hasWings() const
        {
            return Shape != WingShapeType::NONE && MaximumExtension > 0u;
        }

        WingShapeType Shape = WingShapeType::NONE;
        PixelMask WingMask;
        PixelMask WingRootMask;
        PixelMask OuterWingMask;
        std::vector<uint32_t> FuselageHalfWidths;
        uint32_t StartY = 0u;
        uint32_t PeakY = 0u;
        uint32_t EndY = 0u;
        uint32_t MaximumSpan = 0u;
        uint32_t MaximumExtension = 0u;
        uint32_t RootThickness = 0u;
    };
}
