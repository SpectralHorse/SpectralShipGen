#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "PixelMask.h"
#include "ShipLiveryType.h"

namespace PixelShipGenerator
{
    struct LiveryPlacement
    {
        ShipLiveryType Type = ShipLiveryType::CENTER_STRIPE;
        bool Secondary = false;
        bool Asymmetric = false;
        PixelMask Mask;
    };

    struct LiveryData
    {
        void reset(uint32_t width, uint32_t height)
        {
            PrimaryMarkingMask.reset(width, height, false);
            SecondaryMarkingMask.reset(width, height, false);
            Placements.clear();
            TypeCounts.fill(0u);
            TargetMarkingCount = 0u;
        }

        bool empty() const { return Placements.empty(); }

        PixelMask PrimaryMarkingMask;
        PixelMask SecondaryMarkingMask;
        std::vector<LiveryPlacement> Placements;
        std::array<uint32_t, static_cast<std::size_t>(ShipLiveryType::SHIP_LIVERY_TYPE_END)> TypeCounts = {};
        uint32_t TargetMarkingCount = 0u;
    };
}
