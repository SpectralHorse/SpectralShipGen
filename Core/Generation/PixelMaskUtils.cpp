#include "PixelMaskUtils.h"

#include <algorithm>
#include <cstdint>

namespace PixelShipGenerator::PixelMaskUtils
{
    bool isMaskPixel(const PixelMask& mask, int32_t x, int32_t y)
    {
        if (x < 0 || y < 0)
        {
            return false;
        }

        if (x >= static_cast<int32_t>(mask.getWidth()) || y >= static_cast<int32_t>(mask.getHeight()))
        {
            return false;
        }

        return mask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    }

    bool hasNeighbouringMaskPixel(const PixelMask& mask, int32_t x, int32_t y)
    {
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                if (isMaskPixel(mask, x + offsetX, y + offsetY))
                {
                    return true;
                }
            }
        }

        return false;
    }

    uint32_t getMaskNeighbourCount(const PixelMask& mask, int32_t x, int32_t y)
    {
        uint32_t count = 0u;

        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                if (isMaskPixel(mask, x + offsetX, y + offsetY))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    DirectionalMaskExposure getDirectionalMaskExposure(const PixelMask& mask, uint32_t x, uint32_t y)
    {
        DirectionalMaskExposure exposure;

        if (!mask.isInBounds(x, y) || !mask.get(x, y))
        {
            return exposure;
        }

        const int32_t pixelX = static_cast<int32_t>(x);
        const int32_t pixelY = static_cast<int32_t>(y);
        exposure.Left = !isMaskPixel(mask, pixelX - 1, pixelY);
        exposure.Right = !isMaskPixel(mask, pixelX + 1, pixelY);
        exposure.Top = !isMaskPixel(mask, pixelX, pixelY - 1);
        exposure.Bottom = !isMaskPixel(mask, pixelX, pixelY + 1);
        exposure.TopLeft = !isMaskPixel(mask, pixelX - 1, pixelY - 1);
        exposure.BottomRight = !isMaskPixel(mask, pixelX + 1, pixelY + 1);
        return exposure;
    }

    uint32_t getMaskDepth(const PixelMask& mask, uint32_t x, uint32_t y, uint32_t maximumDepth)
    {
        if (!mask.get(x, y))
        {
            return 0u;
        }

        for (uint32_t radius = 1u; radius <= maximumDepth; ++radius)
        {
            const int32_t minimumX = static_cast<int32_t>(x) - static_cast<int32_t>(radius);
            const int32_t maximumX = static_cast<int32_t>(x) + static_cast<int32_t>(radius);
            const int32_t minimumY = static_cast<int32_t>(y) - static_cast<int32_t>(radius);
            const int32_t maximumY = static_cast<int32_t>(y) + static_cast<int32_t>(radius);

            for (int32_t checkX = minimumX; checkX <= maximumX; ++checkX)
            {
                if (!isMaskPixel(mask, checkX, minimumY) || !isMaskPixel(mask, checkX, maximumY))
                {
                    return radius - 1u;
                }
            }

            for (int32_t checkY = minimumY + 1; checkY < maximumY; ++checkY)
            {
                if (!isMaskPixel(mask, minimumX, checkY) || !isMaskPixel(mask, maximumX, checkY))
                {
                    return radius - 1u;
                }
            }
        }

        return maximumDepth;
    }

    MaskBounds calculateMaskBounds(const PixelMask& mask)
    {
        MaskBounds bounds;
        bounds.MinX = mask.getWidth();
        bounds.MinY = mask.getHeight();

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y))
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

    bool masksOverlap(const PixelMask& firstMask, const PixelMask& secondMask)
    {
        for (uint32_t y = 0u; y < firstMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < firstMask.getWidth(); ++x)
            {
                if (firstMask.get(x, y) && secondMask.get(x, y))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void mergeMask(PixelMask& destinationMask, const PixelMask& sourceMask)
    {
        for (uint32_t y = 0u; y < sourceMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < sourceMask.getWidth(); ++x)
            {
                if (sourceMask.get(x, y))
                {
                    destinationMask.set(x, y, true);
                }
            }
        }
    }

    uint32_t getMaskPixelCount(const PixelMask& mask)
    {
        uint32_t pixelCount = 0u;

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y))
                {
                    ++pixelCount;
                }
            }
        }

        return pixelCount;
    }

    uint32_t getOccupiedRowWidth(const PixelMask& mask, uint32_t y)
    {
        uint32_t width = 0u;

        for (uint32_t x = 0u; x < mask.getWidth(); ++x)
        {
            if (mask.get(x, y))
            {
                ++width;
            }
        }

        return width;
    }

    uint32_t getSymmetricWidth(const PixelMask& mask, uint32_t halfWidth)
    {
        if (halfWidth == 0u)
        {
            return 0u;
        }

        return mask.getWidth() % 2u == 0u ? halfWidth * 2u : halfWidth * 2u - 1u;
    }

    void setSymmetricRowWidth(PixelMask& mask, uint32_t y, uint32_t width)
    {
        for (uint32_t x = 0u; x < mask.getWidth(); ++x)
        {
            mask.set(x, y, false);
        }

        if (width == 0u)
        {
            return;
        }

        const uint32_t leftCenter = (mask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = mask.getWidth() / 2u;
        const uint32_t halfWidth = (width + 1u) / 2u;
        const uint32_t leftX = leftCenter - (halfWidth - 1u);
        const uint32_t rightX = rightCenter + (halfWidth - 1u);

        for (uint32_t x = leftX; x <= rightX; ++x)
        {
            mask.set(x, y, true);
        }
    }

    void addMaskRectangle(PixelMask& mask, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height)
    {
        if (width == 0u || height == 0u)
        {
            return;
        }

        if (startX >= mask.getWidth() || startY >= mask.getHeight())
        {
            return;
        }

        const uint32_t endX = std::min(mask.getWidth(), startX + width);
        const uint32_t endY = std::min(mask.getHeight(), startY + height);

        for (uint32_t y = startY; y < endY; ++y)
        {
            for (uint32_t x = startX; x < endX; ++x)
            {
                mask.set(x, y, true);
            }
        }
    }
}
