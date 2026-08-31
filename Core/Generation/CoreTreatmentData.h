#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <SpectralShipGen/PixelMask.h>
#include <SpectralShipGen/ShipCoreTreatmentType.h>

namespace SpectralShipGen
{
    struct CoreTreatmentData
    {
        void reset(uint32_t width, uint32_t height)
        {
            CoreRegionMask.reset(width, height, false);
            RaisedMask.reset(width, height, false);
            RecessedMask.reset(width, height, false);
            SecondaryMaterialMask.reset(width, height, false);
            LuminousMask.reset(width, height, false);
            BoundaryMask.reset(width, height, false);
            TypeCounts.fill(0u);
            TreatmentCount = 0u;
            ComplexityCost = 0u;
            PlacementRejectionCount = 0u;
        }

        bool empty() const { return TreatmentCount == 0u; }

        PixelMask CoreRegionMask;
        PixelMask RaisedMask;
        PixelMask RecessedMask;
        PixelMask SecondaryMaterialMask;
        PixelMask LuminousMask;
        PixelMask BoundaryMask;
        std::array<uint32_t, static_cast<std::size_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)> TypeCounts = {};
        uint32_t TreatmentCount = 0u;
        uint32_t ComplexityCost = 0u;
        uint32_t PlacementRejectionCount = 0u;
    };
}
