#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    struct ShipDimensions
    {
        uint32_t Width = 64u;
        uint32_t Height = 64u;

        constexpr bool isSquare() const { return Width == Height; }
    };

    inline constexpr bool operator==(const ShipDimensions& first, const ShipDimensions& second)
    {
        return first.Width == second.Width && first.Height == second.Height;
    }

    inline constexpr bool operator!=(const ShipDimensions& first, const ShipDimensions& second)
    {
        return !(first == second);
    }
}
