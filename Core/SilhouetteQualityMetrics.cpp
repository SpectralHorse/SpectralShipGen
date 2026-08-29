#include <PixelShipGenerator/SilhouetteQualityMetrics.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace PixelShipGenerator
{
    namespace
    {
        uint32_t percentOf(uint64_t value, uint64_t total)
        {
            return total == 0u ? 0u : static_cast<uint32_t>((value * 100u + total / 2u) / total);
        }

        uint32_t averageRange(const std::vector<uint32_t>& values, std::size_t begin, std::size_t end)
        {
            if (values.empty() || begin >= values.size() || begin >= end)
            {
                return 0u;
            }

            end = std::min(end, values.size());
            uint64_t total = 0u;
            for (std::size_t index = begin; index < end; ++index)
            {
                total += values[index];
            }
            return static_cast<uint32_t>((total + (end - begin) / 2u) / (end - begin));
        }

        uint32_t median3(uint32_t a, uint32_t b, uint32_t c)
        {
            return a + b + c - std::min({ a, b, c }) - std::max({ a, b, c });
        }
    }

    SilhouetteQualityMetrics calculateSilhouetteQualityMetrics(const PixelMask& hullMask)
    {
        SilhouetteQualityMetrics metrics;
        if (hullMask.empty() || hullMask.getWidth() == 0u || hullMask.getHeight() == 0u)
        {
            return metrics;
        }

        uint32_t minX = hullMask.getWidth();
        uint32_t maxX = 0u;
        uint32_t minY = hullMask.getHeight();
        uint32_t maxY = 0u;
        bool occupied = false;
        std::vector<uint32_t> fullRowWidths(hullMask.getHeight(), 0u);

        for (uint32_t y = 0u; y < hullMask.getHeight(); ++y)
        {
            uint32_t rowWidth = 0u;
            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (!hullMask.get(x, y))
                {
                    continue;
                }

                occupied = true;
                ++metrics.PixelCount;
                ++rowWidth;
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
            fullRowWidths[y] = rowWidth;
        }

        if (!occupied)
        {
            return metrics;
        }

        metrics.OccupiedWidth = maxX - minX + 1u;
        metrics.OccupiedHeight = maxY - minY + 1u;
        metrics.BoundingArea = metrics.OccupiedWidth * metrics.OccupiedHeight;
        metrics.NormalizedWidthPercent = percentOf(metrics.OccupiedWidth, hullMask.getWidth());
        metrics.NormalizedHeightPercent = percentOf(metrics.OccupiedHeight, hullMask.getHeight());
        metrics.CanvasFillPercent = percentOf(metrics.PixelCount, static_cast<uint64_t>(hullMask.getWidth()) * hullMask.getHeight());
        metrics.BoundingFillPercent = percentOf(metrics.PixelCount, metrics.BoundingArea);
        metrics.TopUnusedMargin = minY;
        metrics.BottomUnusedMargin = hullMask.getHeight() - 1u - maxY;
        metrics.LeftUnusedMargin = minX;
        metrics.RightUnusedMargin = hullMask.getWidth() - 1u - maxX;

        std::vector<uint32_t> widths;
        widths.reserve(metrics.OccupiedHeight);
        uint32_t minimumWidth = std::numeric_limits<uint32_t>::max();
        uint32_t maximumWidth = 0u;
        uint32_t previousWidth = 0u;
        bool hasPrevious = false;
        for (uint32_t y = minY; y <= maxY; ++y)
        {
            const uint32_t width = fullRowWidths[y];
            widths.push_back(width);
            minimumWidth = std::min(minimumWidth, width);
            maximumWidth = std::max(maximumWidth, width);
            if (hasPrevious)
            {
                const uint32_t delta = width > previousWidth ? width - previousWidth : previousWidth - width;
                metrics.MaximumRowWidthDelta = std::max(metrics.MaximumRowWidthDelta, delta);
            }
            previousWidth = width;
            hasPrevious = true;
        }

        if (maximumWidth == 0u)
        {
            return metrics;
        }

        metrics.WidthVariationPercent = percentOf(maximumWidth - minimumWidth, maximumWidth);

        std::vector<uint32_t> smoothed = widths;
        if (widths.size() >= 3u)
        {
            for (std::size_t index = 1u; index + 1u < widths.size(); ++index)
            {
                smoothed[index] = median3(widths[index - 1u], widths[index], widths[index + 1u]);
            }
        }

        const uint32_t stableTolerance = std::max(1u, hullMask.getWidth() / 64u);
        uint32_t stableRun = smoothed.empty() ? 0u : 1u;
        metrics.LongestStableWidthRun = stableRun;
        for (std::size_t index = 1u; index < smoothed.size(); ++index)
        {
            const uint32_t delta = smoothed[index] > smoothed[index - 1u] ? smoothed[index] - smoothed[index - 1u] : smoothed[index - 1u] - smoothed[index];
            stableRun = delta <= stableTolerance ? stableRun + 1u : 1u;
            metrics.LongestStableWidthRun = std::max(metrics.LongestStableWidthRun, stableRun);
        }
        metrics.LongestStableWidthRunPercent = percentOf(metrics.LongestStableWidthRun, metrics.OccupiedHeight);

        const uint32_t articulationThreshold = std::max({ 1u, hullMask.getWidth() / 40u, maximumWidth / 16u });
        uint32_t anchorWidth = smoothed.front();
        int32_t direction = 0;
        for (std::size_t index = 1u; index < smoothed.size(); ++index)
        {
            const int32_t difference = static_cast<int32_t>(smoothed[index]) - static_cast<int32_t>(anchorWidth);
            if (static_cast<uint32_t>(difference < 0 ? -difference : difference) < articulationThreshold)
            {
                continue;
            }

            const int32_t newDirection = difference > 0 ? 1 : -1;
            if (direction == 0 || newDirection != direction)
            {
                ++metrics.ArticulationCount;
                direction = newDirection;
            }
            anchorWidth = smoothed[index];
        }

        const std::size_t rowCount = smoothed.size();
        const std::size_t shoulderBegin = rowCount * 20u / 100u;
        const std::size_t shoulderEnd = std::max(shoulderBegin + 1u, rowCount * 48u / 100u);
        uint32_t shoulderMaximum = 0u;
        for (std::size_t index = shoulderBegin; index < std::min(shoulderEnd, rowCount); ++index)
        {
            shoulderMaximum = std::max(shoulderMaximum, smoothed[index]);
        }
        const uint32_t frontReference = averageRange(smoothed, rowCount * 5u / 100u, std::max<std::size_t>(1u, rowCount * 18u / 100u));
        const uint32_t rearReference = averageRange(smoothed, rowCount * 50u / 100u, std::max<std::size_t>(rowCount * 50u / 100u + 1u, rowCount * 66u / 100u));
        const uint32_t shoulderReference = std::max(frontReference, rearReference);
        if (shoulderMaximum > shoulderReference)
        {
            metrics.ShoulderProminencePercent = percentOf(shoulderMaximum - shoulderReference, maximumWidth);
        }

        uint32_t maximumInteriorContraction = 0u;
        if (rowCount >= 5u)
        {
            for (std::size_t index = rowCount / 4u; index < rowCount * 3u / 4u; ++index)
            {
                uint32_t beforeMaximum = 0u;
                uint32_t afterMaximum = 0u;
                for (std::size_t before = 0u; before < index; ++before) { beforeMaximum = std::max(beforeMaximum, smoothed[before]); }
                for (std::size_t after = index + 1u; after < rowCount; ++after) { afterMaximum = std::max(afterMaximum, smoothed[after]); }
                const uint32_t surroundingMaximum = std::min(beforeMaximum, afterMaximum);
                if (surroundingMaximum > smoothed[index])
                {
                    maximumInteriorContraction = std::max(maximumInteriorContraction, surroundingMaximum - smoothed[index]);
                }
            }
        }
        metrics.InteriorContractionPercent = percentOf(maximumInteriorContraction, maximumWidth);

        const std::size_t taperRows = std::max<std::size_t>(1u, rowCount * 15u / 100u);
        const uint32_t noseWidth = averageRange(smoothed, 0u, taperRows);
        const uint32_t rearWidth = averageRange(smoothed, rowCount - taperRows, rowCount);
        metrics.NoseTaperPercent = maximumWidth > noseWidth ? percentOf(maximumWidth - noseWidth, maximumWidth) : 0u;
        metrics.RearTaperPercent = maximumWidth > rearWidth ? percentOf(maximumWidth - rearWidth, maximumWidth) : 0u;

        const uint32_t nearMaximumTolerance = std::max(1u, hullMask.getWidth() / 32u);
        const uint32_t nearMaximumThreshold = maximumWidth > nearMaximumTolerance ? maximumWidth - nearMaximumTolerance : maximumWidth;
        uint32_t nearMaximumRows = 0u;
        for (uint32_t width : widths)
        {
            if (width >= nearMaximumThreshold)
            {
                ++nearMaximumRows;
            }
        }
        metrics.NearMaximumRowPercent = percentOf(nearMaximumRows, metrics.OccupiedHeight);
        return metrics;
    }

    const char* getSilhouetteValidationFailureReasonName(SilhouetteValidationFailureReason reason)
    {
        switch (reason)
        {
        case SilhouetteValidationFailureReason::NONE: return "NONE";
        case SilhouetteValidationFailureReason::LEGACY_GEOMETRY_VALIDATION: return "LEGACY_GEOMETRY_VALIDATION";
        case SilhouetteValidationFailureReason::INVALID_WING_REGIONS: return "INVALID_WING_REGIONS";
        case SilhouetteValidationFailureReason::LOW_LATERAL_UTILIZATION: return "LOW_LATERAL_UTILIZATION";
        case SilhouetteValidationFailureReason::LOW_LONGITUDINAL_UTILIZATION: return "LOW_LONGITUDINAL_UTILIZATION";
        case SilhouetteValidationFailureReason::LOW_ARTICULATION: return "LOW_ARTICULATION";
        case SilhouetteValidationFailureReason::EXCESSIVE_SOLID_MASS: return "EXCESSIVE_SOLID_MASS";
        default: return "UNKNOWN";
        }
    }
}
