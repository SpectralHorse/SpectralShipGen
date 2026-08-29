#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "GenerationStatistics.h"
#include "ShipDimensions.h"
#include "ShipGenerationPerformance.h"

namespace PixelShipGeneratorDiagnostics
{
    enum class DiagnosticsDetailLevel : uint32_t
    {
        SUMMARY_ONLY = 0u,
        RAW_SAMPLES_AND_SUMMARY
    };

    enum class DiagnosticsCategory : uint32_t
    {
        GENERATION_METRICS = 1u << 0u,
        RETRY_EFFORT = 1u << 1u,
        PERFORMANCE = 1u << 2u
    };

    constexpr uint32_t DiagnosticsCategoryAll =
        static_cast<uint32_t>(DiagnosticsCategory::GENERATION_METRICS) |
        static_cast<uint32_t>(DiagnosticsCategory::RETRY_EFFORT) |
        static_cast<uint32_t>(DiagnosticsCategory::PERFORMANCE);

    struct DiagnosticsRunConfiguration
    {
        std::vector<PixelShipGenerator::ShipDimensions> Dimensions = { { 44u, 44u } };
        std::vector<PixelShipGenerator::ShipStyle> Styles = { PixelShipGenerator::ShipStyle::FIGHTER };
        std::vector<PixelShipGenerator::ShipFactionType> Factions = { PixelShipGenerator::ShipFactionType::FRONTIER };
        uint64_t SamplesPerConfiguration = 1000u;
        uint64_t DiagnosticSeed = 0x6A09E667F3BCC909ull;
        uint32_t DetailDensity = 50u;
        uint32_t AsymmetricDetailChance = 10u;
        bool AttachmentsEnabled = true;
        uint32_t EnabledCategories = DiagnosticsCategoryAll;
        bool DetailedPerformanceInstrumentation = false;
        DiagnosticsDetailLevel DetailLevel = DiagnosticsDetailLevel::RAW_SAMPLES_AND_SUMMARY;
        std::string BuildConfiguration;
        std::string VersionIdentifier;
    };

    struct DiagnosticsWorkItem
    {
        uint64_t WorkIndex = 0u;
        uint64_t ConfigurationIndex = 0u;
        uint64_t SampleIndex = 0u;
        uint64_t Seed = 0u;
        PixelShipGenerator::ShipDimensions Dimensions;
        PixelShipGenerator::ShipStyle Style = PixelShipGenerator::ShipStyle::FIGHTER;
        PixelShipGenerator::ShipFactionType Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
    };

    struct DiagnosticsProgress
    {
        uint64_t TotalWorkItems = 0u;
        uint64_t CompletedWorkItems = 0u;
        double ProgressPercent = 0.0;
        uint32_t CurrentWidth = 0u;
        uint32_t CurrentHeight = 0u;
        PixelShipGenerator::ShipStyle CurrentStyle = PixelShipGenerator::ShipStyle::FIGHTER;
        PixelShipGenerator::ShipFactionType CurrentFaction = PixelShipGenerator::ShipFactionType::FRONTIER;
        uint64_t CurrentSampleIndex = 0u;
        uint64_t CurrentSeed = 0u;
        uint64_t ElapsedNanoseconds = 0u;
        bool EstimatedRemainingAvailable = false;
        uint64_t EstimatedRemainingNanoseconds = 0u;
        PixelShipGenerator::ShipGenerationPerformanceStage CurrentStage = PixelShipGenerator::ShipGenerationPerformanceStage::SETUP_PLANNING;
    };

    struct DiagnosticsRawSampleResult
    {
        DiagnosticsWorkItem WorkItem;
        bool Success = false;
        std::string ErrorMessage;
        uint64_t TotalGenerationNanoseconds = 0u;
        PixelShipGenerator::ShipGenerationPerformanceInfo Performance;
        uint32_t HullAttemptCount = 0u;
        uint32_t HullValidationRejectionCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(PixelShipGenerator::SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END)> SilhouetteRejectionCounts = {};
        uint32_t StructuralNegativeSpaceAttemptCount = 0u;
        uint32_t StructuralNegativeSpaceSuccessCount = 0u;
        uint32_t MajorFeaturePlacementAttemptCount = 0u;
        uint32_t MajorFeaturePlacementRejectionCount = 0u;
        uint32_t WeaponPlacementAttemptCount = 0u;
        uint32_t WeaponPlacementRejectionCount = 0u;
        uint32_t AttachmentPlacementAttemptCount = 0u;
        uint32_t AttachmentPlacementFailureCount = 0u;
        uint32_t MaterialZoneCount = 0u;
        uint32_t LiveryMarkingCount = 0u;
        uint32_t DetailMotifOccurrenceCount = 0u;
        uint64_t FinalImageSignature = 0u;
    };

