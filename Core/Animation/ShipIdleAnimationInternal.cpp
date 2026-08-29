#include "ShipIdleAnimationInternal.h"

#include <algorithm>
#include <cmath>

namespace PixelShipGenerator
{
namespace IdleAnimationInternal
{
    bool isMaskPixel(const PixelShipGenerator::PixelMask& mask, int32_t x, int32_t y)
    {
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(mask.getWidth()) || y >= static_cast<int32_t>(mask.getHeight()))
        {
            return false;
        }

        return mask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    }

    uint32_t getContinuousLengthDelta(uint32_t available, double signal, uint32_t amplitudePercent)
    {
        if (available == 0u || signal == 0.0)
        {
            return 0u;
        }

        const double scaledSignal = std::clamp(std::abs(signal) * static_cast<double>(amplitudePercent) / 100.0, 0.0, 1.0);
        return std::min(available, static_cast<uint32_t>(std::floor(static_cast<double>(available) * scaledSignal + 0.5)));
    }

    uint32_t getAttachmentMaximumOutwardDistance(const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        switch (placement.Direction)
        {
        case PixelShipGenerator::ShipAttachmentDirection::LEFT: return placement.AnchorX - placement.MinimumX;
        case PixelShipGenerator::ShipAttachmentDirection::RIGHT: return placement.MaximumX - placement.AnchorX;
        case PixelShipGenerator::ShipAttachmentDirection::UP: return placement.AnchorY - placement.MinimumY;
        case PixelShipGenerator::ShipAttachmentDirection::DOWN: return placement.MaximumY - placement.AnchorY;
        default: return 0u;
        }
    }

    OpaqueBounds calculateOpaqueBounds(const PixelShipGenerator::Image& image, uint32_t width, uint32_t height)
    {
        OpaqueBounds bounds;
        bounds.MinX = width;
        bounds.MinY = height;

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (image.getPixel(x, y).A == 0u)
                {
                    continue;
                }

                bounds.MinX = std::min(bounds.MinX, x);
                bounds.MaxX = std::max(bounds.MaxX, x);
                bounds.MinY = std::min(bounds.MinY, y);
                bounds.MaxY = std::max(bounds.MaxY, y);
                bounds.Valid = true;
            }
        }

        return bounds;
    }

}
}
