#include "DiagnosticsRunner.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "ShipGenerator.h"
#include "ShipGenerationSeeds.h"

namespace PixelShipGeneratorDiagnostics
{
    namespace
    {
        using Clock = std::chrono::steady_clock;


        bool categoryEnabled(const DiagnosticsRunConfiguration& configuration, DiagnosticsCategory category)
        {
            return (configuration.EnabledCategories & static_cast<uint32_t>(category)) != 0u;
        }
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

        uint64_t durationNanoseconds(Clock::duration duration)
        {
            const auto count = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            return count <= 0 ? 0u : static_cast<uint64_t>(count);
        }

        DiagnosticGenerationConfiguration makeLegacyConfiguration(const DiagnosticsRunConfiguration& configuration, const DiagnosticsWorkItem& item)
        {
            DiagnosticGenerationConfiguration result;
            result.Width = item.Dimensions.Width;
            result.Height = item.Dimensions.Height;
            result.Style = item.Style;
            result.Faction = item.Faction;
            result.DetailDensity = configuration.DetailDensity;
            result.AsymmetricDetailChance = configuration.AsymmetricDetailChance;
            result.AttachmentsEnabled = configuration.AttachmentsEnabled;
            result.Samples = configuration.SamplesPerConfiguration;
            result.DiagnosticSeed = configuration.DiagnosticSeed;
            return result;
        }

        std::vector<double> collectGenerationTimes(const std::vector<DiagnosticsRawSampleResult>& samples)
        {
            std::vector<double> values;
            values.reserve(samples.size());
            for (const auto& sample : samples) { values.push_back(static_cast<double>(sample.TotalGenerationNanoseconds) / 1000000.0); }
            return values;
        }

