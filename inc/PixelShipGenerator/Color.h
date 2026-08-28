#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    struct Color
    {
        uint8_t R = 0;
        uint8_t G = 0;
        uint8_t B = 0;
        uint8_t A = 0;

        constexpr Color() = default;
        constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
            : R(r), G(g), B(b), A(a)
        {};

        constexpr bool operator==(const Color& other) const
        {
            return R == other.R && G == other.G && B == other.B && A == other.A;
        }

        constexpr bool operator!=(const Color& other) const
        {
            return !(*this == other);
        }
    };
} // namespace PixelShipGenerator
