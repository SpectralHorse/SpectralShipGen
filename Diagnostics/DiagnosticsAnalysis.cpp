#include "DiagnosticsAnalysis.h"

#include <algorithm>
#include <cmath>
#include <set>

#include "ShipGenerationPerformance.h"
#include "BuiltInPresetCatalog.h"
#include "ShipVisualAnchorType.h"

namespace PixelShipGeneratorDiagnostics
{
    namespace
    {
        std::string dimensionsLabel(PixelShipGenerator::ShipDimensions dimensions)
        {
            return std::to_string(dimensions.Width) + "X" + std::to_string(dimensions.Height);
        }

        const char* styleLabel(PixelShipGenerator::ShipStyle style)
        {
            if (style == PixelShipGenerator::ShipStyle::SHIP_STYLE_END) { return "CUSTOM"; }
            for (const auto& entry : PixelShipGenerator::getBuiltInStructuralPresetCatalog()) { if (entry.Preset == style) { return entry.StableId; } }
            return "UNKNOWN";
        }

        const char* factionLabel(PixelShipGenerator::ShipFactionType faction)
        {
            if (faction == PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END) { return "CUSTOM"; }
            for (const auto& entry : PixelShipGenerator::getBuiltInFactionPresetCatalog()) { if (entry.Preset == faction) { return entry.StableId; } }
            return "UNKNOWN";
        }