        DiagnosticsAggregateSummary aggregateRange(const std::vector<const DiagnosticsRawSampleResult*>& samples)
        {
            DiagnosticsAggregateSummary result;
            std::vector<double> generationTimes;
            std::vector<double> hullAttempts;
            std::vector<double> materialZoneCounts;
            std::vector<double> liveryCoverage;
            std::vector<double> liveryPrimaryCoverage;
            std::vector<double> liverySecondaryCoverage;
            std::vector<double> liveryConnectedCoverage;
            std::vector<double> liverySecondaryMaterialCoverage;
            std::vector<double> liveryMechanicalMaterialCoverage;
            std::vector<double> majorFeatureCounts;
            std::vector<double> weaponCounts;
            std::vector<double> engineCounts;
            std::vector<double> complexityUtilization;
            std::array<std::vector<double>, PixelShipGenerator::ShipGenerationPerformanceStageCount> stageTimes;
            generationTimes.reserve(samples.size());
            hullAttempts.reserve(samples.size());
            materialZoneCounts.reserve(samples.size());
            liveryCoverage.reserve(samples.size());
            liveryPrimaryCoverage.reserve(samples.size());
            liverySecondaryCoverage.reserve(samples.size());
            liveryConnectedCoverage.reserve(samples.size());
            liverySecondaryMaterialCoverage.reserve(samples.size());
            liveryMechanicalMaterialCoverage.reserve(samples.size());
            majorFeatureCounts.reserve(samples.size());
            weaponCounts.reserve(samples.size());
            engineCounts.reserve(samples.size());
            complexityUtilization.reserve(samples.size());

            for (const DiagnosticsRawSampleResult* sample : samples)
            {
                ++result.SampleCount;
                if (sample->Success) { ++result.SuccessfulSamples; }
                else { ++result.FailedSamples; }
                generationTimes.push_back(static_cast<double>(sample->TotalGenerationNanoseconds) / 1000000.0);
                hullAttempts.push_back(static_cast<double>(sample->HullAttemptCount));
                materialZoneCounts.push_back(static_cast<double>(sample->MaterialZoneCount));
                liveryCoverage.push_back(static_cast<double>(sample->LiveryCoveragePermille) / 10.0);
                liveryPrimaryCoverage.push_back(static_cast<double>(sample->LiveryPrimaryCoveragePermille) / 10.0);
                liverySecondaryCoverage.push_back(static_cast<double>(sample->LiverySecondaryCoveragePermille) / 10.0);
                liveryConnectedCoverage.push_back(static_cast<double>(sample->LiveryLargestConnectedCoveragePermille) / 10.0);
                liverySecondaryMaterialCoverage.push_back(static_cast<double>(sample->LiverySecondaryMaterialCoveragePermille) / 10.0);
                liveryMechanicalMaterialCoverage.push_back(static_cast<double>(sample->LiveryMechanicalMaterialCoveragePermille) / 10.0);
                result.TotalLiveryCoverageRejections += sample->LiveryCoverageRejectionCount;
                result.TotalLiveryMaterialPreservationRejections += sample->LiveryMaterialPreservationRejectionCount;
                majorFeatureCounts.push_back(static_cast<double>(sample->MajorFeatureCount));
                weaponCounts.push_back(static_cast<double>(sample->WeaponCount));
                engineCounts.push_back(static_cast<double>(sample->EngineCount));
                complexityUtilization.push_back(sample->ComplexityUtilizationPercent);
                const std::size_t primaryAnchor = static_cast<std::size_t>(sample->PrimaryVisualAnchor);
                if (primaryAnchor < result.PrimaryVisualAnchorCounts.size()) { ++result.PrimaryVisualAnchorCounts[primaryAnchor]; }
                const std::size_t secondaryAnchor = static_cast<std::size_t>(sample->SecondaryVisualAnchor);
                if (secondaryAnchor < result.SecondaryVisualAnchorCounts.size()) { ++result.SecondaryVisualAnchorCounts[secondaryAnchor]; }
                if (sample->VisualHierarchyFallbackOccurred) { ++result.VisualHierarchyFallbackCount; }
                result.TotalHullValidationRejections += sample->HullValidationRejectionCount;
                result.StructuralNegativeSpaceAttempts += sample->StructuralNegativeSpaceAttemptCount;
                result.StructuralNegativeSpaceSuccesses += sample->StructuralNegativeSpaceSuccessCount;
                for (std::size_t i = 0u; i < result.SilhouetteRejectionCounts.size(); ++i) { result.SilhouetteRejectionCounts[i] += sample->SilhouetteRejectionCounts[i]; }
                for (std::size_t i = 0u; i < stageTimes.size(); ++i)
                {
                    if (sample->Performance.StageDurationNanoseconds[i] != 0u)
                    {
                        stageTimes[i].push_back(static_cast<double>(sample->Performance.StageDurationNanoseconds[i]) / 1000000.0);
                    }
                }
            }

            result.GenerationTimeMilliseconds = summarizeDiagnosticsValues(std::move(generationTimes));
            result.HullAttempts = summarizeDiagnosticsValues(std::move(hullAttempts));
            result.MaterialZoneCount = summarizeDiagnosticsValues(std::move(materialZoneCounts));
            result.LiveryCoveragePercent = summarizeDiagnosticsValues(std::move(liveryCoverage));
            result.LiveryPrimaryCoveragePercent = summarizeDiagnosticsValues(std::move(liveryPrimaryCoverage));
            result.LiverySecondaryCoveragePercent = summarizeDiagnosticsValues(std::move(liverySecondaryCoverage));
            result.LiveryLargestConnectedCoveragePercent = summarizeDiagnosticsValues(std::move(liveryConnectedCoverage));
            result.LiverySecondaryMaterialCoveragePercent = summarizeDiagnosticsValues(std::move(liverySecondaryMaterialCoverage));
            result.LiveryMechanicalMaterialCoveragePercent = summarizeDiagnosticsValues(std::move(liveryMechanicalMaterialCoverage));
            result.MajorFeatureCount = summarizeDiagnosticsValues(std::move(majorFeatureCounts));
            result.WeaponCount = summarizeDiagnosticsValues(std::move(weaponCounts));
            result.EngineCount = summarizeDiagnosticsValues(std::move(engineCounts));
            result.ComplexityUtilizationPercent = summarizeDiagnosticsValues(std::move(complexityUtilization));
            for (std::size_t i = 0u; i < stageTimes.size(); ++i) { result.StageTimeMilliseconds[i] = summarizeDiagnosticsValues(std::move(stageTimes[i])); }
            result.HullRetryRatePercent = result.SampleCount == 0u ? 0.0 : 100.0 * static_cast<double>(result.TotalHullValidationRejections) / static_cast<double>(result.SampleCount);
            result.StructuralNegativeSpaceAttemptRatePercent = result.SampleCount == 0u ? 0.0 : 100.0 * static_cast<double>(result.StructuralNegativeSpaceAttempts) / static_cast<double>(result.SampleCount);
            result.StructuralNegativeSpaceSuccessRatePercent = result.StructuralNegativeSpaceAttempts == 0u ? 0.0 : 100.0 * static_cast<double>(result.StructuralNegativeSpaceSuccesses) / static_cast<double>(result.StructuralNegativeSpaceAttempts);
            uint64_t maximum = 0u;
            for (std::size_t i = 1u; i < result.SilhouetteRejectionCounts.size(); ++i)
            {
                if (result.SilhouetteRejectionCounts[i] > maximum)
                {
                    maximum = result.SilhouetteRejectionCounts[i];
                    result.MostCommonSilhouetteRejection = static_cast<PixelShipGenerator::SilhouetteValidationFailureReason>(i);
                }
            }
            return result;
        }

