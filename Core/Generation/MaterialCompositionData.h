#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <PixelShipGenerator/PixelMask.h>
#include <PixelShipGenerator/ShipMaterialZoneType.h>

namespace PixelShipGenerator
{
    struct MaterialZonePlacement
    {
        ShipMaterialZoneType Type = ShipMaterialZoneType::WING_SURFACE;
        bool Mechanical = false;
        PixelMask Mask;
    };

    struct MaterialCompositionData
    {
        void reset(uint32_t width, uint32_t height)
        {
            SecondaryHullMask.reset(width, height, false);
            MechanicalMask.reset(width, height, false);
            Placements.clear();
            TypeCounts.fill(0u);
            TargetZoneCount = 0u;
            ContrastStrengthPercent = 100u;
        }

        bool empty() const { return Placements.empty(); }

        PixelMask SecondaryHullMask;
        PixelMask MechanicalMask;
        std::vector<MaterialZonePlacement> Placements;
        std::array<uint32_t, static_cast<std::size_t>(ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END)> TypeCounts = {};
        uint32_t TargetZoneCount = 0u;
        uint32_t ContrastStrengthPercent = 100u;
    };
}
