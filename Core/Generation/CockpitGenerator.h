#pragma once

#include <cstdint>

#include "CockpitData.h"
#include <PixelShipGenerator/PixelMask.h>
#include "ShipGenerationContext.h"

namespace PixelShipGenerator
{
    class CockpitGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        enum class LegacyCockpitShape : uint32_t
        {
            FORWARD_TAPER = 0u,
            CANOPY,
            DIAMOND,
            LEGACY_COCKPIT_SHAPE_END
        };

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

        void generateLegacyCockpit(ShipGenerationContext& context) const;
        void generateLegacyCockpitShape(PixelMask& cockpitMask, LegacyCockpitShape shape, uint32_t startY, uint32_t height, uint32_t maximumHalfWidth) const;
        bool isLegacyCockpitPlacementValid(const PixelMask& cockpitMask, const PixelMask& hullMask) const;

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