        template <typename Setter>
        DiagnosticsChartSeries prepareEnumSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric, uint32_t count, const Setter& setter)
        {
            DiagnosticsChartSeries series;
            series.Metric = metric;
            for (uint32_t index = 0u; index < count; ++index)
            {
                DiagnosticsFilter selected = filter;
                DiagnosticsChartPoint point;
                setter(index, selected, point);
                const DiagnosticsFilteredResult filtered = filterDiagnosticsResult(result, selected);
                if (filtered.Samples.empty()) { continue; }
                point.Value = getDiagnosticsMetricValue(filtered.Summary, metric);
                point.SampleCount = filtered.Samples.size();
                series.Points.push_back(point);
            }
            return series;
        }
    }

    bool diagnosticsSampleMatchesFilter(const DiagnosticsRawSampleResult& sample, const DiagnosticsFilter& filter)
    {
        if (filter.Dimensions.has_value() && sample.WorkItem.Dimensions != *filter.Dimensions) { return false; }
        if (filter.Style.has_value() && sample.WorkItem.Style != *filter.Style) { return false; }
        if (filter.Faction.has_value() && sample.WorkItem.Faction != *filter.Faction) { return false; }
        return true;
    }

    DiagnosticsFilteredResult filterDiagnosticsResult(const DiagnosticsResult& result, const DiagnosticsFilter& filter)
    {
        DiagnosticsFilteredResult filtered;
        filtered.Filter = filter;
        filtered.Samples.reserve(result.Samples.size());
        for (const DiagnosticsRawSampleResult& sample : result.Samples)
        {
            if (diagnosticsSampleMatchesFilter(sample, filter)) { filtered.Samples.push_back(&sample); }
        }
        filtered.Summary = aggregateDiagnosticsSamplePointers(filtered.Samples);
        return filtered;
    }

    double getDiagnosticsMetricValue(const DiagnosticsAggregateSummary& summary, DiagnosticsMetric metric)
    {
        switch (metric)
        {
        case DiagnosticsMetric::GENERATION_AVERAGE_MS: return summary.GenerationTimeMilliseconds.Mean;
        case DiagnosticsMetric::GENERATION_MEDIAN_MS: return summary.GenerationTimeMilliseconds.Median;
        case DiagnosticsMetric::GENERATION_P95_MS: return summary.GenerationTimeMilliseconds.P95;
        case DiagnosticsMetric::GENERATION_MAX_MS: return summary.GenerationTimeMilliseconds.Maximum;
        case DiagnosticsMetric::HULL_AVERAGE_ATTEMPTS: return summary.HullAttempts.Mean;
        case DiagnosticsMetric::HULL_RETRY_RATE_PERCENT: return summary.HullRetryRatePercent;
        case DiagnosticsMetric::NEGATIVE_SPACE_ATTEMPT_RATE_PERCENT: return summary.StructuralNegativeSpaceAttemptRatePercent;
        case DiagnosticsMetric::NEGATIVE_SPACE_SUCCESS_RATE_PERCENT: return summary.StructuralNegativeSpaceSuccessRatePercent;
        case DiagnosticsMetric::MATERIAL_ZONE_AVERAGE: return summary.MaterialZoneCount.Mean;
        case DiagnosticsMetric::LIVERY_COVERAGE_AVERAGE_PERCENT: return summary.LiveryCoveragePercent.Mean;
        case DiagnosticsMetric::LIVERY_COVERAGE_MEDIAN_PERCENT: return summary.LiveryCoveragePercent.Median;
        case DiagnosticsMetric::LIVERY_COVERAGE_P95_PERCENT: return summary.LiveryCoveragePercent.P95;
        case DiagnosticsMetric::LIVERY_CONNECTED_P95_PERCENT: return summary.LiveryLargestConnectedCoveragePercent.P95;
        case DiagnosticsMetric::MAJOR_FEATURE_AVERAGE: return summary.MajorFeatureCount.Mean;
        case DiagnosticsMetric::WEAPON_AVERAGE: return summary.WeaponCount.Mean;
        case DiagnosticsMetric::ENGINE_AVERAGE: return summary.EngineCount.Mean;
        case DiagnosticsMetric::COMPLEXITY_UTILIZATION_PERCENT: return summary.ComplexityUtilizationPercent.Mean;
        default: return 0.0;
        }
    }

    const char* getDiagnosticsMetricName(DiagnosticsMetric metric)
    {
        switch (metric)
        {
        case DiagnosticsMetric::GENERATION_AVERAGE_MS: return "GEN AVG";
        case DiagnosticsMetric::GENERATION_MEDIAN_MS: return "GEN MEDIAN";
        case DiagnosticsMetric::GENERATION_P95_MS: return "GEN P95";
        case DiagnosticsMetric::GENERATION_MAX_MS: return "GEN MAX";
        case DiagnosticsMetric::HULL_AVERAGE_ATTEMPTS: return "HULL AVG ATTEMPTS";
        case DiagnosticsMetric::HULL_RETRY_RATE_PERCENT: return "HULL RETRY RATE";
        case DiagnosticsMetric::NEGATIVE_SPACE_ATTEMPT_RATE_PERCENT: return "NEG SPACE ATTEMPT";
        case DiagnosticsMetric::NEGATIVE_SPACE_SUCCESS_RATE_PERCENT: return "NEG SPACE SUCCESS";
        case DiagnosticsMetric::MATERIAL_ZONE_AVERAGE: return "MATERIAL ZONES";
        case DiagnosticsMetric::LIVERY_COVERAGE_AVERAGE_PERCENT: return "LIVERY COVERAGE AVG";
        case DiagnosticsMetric::LIVERY_COVERAGE_MEDIAN_PERCENT: return "LIVERY COVERAGE MEDIAN";
        case DiagnosticsMetric::LIVERY_COVERAGE_P95_PERCENT: return "LIVERY COVERAGE P95";
        case DiagnosticsMetric::LIVERY_CONNECTED_P95_PERCENT: return "LIVERY CONNECTED P95";
        case DiagnosticsMetric::MAJOR_FEATURE_AVERAGE: return "MAJOR FEATURES";
        case DiagnosticsMetric::WEAPON_AVERAGE: return "WEAPONS";
        case DiagnosticsMetric::ENGINE_AVERAGE: return "ENGINES";
        case DiagnosticsMetric::COMPLEXITY_UTILIZATION_PERCENT: return "COMPLEXITY USE";
        default: return "UNKNOWN";
        }
    }

    const char* getDiagnosticsMetricUnit(DiagnosticsMetric metric)
    {
        switch (metric)
        {
        case DiagnosticsMetric::GENERATION_AVERAGE_MS:
        case DiagnosticsMetric::GENERATION_MEDIAN_MS:
        case DiagnosticsMetric::GENERATION_P95_MS:
        case DiagnosticsMetric::GENERATION_MAX_MS: return "MS";
        case DiagnosticsMetric::HULL_RETRY_RATE_PERCENT:
        case DiagnosticsMetric::NEGATIVE_SPACE_ATTEMPT_RATE_PERCENT:
        case DiagnosticsMetric::NEGATIVE_SPACE_SUCCESS_RATE_PERCENT:
        case DiagnosticsMetric::LIVERY_COVERAGE_AVERAGE_PERCENT:
        case DiagnosticsMetric::LIVERY_COVERAGE_MEDIAN_PERCENT:
        case DiagnosticsMetric::LIVERY_COVERAGE_P95_PERCENT:
        case DiagnosticsMetric::LIVERY_CONNECTED_P95_PERCENT:
        case DiagnosticsMetric::COMPLEXITY_UTILIZATION_PERCENT: return "%";
        default: return "";
        }
    }

    bool isDiagnosticsPercentagePointMetric(DiagnosticsMetric metric)
    {
        return metric == DiagnosticsMetric::HULL_RETRY_RATE_PERCENT ||
            metric == DiagnosticsMetric::NEGATIVE_SPACE_ATTEMPT_RATE_PERCENT ||
            metric == DiagnosticsMetric::NEGATIVE_SPACE_SUCCESS_RATE_PERCENT ||
            metric == DiagnosticsMetric::LIVERY_COVERAGE_AVERAGE_PERCENT ||
            metric == DiagnosticsMetric::LIVERY_COVERAGE_MEDIAN_PERCENT ||
            metric == DiagnosticsMetric::LIVERY_COVERAGE_P95_PERCENT ||
            metric == DiagnosticsMetric::LIVERY_CONNECTED_P95_PERCENT ||
            metric == DiagnosticsMetric::COMPLEXITY_UTILIZATION_PERCENT;
    }

    DiagnosticsChartSeries prepareResolutionSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        DiagnosticsChartSeries series;
        series.Label = "RESOLUTION";
        series.Metric = metric;
        std::vector<PixelShipGenerator::ShipDimensions> dimensions;
        for (const DiagnosticsRawSampleResult& sample : result.Samples)
        {
            if (filter.Style.has_value() && sample.WorkItem.Style != *filter.Style) { continue; }
            if (filter.Faction.has_value() && sample.WorkItem.Faction != *filter.Faction) { continue; }
            if (std::find(dimensions.begin(), dimensions.end(), sample.WorkItem.Dimensions) == dimensions.end()) { dimensions.push_back(sample.WorkItem.Dimensions); }
        }
        std::sort(dimensions.begin(), dimensions.end(), [](auto a, auto b)
            {
                const uint64_t areaA = static_cast<uint64_t>(a.Width) * a.Height;
                const uint64_t areaB = static_cast<uint64_t>(b.Width) * b.Height;
                if (areaA != areaB) { return areaA < areaB; }
                if (a.Width != b.Width) { return a.Width < b.Width; }
                return a.Height < b.Height;
            });
        for (const auto dimensionsValue : dimensions)
        {
            DiagnosticsFilter selected = filter;
            selected.Dimensions = dimensionsValue;
            const DiagnosticsFilteredResult filtered = filterDiagnosticsResult(result, selected);
            if (filtered.Samples.empty()) { continue; }
            series.Points.push_back({ dimensionsLabel(dimensionsValue), getDiagnosticsMetricValue(filtered.Summary, metric), filtered.Samples.size(), dimensionsValue, filter.Style, filter.Faction });
        }
        return series;
    }

    DiagnosticsChartSeries prepareStyleSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        DiagnosticsChartSeries series = prepareEnumSeries(result, filter, metric, static_cast<uint32_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END), [](uint32_t index, DiagnosticsFilter& selected, DiagnosticsChartPoint& point)
            {
                const auto style = static_cast<PixelShipGenerator::ShipStyle>(index);
                selected.Style = style;
                point.Label = styleLabel(style);
                point.Style = style;
                point.Dimensions = selected.Dimensions;
                point.Faction = selected.Faction;
            });
        series.Label = "STYLE";
        return series;
    }

    DiagnosticsChartSeries prepareFactionSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        DiagnosticsChartSeries series = prepareEnumSeries(result, filter, metric, static_cast<uint32_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END), [](uint32_t index, DiagnosticsFilter& selected, DiagnosticsChartPoint& point)
            {
                const auto faction = static_cast<PixelShipGenerator::ShipFactionType>(index);
                selected.Faction = faction;
                point.Label = factionLabel(faction);
                point.Faction = faction;
                point.Dimensions = selected.Dimensions;
                point.Style = selected.Style;
            });
        series.Label = "FACTION";
        return series;
    }

    DiagnosticsChartSeries prepareStageSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter)
    {
        DiagnosticsChartSeries series;
        series.Label = "STAGE";
        const DiagnosticsFilteredResult filtered = filterDiagnosticsResult(result, filter);
        for (std::size_t stage = 0u; stage < filtered.Summary.StageTimeMilliseconds.size(); ++stage)
        {
            const auto& summary = filtered.Summary.StageTimeMilliseconds[stage];
            if (summary.Count == 0u) { continue; }
            series.Points.push_back({ PixelShipGenerator::getShipGenerationPerformanceStageName(static_cast<PixelShipGenerator::ShipGenerationPerformanceStage>(stage)), summary.Mean, summary.Count, filter.Dimensions, filter.Style, filter.Faction });
        }
        return series;
    }

    DiagnosticsChartSeries prepareSilhouetteRejectionSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter)
    {
        DiagnosticsChartSeries series;
        series.Label = "REJECTION";
        const DiagnosticsFilteredResult filtered = filterDiagnosticsResult(result, filter);
        for (std::size_t reason = 1u; reason < filtered.Summary.SilhouetteRejectionCounts.size(); ++reason)
        {
            const uint64_t count = filtered.Summary.SilhouetteRejectionCounts[reason];
            if (count == 0u) { continue; }
            series.Points.push_back({ PixelShipGenerator::getSilhouetteValidationFailureReasonName(static_cast<PixelShipGenerator::SilhouetteValidationFailureReason>(reason)), static_cast<double>(count), filtered.Samples.size(), filter.Dimensions, filter.Style, filter.Faction });
        }
        return series;
    }

    DiagnosticsChartSeries prepareVisualAnchorSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter)
    {
        DiagnosticsChartSeries series;
        series.Label = "PRIMARY ANCHOR %";
        const DiagnosticsFilteredResult filtered = filterDiagnosticsResult(result, filter);
        const double denominator = filtered.Samples.empty() ? 1.0 : static_cast<double>(filtered.Samples.size());
        for (std::size_t anchor = 0u; anchor < filtered.Summary.PrimaryVisualAnchorCounts.size(); ++anchor)
        {
            const uint64_t count = filtered.Summary.PrimaryVisualAnchorCounts[anchor];
            if (count == 0u) { continue; }
            series.Points.push_back({ PixelShipGenerator::getShipVisualAnchorTypeName(static_cast<PixelShipGenerator::ShipVisualAnchorType>(anchor)), 100.0 * static_cast<double>(count) / denominator, filtered.Samples.size(), filter.Dimensions, filter.Style, filter.Faction });
        }
        return series;
    }

    DiagnosticsComparisonCompatibility evaluateDiagnosticsCompatibility(const DiagnosticsResult& baseline, const DiagnosticsResult& current)
    {
        DiagnosticsComparisonCompatibility result;
        std::set<std::pair<uint32_t, uint32_t>> baselineDimensions;
        std::set<std::pair<uint32_t, uint32_t>> currentDimensions;
        std::set<uint32_t> baselineStyles;
        std::set<uint32_t> currentStyles;
        std::set<uint32_t> baselineFactions;
        std::set<uint32_t> currentFactions;
        for (const auto& sample : baseline.Samples)
        {
            baselineDimensions.insert({ sample.WorkItem.Dimensions.Width, sample.WorkItem.Dimensions.Height });
            baselineStyles.insert(static_cast<uint32_t>(sample.WorkItem.Style));
            baselineFactions.insert(static_cast<uint32_t>(sample.WorkItem.Faction));
        }
        for (const auto& sample : current.Samples)
        {
            currentDimensions.insert({ sample.WorkItem.Dimensions.Width, sample.WorkItem.Dimensions.Height });
            currentStyles.insert(static_cast<uint32_t>(sample.WorkItem.Style));
            currentFactions.insert(static_cast<uint32_t>(sample.WorkItem.Faction));
        }
        for (const auto& value : baselineDimensions) { if (currentDimensions.count(value) != 0u) { ++result.MatchingDimensionCount; } }
        for (const auto value : baselineStyles) { if (currentStyles.count(value) != 0u) { ++result.MatchingStyleCount; } }
        for (const auto value : baselineFactions) { if (currentFactions.count(value) != 0u) { ++result.MatchingFactionCount; } }
        result.HasComparableData = result.MatchingDimensionCount > 0u && result.MatchingStyleCount > 0u && result.MatchingFactionCount > 0u;
        if (!result.HasComparableData) { result.Message = "No overlapping dimension/style/faction data."; }
        else if (result.MatchingDimensionCount < baselineDimensions.size() || result.MatchingDimensionCount < currentDimensions.size() ||
            result.MatchingStyleCount < baselineStyles.size() || result.MatchingStyleCount < currentStyles.size() ||
            result.MatchingFactionCount < baselineFactions.size() || result.MatchingFactionCount < currentFactions.size())
        {
            result.Message = "Partial overlap; comparison uses matching filtered groups only.";
        }
        else { result.Message = "Runs have compatible diagnostic dimensions/styles/factions."; }
        return result;
    }

    DiagnosticsMetricDelta compareDiagnosticsMetric(const DiagnosticsResult& baseline, const DiagnosticsResult& current, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        DiagnosticsMetricDelta delta;
        const DiagnosticsFilteredResult baselineFiltered = filterDiagnosticsResult(baseline, filter);
        const DiagnosticsFilteredResult currentFiltered = filterDiagnosticsResult(current, filter);
        delta.BaselineSamples = baselineFiltered.Samples.size();
        delta.CurrentSamples = currentFiltered.Samples.size();
        if (baselineFiltered.Samples.empty() || currentFiltered.Samples.empty()) { return delta; }
        delta.Available = true;
        delta.Baseline = getDiagnosticsMetricValue(baselineFiltered.Summary, metric);
        delta.Current = getDiagnosticsMetricValue(currentFiltered.Summary, metric);
        delta.Absolute = delta.Current - delta.Baseline;
        delta.PercentagePointMetric = isDiagnosticsPercentagePointMetric(metric);
        if (delta.PercentagePointMetric) { delta.PercentagePointDelta = delta.Absolute; }
        if (std::abs(delta.Baseline) > 1e-12)
        {
            delta.RelativeAvailable = true;
            delta.RelativePercent = 100.0 * delta.Absolute / std::abs(delta.Baseline);
        }
        return delta;
    }

    std::optional<DiagnosticsChartPoint> findMostExpensiveDimension(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        const auto series = prepareResolutionSeries(result, filter, metric);
        if (series.Points.empty()) { return std::nullopt; }
        return *std::max_element(series.Points.begin(), series.Points.end(), [](const auto& a, const auto& b) { return a.Value < b.Value; });
    }

    std::optional<DiagnosticsChartPoint> findSlowestStyle(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        const auto series = prepareStyleSeries(result, filter, metric);
        if (series.Points.empty()) { return std::nullopt; }
        return *std::max_element(series.Points.begin(), series.Points.end(), [](const auto& a, const auto& b) { return a.Value < b.Value; });
    }

    std::optional<DiagnosticsChartPoint> findSlowestFaction(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric)
    {
        const auto series = prepareFactionSeries(result, filter, metric);
        if (series.Points.empty()) { return std::nullopt; }
        return *std::max_element(series.Points.begin(), series.Points.end(), [](const auto& a, const auto& b) { return a.Value < b.Value; });
    }

    std::optional<DiagnosticsChartPoint> findMostExpensiveStage(const DiagnosticsResult& result, const DiagnosticsFilter& filter)
    {
        const auto series = prepareStageSeries(result, filter);
        if (series.Points.empty()) { return std::nullopt; }
        return *std::max_element(series.Points.begin(), series.Points.end(), [](const auto& a, const auto& b) { return a.Value < b.Value; });
    }
}
