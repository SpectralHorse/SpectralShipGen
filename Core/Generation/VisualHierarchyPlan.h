#pragma once

#include <cstdint>

#include "GenerationComplexityBudget.h"
#include "GenerationSpatialBudget.h"
#include "ShipVisualAnchorType.h"

namespace PixelShipGenerator
{
    struct VisualHierarchyPlan
    {
        ShipVisualAnchorType PrimaryAnchor = ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
        ShipVisualAnchorType SecondaryAnchor = ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
        GenerationComplexityCategory ReservedCategory = GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END;
        GenerationSpatialRegion TargetRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        uint32_t ReservedComplexity = 0u;
        uint32_t PrimaryInfluencePercent = 100u;
        uint32_t SecondaryInfluencePercent = 100u;
        bool FallbackOccurred = false;
        bool InfluenceEnabled = true;

        bool isPrimary(ShipVisualAnchorType type) const { return PrimaryAnchor == type; }
        bool isSecondary(ShipVisualAnchorType type) const { return SecondaryAnchor == type; }
        bool targets(ShipVisualAnchorType type) const { return isPrimary(type) || isSecondary(type); }

        uint32_t getAnchorWeightPercent(ShipVisualAnchorType type) const
        {
            if (!InfluenceEnabled) { return 100u; }
            if (isPrimary(type)) { return PrimaryInfluencePercent; }
            if (isSecondary(type)) { return SecondaryInfluencePercent; }
            return 100u;
        }

        uint32_t getCompetingFeaturePercent(ShipVisualAnchorType type) const
        {
            if (!InfluenceEnabled || targets(type)) { return 100u; }
            return 88u;
        }
    };
}
