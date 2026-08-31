#pragma once

#include <cstdint>

#include <PixelShipGenerator/PixelMask.h>

namespace PixelShipGenerator::PixelMaskUtils
{
    struct MaskBounds
    {
        uint32_t MinX = 0u;
        uint32_t MaxX = 0u;
        uint32_t MinY = 0u;
        uint32_t MaxY = 0u;
        bool Valid = false;
    };

    struct DirectionalMaskExposure
    {
        bool Left = false;
        bool Right = false;
        bool Top = false;
        bool Bottom = false;
        bool TopLeft = false;
        bool BottomRight = false;

        uint32_t getHighlightExposure() const
        {
            return static_cast<uint32_t>(Left) + static_cast<uint32_t>(Top) + static_cast<uint32_t>(TopLeft);
        }

        uint32_t getShadowExposure() const
        {
            return static_cast<uint32_t>(Right) + static_cast<uint32_t>(Bottom) + static_cast<uint32_t>(BottomRight);
        }

        bool isBoundary() const
        {
            return Left || Right || Top || Bottom;
        }
    };

    bool isMaskPixel(const PixelMask& mask, int32_t x, int32_t y);
    bool hasNeighbouringMaskPixel(const PixelMask& mask, int32_t x, int32_t y);
    uint32_t getMaskNeighbourCount(const PixelMask& mask, int32_t x, int32_t y);
    DirectionalMaskExposure getDirectionalMaskExposure(const PixelMask& mask, uint32_t x, uint32_t y);
    uint32_t getMaskDepth(const PixelMask& mask, uint32_t x, uint32_t y, uint32_t maximumDepth);
    MaskBounds calculateMaskBounds(const PixelMask& mask);
    bool masksOverlap(const PixelMask& firstMask, const PixelMask& secondMask);
    void mergeMask(PixelMask& destinationMask, const PixelMask& sourceMask);
    uint32_t getMaskPixelCount(const PixelMask& mask);
    uint32_t getMaskOverlapPixelCount(const PixelMask& firstMask, const PixelMask& secondMask);
    uint32_t getLargestConnectedMaskPixelCount(const PixelMask& mask);
    uint32_t getOccupiedRowWidth(const PixelMask& mask, uint32_t y);
    uint32_t getSymmetricWidth(const PixelMask& mask, uint32_t halfWidth);
    void setSymmetricRowWidth(PixelMask& mask, uint32_t y, uint32_t width);
    void addMaskRectangle(PixelMask& mask, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height);
}
