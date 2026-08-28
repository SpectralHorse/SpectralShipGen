#pragma once

#include <cstdint>
#include <vector>

#include "PixelMask.h"
#include "ShipMajorFeatureType.h"

namespace PixelShipGenerator
{
    enum class MajorFeatureRegion : uint32_t
    {
        CENTRAL_FUSELAGE = 0u,
        FORWARD_FUSELAGE,
        REAR_FUSELAGE,
        WING,
        WING_ROOT
    };

    struct MajorFeaturePlacement
    {
        ShipMajorFeatureType Type = ShipMajorFeatureType::CENTRAL_SPINE;
        MajorFeatureRegion Region = MajorFeatureRegion::CENTRAL_FUSELAGE;
        uint32_t MinX = 0u;
        uint32_t MaxX = 0u;
        uint32_t MinY = 0u;
        uint32_t MaxY = 0u;
        bool Symmetric = true;
    };

    struct MajorFeatureData
    {
        void reset(uint32_t width, uint32_t height)
        {
            OccupiedMask = PixelMask(width, height, false);
            RaisedMask = PixelMask(width, height, false);
            RecessedMask = PixelMask(width, height, false);
            MechanicalMask = PixelMask(width, height, false);
            EmissiveMask = PixelMask(width, height, false);
            Placements.clear();
        }

        bool empty() const
        {
            return Placements.empty();
        }

        PixelMask OccupiedMask;
        PixelMask RaisedMask;
        PixelMask RecessedMask;
        PixelMask MechanicalMask;
        PixelMask EmissiveMask;
        std::vector<MajorFeaturePlacement> Placements;
    };
}
