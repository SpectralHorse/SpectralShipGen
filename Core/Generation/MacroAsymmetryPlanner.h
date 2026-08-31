#pragma once

#include <cstdint>

#include <SpectralShipGen/MacroAsymmetry.h>
#include <SpectralShipGen/PixelMask.h>
#include "ShipGenerationContext.h"

namespace SpectralShipGen
{
    class MacroAsymmetryPlanner
    {
    public:
        void createPlan(ShipGenerationContext& context) const;

        static bool candidateMatchesDominantSide(const ShipGenerationContext& context, const PixelMask& mask);
        static uint32_t calculateVisualWeight(const ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost);
        static uint32_t calculateBalanceScore(const ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost);
        static bool canAcceptCandidate(const ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost);
        static void fulfill(ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost);
        static void reject(ShipGenerationContext& context);
        static GenerationSpatialRegion getOppositeRegion(const ShipGenerationContext& context);
    };
}
