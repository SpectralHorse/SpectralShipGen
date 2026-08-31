#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace SpectralShipGen
{
    class PixelMask
    {
    public:
        PixelMask() = default;
        PixelMask(uint32_t width, uint32_t height, bool fillValue = false);

        void reset(uint32_t width, uint32_t height, bool fillValue = false);
        void clear(bool fillValue = false);

        uint32_t getWidth() const;
        uint32_t getHeight() const;

        bool empty() const;

        bool isInBounds(uint32_t x, uint32_t y) const;
        bool get(uint32_t x, uint32_t y) const;
        void set(uint32_t x, uint32_t y, bool value = true);

    private:
        std::size_t getIndex(uint32_t x, uint32_t y) const;

    private:
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        std::vector<uint8_t> m_Data;
    };
} // namespace SpectralShipGen
