#include <PixelShipGenerator/PixelMask.h>

#include <algorithm>
#include <cassert>

namespace PixelShipGenerator
{
    PixelMask::PixelMask(uint32_t width, uint32_t height, bool fillValue)
    {
        reset(width, height, fillValue);
    }

    void PixelMask::reset(uint32_t width, uint32_t height, bool fillValue)
    {
        m_Width = width;
        m_Height = height;

        const std::size_t pixelCount = static_cast<std::size_t>(m_Width) * static_cast<std::size_t>(m_Height);
        m_Data.assign(pixelCount, fillValue ? 1u : 0u);
    }

    void PixelMask::clear(bool fillValue)
    {
        std::fill(m_Data.begin(), m_Data.end(), fillValue ? 1u : 0u);
    }

    uint32_t PixelMask::getWidth() const
    {
        return m_Width;
    }

    uint32_t PixelMask::getHeight() const
    {
        return m_Height;
    }

    bool PixelMask::empty() const
    {
        return m_Data.empty();
    }

    bool PixelMask::isInBounds(uint32_t x, uint32_t y) const
    {
        return x < m_Width && y < m_Height;
    }

    bool PixelMask::get(uint32_t x, uint32_t y) const
    {
        assert(isInBounds(x, y));

        return m_Data[getIndex(x, y)] != 0;
    }

    void PixelMask::set(uint32_t x, uint32_t y, bool value)
    {
        assert(isInBounds(x, y));

        m_Data[getIndex(x, y)] = value ? 1u : 0u;
    }

    std::size_t PixelMask::getIndex(uint32_t x, uint32_t y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_Width) + static_cast<std::size_t>(x);
    }
} // namespace PixelShipGenerator
