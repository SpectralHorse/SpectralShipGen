#include "PixelMaskUtils.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <utility>
#include <vector>

namespace SpectralShipGen::PixelMaskUtils
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

    uint32_t getMaskOverlapPixelCount(const PixelMask& firstMask, const PixelMask& secondMask)
    {
        uint32_t pixelCount = 0u;
        for (uint32_t y = 0u; y < firstMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < firstMask.getWidth(); ++x)
            {
                if (firstMask.get(x, y) && secondMask.get(x, y)) { ++pixelCount; }
            }
        }
        return pixelCount;
    }

    uint32_t getLargestConnectedMaskPixelCount(const PixelMask& mask)
    {
        if (mask.empty()) { return 0u; }
        const uint32_t width = mask.getWidth();
        const uint32_t height = mask.getHeight();
        std::vector<uint8_t> visited(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0u);
        uint32_t largest = 0u;
        constexpr int32_t OffsetX[4] = { 1, -1, 0, 0 };
        constexpr int32_t OffsetY[4] = { 0, 0, 1, -1 };

        for (uint32_t startY = 0u; startY < height; ++startY)
        {
            for (uint32_t startX = 0u; startX < width; ++startX)
            {
                const std::size_t startIndex = static_cast<std::size_t>(startY) * width + startX;
                if (!mask.get(startX, startY) || visited[startIndex] != 0u) { continue; }

                uint32_t componentSize = 0u;
                std::deque<std::pair<uint32_t, uint32_t>> pending;
                pending.emplace_back(startX, startY);
                visited[startIndex] = 1u;

                while (!pending.empty())
                {
                    const auto [x, y] = pending.front();
                    pending.pop_front();
                    ++componentSize;

                    for (uint32_t direction = 0u; direction < 4u; ++direction)
                    {
                        const int32_t nextX = static_cast<int32_t>(x) + OffsetX[direction];
                        const int32_t nextY = static_cast<int32_t>(y) + OffsetY[direction];
                        if (nextX < 0 || nextY < 0 || nextX >= static_cast<int32_t>(width) || nextY >= static_cast<int32_t>(height)) { continue; }
                        const uint32_t ux = static_cast<uint32_t>(nextX);
                        const uint32_t uy = static_cast<uint32_t>(nextY);
                        const std::size_t index = static_cast<std::size_t>(uy) * width + ux;
                        if (!mask.get(ux, uy) || visited[index] != 0u) { continue; }
                        visited[index] = 1u;
                        pending.emplace_back(ux, uy);
                    }
                }
                largest = std::max(largest, componentSize);
            }
        }
        return largest;
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
