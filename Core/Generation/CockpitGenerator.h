#pragma once

#include <cstdint>

#include "CockpitData.h"
#include <SpectralShipGen/PixelMask.h>
#include "ShipGenerationContext.h"

namespace SpectralShipGen
{
    class CockpitGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct CandidateCockpit
        {
            CandidateCockpit(uint32_t width, uint32_t height)
                : OccupiedMask(width, height, false)
            {
                Data.reset(width, height);
            }

            PixelMask OccupiedMask;
            CockpitData Data;
        };


        CockpitSizeClass selectSizeClass(ShipGenerationContext& context, uint32_t attempt) const;
        CockpitShapeType selectShapeType(ShipGenerationContext& context, CockpitSizeClass sizeClass) const;
        void generateCockpitFootprint(CandidateCockpit& candidate, CockpitShapeType shapeType, uint32_t startY, uint32_t height, uint32_t maximumHalfWidth) const;
        void deriveSemanticMasks(CandidateCockpit& candidate) const;
        bool isCockpitPlacementValid(const CandidateCockpit& candidate, const ShipGenerationContext& context, uint32_t maximumCockpitPixelCount) const;
        uint32_t getCockpitComplexityCost(CockpitSizeClass sizeClass, CockpitShapeType shapeType) const;
        uint32_t getMaximumCockpitHullPercent(const ShipGenerationContext& context, CockpitSizeClass sizeClass) const;

        static constexpr uint32_t MaximumCockpitGenerationAttempts = 20u;
    };
}
