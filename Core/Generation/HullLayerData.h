#pragma once

#include <cstdint>
#include <vector>

#include <SpectralShipGen/PixelMask.h>
#include <SpectralShipGen/ShipHullLayerType.h>

namespace SpectralShipGen
{
    struct HullLayerPlacement
    {
        ShipHullLayerType Type = ShipHullLayerType::CENTRAL_DORSAL_PLATE;
        uint32_t Order = 0u;
        uint32_t MinX = 0u;
        uint32_t MaxX = 0u;
        uint32_t MinY = 0u;
        uint32_t MaxY = 0u;
        PixelMask Mask;
    };

    struct HullLayerData
    {
        void reset(uint32_t width, uint32_t height)
        {
            OccupiedMask = PixelMask(width, height, false);
            LowerMask = PixelMask(width, height, false);
            UpperMask = PixelMask(width, height, false);
            BoundaryMask = PixelMask(width, height, false);
            Placements.clear();
        }

        bool empty() const { return Placements.empty(); }

        PixelMask OccupiedMask;
        PixelMask LowerMask;
        PixelMask UpperMask;
        PixelMask BoundaryMask;
        std::vector<HullLayerPlacement> Placements;
    };
}