        struct EtaEstimator
        {
            std::map<std::pair<uint32_t, uint32_t>, std::pair<uint64_t, uint64_t>> Buckets;
            uint64_t TotalNanoseconds = 0u;
            uint64_t TotalPixelSamples = 0u;

            void observe(const DiagnosticsWorkItem& item, uint64_t nanoseconds)
            {
                auto& bucket = Buckets[{ item.Dimensions.Width, item.Dimensions.Height }];
                bucket.first += nanoseconds;
                ++bucket.second;
                TotalNanoseconds += nanoseconds;
                TotalPixelSamples += static_cast<uint64_t>(item.Dimensions.Width) * item.Dimensions.Height;
            }

            std::optional<uint64_t> estimate(const std::vector<DiagnosticsWorkItem>& schedule, std::size_t nextIndex) const
            {
                if (nextIndex >= schedule.size()) { return 0u; }
                if (Buckets.empty()) { return std::nullopt; }
                long double estimateNs = 0.0L;
                const long double nsPerPixel = TotalPixelSamples == 0u ? 0.0L : static_cast<long double>(TotalNanoseconds) / static_cast<long double>(TotalPixelSamples);
                for (std::size_t i = nextIndex; i < schedule.size(); ++i)
                {
                    const auto key = std::make_pair(schedule[i].Dimensions.Width, schedule[i].Dimensions.Height);
                    const auto found = Buckets.find(key);
                    if (found != Buckets.end() && found->second.second != 0u)
                    {
                        estimateNs += static_cast<long double>(found->second.first) / found->second.second;
                    }
                    else
                    {
                        estimateNs += nsPerPixel * static_cast<long double>(schedule[i].Dimensions.Width) * schedule[i].Dimensions.Height;
                    }
                }
                if (!std::isfinite(static_cast<double>(estimateNs)) || estimateNs < 0.0L) { return std::nullopt; }
                return static_cast<uint64_t>(estimateNs);
            }
        };

        bool groupMatches(const DiagnosticsRawSampleResult& sample, const DiagnosticsGroupedResult& group, DiagnosticsGrouping grouping)
        {
            if ((grouping == DiagnosticsGrouping::DIMENSIONS || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE_FACTION) && group.Dimensions.has_value())
            {
                if (sample.WorkItem.Dimensions.Width != group.Dimensions->Width || sample.WorkItem.Dimensions.Height != group.Dimensions->Height) { return false; }
            }
            if ((grouping == DiagnosticsGrouping::STYLE || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE_FACTION) && group.Style.has_value() && sample.WorkItem.Style != *group.Style) { return false; }
            if ((grouping == DiagnosticsGrouping::FACTION || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE_FACTION) && group.Faction.has_value() && sample.WorkItem.Faction != *group.Faction) { return false; }
            return true;
        }
    }

    std::vector<DiagnosticsWorkItem> resolveDiagnosticsWorkSchedule(const DiagnosticsRunConfiguration& configuration)
    {
        std::vector<DiagnosticsWorkItem> result;
        uint64_t workIndex = 0u;
        uint64_t configurationIndex = 0u;
        for (const PixelShipGenerator::ShipDimensions dimensions : configuration.Dimensions)
        {
            for (const PixelShipGenerator::ShipStyle style : configuration.Styles)
            {
                for (const PixelShipGenerator::ShipFactionType faction : configuration.Factions)
                {
                    for (uint64_t sampleIndex = 0u; sampleIndex < configuration.SamplesPerConfiguration; ++sampleIndex)
                    {
                        DiagnosticsWorkItem item;
                        item.WorkIndex = workIndex++;
                        item.ConfigurationIndex = configurationIndex;
                        item.SampleIndex = sampleIndex;
                        item.Seed = deriveDiagnosticSampleSeed(configuration.DiagnosticSeed, sampleIndex);
                        item.Dimensions = dimensions;
                        item.Style = style;
                        item.Faction = faction;
                        result.push_back(item);
                    }
                    ++configurationIndex;
                }
            }
        }
        return result;
    }