    struct DiagnosticsDistributionSummary
    {
        uint64_t Count = 0u;
        double Mean = 0.0;
        double Median = 0.0;
        double Minimum = 0.0;
        double Maximum = 0.0;
        double P95 = 0.0;
        double StandardDeviation = 0.0;
    };

    struct DiagnosticsAggregateSummary
    {
        uint64_t SampleCount = 0u;
        uint64_t SuccessfulSamples = 0u;
        uint64_t FailedSamples = 0u;
        DiagnosticsDistributionSummary GenerationTimeMilliseconds;
        DiagnosticsDistributionSummary HullAttempts;
        DiagnosticsDistributionSummary MaterialZoneCount;
        std::array<DiagnosticsDistributionSummary, PixelShipGenerator::ShipGenerationPerformanceStageCount> StageTimeMilliseconds;
        uint64_t TotalHullValidationRejections = 0u;
        uint64_t StructuralNegativeSpaceAttempts = 0u;
        uint64_t StructuralNegativeSpaceSuccesses = 0u;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END)> SilhouetteRejectionCounts = {};
        double HullRetryRatePercent = 0.0;
        double StructuralNegativeSpaceAttemptRatePercent = 0.0;
        double StructuralNegativeSpaceSuccessRatePercent = 0.0;
        PixelShipGenerator::SilhouetteValidationFailureReason MostCommonSilhouetteRejection = PixelShipGenerator::SilhouetteValidationFailureReason::NONE;
    };

    struct DiagnosticsConfigurationResult
    {
        DiagnosticGenerationConfiguration Configuration;
        GenerationStatistics Statistics;
        DiagnosticsAggregateSummary PerformanceSummary;
    };

    enum class DiagnosticsGrouping : uint32_t
    {
        DIMENSIONS = 0u,
        STYLE,
        FACTION,
        DIMENSIONS_STYLE,
        DIMENSIONS_STYLE_FACTION
    };

    struct DiagnosticsGroupedResult
    {
        std::optional<PixelShipGenerator::ShipDimensions> Dimensions;
        std::optional<PixelShipGenerator::ShipStyle> Style;
        std::optional<PixelShipGenerator::ShipFactionType> Faction;
        DiagnosticsAggregateSummary Summary;
    };

    struct DiagnosticsResult
    {
        DiagnosticsRunConfiguration Configuration;
        bool Completed = false;
        bool Cancelled = false;
        uint64_t ScheduledWorkItems = 0u;
        uint64_t CompletedWorkItems = 0u;
        uint64_t ElapsedNanoseconds = 0u;
        std::vector<DiagnosticsRawSampleResult> Samples;
        GenerationStatistics OverallStatistics;
        DiagnosticsAggregateSummary OverallSummary;
        std::vector<DiagnosticsConfigurationResult> ConfigurationResults;
    };

    using DiagnosticsProgressCallback = std::function<void(const DiagnosticsProgress&)>;
    using DiagnosticsCancellationCallback = std::function<bool()>;
    using DiagnosticsSampleCallback = std::function<void(const DiagnosticsRawSampleResult&)>;

    std::vector<DiagnosticsWorkItem> resolveDiagnosticsWorkSchedule(const DiagnosticsRunConfiguration& configuration);
    DiagnosticsDistributionSummary summarizeDiagnosticsValues(std::vector<double> values);
    DiagnosticsAggregateSummary aggregateDiagnosticsSamples(const std::vector<DiagnosticsRawSampleResult>& samples);
    std::vector<DiagnosticsGroupedResult> groupDiagnosticsSamples(const std::vector<DiagnosticsRawSampleResult>& samples, DiagnosticsGrouping grouping);

    class DiagnosticsRunner
    {
    public:
        DiagnosticsResult run(const DiagnosticsRunConfiguration& configuration,
            const DiagnosticsProgressCallback& progressCallback = {},
            const DiagnosticsCancellationCallback& cancellationCallback = {},
            const DiagnosticsSampleCallback& sampleCallback = {}) const;
    };

    void printDiagnosticsResultSummary(std::ostream& output, const DiagnosticsResult& result);
    void writeDiagnosticsResultCsv(std::ostream& output, const DiagnosticsResult& result);
}
