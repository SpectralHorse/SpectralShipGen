#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include <PixelShipGenerator/Diagnostics/GenerationStatistics.h>

namespace
{
    bool isNormalizedStatisticValid(const PixelShipGeneratorDiagnostics::NumericStatistics& statistics)
    {
        if (statistics.Count == 0u)
        {
            return true;
        }

        return statistics.Minimum >= 0.0 && statistics.Maximum <= 1.0;
    }
}

int PixelShipGeneratorTests::runGeneratorStatisticsRegression()
{
    using namespace PixelShipGeneratorDiagnostics;

    DiagnosticGenerationConfiguration deterministicConfiguration;
    deterministicConfiguration.Width = 44u;
    deterministicConfiguration.Height = 44u;
    deterministicConfiguration.Style = PixelShipGenerator::ShipStyle::HEAVY;
    deterministicConfiguration.Faction = PixelShipGenerator::ShipFactionType::XENO;
    deterministicConfiguration.Samples = 64u;
    deterministicConfiguration.DiagnosticSeed = 0x123456789ABCDEF0ull;

    const GenerationStatistics first = collectGenerationStatistics(deterministicConfiguration);
    const GenerationStatistics second = collectGenerationStatistics(deterministicConfiguration);

    if (first.deterministicSignature() != second.deterministicSignature())
    {
        std::cerr << "Diagnostic regression failed: identical batch configuration produced different aggregate statistics.\n";
        return 1;
    }

    DiagnosticGenerationConfiguration differentSeedConfiguration = deterministicConfiguration;
    differentSeedConfiguration.DiagnosticSeed ^= 0x9E3779B97F4A7C15ull;
    const GenerationStatistics differentSeed = collectGenerationStatistics(differentSeedConfiguration);

    if (first.deterministicSignature() == differentSeed.deterministicSignature())
    {
        std::cerr << "Diagnostic regression failed: changing the diagnostic root seed did not change aggregate statistics.\n";
        return 1;
    }

    constexpr std::array<uint32_t, 7u> resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };

    for (const uint32_t resolution : resolutions)
    {
        DiagnosticGenerationConfiguration configuration;
        configuration.Width = resolution;
        configuration.Height = resolution;
        configuration.Style = PixelShipGenerator::ShipStyle::FIGHTER;
        configuration.Faction = PixelShipGenerator::ShipFactionType::MILITARY;
        configuration.Samples = 8u;
        configuration.DiagnosticSeed = 0xCAFEBABE12345678ull;
        const GenerationStatistics statistics = collectGenerationStatistics(configuration);

        if (statistics.RequestedGenerations != configuration.Samples || statistics.SuccessfulGenerations + statistics.FailedGenerations != configuration.Samples)
        {
            std::cerr << "Diagnostic regression failed for resolution " << resolution << ": sample accounting mismatch.\n";
            return 1;
        }

        if (!isNormalizedStatisticValid(statistics.HullNormalizedWidth) || !isNormalizedStatisticValid(statistics.HullNormalizedHeight) || !isNormalizedStatisticValid(statistics.HullCanvasDensity) || !isNormalizedStatisticValid(statistics.HullBoundingFillDensity) || !isNormalizedStatisticValid(statistics.DetailMaskCanvasDensity))
        {
            std::cerr << "Diagnostic regression failed for resolution " << resolution << ": normalized statistic outside [0, 1].\n";
            return 1;
        }
    }


    DiagnosticGenerationConfiguration rectangularConfiguration;
    rectangularConfiguration.Width = 64u;
    rectangularConfiguration.Height = 32u;
    rectangularConfiguration.Style = PixelShipGenerator::ShipStyle::INDUSTRIAL;
    rectangularConfiguration.Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
    rectangularConfiguration.Samples = 4u;
    rectangularConfiguration.DiagnosticSeed = 0xA4093822299F31D0ull;
    const GenerationStatistics rectangularStatistics = collectGenerationStatistics(rectangularConfiguration);

    std::ostringstream summaryOutput;
    printGenerationStatistics(summaryOutput, rectangularConfiguration, rectangularStatistics);
    const std::string summaryText = summaryOutput.str();
    if (summaryText.find("Scale tier:") == std::string::npos || summaryText.find("Horizontal capacity:") == std::string::npos || summaryText.find("Longitudinal capacity:") == std::string::npos || summaryText.find("Average articulation events:") == std::string::npos || summaryText.find("Average structural negative-space features:") == std::string::npos || summaryText.find("Void REAR_FORK:") == std::string::npos || summaryText.find("LOW_ARTICULATION:") == std::string::npos || summaryText.find("## Visual Hierarchy") == std::string::npos || summaryText.find("## Material Composition") == std::string::npos || summaryText.find("Average material zones:") == std::string::npos || summaryText.find("Zone WING_SURFACE:") == std::string::npos || summaryText.find("## Procedural Livery") == std::string::npos || summaryText.find("Average livery markings:") == std::string::npos || summaryText.find("Marking WING_BAND:") == std::string::npos || summaryText.find("## Detail Motif Grammar") == std::string::npos || summaryText.find("Average primary motif occurrences:") == std::string::npos || summaryText.find("Primary PAIRED_VENTS:") == std::string::npos || summaryText.find("Primary WEAPONS:") == std::string::npos || summaryText.find("Fallback rate:") == std::string::npos || summaryText.find("## Complexity Budget") == std::string::npos || summaryText.find("Average utilization:") == std::string::npos || summaryText.find("Average cockpit pixels:") == std::string::npos || summaryText.find("Average cockpit complexity cost:") == std::string::npos || summaryText.find("## Hull Layers") == std::string::npos || summaryText.find("## Semantic Spatial Load") == std::string::npos || summaryText.find("LEFT_OUTER_WING") == std::string::npos)
    {
        std::cerr << "Diagnostic regression failed: scale-trait summary fields are missing.\n";
        return 1;
    }


    if (rectangularStatistics.SilhouetteArticulationCount.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.SilhouetteGuidanceAppliedCount.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.StructuralNegativeSpaceCount.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.StructuralNegativeSpacePixelCount.Count != rectangularStatistics.SuccessfulGenerations)
    {
        std::cerr << "Diagnostic regression failed: silhouette statistics were not recorded for successful generations.\n";
        return 1;
    }

    if (rectangularStatistics.VisualHierarchyReservedComplexity.Count != rectangularStatistics.SuccessfulGenerations)
    {
        std::cerr << "Diagnostic regression failed: visual hierarchy statistics were not recorded for successful generations.\n";
        return 1;
    }

    if (rectangularStatistics.LiveryMarkingCount.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.LiveryPrimaryPixelCount.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.LiverySecondaryPixelCount.Count != rectangularStatistics.SuccessfulGenerations)
    {
        std::cerr << "Diagnostic regression failed: livery statistics were not recorded for successful generations.\n";
        return 1;
    }

    if (rectangularStatistics.DetailMotifPrimaryOccurrences.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.DetailMotifSecondaryOccurrences.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.DetailMotifRejectedPlacements.Count != rectangularStatistics.SuccessfulGenerations)
    {
        std::cerr << "Diagnostic regression failed: detail motif statistics were not recorded for successful generations.\n";
        return 1;
    }

    if (rectangularStatistics.SpatialAverageUtilizationPercent.Count != rectangularStatistics.SuccessfulGenerations || rectangularStatistics.SpatialOverloadRejections.Count != rectangularStatistics.SuccessfulGenerations)
    {
        std::cerr << "Diagnostic regression failed: semantic spatial statistics were not recorded for successful generations.\n";
        return 1;
    }

    std::ostringstream csvHeaderOutput;
    writeGenerationStatisticsCsvHeader(csvHeaderOutput);
    const std::string csvHeader = csvHeaderOutput.str();
    if (csvHeader.find("scale_tier") == std::string::npos || csvHeader.find("horizontal_capacity") == std::string::npos || csvHeader.find("animation_complexity") == std::string::npos || csvHeader.find("average_silhouette_articulation") == std::string::npos || csvHeader.find("average_structural_negative_space_count") == std::string::npos || csvHeader.find("negative_space_REAR_FORK_ship_percent") == std::string::npos || csvHeader.find("silhouette_retry_LOW_ARTICULATION") == std::string::npos || csvHeader.find("average_visual_hierarchy_reserved_complexity") == std::string::npos || csvHeader.find("average_material_zone_count") == std::string::npos || csvHeader.find("material_zone_AXIAL_BAND_ship_percent") == std::string::npos || csvHeader.find("average_livery_marking_count") == std::string::npos || csvHeader.find("livery_WING_BAND_ship_percent") == std::string::npos || csvHeader.find("average_primary_detail_motif_occurrences") == std::string::npos || csvHeader.find("primary_detail_motif_PAIRED_VENTS_percent") == std::string::npos || csvHeader.find("secondary_detail_motif_RECESSED_SLOT_percent") == std::string::npos || csvHeader.find("visual_hierarchy_fallback_percent") == std::string::npos || csvHeader.find("primary_anchor_WEAPONS_percent") == std::string::npos || csvHeader.find("secondary_anchor_CENTRAL_CORE_percent") == std::string::npos || csvHeader.find("average_complexity_utilization_percent") == std::string::npos || csvHeader.find("complexity_LARGE_WEAPON_consumed") == std::string::npos || csvHeader.find("complexity_COCKPIT_STRUCTURE_consumed") == std::string::npos || csvHeader.find("average_cockpit_glass_pixels") == std::string::npos || csvHeader.find("cockpit_size_MASSIVE_ship_percent") == std::string::npos || csvHeader.find("average_hull_layer_count") == std::string::npos || csvHeader.find("complexity_HULL_LAYER_consumed") == std::string::npos || csvHeader.find("average_spatial_utilization_percent") == std::string::npos || csvHeader.find("spatial_LEFT_OUTER_WING_load_percent") == std::string::npos || csvHeader.find("spatial_MID_FUSELAGE_rejections") == std::string::npos)
    {
        std::cerr << "Diagnostic regression failed: scale-trait CSV columns are missing.\n";
        return 1;
    }

    std::ostringstream csvRowOutput;
    writeGenerationStatisticsCsvRow(csvRowOutput, rectangularConfiguration, rectangularStatistics);
    const std::string csvRow = csvRowOutput.str();
    if (std::count(csvHeader.begin(), csvHeader.end(), ',') != std::count(csvRow.begin(), csvRow.end(), ','))
    {
        std::cerr << "Diagnostic regression failed: statistics CSV header/row column counts differ.\n";
        return 1;
    }

    DiagnosticGenerationConfiguration zeroSamplesConfiguration;
    zeroSamplesConfiguration.Samples = 0u;
    const GenerationStatistics zeroSamplesStatistics = collectGenerationStatistics(zeroSamplesConfiguration);

    if (zeroSamplesStatistics.RequestedGenerations != 0u || zeroSamplesStatistics.HullAttempts.average() != 0.0)
    {
        std::cerr << "Diagnostic regression failed: zero-sample batch was not handled cleanly.\n";
        return 1;
    }

    std::cout << "Ship generator statistics regression passed.\n";
    return 0;
}
