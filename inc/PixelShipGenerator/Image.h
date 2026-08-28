#pragma once

#include "Color.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace PixelShipGenerator
{
    class Image
    {
    public:
        Image() = default;
        Image(uint32_t width, uint32_t height, const Color& fillColor = Color());

        void reset(uint32_t width, uint32_t height, const Color& fillColor = Color());
        void clear(const Color& color = Color());

        uint32_t getWidth() const;
        uint32_t getHeight() const;

        bool empty() const;
        bool isInBounds(uint32_t x, uint32_t y) const;

        const Color& getPixel(uint32_t x, uint32_t y) const;
        void setPixel(uint32_t x, uint32_t y, const Color& color);

        const std::vector<Color>& getPixels() const;

    private:
        std::size_t getIndex(uint32_t x, uint32_t y) const;

    private:
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        std::vector<Color> m_Pixels;
    };
} // namespace PixelShipGenerator
