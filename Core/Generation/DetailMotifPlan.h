#pragma once

#include <cstdint>

#include <PixelShipGenerator/GenerationSpatialBudget.h>
#include <PixelShipGenerator/PixelMask.h>
#include <PixelShipGenerator/ShipDetailMotifType.h>

namespace PixelShipGenerator
{
    struct DetailMotifPlan
    {
        void reset(uint32_t width, uint32_t height)
        {
            PrimaryMotif = ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
            SecondaryMotif = ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
            PrimaryPreferredRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
            SecondaryPreferredRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
            PrimaryTargetOccurrences = 0u;
            SecondaryTargetOccurrences = 0u;
            PrimaryOccurrences = 0u;
            SecondaryOccurrences = 0u;
            RejectedPlacements = 0u;
            MirroredChance = 80u;
            PrimaryMask.reset(width, height, false);
            SecondaryMask.reset(width, height, false);
        }

        bool hasPrimary() const { return PrimaryMotif != ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END; }
        bool hasSecondary() const { return SecondaryMotif != ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END; }
        uint32_t getOccurrenceCount() const { return PrimaryOccurrences + SecondaryOccurrences; }

        ShipDetailMotifType PrimaryMotif = ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
        ShipDetailMotifType SecondaryMotif = ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
        GenerationSpatialRegion PrimaryPreferredRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        GenerationSpatialRegion SecondaryPreferredRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        uint32_t PrimaryTargetOccurrences = 0u;
        uint32_t SecondaryTargetOccurrences = 0u;
        uint32_t PrimaryOccurrences = 0u;
        uint32_t SecondaryOccurrences = 0u;
        uint32_t RejectedPlacements = 0u;
        uint32_t MirroredChance = 80u;
        PixelMask PrimaryMask;
        PixelMask SecondaryMask;
    };
}
