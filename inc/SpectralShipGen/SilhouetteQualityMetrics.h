#pragma once

#include <cstdint>

#include <SpectralShipGen/PixelMask.h>

namespace SpectralShipGen
{
    enum class SilhouetteValidationFailureReason : uint32_t
    {
        NONE = 0u,
        BASE_GEOMETRY_VALIDATION,
        INVALID_WING_REGIONS,
        LOW_LATERAL_UTILIZATION,
        LOW_LONGITUDINAL_UTILIZATION,
        LOW_ARTICULATION,
        EXCESSIVE_SOLID_MASS,
        SILHOUETTE_VALIDATION_FAILURE_REASON_END
    };

    struct SilhouetteQualityMetrics
    {
        uint32_t PixelCount = 0u;
        uint32_t OccupiedWidth = 0u;
        uint32_t OccupiedHeight = 0u;
        uint32_t BoundingArea = 0u;

        uint32_t NormalizedWidthPercent = 0u;
        uint32_t NormalizedHeightPercent = 0u;
        uint32_t CanvasFillPercent = 0u;
        uint32_t BoundingFillPercent = 0u;
        uint32_t WidthVariationPercent = 0u;

        uint32_t ArticulationCount = 0u;
        uint32_t ShoulderProminencePercent = 0u;
        uint32_t InteriorContractionPercent = 0u;
        uint32_t NoseTaperPercent = 0u;
        uint32_t RearTaperPercent = 0u;
        uint32_t LongestStableWidthRun = 0u;
        uint32_t LongestStableWidthRunPercent = 0u;
        uint32_t NearMaximumRowPercent = 0u;
        uint32_t MaximumRowWidthDelta = 0u;

        uint32_t TopUnusedMargin = 0u;
        uint32_t BottomUnusedMargin = 0u;
        uint32_t LeftUnusedMargin = 0u;
        uint32_t RightUnusedMargin = 0u;
    };

    SilhouetteQualityMetrics calculateSilhouetteQualityMetrics(const PixelMask& hullMask);
    const char* getSilhouetteValidationFailureReasonName(SilhouetteValidationFailureReason reason);
}
