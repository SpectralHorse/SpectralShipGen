#include "CoreRegressionSuites.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <PixelShipGenerator/Diagnostics/DiagnosticsAnalysis.h>
#include <PixelShipGenerator/Diagnostics/DiagnosticsResultSerializer.h>

namespace
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorDiagnostics;

    bool nearlyEqual(double a, double b, double epsilon = 1e-9)
    {
        return std::abs(a - b) <= epsilon;
    }

    DiagnosticsRawSampleResult makeSample(uint64_t workIndex,
                                           uint64_t configurationIndex,
                                           ShipDimensions dimensions,
                                           ShipStyle style,
                                           ShipFactionType faction,
                                           double milliseconds,
                                           uint32_t hullAttempts,
                                           uint32_t materialZones,
                                           uint32_t majorFeatures,
                                           uint32_t weapons,
                                           uint32_t engines,
                                           double complexity,
                                           ShipVisualAnchorType anchor)
    {
        DiagnosticsRawSampleResult sample;
        sample.WorkItem.WorkIndex = workIndex;
        sample.WorkItem.ConfigurationIndex = configurationIndex;
        sample.WorkItem.SampleIndex = 0u;
        sample.WorkItem.Seed = 0xF000000000000000ull + workIndex;
        sample.WorkItem.Dimensions = dimensions;
        sample.WorkItem.Style = style;
        sample.WorkItem.Faction = faction;
        sample.Success = true;
        sample.TotalGenerationNanoseconds = static_cast<uint64_t>(milliseconds * 1000000.0);
        sample.Performance.TotalDurationNanoseconds = sample.TotalGenerationNanoseconds;
        sample.Performance.StageDurationNanoseconds[static_cast<std::size_t>(ShipGenerationPerformanceStage::HULL_GENERATION)] = sample.TotalGenerationNanoseconds / 2u;
        sample.HullAttemptCount = hullAttempts;
        sample.HullValidationRejectionCount = hullAttempts > 1u ? hullAttempts - 1u : 0u;
        sample.SilhouetteRejectionCounts[static_cast<std::size_t>(SilhouetteValidationFailureReason::LOW_ARTICULATION)] = sample.HullValidationRejectionCount;
        sample.StructuralNegativeSpaceAttemptCount = workIndex % 2u == 0u ? 1u : 0u;
        sample.StructuralNegativeSpaceSuccessCount = workIndex % 4u == 0u ? 1u : 0u;
        sample.MaterialZoneCount = materialZones;
        sample.MajorFeatureCount = majorFeatures;
        sample.WeaponCount = weapons;
        sample.EngineCount = engines;
        sample.LiveryCoveragePermille = 20u + static_cast<uint32_t>(workIndex) * 20u;
        sample.LiveryPrimaryCoveragePermille = 15u + static_cast<uint32_t>(workIndex) * 15u;
        sample.LiverySecondaryCoveragePermille = 5u + static_cast<uint32_t>(workIndex) * 5u;
        sample.LiveryLargestConnectedCoveragePermille = 10u + static_cast<uint32_t>(workIndex) * 10u;
        sample.LiverySecondaryMaterialCoveragePermille = 50u + static_cast<uint32_t>(workIndex) * 25u;
        sample.LiveryMechanicalMaterialCoveragePermille = 20u + static_cast<uint32_t>(workIndex) * 10u;
        sample.LiveryCoverageRejectionCount = static_cast<uint32_t>(workIndex);
        sample.LiveryMaterialPreservationRejectionCount = static_cast<uint32_t>(workIndex % 2u);
        sample.ComplexityUtilizationPercent = complexity;
        sample.PrimaryVisualAnchor = anchor;
        sample.FinalImageSignature = 0xABCDEF0012340000ull + workIndex;
        return sample;
    }

    DiagnosticsResult makeResult(double scale = 1.0)
    {
        DiagnosticsResult result;
        result.Configuration.Dimensions = { { 24u, 24u }, { 64u, 64u } };
        result.Configuration.Styles = { ShipStyle::SLEEK, ShipStyle::INDUSTRIAL };
        result.Configuration.Factions = { ShipFactionType::FRONTIER };
        result.Configuration.SamplesPerConfiguration = 1u;
        result.Configuration.DiagnosticSeed = 0x123456789ABCDEF0ull;
        result.Configuration.DetailedPerformanceInstrumentation = true;
        result.Configuration.BuildConfiguration = "Release";
        result.Configuration.VersionIdentifier = "task66-regression";
        result.Completed = true;
        result.ScheduledWorkItems = 4u;
        result.CompletedWorkItems = 4u;
        result.ElapsedNanoseconds = static_cast<uint64_t>(15.0 * scale * 1000000.0);
        result.Samples = {
            makeSample(0u, 0u, {24u,24u}, ShipStyle::SLEEK, ShipFactionType::FRONTIER, 1.0 * scale, 1u, 1u, 0u, 1u, 1u, 35.0, ShipVisualAnchorType::SILHOUETTE),
            makeSample(1u, 1u, {24u,24u}, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER, 2.0 * scale, 2u, 3u, 2u, 1u, 2u, 65.0, ShipVisualAnchorType::ENGINES),
            makeSample(2u, 2u, {64u,64u}, ShipStyle::SLEEK, ShipFactionType::FRONTIER, 4.0 * scale, 1u, 2u, 1u, 0u, 2u, 45.0, ShipVisualAnchorType::COCKPIT),
            makeSample(3u, 3u, {64u,64u}, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER, 8.0 * scale, 3u, 4u, 3u, 2u, 3u, 80.0, ShipVisualAnchorType::NEGATIVE_SPACE)
        };
        result.OverallSummary = aggregateDiagnosticsSamples(result.Samples);
        return result;
    }
}

