#include "RegressionSuites.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#include "DiagnosticsRunner.h"
#include "ShipGenerator.h"

namespace
{
    uint64_t imageSignature(const PixelShipGenerator::Image& image)
    {
        uint64_t hash = 1469598103934665603ull;
        for (const PixelShipGenerator::Color& color : image.getPixels())
        {
            const uint8_t bytes[] = { color.R, color.G, color.B, color.A };
            for (const uint8_t value : bytes) { hash = (hash ^ value) * 1099511628211ull; }
        }
        return hash;
    }

    bool finiteSummary(const PixelShipGeneratorDiagnostics::DiagnosticsDistributionSummary& summary)
    {
        return std::isfinite(summary.Mean) && std::isfinite(summary.Median) && std::isfinite(summary.Minimum) &&
               std::isfinite(summary.Maximum) && std::isfinite(summary.P95) && std::isfinite(summary.StandardDeviation);
    }
}

int PixelShipGeneratorTests::runDiagnosticsRunnerRegression()
{
    using namespace PixelShipGeneratorDiagnostics;

    DiagnosticsRunConfiguration configuration;
    configuration.Dimensions = { { 32u, 32u }, { 48u, 64u } };
    configuration.Styles = { PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipStyle::DELTA };
    configuration.Factions = { PixelShipGenerator::ShipFactionType::MILITARY };
    configuration.SamplesPerConfiguration = 2u;
    configuration.DiagnosticSeed = 0x6411223344556677ull;
    configuration.DetailedPerformanceInstrumentation = true;

    const std::vector<DiagnosticsWorkItem> firstSchedule = resolveDiagnosticsWorkSchedule(configuration);
    const std::vector<DiagnosticsWorkItem> secondSchedule = resolveDiagnosticsWorkSchedule(configuration);
    if (firstSchedule.size() != 8u || firstSchedule.size() != secondSchedule.size())
    {
        std::cerr << "Diagnostics runner regression failed: deterministic work schedule size mismatch.\n";
        return 1;
    }
    for (std::size_t index = 0u; index < firstSchedule.size(); ++index)
    {
        const auto& first = firstSchedule[index];
        const auto& second = secondSchedule[index];
        if (first.WorkIndex != second.WorkIndex || first.ConfigurationIndex != second.ConfigurationIndex || first.SampleIndex != second.SampleIndex || first.Seed != second.Seed || first.Dimensions != second.Dimensions || first.Style != second.Style || first.Faction != second.Faction)
        {
            std::cerr << "Diagnostics runner regression failed: work schedule is not deterministic.\n";
            return 1;
        }
    }

    DiagnosticsProgress lastProgress;
    bool sawEta = false;
    const DiagnosticsResult result = DiagnosticsRunner().run(configuration, [&](const DiagnosticsProgress& progress)
    {
        lastProgress = progress;
        if (progress.EstimatedRemainingAvailable)
        {
            sawEta = true;
            if (!std::isfinite(static_cast<double>(progress.EstimatedRemainingNanoseconds)) || progress.EstimatedRemainingNanoseconds > (uint64_t(1) << 62u))
            {
                std::cerr << "Diagnostics runner regression failed: ETA became invalid.\n";
            }
        }
    });

    if (!result.Completed || result.Cancelled || result.CompletedWorkItems != firstSchedule.size() || lastProgress.CompletedWorkItems != firstSchedule.size() || std::abs(lastProgress.ProgressPercent - 100.0) > 0.0001 || !sawEta)
    {
        std::cerr << "Diagnostics runner regression failed: progress/completion state is invalid.\n";
        return 1;
    }
    if (result.Samples.size() != firstSchedule.size() || result.ConfigurationResults.size() != 4u || !finiteSummary(result.OverallSummary.GenerationTimeMilliseconds))
    {
        std::cerr << "Diagnostics runner regression failed: result model is incomplete.\n";
        return 1;
    }

    PixelShipGenerator::ShipGenerationSettings directSettings;
    directSettings.Seed = firstSchedule.front().Seed;
    directSettings.Dimensions = firstSchedule.front().Dimensions;
    directSettings.Style = firstSchedule.front().Style;
    directSettings.Faction = firstSchedule.front().Faction;
    directSettings.DetailDensity = configuration.DetailDensity;
    directSettings.AsymmetricDetailChance = configuration.AsymmetricDetailChance;
    directSettings.AttachmentsEnabled = configuration.AttachmentsEnabled;
    PixelShipGenerator::ShipGenerator generator;
    const PixelShipGenerator::GeneratedShip directShip = generator.generate(directSettings);
    if (imageSignature(directShip.FinalImage) != result.Samples.front().FinalImageSignature)
    {
        std::cerr << "Diagnostics runner regression failed: reusable runner generated a different sample.\n";
        return 1;
    }

    PixelShipGenerator::ShipGenerationDebugInfo plainDebug;
    PixelShipGenerator::ShipGenerationDebugInfo timedDebug;
    PixelShipGenerator::ShipGenerationPerformanceInfo performance;
    const PixelShipGenerator::GeneratedShip plainShip = generator.generate(directSettings, &plainDebug);
    const PixelShipGenerator::GeneratedShip timedShip = generator.generate(directSettings, &timedDebug, &performance);
    if (imageSignature(plainShip.FinalImage) != imageSignature(timedShip.FinalImage) || performance.TotalDurationNanoseconds == 0u)
    {
        std::cerr << "Diagnostics runner regression failed: timing instrumentation altered generation or recorded no total duration.\n";
        return 1;
    }

    uint64_t completedForCancellation = 0u;
    DiagnosticsRunConfiguration cancellationConfiguration = configuration;
    cancellationConfiguration.SamplesPerConfiguration = 4u;
    const DiagnosticsResult cancelled = DiagnosticsRunner().run(cancellationConfiguration,
        [&](const DiagnosticsProgress& progress) { completedForCancellation = progress.CompletedWorkItems; },
        [&]() { return completedForCancellation >= 3u; });
    if (!cancelled.Cancelled || cancelled.Completed || cancelled.CompletedWorkItems != 3u || cancelled.Samples.size() != 3u || cancelled.OverallSummary.SampleCount != 3u)
    {
        std::cerr << "Diagnostics runner regression failed: cancellation did not return a coherent partial result.\n";
        return 1;
    }

    const DiagnosticsDistributionSummary empty = summarizeDiagnosticsValues({});
    const DiagnosticsDistributionSummary single = summarizeDiagnosticsValues({ 7.0 });
    const DiagnosticsDistributionSummary distribution = summarizeDiagnosticsValues({ 1.0, 2.0, 3.0, 4.0, 100.0 });
    if (!finiteSummary(empty) || single.Mean != 7.0 || single.Median != 7.0 || single.P95 != 7.0 || distribution.Mean != 22.0 || distribution.Median != 3.0 || distribution.P95 != 100.0 || distribution.Minimum != 1.0 || distribution.Maximum != 100.0)
    {
        std::cerr << "Diagnostics runner regression failed: aggregate distribution calculations are incorrect.\n";
        return 1;
    }

    std::vector<DiagnosticsRawSampleResult> synthetic(2u);
    synthetic[0].Success = true;
    synthetic[0].TotalGenerationNanoseconds = 1000000u;
    synthetic[0].HullAttemptCount = 1u;
    synthetic[0].HullValidationRejectionCount = 0u;
    synthetic[0].StructuralNegativeSpaceAttemptCount = 2u;
    synthetic[0].StructuralNegativeSpaceSuccessCount = 1u;
    synthetic[1].Success = true;
    synthetic[1].TotalGenerationNanoseconds = 3000000u;
    synthetic[1].HullAttemptCount = 2u;
    synthetic[1].HullValidationRejectionCount = 1u;
    synthetic[1].StructuralNegativeSpaceAttemptCount = 2u;
    synthetic[1].StructuralNegativeSpaceSuccessCount = 1u;
    const DiagnosticsAggregateSummary aggregate = aggregateDiagnosticsSamples(synthetic);
    if (aggregate.GenerationTimeMilliseconds.Median != 2.0 || aggregate.HullAttempts.Mean != 1.5 || aggregate.HullRetryRatePercent != 50.0 || aggregate.StructuralNegativeSpaceAttemptRatePercent != 200.0 || aggregate.StructuralNegativeSpaceSuccessRatePercent != 50.0)
    {
        std::cerr << "Diagnostics runner regression failed: retry/rate aggregation is incorrect.\n";
        return 1;
    }

    const auto byDimensionsStyle = groupDiagnosticsSamples(result.Samples, DiagnosticsGrouping::DIMENSIONS_STYLE);
    if (byDimensionsStyle.size() != 4u)
    {
        std::cerr << "Diagnostics runner regression failed: reusable grouping produced the wrong group count.\n";
        return 1;
    }

    std::ostringstream csv;
    writeDiagnosticsResultCsv(csv, result);
    const std::string csvText = csv.str();
    if (csvText.find("generation_time_median_ms") == std::string::npos || csvText.find("stage_HULL_GENERATION_mean_ms") == std::string::npos || csvText.find("negative_space_success_rate_percent") == std::string::npos)
    {
        std::cerr << "Diagnostics runner regression failed: extended CSV fields are missing.\n";
        return 1;
    }

    DiagnosticGenerationConfiguration legacy;
    legacy.Width = 32u;
    legacy.Height = 32u;
    legacy.Samples = 4u;
    legacy.DiagnosticSeed = 0x12345678ull;
    if (collectGenerationStatistics(legacy).deterministicSignature() != collectGenerationStatistics(legacy).deterministicSignature())
    {
        std::cerr << "Diagnostics runner regression failed: legacy statistics workflow is no longer deterministic.\n";
        return 1;
    }

    std::cout << "Reusable diagnostics runner regression passed.\n";
    return 0;
}