    DiagnosticsDistributionSummary summarizeDiagnosticsValues(std::vector<double> values)
    {
        DiagnosticsDistributionSummary result;
        if (values.empty()) { return result; }
        std::sort(values.begin(), values.end());
        result.Count = values.size();
        result.Minimum = values.front();
        result.Maximum = values.back();
        result.Mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
        const std::size_t middle = values.size() / 2u;
        result.Median = values.size() % 2u == 0u ? (values[middle - 1u] + values[middle]) * 0.5 : values[middle];
        const std::size_t p95Index = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(values.size()))) - 1u;
        result.P95 = values[std::min(p95Index, values.size() - 1u)];
        long double variance = 0.0L;
        for (const double value : values) { const long double delta = value - result.Mean; variance += delta * delta; }
        result.StandardDeviation = std::sqrt(static_cast<double>(variance / values.size()));
        return result;
    }

    DiagnosticsAggregateSummary aggregateDiagnosticsSamplePointers(const std::vector<const DiagnosticsRawSampleResult*>& samples)
    {
        return aggregateRange(samples);
    }

    DiagnosticsAggregateSummary aggregateDiagnosticsSamples(const std::vector<DiagnosticsRawSampleResult>& samples)
    {
        std::vector<const DiagnosticsRawSampleResult*> pointers;
        pointers.reserve(samples.size());
        for (const auto& sample : samples) { pointers.push_back(&sample); }
        return aggregateRange(pointers);
    }

    std::vector<DiagnosticsGroupedResult> groupDiagnosticsSamples(const std::vector<DiagnosticsRawSampleResult>& samples, DiagnosticsGrouping grouping)
    {
        std::vector<DiagnosticsGroupedResult> groups;
        for (const auto& sample : samples)
        {
            DiagnosticsGroupedResult key;
            if (grouping == DiagnosticsGrouping::DIMENSIONS || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE_FACTION) { key.Dimensions = sample.WorkItem.Dimensions; }
            if (grouping == DiagnosticsGrouping::STYLE || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE_FACTION) { key.Style = sample.WorkItem.Style; }
            if (grouping == DiagnosticsGrouping::FACTION || grouping == DiagnosticsGrouping::DIMENSIONS_STYLE_FACTION) { key.Faction = sample.WorkItem.Faction; }
            const auto found = std::find_if(groups.begin(), groups.end(), [&](const DiagnosticsGroupedResult& group)
                {
                    return group.Dimensions == key.Dimensions && group.Style == key.Style && group.Faction == key.Faction;
                });
            if (found == groups.end()) { groups.push_back(key); }
        }
        for (auto& group : groups)
        {
            std::vector<const DiagnosticsRawSampleResult*> selected;
            for (const auto& sample : samples) { if (groupMatches(sample, group, grouping)) { selected.push_back(&sample); } }
            group.Summary = aggregateRange(selected);
        }
        return groups;
    }

    DiagnosticsResult DiagnosticsRunner::run(const DiagnosticsRunConfiguration& configuration, const DiagnosticsProgressCallback& progressCallback, const DiagnosticsCancellationCallback& cancellationCallback, const DiagnosticsSampleCallback& sampleCallback) const
    {
        DiagnosticsResult result;
        result.Configuration = configuration;
        const std::vector<DiagnosticsWorkItem> schedule = resolveDiagnosticsWorkSchedule(configuration);
        result.ScheduledWorkItems = schedule.size();
        const uint64_t configurationCount = static_cast<uint64_t>(configuration.Dimensions.size()) * configuration.Styles.size() * configuration.Factions.size();
        result.ConfigurationResults.resize(static_cast<std::size_t>(configurationCount));
        uint64_t configurationIndex = 0u;
        for (const PixelShipGenerator::ShipDimensions dimensions : configuration.Dimensions)
        {
            for (const PixelShipGenerator::ShipStyle style : configuration.Styles)
            {
                for (const PixelShipGenerator::ShipFactionType faction : configuration.Factions)
                {
                    DiagnosticsWorkItem representative;
                    representative.ConfigurationIndex = configurationIndex;
                    representative.Dimensions = dimensions;
                    representative.Style = style;
                    representative.Faction = faction;
                    result.ConfigurationResults[static_cast<std::size_t>(configurationIndex)].Configuration = makeLegacyConfiguration(configuration, representative);
                    ++configurationIndex;
                }
            }
        }
        if (schedule.empty())
        {
            result.Completed = true;
            if (progressCallback) { progressCallback({ 0u, 0u, 100.0 }); }
            return result;
        }

        PixelShipGenerator::ShipGenerator generator;
        EtaEstimator eta;
        const auto runStart = Clock::now();
        std::vector<DiagnosticsRawSampleResult> allSamples;
        allSamples.reserve(schedule.size());

        for (std::size_t index = 0u; index < schedule.size(); ++index)
        {
            if (cancellationCallback && cancellationCallback()) { result.Cancelled = true; break; }
            const DiagnosticsWorkItem& item = schedule[index];
            if (progressCallback)
            {
                DiagnosticsProgress progress;
                progress.TotalWorkItems = schedule.size();
                progress.CompletedWorkItems = result.CompletedWorkItems;
                progress.ProgressPercent = 100.0 * static_cast<double>(result.CompletedWorkItems) / static_cast<double>(schedule.size());
                progress.CurrentWidth = item.Dimensions.Width;
                progress.CurrentHeight = item.Dimensions.Height;
                progress.CurrentStyle = item.Style;
                progress.CurrentFaction = item.Faction;
                progress.CurrentSampleIndex = item.SampleIndex;
                progress.CurrentSeed = item.Seed;
                progress.ElapsedNanoseconds = durationNanoseconds(Clock::now() - runStart);
                const auto remaining = eta.estimate(schedule, index);
                progress.EstimatedRemainingAvailable = remaining.has_value();
                progress.EstimatedRemainingNanoseconds = remaining.value_or(0u);
                progress.CurrentStage = PixelShipGenerator::ShipGenerationPerformanceStage::SETUP_PLANNING;
                progressCallback(progress);
            }
            DiagnosticsRawSampleResult sample;
            sample.WorkItem = item;
            PixelShipGenerator::ShipGenerationSettings settings;
            settings.Seed = item.Seed;
            settings.Dimensions = item.Dimensions;
            settings.Style = item.Style;
            settings.Faction = item.Faction;
            settings.DetailDensity = configuration.DetailDensity;
            settings.AsymmetricDetailChance = configuration.AsymmetricDetailChance;
            settings.AttachmentsEnabled = configuration.AttachmentsEnabled;
            PixelShipGenerator::ShipGenerationDebugInfo debugInfo;
            PixelShipGenerator::ShipGenerationPerformanceInfo performance;
            const auto sampleStart = Clock::now();
            try
            {
                const bool detailedTiming = configuration.DetailedPerformanceInstrumentation && categoryEnabled(configuration, DiagnosticsCategory::PERFORMANCE);
                const PixelShipGenerator::GeneratedShip ship = detailedTiming ? generator.generate(settings, &debugInfo, &performance) : generator.generate(settings, &debugInfo);
                sample.Success = true;
                sample.FinalImageSignature = imageSignature(ship.FinalImage);
                result.OverallStatistics.recordSuccess(ship, debugInfo, makeLegacyConfiguration(configuration, item));
                result.ConfigurationResults[static_cast<std::size_t>(item.ConfigurationIndex)].Statistics.recordSuccess(ship, debugInfo, makeLegacyConfiguration(configuration, item));
            }
            catch (const std::exception& exception)
            {
                sample.ErrorMessage = exception.what();
                result.OverallStatistics.recordFailure(debugInfo);
                result.ConfigurationResults[static_cast<std::size_t>(item.ConfigurationIndex)].Statistics.recordFailure(debugInfo);
            }
            sample.TotalGenerationNanoseconds = durationNanoseconds(Clock::now() - sampleStart);
            if (configuration.DetailedPerformanceInstrumentation && categoryEnabled(configuration, DiagnosticsCategory::PERFORMANCE)) { sample.Performance = performance; }
            else { sample.Performance.TotalDurationNanoseconds = sample.TotalGenerationNanoseconds; }
            if (categoryEnabled(configuration, DiagnosticsCategory::RETRY_EFFORT))
            {
                sample.HullAttemptCount = debugInfo.HullGenerationAttemptCount;
                sample.HullValidationRejectionCount = debugInfo.HullValidationRejectionCount;
                sample.SilhouetteRejectionCounts = debugInfo.SilhouetteValidationFailureCounts;
                sample.StructuralNegativeSpaceAttemptCount = debugInfo.StructuralNegativeSpaceAttemptCount;
                sample.StructuralNegativeSpaceSuccessCount = debugInfo.StructuralNegativeSpaceSuccessCount;
                sample.MajorFeaturePlacementAttemptCount = debugInfo.MajorFeaturePlacementAttemptCount;
                sample.MajorFeaturePlacementRejectionCount = debugInfo.MajorFeaturePlacementRejectionCount;
                sample.WeaponPlacementAttemptCount = debugInfo.WeaponPlacementAttemptCount;
                sample.WeaponPlacementRejectionCount = debugInfo.WeaponPlacementRejectionCount;
                sample.WeaponRequestedGroupCount = debugInfo.WeaponRequestedGroupCount;
                sample.WeaponRealizedGroupCount = debugInfo.WeaponRealizedGroupCount;
                sample.WeaponGenerationChanceSkipCount = debugInfo.WeaponGenerationChanceSkipCount;
                sample.WeaponNoHardpointFailureCount = debugInfo.WeaponNoHardpointFailureCount;
                sample.WeaponTypeSelectionFailureCount = debugInfo.WeaponTypeSelectionFailureCount;
                sample.WeaponCandidateGeometryFailureCount = debugInfo.WeaponCandidateGeometryFailureCount;
                sample.WeaponSemanticCollisionFailureCount = debugInfo.WeaponSemanticCollisionFailureCount;
                sample.WeaponConnectivityFailureCount = debugInfo.WeaponConnectivityFailureCount;
                sample.WeaponFiringClearanceFailureCount = debugInfo.WeaponFiringClearanceFailureCount;
                sample.WeaponSymmetryPairFailureCount = debugInfo.WeaponSymmetryPairFailureCount;
                sample.WeaponSpatialBudgetRejectionCount = debugInfo.WeaponSpatialBudgetRejectionCount;
                sample.WeaponComplexityBudgetRejectionCount = debugInfo.WeaponComplexityBudgetRejectionCount;
                sample.WeaponCoveragePermille = debugInfo.WeaponCoveragePermille;
                sample.WeaponVisualAnchorOpportunity = debugInfo.WeaponVisualAnchorOpportunity;
                sample.WeaponVisualAnchorRealized = debugInfo.WeaponVisualAnchorRealized;
                sample.AttachmentPlacementAttemptCount = debugInfo.AttachmentPlacementAttemptCount;
                sample.AttachmentPlacementFailureCount = debugInfo.AttachmentPlacementFailureCount;
            }
            if (categoryEnabled(configuration, DiagnosticsCategory::GENERATION_METRICS))
            {
                sample.MaterialZoneCount = debugInfo.MaterialZoneCount;
                sample.LiveryMarkingCount = debugInfo.LiveryMarkingCount;
                sample.LiveryPrimaryCoveragePermille = debugInfo.LiveryPrimaryCoveragePermille;
                sample.LiverySecondaryCoveragePermille = debugInfo.LiverySecondaryCoveragePermille;
                sample.LiveryCoveragePermille = debugInfo.LiveryCoveragePermille;
                sample.LiveryLargestConnectedCoveragePermille = debugInfo.LiveryLargestConnectedCoveragePermille;
                sample.LiverySecondaryMaterialCoveragePermille = debugInfo.LiverySecondaryMaterialCoveragePermille;
                sample.LiveryMechanicalMaterialCoveragePermille = debugInfo.LiveryMechanicalMaterialCoveragePermille;
                sample.LiveryCoverageRejectionCount = debugInfo.LiveryCoverageRejectionCount;
                sample.LiveryMaterialPreservationRejectionCount = debugInfo.LiveryMaterialPreservationRejectionCount;
                sample.DetailMotifOccurrenceCount = debugInfo.PrimaryDetailMotifOccurrenceCount + debugInfo.SecondaryDetailMotifOccurrenceCount;
                sample.MajorFeatureCount = debugInfo.MajorFeatureCount;
                sample.WeaponCount = debugInfo.WeaponCount;
                sample.EngineCount = debugInfo.EngineCount;
                sample.ComplexityUtilizationPercent = debugInfo.ComplexityInitialBudget == 0u ? 0.0 :
                    100.0 * static_cast<double>(debugInfo.ComplexityConsumedBudget) / static_cast<double>(debugInfo.ComplexityInitialBudget);
                sample.PrimaryVisualAnchor = debugInfo.PrimaryVisualAnchor;
                sample.SecondaryVisualAnchor = debugInfo.SecondaryVisualAnchor;
                sample.VisualHierarchyFallbackOccurred = debugInfo.VisualHierarchyFallbackOccurred;
            }
            eta.observe(item, sample.TotalGenerationNanoseconds);
            allSamples.push_back(sample);
            if (sampleCallback) { sampleCallback(allSamples.back()); }
            ++result.CompletedWorkItems;

            if (progressCallback)
            {
                DiagnosticsProgress progress;
                progress.TotalWorkItems = schedule.size();
                progress.CompletedWorkItems = result.CompletedWorkItems;
                progress.ProgressPercent = 100.0 * static_cast<double>(result.CompletedWorkItems) / static_cast<double>(schedule.size());
                progress.CurrentWidth = item.Dimensions.Width;
                progress.CurrentHeight = item.Dimensions.Height;
                progress.CurrentStyle = item.Style;
                progress.CurrentFaction = item.Faction;
                progress.CurrentSampleIndex = item.SampleIndex;
                progress.CurrentSeed = item.Seed;
                progress.ElapsedNanoseconds = durationNanoseconds(Clock::now() - runStart);
                const auto remaining = eta.estimate(schedule, index + 1u);
                progress.EstimatedRemainingAvailable = remaining.has_value();
                progress.EstimatedRemainingNanoseconds = remaining.value_or(0u);
                progress.CurrentStage = PixelShipGenerator::ShipGenerationPerformanceStage::SHIP_GENERATION_PERFORMANCE_STAGE_END;
                progressCallback(progress);
            }
        }

        result.ElapsedNanoseconds = durationNanoseconds(Clock::now() - runStart);
        result.Completed = !result.Cancelled && result.CompletedWorkItems == result.ScheduledWorkItems;
        result.OverallSummary = aggregateDiagnosticsSamples(allSamples);
        for (std::size_t configurationIndex = 0u; configurationIndex < result.ConfigurationResults.size(); ++configurationIndex)
        {
            std::vector<DiagnosticsRawSampleResult> selected;
            for (const auto& sample : allSamples) { if (sample.WorkItem.ConfigurationIndex == configurationIndex) { selected.push_back(sample); } }
            result.ConfigurationResults[configurationIndex].PerformanceSummary = aggregateDiagnosticsSamples(selected);
        }
        if (configuration.DetailLevel == DiagnosticsDetailLevel::RAW_SAMPLES_AND_SUMMARY) { result.Samples = std::move(allSamples); }
        return result;
    }

    void printDiagnosticsResultSummary(std::ostream& output, const DiagnosticsResult& result)
    {
        output << std::fixed << std::setprecision(3);
        output << "\n## Diagnostics Run\n";
        output << "Status: " << (result.Cancelled ? "CANCELLED" : (result.Completed ? "COMPLETE" : "INCOMPLETE")) << '\n';
        output << "Samples: " << result.CompletedWorkItems << '/' << result.ScheduledWorkItems << '\n';
        output << "Elapsed ms: " << static_cast<double>(result.ElapsedNanoseconds) / 1000000.0 << '\n';
        output << "Generation time ms: avg " << result.OverallSummary.GenerationTimeMilliseconds.Mean << " | median " << result.OverallSummary.GenerationTimeMilliseconds.Median << " | P95 " << result.OverallSummary.GenerationTimeMilliseconds.P95 << " | max " << result.OverallSummary.GenerationTimeMilliseconds.Maximum << '\n';
        output << "Hull average attempts: " << result.OverallSummary.HullAttempts.Mean << '\n';
        output << "Hull retry attempts / 100 ships: " << result.OverallSummary.HullRetryRatePercent << '\n';
        output << "Negative-space attempt rate: " << result.OverallSummary.StructuralNegativeSpaceAttemptRatePercent << "%\n";
        output << "Negative-space success rate: " << result.OverallSummary.StructuralNegativeSpaceSuccessRatePercent << "%\n";
        output << "Average material zones: " << result.OverallSummary.MaterialZoneCount.Mean << '\n';
        output << "Livery coverage %: avg " << result.OverallSummary.LiveryCoveragePercent.Mean << " | median " << result.OverallSummary.LiveryCoveragePercent.Median << " | P95 " << result.OverallSummary.LiveryCoveragePercent.P95 << '\n';
        output << "Livery largest connected coverage %: avg " << result.OverallSummary.LiveryLargestConnectedCoveragePercent.Mean << " | P95 " << result.OverallSummary.LiveryLargestConnectedCoveragePercent.P95 << '\n';
        output << "Most common silhouette rejection: " << PixelShipGenerator::getSilhouetteValidationFailureReasonName(result.OverallSummary.MostCommonSilhouetteRejection) << '\n';
        if (result.Configuration.DetailedPerformanceInstrumentation)
        {
            output << "Stage timing (mean ms):\n";
            for (std::size_t stage = 0u; stage < result.OverallSummary.StageTimeMilliseconds.size(); ++stage)
            {
                const auto& timing = result.OverallSummary.StageTimeMilliseconds[stage];
                if (timing.Count == 0u) { continue; }
                output << "  " << PixelShipGenerator::getShipGenerationPerformanceStageName(static_cast<PixelShipGenerator::ShipGenerationPerformanceStage>(stage)) << ": " << timing.Mean << '\n';
            }
        }
        if (!result.Configuration.BuildConfiguration.empty()) { output << "Build: " << result.Configuration.BuildConfiguration << '\n'; }
        if (!result.Configuration.VersionIdentifier.empty()) { output << "Version: " << result.Configuration.VersionIdentifier << '\n'; }
        output << "Timing mode: " << (result.Configuration.DetailedPerformanceInstrumentation ? "DETAILED" : "TOTAL_ONLY") << '\n';
    }

    void writeDiagnosticsResultCsv(std::ostream& output, const DiagnosticsResult& result)
    {
        if (!result.PersistedCsvSnapshot.empty()) { output << result.PersistedCsvSnapshot; return; }
        std::ostringstream legacyHeader;
        writeGenerationStatisticsCsvHeader(legacyHeader);
        std::string header = legacyHeader.str();
        while (!header.empty() && (header.back() == '\n' || header.back() == '\r')) { header.pop_back(); }
        output << header << ",timing_mode,build_configuration,version_identifier,generation_time_mean_ms,generation_time_median_ms,generation_time_p95_ms,generation_time_max_ms,hull_retry_attempts_per_100,negative_space_attempt_rate_percent,negative_space_success_rate_percent,livery_coverage_mean_percent,livery_coverage_median_percent,livery_coverage_p95_percent,livery_largest_connected_mean_percent,livery_largest_connected_p95_percent,livery_coverage_rejections,livery_material_rejections";
        for (std::size_t stage = 0u; stage < PixelShipGenerator::ShipGenerationPerformanceStageCount; ++stage)
        {
            output << ",stage_" << PixelShipGenerator::getShipGenerationPerformanceStageName(static_cast<PixelShipGenerator::ShipGenerationPerformanceStage>(stage)) << "_mean_ms";
        }
        output << '\n';
        for (const auto& configurationResult : result.ConfigurationResults)
        {
            std::ostringstream legacyRow;
            writeGenerationStatisticsCsvRow(legacyRow, configurationResult.Configuration, configurationResult.Statistics);
            std::string row = legacyRow.str();
            while (!row.empty() && (row.back() == '\n' || row.back() == '\r')) { row.pop_back(); }
            const auto& summary = configurationResult.PerformanceSummary;
            output << row << ',' << (result.Configuration.DetailedPerformanceInstrumentation ? "DETAILED" : "TOTAL_ONLY") << ',' << result.Configuration.BuildConfiguration << ',' << result.Configuration.VersionIdentifier << ',' << summary.GenerationTimeMilliseconds.Mean << ',' << summary.GenerationTimeMilliseconds.Median << ',' << summary.GenerationTimeMilliseconds.P95 << ',' << summary.GenerationTimeMilliseconds.Maximum << ',' << summary.HullRetryRatePercent << ',' << summary.StructuralNegativeSpaceAttemptRatePercent << ',' << summary.StructuralNegativeSpaceSuccessRatePercent << ',' << summary.LiveryCoveragePercent.Mean << ',' << summary.LiveryCoveragePercent.Median << ',' << summary.LiveryCoveragePercent.P95 << ',' << summary.LiveryLargestConnectedCoveragePercent.Mean << ',' << summary.LiveryLargestConnectedCoveragePercent.P95 << ',' << summary.TotalLiveryCoverageRejections << ',' << summary.TotalLiveryMaterialPreservationRejections;
            for (const auto& stage : summary.StageTimeMilliseconds) { output << ',' << stage.Mean; }
            output << '\n';
        }
    }
}
