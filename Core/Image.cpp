#include <SpectralShipGen/Image.h>

#include <algorithm>
#include <cassert>

namespace SpectralShipGen
{
    Image::Image(uint32_t width, uint32_t height, const Color& fillColor)
    {
        reset(width, height, fillColor);
    }

    void Image::reset(uint32_t width, uint32_t height, const Color& fillColor)
    {
        m_Width = width;
        m_Height = height;

        const std::size_t pixelCount = static_cast<std::size_t>(m_Width) * static_cast<std::size_t>(m_Height);
        m_Pixels.assign(pixelCount, fillColor);
    }

    void Image::clear(const Color& color)
    {
        std::fill(m_Pixels.begin(), m_Pixels.end(), color);
    }

    uint32_t Image::getWidth() const
    {
        return m_Width;
    }

    uint32_t Image::getHeight() const
    {
        return m_Height;
    }

    bool Image::empty() const
    {
        return m_Pixels.empty();
    }

    bool Image::isInBounds(uint32_t x, uint32_t y) const
    {
        return x < m_Width && y < m_Height;
    }

    const Color& Image::getPixel(uint32_t x, uint32_t y) const
    {
        assert(isInBounds(x, y));

        return m_Pixels[getIndex(x, y)];
    }

    void Image::setPixel(uint32_t x, uint32_t y, const Color& color)
    {
        assert(isInBounds(x, y));

        m_Pixels[getIndex(x, y)] = color;
    }

    const std::vector<Color>& Image::getPixels() const
    {
        return m_Pixels;
    }

    std::size_t Image::getIndex(uint32_t x, uint32_t y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_Width) + static_cast<std::size_t>(x);
    }
} // namespace SpectralShipGen
