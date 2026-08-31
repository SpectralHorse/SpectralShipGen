#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SpectralShipGen/Diagnostics/DiagnosticsRunner.h>

namespace SpectralShipGenDiagnostics
{
    struct DiagnosticsFilter
    {
        std::optional<SpectralShipGen::ShipDimensions> Dimensions;
        std::optional<SpectralShipGen::ShipStyle> Style;
        std::optional<SpectralShipGen::ShipFactionType> Faction;
    };

    enum class DiagnosticsMetric : uint32_t
    {
        GENERATION_AVERAGE_MS = 0u,
        GENERATION_MEDIAN_MS,
        GENERATION_P95_MS,
        GENERATION_MAX_MS,
        HULL_AVERAGE_ATTEMPTS,
        HULL_RETRY_RATE_PERCENT,
        NEGATIVE_SPACE_ATTEMPT_RATE_PERCENT,
        NEGATIVE_SPACE_SUCCESS_RATE_PERCENT,
        MATERIAL_ZONE_AVERAGE,
        LIVERY_COVERAGE_AVERAGE_PERCENT,
        LIVERY_COVERAGE_MEDIAN_PERCENT,
        LIVERY_COVERAGE_P95_PERCENT,
        LIVERY_CONNECTED_P95_PERCENT,
        MAJOR_FEATURE_AVERAGE,
        WEAPON_AVERAGE,
        ENGINE_AVERAGE,
        COMPLEXITY_UTILIZATION_PERCENT
    };

    struct DiagnosticsChartPoint
    {
        std::string Label;
        double Value = 0.0;
        uint64_t SampleCount = 0u;
        std::optional<SpectralShipGen::ShipDimensions> Dimensions;
        std::optional<SpectralShipGen::ShipStyle> Style;
        std::optional<SpectralShipGen::ShipFactionType> Faction;
    };

    struct DiagnosticsChartSeries
    {
        std::string Label;
        DiagnosticsMetric Metric = DiagnosticsMetric::GENERATION_MEDIAN_MS;
        std::vector<DiagnosticsChartPoint> Points;
    };

    struct DiagnosticsFilteredResult
    {
        DiagnosticsFilter Filter;
        DiagnosticsAggregateSummary Summary;
        std::vector<const DiagnosticsRawSampleResult*> Samples;
    };

    struct DiagnosticsMetricDelta
    {
        bool Available = false;
        double Baseline = 0.0;
        double Current = 0.0;
        double Absolute = 0.0;
        bool RelativeAvailable = false;
        double RelativePercent = 0.0;
        bool PercentagePointMetric = false;
        double PercentagePointDelta = 0.0;
        uint64_t BaselineSamples = 0u;
        uint64_t CurrentSamples = 0u;
    };

    struct DiagnosticsComparisonCompatibility
    {
        uint64_t MatchingDimensionCount = 0u;
        uint64_t MatchingStyleCount = 0u;
        uint64_t MatchingFactionCount = 0u;
        bool HasComparableData = false;
        std::string Message;
    };

    bool diagnosticsSampleMatchesFilter(const DiagnosticsRawSampleResult& sample, const DiagnosticsFilter& filter);
    DiagnosticsFilteredResult filterDiagnosticsResult(const DiagnosticsResult& result, const DiagnosticsFilter& filter);
    double getDiagnosticsMetricValue(const DiagnosticsAggregateSummary& summary, DiagnosticsMetric metric);
    const char* getDiagnosticsMetricName(DiagnosticsMetric metric);
    const char* getDiagnosticsMetricUnit(DiagnosticsMetric metric);
    bool isDiagnosticsPercentagePointMetric(DiagnosticsMetric metric);

    DiagnosticsChartSeries prepareResolutionSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric);
    DiagnosticsChartSeries prepareStyleSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric);
    DiagnosticsChartSeries prepareFactionSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric);
    DiagnosticsChartSeries prepareStageSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter);
    DiagnosticsChartSeries prepareSilhouetteRejectionSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter);
    DiagnosticsChartSeries prepareVisualAnchorSeries(const DiagnosticsResult& result, const DiagnosticsFilter& filter);

    DiagnosticsComparisonCompatibility evaluateDiagnosticsCompatibility(const DiagnosticsResult& baseline, const DiagnosticsResult& current);
    DiagnosticsMetricDelta compareDiagnosticsMetric(const DiagnosticsResult& baseline, const DiagnosticsResult& current, const DiagnosticsFilter& filter, DiagnosticsMetric metric);

    std::optional<DiagnosticsChartPoint> findMostExpensiveDimension(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric = DiagnosticsMetric::GENERATION_MEDIAN_MS);
    std::optional<DiagnosticsChartPoint> findSlowestStyle(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric = DiagnosticsMetric::GENERATION_MEDIAN_MS);
    std::optional<DiagnosticsChartPoint> findSlowestFaction(const DiagnosticsResult& result, const DiagnosticsFilter& filter, DiagnosticsMetric metric = DiagnosticsMetric::GENERATION_MEDIAN_MS);
    std::optional<DiagnosticsChartPoint> findMostExpensiveStage(const DiagnosticsResult& result, const DiagnosticsFilter& filter);
}
