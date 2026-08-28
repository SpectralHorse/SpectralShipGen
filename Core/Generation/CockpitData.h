#pragma once

#include <cstdint>

#include "PixelMask.h"
#include "ShipCockpitType.h"

namespace PixelShipGenerator
{
    struct CockpitData
    {
        void reset(uint32_t width, uint32_t height)
        {
            SizeClass = CockpitSizeClass::COMPACT;
            ShapeType = CockpitShapeType::COMPACT_CANOPY;
            GlassMask.reset(width, height, false);
            FrameMask.reset(width, height, false);
            BaseMask.reset(width, height, false);
            UpperSectionMask.reset(width, height, false);
            ComplexityCost = 0u;
        }

        CockpitSizeClass SizeClass = CockpitSizeClass::COMPACT;
        CockpitShapeType ShapeType = CockpitShapeType::COMPACT_CANOPY;
        PixelMask GlassMask;
        PixelMask FrameMask;
        PixelMask BaseMask;
        // Optional second raised stage used by DORSAL/LAYERED bridge forms.
        // It is a subset of GlassMask and represents discrete semantic height,
        // not a separate depth system.
        PixelMask UpperSectionMask;
        uint32_t ComplexityCost = 0u;
    };
}