namespace PixelShipGeneratorTests
{
    int runDiagnosticsDashboardRegression()
    {
        using namespace PixelShipGenerator;
        using namespace PixelShipGeneratorDiagnostics;

        const DiagnosticsResult result = makeResult();
        if (!nearlyEqual(result.OverallSummary.GenerationTimeMilliseconds.Mean, 3.75) ||
            !nearlyEqual(result.OverallSummary.GenerationTimeMilliseconds.Median, 3.0) ||
            !nearlyEqual(result.OverallSummary.MaterialZoneCount.Mean, 2.5) ||
            !nearlyEqual(result.OverallSummary.LiveryCoveragePercent.Mean, 5.0) ||
            !nearlyEqual(result.OverallSummary.LiveryCoveragePercent.Median, 5.0) ||
            !nearlyEqual(result.OverallSummary.LiveryCoveragePercent.P95, 8.0) ||
            !nearlyEqual(result.OverallSummary.LiveryLargestConnectedCoveragePercent.P95, 4.0) ||
            result.OverallSummary.TotalLiveryCoverageRejections != 6u ||
            result.OverallSummary.TotalLiveryMaterialPreservationRejections != 2u)
        {
            std::cerr << "Task-66 aggregate summary values are incorrect.\n";
            return 1;
        }

        DiagnosticsFilter filter;
        filter.Dimensions = ShipDimensions{ 64u, 64u };
        filter.Style = ShipStyle::INDUSTRIAL;
        const auto filtered = filterDiagnosticsResult(result, filter);
        if (filtered.Samples.size() != 1u || !nearlyEqual(filtered.Summary.GenerationTimeMilliseconds.Median, 8.0))
        {
            std::cerr << "Task-66 filter query returned the wrong sample set.\n";
            return 1;
        }
        if (result.Samples.size() != 4u || result.Samples[3].FinalImageSignature != 0xABCDEF0012340003ull)
        {
            std::cerr << "Task-66 filtering mutated the underlying result.\n";
            return 1;
        }

        const auto resolution = prepareResolutionSeries(result, {}, DiagnosticsMetric::GENERATION_MEDIAN_MS);
        if (resolution.Points.size() != 2u || resolution.Points[0].Label != "24X24" || resolution.Points[1].Label != "64X64" ||
            !nearlyEqual(resolution.Points[0].Value, 1.5) || !nearlyEqual(resolution.Points[1].Value, 6.0))
        {
            std::cerr << "Task-66 resolution chart series is incorrect.\n";
            return 1;
        }
        const auto styles = prepareStyleSeries(result, {}, DiagnosticsMetric::GENERATION_MEDIAN_MS);
        if (styles.Points.size() != 2u || styles.Points[0].Label != "SLEEK" || styles.Points[1].Label != "INDUSTRIAL" ||
            !nearlyEqual(styles.Points[0].Value, 2.5) || !nearlyEqual(styles.Points[1].Value, 5.0))
        {
            std::cerr << "Task-66 style chart series is incorrect.\n";
            return 1;
        }
        const auto liveryStyles = prepareStyleSeries(result, {}, DiagnosticsMetric::LIVERY_COVERAGE_P95_PERCENT);
        if (liveryStyles.Points.size() != 2u || liveryStyles.Points[0].Label != "SLEEK" || liveryStyles.Points[1].Label != "INDUSTRIAL" ||
            !nearlyEqual(liveryStyles.Points[0].Value, 6.0) || !nearlyEqual(liveryStyles.Points[1].Value, 8.0))
        {
            std::cerr << "Task-77 livery coverage style series is incorrect.\n";
            return 1;
        }

        const auto anchors = prepareVisualAnchorSeries(result, {});
        if (anchors.Points.size() != 4u)
        {
            std::cerr << "Task-66 visual-anchor chart series lost categories.\n";
            return 1;
        }

        const DiagnosticsResult baseline = makeResult(0.5);
        const auto compatibility = evaluateDiagnosticsCompatibility(baseline, result);
        if (!compatibility.HasComparableData || compatibility.MatchingDimensionCount != 2u || compatibility.MatchingStyleCount != 2u)
        {
            std::cerr << "Task-66 compatible baseline/current runs were not recognized.\n";
            return 1;
        }
        const auto delta = compareDiagnosticsMetric(baseline, result, {}, DiagnosticsMetric::GENERATION_MEDIAN_MS);
        if (!delta.Available || !delta.RelativeAvailable || !nearlyEqual(delta.Baseline, 1.5) || !nearlyEqual(delta.Current, 3.0) ||
            !nearlyEqual(delta.Absolute, 1.5) || !nearlyEqual(delta.RelativePercent, 100.0))
        {
            std::cerr << "Task-66 absolute/relative comparison delta is incorrect.\n";
            return 1;
        }
        const auto retryDelta = compareDiagnosticsMetric(baseline, result, {}, DiagnosticsMetric::HULL_RETRY_RATE_PERCENT);
        if (!retryDelta.Available || !retryDelta.PercentagePointMetric || !nearlyEqual(retryDelta.PercentagePointDelta, 0.0))
        {
            std::cerr << "Task-66 percentage-point comparison semantics are incorrect.\n";
            return 1;
        }

        DiagnosticsResult incompatible = result;
        for (auto& sample : incompatible.Samples) { sample.WorkItem.Faction = ShipFactionType::RELIC; }
        incompatible.Configuration.Factions = { ShipFactionType::RELIC };
        const auto incompatibleCheck = evaluateDiagnosticsCompatibility(baseline, incompatible);
        if (incompatibleCheck.HasComparableData)
        {
            std::cerr << "Task-66 missing comparison groups were not detected.\n";
            return 1;
        }

        DiagnosticsResult partial = result;
        partial.Completed = false;
        partial.Cancelled = true;
        partial.ScheduledWorkItems = 8u;
        partial.CompletedWorkItems = 4u;
        const std::string json = serializeDiagnosticsResultJson(partial);
        auto loaded = deserializeDiagnosticsResultJson(json);
        if (!loaded.Success || !loaded.Result.Cancelled || loaded.Result.Completed || loaded.Result.CompletedWorkItems != 4u || loaded.Result.ScheduledWorkItems != 8u ||
            loaded.Result.Samples.size() != partial.Samples.size() || loaded.Result.Samples.front().WorkItem.Seed != partial.Samples.front().WorkItem.Seed ||
            loaded.Result.Samples.back().FinalImageSignature != partial.Samples.back().FinalImageSignature ||
            loaded.Result.Samples.back().LiveryCoveragePermille != partial.Samples.back().LiveryCoveragePermille ||
            loaded.Result.Samples.back().LiveryLargestConnectedCoveragePermille != partial.Samples.back().LiveryLargestConnectedCoveragePermille ||
            !nearlyEqual(loaded.Result.OverallSummary.GenerationTimeMilliseconds.Median, partial.OverallSummary.GenerationTimeMilliseconds.Median) ||
            !nearlyEqual(loaded.Result.OverallSummary.LiveryCoveragePercent.P95, partial.OverallSummary.LiveryCoveragePercent.P95))
        {
            std::cerr << "Task-66 .shipdiag.json round-trip did not preserve the result. Error: " << loaded.Error << '\n';
            return 1;
        }
        if (loaded.Result.PersistedCsvSnapshot.empty())
        {
            std::cerr << "Task-66 persisted run did not retain full CSV export data.\n";
            return 1;
        }

        std::string unsupported = json;
        const std::string token = "\"schema_version\":1";
        const std::size_t schemaPosition = unsupported.find(token);
        if (schemaPosition == std::string::npos)
        {
            std::cerr << "Task-66 serializer output does not expose schema version.\n";
            return 1;
        }
        unsupported.replace(schemaPosition, token.size(), "\"schema_version\":2");
        if (deserializeDiagnosticsResultJson(unsupported).Success)
        {
            std::cerr << "Task-66 accepted an unsupported future schema version.\n";
            return 1;
        }

        const std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "pixel_ship_generator_task66_regression.shipdiag.json";
        std::string saveError;
        if (!saveDiagnosticsResultJson(tempPath, partial, saveError))
        {
            std::cerr << "Task-66 failed to save persisted run: " << saveError << '\n';
            return 1;
        }
        const auto fileLoad = loadDiagnosticsResultJson(tempPath);
        std::error_code removeError;
        std::filesystem::remove(tempPath, removeError);
        if (!fileLoad.Success || fileLoad.Result.Samples.size() != 4u)
        {
            std::cerr << "Task-66 file persistence round-trip failed: " << fileLoad.Error << '\n';
            return 1;
        }

        std::cout << "Task-66 diagnostics dashboard analysis regression passed.\n";
        return 0;
    }
}
