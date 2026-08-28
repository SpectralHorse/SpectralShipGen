#pragma once

#include <cstdint>
#include <vector>

#include "PixelMask.h"
#include "ShipStructuralNegativeSpaceType.h"

namespace PixelShipGenerator
{
    struct StructuralNegativeSpacePlacement
    {
        ShipStructuralNegativeSpaceType Type = ShipStructuralNegativeSpaceType::WING_CHANNEL;
        uint32_t MinX = 0u;
        uint32_t MaxX = 0u;
        uint32_t MinY = 0u;
        uint32_t MaxY = 0u;
        uint32_t PixelCount = 0u;
    };

    struct StructuralNegativeSpaceData
    {
        void reset(uint32_t width, uint32_t height)
        {
            ReservedMask = PixelMask(width, height, false);
            Placements.clear();
        }

        bool empty() const { return Placements.empty(); }

        bool hasType(ShipStructuralNegativeSpaceType type) const
        {
            for (const StructuralNegativeSpacePlacement& placement : Placements)
            {
                if (placement.Type == type) { return true; }
            }
            return false;
        }

        PixelMask ReservedMask;
        std::vector<StructuralNegativeSpacePlacement> Placements;
    };
}
