#pragma once

#include "ShipGenerationContext.h"

namespace PixelShipGenerator
{
    class VisualHierarchyPlanner
    {
    public:
        void createPlan(ShipGenerationContext& context) const;
        void resolveAfterHull(ShipGenerationContext& context) const;
        void applySpatialPreference(ShipGenerationContext& context) const;

        static GenerationComplexityCategory getReservedCategory(ShipVisualAnchorType anchor);
        static GenerationSpatialRegion getTargetRegion(const ShipGenerationContext& context, ShipVisualAnchorType anchor);
        static uint32_t applyAnchorPercent(const ShipGenerationContext& context, ShipVisualAnchorType anchor, uint32_t value);
        static uint32_t applyCompetingPercent(const ShipGenerationContext& context, ShipVisualAnchorType anchor, uint32_t value);

    private:
        bool isAnchorFeasible(const ShipGenerationContext& context, ShipVisualAnchorType anchor, bool hullAvailable) const;
        ShipVisualAnchorType selectPrimary(const ShipGenerationContext& context, bool hullAvailable, ShipVisualAnchorType excluded = ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END) const;
        ShipVisualAnchorType selectSecondary(const ShipGenerationContext& context, ShipVisualAnchorType primary) const;
        uint32_t getStyleWeight(const ShipGenerationContext& context, ShipVisualAnchorType anchor) const;
        uint32_t getFactionWeightPercent(ShipFactionType faction, ShipVisualAnchorType anchor) const;
        uint64_t getPlanningSeed(const ShipGenerationContext& context, uint64_t salt) const;
    };
}
