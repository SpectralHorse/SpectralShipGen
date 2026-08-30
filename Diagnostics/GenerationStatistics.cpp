#include "GenerationStatistics.h"
#include "GenerationScaleTraits.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <iomanip>
#include <limits>
#include <numeric>
#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

#include "DiagnosticsRunner.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{

    const char* styleName(PixelShipGenerator::ShipStyle style)
    {
        using PixelShipGenerator::ShipStyle;
        switch (style)
        {
        case ShipStyle::SLEEK: return "SLEEK";
        case ShipStyle::FIGHTER: return "FIGHTER";
        case ShipStyle::HEAVY: return "HEAVY";
        case ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case ShipStyle::DELTA: return "DELTA";
        case ShipStyle::SHIP_STYLE_END: return "CUSTOM";
        default: return "UNKNOWN";
        }
    }

    const char* factionName(PixelShipGenerator::ShipFactionType faction)
    {
        using PixelShipGenerator::ShipFactionType;
        switch (faction)
        {
        case ShipFactionType::FRONTIER: return "FRONTIER";
        case ShipFactionType::MILITARY: return "MILITARY";
        case ShipFactionType::ASCENDANT: return "ASCENDANT";
        case ShipFactionType::XENO: return "XENO";
        case ShipFactionType::CORPORATE: return "CORPORATE";
        case ShipFactionType::RELIC: return "RELIC";
        case ShipFactionType::SHIP_FACTION_TYPE_END: return "CUSTOM";
        default: return "UNKNOWN";
        }
    }
    struct MaskMetrics
    {
        uint32_t PixelCount = 0u;
        uint32_t Width = 0u;
        uint32_t Height = 0u;
        uint32_t BoundingArea = 0u;
        bool Valid = false;
    };

    MaskMetrics calculateMaskMetrics(const PixelShipGenerator::PixelMask& mask)
    {
        MaskMetrics result;
        uint32_t minimumX = mask.getWidth();
        uint32_t minimumY = mask.getHeight();
        uint32_t maximumX = 0u;
        uint32_t maximumY = 0u;

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y))
                {
                    continue;
                }

                ++result.PixelCount;
                minimumX = std::min(minimumX, x);
                minimumY = std::min(minimumY, y);
                maximumX = std::max(maximumX, x);
                maximumY = std::max(maximumY, y);
            }
        }

        if (result.PixelCount == 0u)
        {
            return result;
        }

        result.Valid = true;
        result.Width = maximumX - minimumX + 1u;
        result.Height = maximumY - minimumY + 1u;
        result.BoundingArea = result.Width * result.Height;
        return result;
    }

    uint32_t countMaskPixels(const PixelShipGenerator::PixelMask& mask)
    {
        uint32_t count = 0u;

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    const char* getHullModifierName(PixelShipGenerator::HullModifierType type)
    {
        using PixelShipGenerator::HullModifierType;
        switch (type)
        {
        case HullModifierType::BROADER_SHOULDERS: return "BROADER_SHOULDERS";
        case HullModifierType::SIDE_LOBES: return "SIDE_LOBES";
        case HullModifierType::STEPPED_WING_EXTENSION: return "STEPPED_WING_EXTENSION";
        case HullModifierType::NARROW_WAIST: return "NARROW_WAIST";
        case HullModifierType::WING_CUTOUT: return "WING_CUTOUT";
        case HullModifierType::SPLIT_NOSE: return "SPLIT_NOSE";
        default: return "UNKNOWN";
        }
    }


    const char* getHullLayerName(PixelShipGenerator::ShipHullLayerType type)
    {
        return PixelShipGenerator::getShipHullLayerTypeName(type);
    }

    const char* getWingShapeName(PixelShipGenerator::WingShapeType type)
    {
        using PixelShipGenerator::WingShapeType;
        switch (type)
        {
        case WingShapeType::NONE: return "NONE";
        case WingShapeType::SMALL: return "SMALL";
        case WingShapeType::SWEPT: return "SWEPT";
        case WingShapeType::BROAD: return "BROAD";
        default: return "UNKNOWN";
        }
    }

    const char* getMajorFeatureName(PixelShipGenerator::ShipMajorFeatureType type)
    {
        using PixelShipGenerator::ShipMajorFeatureType;
        switch (type)
        {
        case ShipMajorFeatureType::CENTRAL_SPINE: return "CENTRAL_SPINE";
        case ShipMajorFeatureType::ARMOR_PLATE: return "ARMOR_PLATE";
        case ShipMajorFeatureType::RECESSED_BAY: return "RECESSED_BAY";
        case ShipMajorFeatureType::VENT_BANK: return "VENT_BANK";
        case ShipMajorFeatureType::WING_PLATE: return "WING_PLATE";
        case ShipMajorFeatureType::TECH_CORE: return "TECH_CORE";
        default: return "UNKNOWN";
        }
    }

    const char* getWeaponTypeName(PixelShipGenerator::ShipWeaponType type)
    {
        using PixelShipGenerator::ShipWeaponType;
        switch (type)
        {
        case ShipWeaponType::SINGLE_CANNON: return "SINGLE_CANNON";
        case ShipWeaponType::TWIN_CANNON: return "TWIN_CANNON";
        case ShipWeaponType::COMPACT_TURRET: return "COMPACT_TURRET";
        case ShipWeaponType::RAIL_WEAPON: return "RAIL_WEAPON";
        case ShipWeaponType::WEAPON_POD: return "WEAPON_POD";
        default: return "UNKNOWN";
        }
    }

    const char* getWeaponRegionName(PixelShipGenerator::ShipWeaponHardpointRegion region)
    {
        using PixelShipGenerator::ShipWeaponHardpointRegion;
        switch (region)
        {
        case ShipWeaponHardpointRegion::CENTRAL_NOSE: return "CENTRAL_NOSE";
        case ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE: return "FORWARD_FUSELAGE_SIDE";
        case ShipWeaponHardpointRegion::WING_ROOT: return "WING_ROOT";
        case ShipWeaponHardpointRegion::OUTER_WING: return "OUTER_WING";
        case ShipWeaponHardpointRegion::FORWARD_SHOULDER: return "FORWARD_SHOULDER";
        case ShipWeaponHardpointRegion::CENTRAL_BODY: return "CENTRAL_BODY";
        default: return "UNKNOWN";
        }
    }

    const char* getEngineLayoutName(PixelShipGenerator::EngineLayoutType type)
    {
        using PixelShipGenerator::EngineLayoutType;
        switch (type)
        {
        case EngineLayoutType::CENTRAL: return "CENTRAL";
        case EngineLayoutType::TWIN: return "TWIN";
        case EngineLayoutType::QUAD: return "QUAD";
        case EngineLayoutType::CENTRAL_AUXILIARY: return "CENTRAL_AUXILIARY";
        case EngineLayoutType::WIDE_BANK: return "WIDE_BANK";
        default: return "UNKNOWN";
        }
    }

    const char* getEngineSizeName(PixelShipGenerator::EngineSizeClass type)
    {
        using PixelShipGenerator::EngineSizeClass;
        switch (type)
        {
        case EngineSizeClass::SMALL: return "SMALL";
        case EngineSizeClass::MEDIUM: return "MEDIUM";
        case EngineSizeClass::LARGE: return "LARGE";
        default: return "UNKNOWN";
        }
    }

    const char* getAttachmentTypeName(PixelShipGenerator::ShipAttachmentType type)
    {
        using PixelShipGenerator::ShipAttachmentType;
        switch (type)
        {
        case ShipAttachmentType::WEAPON_MOUNT: return "WEAPON_MOUNT";
        case ShipAttachmentType::SENSOR_ARRAY: return "SENSOR_ARRAY";
        case ShipAttachmentType::AUXILIARY_POD: return "AUXILIARY_POD";
        case ShipAttachmentType::RADIATOR: return "RADIATOR";
        case ShipAttachmentType::ARMOR_FIN: return "ARMOR_FIN";
        case ShipAttachmentType::TECHNOLOGY_NODE: return "TECHNOLOGY_NODE";
        default: return "UNKNOWN";
        }
    }

    const char* getSupplementalDetailTypeName(PixelShipGenerator::SupplementalSurfaceDetailType type)
    {
        using PixelShipGenerator::SupplementalSurfaceDetailType;
        switch (type)
        {
        case SupplementalSurfaceDetailType::PANEL_SEAM: return "PANEL_SEAM";
        case SupplementalSurfaceDetailType::GEOMETRIC_MARKING: return "GEOMETRIC_MARKING";
        case SupplementalSurfaceDetailType::MECHANICAL_EXPOSURE: return "MECHANICAL_EXPOSURE";
        case SupplementalSurfaceDetailType::REPEATING_MOTIF: return "REPEATING_MOTIF";
        case SupplementalSurfaceDetailType::IDENTIFICATION_MARKING: return "IDENTIFICATION_MARKING";
        case SupplementalSurfaceDetailType::LUMINOUS_CHANNEL: return "LUMINOUS_CHANNEL";
        default: return "UNKNOWN";
        }
    }

    double percentage(uint64_t numerator, uint64_t denominator)
    {
        return denominator == 0u ? 0.0 : (static_cast<double>(numerator) * 100.0) / static_cast<double>(denominator);
    }

    std::string createModifierCombinationKey(const std::vector<PixelShipGenerator::HullModifierType>& modifiers)
    {
        if (modifiers.empty())
        {
            return "NONE";
        }

        std::ostringstream stream;

        for (std::size_t index = 0u; index < modifiers.size(); ++index)
        {
            if (index > 0u)
            {
                stream << '+';
            }

            stream << getHullModifierName(modifiers[index]);
        }

        return stream.str();
    }

    void hashBytes(uint64_t& hash, const void* data, std::size_t size)
    {
        const auto* bytes = static_cast<const uint8_t*>(data);

        for (std::size_t index = 0u; index < size; ++index)
        {
            hash ^= bytes[index];
            hash *= 1099511628211ull;
        }
    }

    template <typename T>
    void hashValue(uint64_t& hash, const T& value)
    {
        hashBytes(hash, &value, sizeof(T));
    }

    void hashNumericStatistics(uint64_t& hash, const PixelShipGeneratorDiagnostics::NumericStatistics& statistics)
    {
        hashValue(hash, statistics.Count);
        hashValue(hash, statistics.Sum);
        hashValue(hash, statistics.Minimum);
        hashValue(hash, statistics.Maximum);
    }

    void addDebugAttemptStatistics(PixelShipGeneratorDiagnostics::GenerationStatistics& statistics, const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo)
    {
        statistics.HullAttempts.add(static_cast<double>(debugInfo.HullGenerationAttemptCount));
        statistics.HullValidationRejectionCount += debugInfo.HullValidationRejectionCount;
        for (std::size_t index = 0u; index < statistics.SilhouetteValidationFailureCounts.size(); ++index)
        {
            statistics.SilhouetteValidationFailureCounts[index] += debugInfo.SilhouetteValidationFailureCounts[index];
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            statistics.HullModifierAttemptCounts[index] += debugInfo.HullModifierAttemptCounts[index];
            statistics.HullModifierRejectionCounts[index] += debugInfo.HullModifierRejectionCounts[index];
        }
    }
}

namespace PixelShipGeneratorDiagnostics
{
    void NumericStatistics::add(double value)
    {
        if (Count == 0u)
        {
            Minimum = value;
            Maximum = value;
        }
        else
        {
            Minimum = std::min(Minimum, value);
            Maximum = std::max(Maximum, value);
        }

        ++Count;
        Sum += value;
    }

    double NumericStatistics::average() const
    {
        return Count == 0u ? 0.0 : Sum / static_cast<double>(Count);
    }

    void GenerationStatistics::recordSuccess(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo, const DiagnosticGenerationConfiguration& configuration)
    {
        ++RequestedGenerations;
        ++SuccessfulGenerations;
        addDebugAttemptStatistics(*this, debugInfo);

        if (debugInfo.HullGenerationAttemptCount == 1u)
        {
            ++FirstAttemptSuccessCount;
        }

        const MaskMetrics hullMetrics = calculateMaskMetrics(ship.HullMask);
        const uint64_t canvasArea = static_cast<uint64_t>(configuration.Width) * configuration.Height;

        ComplexityInitialBudget.add(static_cast<double>(debugInfo.ComplexityInitialBudget));
        ComplexityConsumedBudget.add(static_cast<double>(debugInfo.ComplexityConsumedBudget));
        ComplexityUnusedBudget.add(static_cast<double>(debugInfo.ComplexityUnusedBudget));
        ComplexityUtilizationPercent.add(debugInfo.ComplexityInitialBudget == 0u ? 0.0 : (static_cast<double>(debugInfo.ComplexityConsumedBudget) * 100.0) / static_cast<double>(debugInfo.ComplexityInitialBudget));
        VisualHierarchyReservedComplexity.add(static_cast<double>(debugInfo.VisualHierarchyReservedComplexity));
        if (debugInfo.VisualHierarchyFallbackOccurred) { ++VisualHierarchyFallbackCount; }
        if (debugInfo.PrimaryVisualAnchor != PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)
        {
            ++PrimaryVisualAnchorCounts[static_cast<std::size_t>(debugInfo.PrimaryVisualAnchor)];
            if (debugInfo.SecondaryVisualAnchor != PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)
            {
                ++SecondaryVisualAnchorCounts[static_cast<std::size_t>(debugInfo.SecondaryVisualAnchor)];
                ++VisualAnchorCombinationFrequencies[std::string(PixelShipGenerator::getShipVisualAnchorTypeName(debugInfo.PrimaryVisualAnchor)) + "+" + PixelShipGenerator::getShipVisualAnchorTypeName(debugInfo.SecondaryVisualAnchor)];
            }
            else
            {
                ++VisualAnchorCombinationFrequencies[std::string(PixelShipGenerator::getShipVisualAnchorTypeName(debugInfo.PrimaryVisualAnchor)) + "+NONE"];
            }
        }
        for (std::size_t index = 0u; index < PixelShipGenerator::GenerationComplexityBudget::CategoryCount; ++index)
        {
            ComplexityCategoryAllocations[index].add(static_cast<double>(debugInfo.ComplexityCategoryAllocations[index]));
            ComplexityCategoryConsumed[index].add(static_cast<double>(debugInfo.ComplexityCategoryConsumed[index]));
        }

        uint32_t spatialUtilizationTotal = 0u;
        uint32_t spatialRegionCount = 0u;
        for (std::size_t index = 0u; index < PixelShipGenerator::GenerationSpatialBudget::RegionCount; ++index)
        {
            const uint32_t capacity = debugInfo.SpatialRegionCapacities[index];
            const uint32_t utilization = capacity == 0u ? 0u : (debugInfo.SpatialRegionLoads[index] * 100u) / capacity;
            SpatialRegionLoadPercent[index].add(static_cast<double>(utilization));
            SpatialRegionDominantCount[index].add(static_cast<double>(debugInfo.SpatialRegionDominantCounts[index]));
            SpatialRegionRejectionCount[index].add(static_cast<double>(debugInfo.SpatialRegionRejections[index]));
            if (capacity != 0u) { spatialUtilizationTotal += utilization; ++spatialRegionCount; }
        }
        SpatialAverageUtilizationPercent.add(spatialRegionCount == 0u ? 0.0 : static_cast<double>(spatialUtilizationTotal) / spatialRegionCount);
        SpatialOverloadRejections.add(static_cast<double>(debugInfo.SpatialOverloadRejectionCount));
        if (debugInfo.MacroAsymmetryPlanned)
        {
            ++MacroAsymmetryPlannedCount;
            if (debugInfo.MacroAsymmetryFulfilled) { ++MacroAsymmetryFulfilledCount; }
            if (debugInfo.MacroAsymmetryRejected) { ++MacroAsymmetryRejectedCount; }
            if (debugInfo.MacroAsymmetryFeatureCategory != PixelShipGenerator::MacroAsymmetryCategory::MACRO_ASYMMETRY_CATEGORY_END) { ++MacroAsymmetryCategoryCounts[static_cast<std::size_t>(debugInfo.MacroAsymmetryFeatureCategory)]; }
            if (debugInfo.MacroAsymmetryDominantSide != PixelShipGenerator::MacroAsymmetrySide::MACRO_ASYMMETRY_SIDE_END) { ++MacroAsymmetrySideCounts[static_cast<std::size_t>(debugInfo.MacroAsymmetryDominantSide)]; }
            if (debugInfo.MacroAsymmetryFulfilled) { MacroAsymmetryBalanceScore.add(static_cast<double>(debugInfo.MacroAsymmetryBalanceScore)); }
        }

        SilhouetteArticulationCount.add(static_cast<double>(debugInfo.SilhouetteMetrics.ArticulationCount));
        SilhouetteShoulderProminencePercent.add(static_cast<double>(debugInfo.SilhouetteMetrics.ShoulderProminencePercent));
        SilhouetteInteriorContractionPercent.add(static_cast<double>(debugInfo.SilhouetteMetrics.InteriorContractionPercent));
        SilhouetteNoseTaperPercent.add(static_cast<double>(debugInfo.SilhouetteMetrics.NoseTaperPercent));
        SilhouetteRearTaperPercent.add(static_cast<double>(debugInfo.SilhouetteMetrics.RearTaperPercent));
        SilhouetteLongestStableRunPercent.add(static_cast<double>(debugInfo.SilhouetteMetrics.LongestStableWidthRunPercent));
        SilhouetteNearMaximumRowPercent.add(static_cast<double>(debugInfo.SilhouetteMetrics.NearMaximumRowPercent));
        SilhouetteTopUnusedMargin.add(static_cast<double>(debugInfo.SilhouetteMetrics.TopUnusedMargin));
        SilhouetteBottomUnusedMargin.add(static_cast<double>(debugInfo.SilhouetteMetrics.BottomUnusedMargin));
        SilhouetteLeftUnusedMargin.add(static_cast<double>(debugInfo.SilhouetteMetrics.LeftUnusedMargin));
        SilhouetteRightUnusedMargin.add(static_cast<double>(debugInfo.SilhouetteMetrics.RightUnusedMargin));
        SilhouetteGuidanceAppliedCount.add(static_cast<double>(debugInfo.SilhouetteGuidanceAppliedCount));
        StructuralNegativeSpaceCount.add(static_cast<double>(debugInfo.StructuralNegativeSpaceCount));
        StructuralNegativeSpacePixelCount.add(static_cast<double>(debugInfo.StructuralNegativeSpacePixelCount));
        for (std::size_t index = 0u; index < StructuralNegativeSpaceTypeCounts.size(); ++index) { StructuralNegativeSpaceTypeCounts[index] += debugInfo.StructuralNegativeSpaceTypeCounts[index]; }
        MaterialZoneCount.add(static_cast<double>(debugInfo.MaterialZoneCount));
        MaterialSecondaryHullPixelCount.add(static_cast<double>(debugInfo.MaterialSecondaryHullPixelCount));
        MaterialMechanicalPixelCount.add(static_cast<double>(debugInfo.MaterialMechanicalPixelCount));
        for (std::size_t index = 0u; index < MaterialZoneTypeCounts.size(); ++index) { MaterialZoneTypeCounts[index] += debugInfo.MaterialZoneTypeCounts[index]; }
        LiveryMarkingCount.add(static_cast<double>(debugInfo.LiveryMarkingCount));
        LiveryPrimaryPixelCount.add(static_cast<double>(debugInfo.LiveryPrimaryPixelCount));
        LiverySecondaryPixelCount.add(static_cast<double>(debugInfo.LiverySecondaryPixelCount));
        for (std::size_t index = 0u; index < LiveryTypeCounts.size(); ++index) { LiveryTypeCounts[index] += debugInfo.LiveryTypeCounts[index]; }
        DetailMotifPrimaryOccurrences.add(static_cast<double>(debugInfo.PrimaryDetailMotifOccurrenceCount));
        DetailMotifSecondaryOccurrences.add(static_cast<double>(debugInfo.SecondaryDetailMotifOccurrenceCount));
        DetailMotifRejectedPlacements.add(static_cast<double>(debugInfo.DetailMotifRejectedPlacementCount));
        if (debugInfo.PrimaryDetailMotif != PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END) { ++PrimaryDetailMotifCounts[static_cast<std::size_t>(debugInfo.PrimaryDetailMotif)]; }
        if (debugInfo.SecondaryDetailMotif != PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END) { ++SecondaryDetailMotifCounts[static_cast<std::size_t>(debugInfo.SecondaryDetailMotif)]; }

        if (hullMetrics.Valid)
        {
            HullOccupiedWidth.add(static_cast<double>(hullMetrics.Width));
            HullOccupiedHeight.add(static_cast<double>(hullMetrics.Height));
            HullBoundingArea.add(static_cast<double>(hullMetrics.BoundingArea));
            HullPixelCount.add(static_cast<double>(hullMetrics.PixelCount));
            HullNormalizedWidth.add(configuration.Width == 0u ? 0.0 : static_cast<double>(hullMetrics.Width) / configuration.Width);
            HullNormalizedHeight.add(configuration.Height == 0u ? 0.0 : static_cast<double>(hullMetrics.Height) / configuration.Height);
            HullCanvasDensity.add(canvasArea == 0u ? 0.0 : static_cast<double>(hullMetrics.PixelCount) / static_cast<double>(canvasArea));
            HullBoundingFillDensity.add(hullMetrics.BoundingArea == 0u ? 0.0 : static_cast<double>(hullMetrics.PixelCount) / hullMetrics.BoundingArea);
        }

        WingMaximumSpan.add(static_cast<double>(debugInfo.WingMaximumSpan));
        WingMaximumExtension.add(static_cast<double>(debugInfo.WingMaximumExtension));
        WingRootThickness.add(static_cast<double>(debugInfo.WingRootThickness));
        WingPixelCount.add(static_cast<double>(debugInfo.WingPixelCount));
        WingRootPixelCount.add(static_cast<double>(debugInfo.WingRootPixelCount));
        OuterWingPixelCount.add(static_cast<double>(debugInfo.OuterWingPixelCount));
        WingStartNormalizedY.add(configuration.Height == 0u ? 0.0 : static_cast<double>(debugInfo.WingStartY) / configuration.Height);
        WingEndNormalizedY.add(configuration.Height == 0u ? 0.0 : static_cast<double>(debugInfo.WingEndY) / configuration.Height);
        if (debugInfo.WingShape != PixelShipGenerator::WingShapeType::WING_SHAPE_TYPE_END) { ++WingShapeCounts[static_cast<std::size_t>(debugInfo.WingShape)]; }

        HullLayerCount.add(static_cast<double>(debugInfo.HullLayerCount));
        HullLayerPixelCount.add(static_cast<double>(debugInfo.HullLayerPixelCount));
        HullLayerPlacementRejections.add(static_cast<double>(debugInfo.HullLayerPlacementRejectionCount));
        for (std::size_t index = 0u; index < HullLayerTypeCounts.size(); ++index) { HullLayerTypeCounts[index] += debugInfo.HullLayerTypeCounts[index]; }
        CoreTreatmentCount.add(static_cast<double>(debugInfo.CoreTreatmentCount));
        CoreRegionPixelCount.add(static_cast<double>(debugInfo.CoreRegionPixelCount));
        CoreRaisedPixelCount.add(static_cast<double>(debugInfo.CoreRaisedPixelCount));
        CoreRecessedPixelCount.add(static_cast<double>(debugInfo.CoreRecessedPixelCount));
        CoreSecondaryMaterialPixelCount.add(static_cast<double>(debugInfo.CoreSecondaryMaterialPixelCount));
        CoreLuminousPixelCount.add(static_cast<double>(debugInfo.CoreLuminousPixelCount));
        CoreTreatmentComplexityCost.add(static_cast<double>(debugInfo.CoreTreatmentComplexityCost));
        CoreTreatmentPlacementRejections.add(static_cast<double>(debugInfo.CoreTreatmentPlacementRejectionCount));
        for (std::size_t index = 0u; index < CoreTreatmentTypeCounts.size(); ++index) { CoreTreatmentTypeCounts[index] += debugInfo.CoreTreatmentTypeCounts[index]; }

        HullModifierCount.add(static_cast<double>(debugInfo.AppliedHullModifiers.size()));
        ++HullModifierCountFrequencies[static_cast<uint32_t>(debugInfo.AppliedHullModifiers.size())];
        ++HullModifierCombinationFrequencies[createModifierCombinationKey(debugInfo.AppliedHullModifiers)];

        for (const PixelShipGenerator::HullModifierType type : debugInfo.AppliedHullModifiers)
        {
            if (type != PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END)
            {
                ++HullModifierOccurrenceCounts[static_cast<std::size_t>(type)];
            }
        }

        CockpitPlacementAttempts.add(static_cast<double>(debugInfo.CockpitPlacementAttemptCount));

        if (debugInfo.CockpitPlacementSucceeded)
        {
            ++CockpitPlacementSuccessCount;
            CockpitPixelCount.add(static_cast<double>(debugInfo.CockpitPixelCount));
            const MaskMetrics cockpitMetrics = calculateMaskMetrics(ship.CockpitMask);
            if (cockpitMetrics.Valid)
            {
                CockpitNormalizedWidth.add(configuration.Width == 0u ? 0.0 : static_cast<double>(cockpitMetrics.Width) / configuration.Width);
                CockpitNormalizedHeight.add(configuration.Height == 0u ? 0.0 : static_cast<double>(cockpitMetrics.Height) / configuration.Height);
            }
            CockpitGlassPixelCount.add(static_cast<double>(debugInfo.CockpitGlassPixelCount));
            CockpitFramePixelCount.add(static_cast<double>(debugInfo.CockpitFramePixelCount));
            CockpitBasePixelCount.add(static_cast<double>(debugInfo.CockpitBasePixelCount));
            CockpitUpperSectionPixelCount.add(static_cast<double>(debugInfo.CockpitUpperSectionPixelCount));
            CockpitComplexityCost.add(static_cast<double>(debugInfo.CockpitComplexityCost));
            if (debugInfo.CockpitSize != PixelShipGenerator::CockpitSizeClass::COCKPIT_SIZE_CLASS_END) { ++CockpitSizeCounts[static_cast<std::size_t>(debugInfo.CockpitSize)]; }
            if (debugInfo.CockpitShape != PixelShipGenerator::CockpitShapeType::COCKPIT_SHAPE_TYPE_END) { ++CockpitShapeCounts[static_cast<std::size_t>(debugInfo.CockpitShape)]; }
        }
        else
        {
            ++CockpitPlacementFailureCount;
        }

        MajorFeatureCount.add(static_cast<double>(debugInfo.MajorFeatureCount));
        MajorFeaturePixelCount.add(static_cast<double>(debugInfo.MajorFeaturePixelCount));
        MajorFeaturePlacementRejections.add(static_cast<double>(debugInfo.MajorFeaturePlacementRejectionCount));
        for (std::size_t index = 0u; index < MajorFeatureTypeCounts.size(); ++index) { MajorFeatureTypeCounts[index] += debugInfo.MajorFeatureTypeCounts[index]; }

        WeaponHardpointCount.add(static_cast<double>(debugInfo.WeaponHardpointCount));
        WeaponCount.add(static_cast<double>(debugInfo.WeaponCount));
        WeaponPixelCount.add(static_cast<double>(debugInfo.WeaponPixelCount));
        WeaponPlacementRejections.add(static_cast<double>(debugInfo.WeaponPlacementRejectionCount));
        for (std::size_t index = 0u; index < WeaponTypeCounts.size(); ++index) { WeaponTypeCounts[index] += debugInfo.WeaponTypeCounts[index]; }
        for (const PixelShipGenerator::WeaponUnitDebugInfo& weaponUnit : debugInfo.WeaponUnits)
        {
            if (weaponUnit.Region != PixelShipGenerator::ShipWeaponHardpointRegion::SHIP_WEAPON_HARDPOINT_REGION_END)
            {
                ++WeaponRegionCounts[static_cast<std::size_t>(weaponUnit.Region)];
            }
        }

        EngineCount.add(static_cast<double>(debugInfo.EngineCount));

        if (debugInfo.EngineCount == 0u)
        {
            ++ZeroEngineCount;
        }

        if (debugInfo.EngineCount > 0u && debugInfo.EngineLayout != PixelShipGenerator::EngineLayoutType::ENGINE_LAYOUT_TYPE_END)
        {
            ++EngineLayoutCounts[static_cast<std::size_t>(debugInfo.EngineLayout)];
        }

        for (const PixelShipGenerator::EngineUnitDebugInfo& engineUnit : debugInfo.EngineUnits)
        {
            EngineHousingWidth.add(static_cast<double>(engineUnit.HousingWidth));
            EngineNozzleWidth.add(static_cast<double>(engineUnit.NozzleWidth));
            EngineExhaustLength.add(static_cast<double>(engineUnit.ExhaustLength));
            if (engineUnit.Nacelle) { ++NacelleEngineCount; }
            if (engineUnit.SizeClass != PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END) { ++EngineSizeCounts[static_cast<std::size_t>(engineUnit.SizeClass)]; }
        }

        const uint64_t attachmentCount = static_cast<uint64_t>(ship.AttachmentPlacements.size());
        AttachmentCount.add(static_cast<double>(attachmentCount));
        AttachmentGroupCount.add(static_cast<double>(debugInfo.AttachmentPlacedGroupCount));
        AttachmentPlacementAttemptCount += debugInfo.AttachmentPlacementAttemptCount;
        AttachmentPlacementFailureCount += debugInfo.AttachmentPlacementFailureCount;
        AttachmentSymmetricPairAttemptCount += debugInfo.AttachmentSymmetricPairAttemptCount;
        AttachmentSymmetricPairFailureCount += debugInfo.AttachmentSymmetricPairFailureCount;

        if (attachmentCount == 0u)
        {
            ++ZeroAttachmentCount;
        }

        for (const PixelShipGenerator::ShipAttachmentPlacement& placement : ship.AttachmentPlacements)
        {
            if (placement.Type != PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END)
            {
                ++AttachmentTypeCounts[static_cast<std::size_t>(placement.Type)];
            }

            if (placement.SymmetryGroup != 0u)
            {
                ++SymmetricAttachmentPlacementCount;
            }
            else
            {
                ++AsymmetricAttachmentPlacementCount;
            }
        }

        AccentPatternCount.add(static_cast<double>(debugInfo.AccentPatternCount));
        MechanicalPatternCount.add(static_cast<double>(debugInfo.MechanicalPatternCount));
        LightPatternCount.add(static_cast<double>(debugInfo.LightPatternCount));

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END); ++index)
        {
            SupplementalSurfaceDetailCounts[index] += debugInfo.SupplementalSurfaceDetailCounts[index];
        }

        const uint64_t detailPixels = static_cast<uint64_t>(countMaskPixels(ship.AccentMask)) + countMaskPixels(ship.MechanicalDetailMask) + countMaskPixels(ship.LightMask);
        DetailMaskPixelCount.add(static_cast<double>(detailPixels));
        DetailMaskCanvasDensity.add(canvasArea == 0u ? 0.0 : static_cast<double>(detailPixels) / static_cast<double>(canvasArea));
    }

    void GenerationStatistics::recordFailure(const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo)
    {
        ++RequestedGenerations;
        ++FailedGenerations;
        addDebugAttemptStatistics(*this, debugInfo);
    }

    uint64_t GenerationStatistics::deterministicSignature() const
    {
        uint64_t hash = 1469598103934665603ull;
        hashValue(hash, RequestedGenerations);
        hashValue(hash, SuccessfulGenerations);
        hashValue(hash, FailedGenerations);
        hashValue(hash, FirstAttemptSuccessCount);
        hashValue(hash, HullValidationRejectionCount);
        hashNumericStatistics(hash, HullAttempts);
        hashNumericStatistics(hash, HullOccupiedWidth);
        hashNumericStatistics(hash, HullOccupiedHeight);
        hashNumericStatistics(hash, HullBoundingArea);
        hashNumericStatistics(hash, HullPixelCount);
        hashNumericStatistics(hash, HullNormalizedWidth);
        hashNumericStatistics(hash, HullNormalizedHeight);
        hashNumericStatistics(hash, HullCanvasDensity);
        hashNumericStatistics(hash, HullBoundingFillDensity);
        hashNumericStatistics(hash, SilhouetteArticulationCount);
        hashNumericStatistics(hash, SilhouetteShoulderProminencePercent);
        hashNumericStatistics(hash, SilhouetteInteriorContractionPercent);
        hashNumericStatistics(hash, SilhouetteNoseTaperPercent);
        hashNumericStatistics(hash, SilhouetteRearTaperPercent);
        hashNumericStatistics(hash, SilhouetteLongestStableRunPercent);
        hashNumericStatistics(hash, SilhouetteNearMaximumRowPercent);
        hashNumericStatistics(hash, SilhouetteTopUnusedMargin);
        hashNumericStatistics(hash, SilhouetteBottomUnusedMargin);
        hashNumericStatistics(hash, SilhouetteLeftUnusedMargin);
        hashNumericStatistics(hash, SilhouetteRightUnusedMargin);
        hashNumericStatistics(hash, SilhouetteGuidanceAppliedCount);
        hashNumericStatistics(hash, StructuralNegativeSpaceCount);
        hashNumericStatistics(hash, StructuralNegativeSpacePixelCount);
        hashBytes(hash, StructuralNegativeSpaceTypeCounts.data(), sizeof(StructuralNegativeSpaceTypeCounts));
        hashNumericStatistics(hash, MaterialZoneCount);
        hashNumericStatistics(hash, MaterialSecondaryHullPixelCount);
        hashNumericStatistics(hash, MaterialMechanicalPixelCount);
        hashBytes(hash, MaterialZoneTypeCounts.data(), sizeof(MaterialZoneTypeCounts));
        hashNumericStatistics(hash, LiveryMarkingCount);
        hashNumericStatistics(hash, LiveryPrimaryPixelCount);
        hashNumericStatistics(hash, LiverySecondaryPixelCount);
        hashBytes(hash, LiveryTypeCounts.data(), sizeof(LiveryTypeCounts));
        hashNumericStatistics(hash, DetailMotifPrimaryOccurrences);
        hashNumericStatistics(hash, DetailMotifSecondaryOccurrences);
        hashNumericStatistics(hash, DetailMotifRejectedPlacements);
        hashBytes(hash, PrimaryDetailMotifCounts.data(), sizeof(PrimaryDetailMotifCounts));
        hashBytes(hash, SecondaryDetailMotifCounts.data(), sizeof(SecondaryDetailMotifCounts));
        hashBytes(hash, SilhouetteValidationFailureCounts.data(), sizeof(SilhouetteValidationFailureCounts));
        hashNumericStatistics(hash, WingMaximumSpan);
        hashNumericStatistics(hash, WingMaximumExtension);
        hashNumericStatistics(hash, WingRootThickness);
        hashNumericStatistics(hash, WingPixelCount);
        hashNumericStatistics(hash, WingRootPixelCount);
        hashNumericStatistics(hash, OuterWingPixelCount);
        hashNumericStatistics(hash, WingStartNormalizedY);
        hashNumericStatistics(hash, WingEndNormalizedY);
        hashBytes(hash, WingShapeCounts.data(), sizeof(WingShapeCounts));
        hashNumericStatistics(hash, ComplexityInitialBudget);
        hashNumericStatistics(hash, ComplexityConsumedBudget);
        hashNumericStatistics(hash, ComplexityUnusedBudget);
        hashNumericStatistics(hash, ComplexityUtilizationPercent);
        hashNumericStatistics(hash, VisualHierarchyReservedComplexity);
        hashValue(hash, VisualHierarchyFallbackCount);
        hashBytes(hash, PrimaryVisualAnchorCounts.data(), sizeof(PrimaryVisualAnchorCounts));
        hashBytes(hash, SecondaryVisualAnchorCounts.data(), sizeof(SecondaryVisualAnchorCounts));
        for (const auto& entry : VisualAnchorCombinationFrequencies) { hashBytes(hash, entry.first.data(), entry.first.size()); hashValue(hash, entry.second); }
        for (const NumericStatistics& category : ComplexityCategoryAllocations) { hashNumericStatistics(hash, category); }
        for (const NumericStatistics& category : ComplexityCategoryConsumed) { hashNumericStatistics(hash, category); }
        hashNumericStatistics(hash, SpatialAverageUtilizationPercent);
        hashNumericStatistics(hash, SpatialOverloadRejections);
        hashValue(hash, MacroAsymmetryPlannedCount);
        hashValue(hash, MacroAsymmetryFulfilledCount);
        hashValue(hash, MacroAsymmetryRejectedCount);
        hashNumericStatistics(hash, MacroAsymmetryBalanceScore);
        hashBytes(hash, MacroAsymmetryCategoryCounts.data(), sizeof(MacroAsymmetryCategoryCounts));
        hashBytes(hash, MacroAsymmetrySideCounts.data(), sizeof(MacroAsymmetrySideCounts));
        for (const NumericStatistics& region : SpatialRegionLoadPercent) { hashNumericStatistics(hash, region); }
        for (const NumericStatistics& region : SpatialRegionDominantCount) { hashNumericStatistics(hash, region); }
        for (const NumericStatistics& region : SpatialRegionRejectionCount) { hashNumericStatistics(hash, region); }
        hashNumericStatistics(hash, HullLayerCount);
        hashNumericStatistics(hash, HullLayerPixelCount);
        hashNumericStatistics(hash, HullLayerPlacementRejections);
        for (uint64_t value : HullLayerTypeCounts) { hashValue(hash, value); }
        hashNumericStatistics(hash, CoreTreatmentCount);
        hashNumericStatistics(hash, CoreRegionPixelCount);
        hashNumericStatistics(hash, CoreRaisedPixelCount);
        hashNumericStatistics(hash, CoreRecessedPixelCount);
        hashNumericStatistics(hash, CoreSecondaryMaterialPixelCount);
        hashNumericStatistics(hash, CoreLuminousPixelCount);
        hashNumericStatistics(hash, CoreTreatmentComplexityCost);
        hashNumericStatistics(hash, CoreTreatmentPlacementRejections);
        hashBytes(hash, CoreTreatmentTypeCounts.data(), sizeof(CoreTreatmentTypeCounts));
        hashNumericStatistics(hash, HullModifierCount);
        hashBytes(hash, HullModifierOccurrenceCounts.data(), sizeof(HullModifierOccurrenceCounts));
        hashBytes(hash, HullModifierAttemptCounts.data(), sizeof(HullModifierAttemptCounts));
        hashBytes(hash, HullModifierRejectionCounts.data(), sizeof(HullModifierRejectionCounts));

        for (const auto& entry : HullModifierCountFrequencies)
        {
            hashValue(hash, entry.first);
            hashValue(hash, entry.second);
        }

        for (const auto& entry : HullModifierCombinationFrequencies)
        {
            hashBytes(hash, entry.first.data(), entry.first.size());
            hashValue(hash, entry.second);
        }

        hashNumericStatistics(hash, CockpitPlacementAttempts);
        hashValue(hash, CockpitPlacementSuccessCount);
        hashValue(hash, CockpitPlacementFailureCount);
        hashNumericStatistics(hash, CockpitPixelCount);
        hashNumericStatistics(hash, CockpitNormalizedWidth);
        hashNumericStatistics(hash, CockpitNormalizedHeight);
        hashNumericStatistics(hash, CockpitGlassPixelCount);
        hashNumericStatistics(hash, CockpitFramePixelCount);
        hashNumericStatistics(hash, CockpitBasePixelCount);
        hashNumericStatistics(hash, CockpitUpperSectionPixelCount);
        hashNumericStatistics(hash, CockpitComplexityCost);
        hashBytes(hash, CockpitSizeCounts.data(), sizeof(CockpitSizeCounts));
        hashBytes(hash, CockpitShapeCounts.data(), sizeof(CockpitShapeCounts));
        hashNumericStatistics(hash, MajorFeatureCount);
        hashNumericStatistics(hash, MajorFeaturePixelCount);
        hashNumericStatistics(hash, MajorFeaturePlacementRejections);
        hashBytes(hash, MajorFeatureTypeCounts.data(), sizeof(MajorFeatureTypeCounts));
        hashNumericStatistics(hash, WeaponHardpointCount);
        hashNumericStatistics(hash, WeaponCount);
        hashNumericStatistics(hash, WeaponPixelCount);
        hashNumericStatistics(hash, WeaponPlacementRejections);
        hashBytes(hash, WeaponTypeCounts.data(), sizeof(WeaponTypeCounts));
        hashBytes(hash, WeaponRegionCounts.data(), sizeof(WeaponRegionCounts));
        hashNumericStatistics(hash, EngineCount);
        hashValue(hash, ZeroEngineCount);
        hashNumericStatistics(hash, EngineHousingWidth);
        hashNumericStatistics(hash, EngineNozzleWidth);
        hashNumericStatistics(hash, EngineExhaustLength);
        hashValue(hash, NacelleEngineCount);
        hashBytes(hash, EngineLayoutCounts.data(), EngineLayoutCounts.size() * sizeof(uint64_t));
        hashBytes(hash, EngineSizeCounts.data(), EngineSizeCounts.size() * sizeof(uint64_t));
        hashNumericStatistics(hash, AttachmentCount);
        hashNumericStatistics(hash, AttachmentGroupCount);
        hashValue(hash, ZeroAttachmentCount);
        hashValue(hash, AttachmentPlacementAttemptCount);
        hashValue(hash, AttachmentPlacementFailureCount);
        hashValue(hash, AttachmentSymmetricPairAttemptCount);
        hashValue(hash, AttachmentSymmetricPairFailureCount);
        hashValue(hash, SymmetricAttachmentPlacementCount);
        hashValue(hash, AsymmetricAttachmentPlacementCount);
        hashBytes(hash, AttachmentTypeCounts.data(), sizeof(AttachmentTypeCounts));
        hashNumericStatistics(hash, AccentPatternCount);
        hashNumericStatistics(hash, MechanicalPatternCount);
        hashNumericStatistics(hash, LightPatternCount);
        hashNumericStatistics(hash, DetailMaskPixelCount);
        hashNumericStatistics(hash, DetailMaskCanvasDensity);
        hashBytes(hash, SupplementalSurfaceDetailCounts.data(), sizeof(SupplementalSurfaceDetailCounts));
        return hash;
    }

    uint64_t deriveDiagnosticSampleSeed(uint64_t diagnosticSeed, uint64_t sampleIndex)
    {
        const uint64_t indexValue = sampleIndex * 0x9E3779B97F4A7C15ull;
        return PixelShipGenerator::mixGenerationSeed64(diagnosticSeed ^ 0xD1B54A32D192ED03ull ^ indexValue);
    }

    GenerationStatistics collectGenerationStatistics(const DiagnosticGenerationConfiguration& configuration)
    {
        DiagnosticsRunConfiguration runConfiguration;
        runConfiguration.Dimensions = { { configuration.Width, configuration.Height } };
        runConfiguration.Styles = { configuration.Style };
        runConfiguration.Factions = { configuration.Faction };
        runConfiguration.SamplesPerConfiguration = configuration.Samples;
        runConfiguration.DiagnosticSeed = configuration.DiagnosticSeed;
        runConfiguration.DetailDensity = configuration.DetailDensity;
        runConfiguration.AsymmetricDetailChance = configuration.AsymmetricDetailChance;
        runConfiguration.AttachmentsEnabled = configuration.AttachmentsEnabled;
        runConfiguration.DetailLevel = DiagnosticsDetailLevel::SUMMARY_ONLY;
        const DiagnosticsResult result = DiagnosticsRunner().run(runConfiguration);
        return result.ConfigurationResults.empty() ? GenerationStatistics() : result.ConfigurationResults.front().Statistics;
    }

    void printGenerationStatistics(std::ostream& output, const DiagnosticGenerationConfiguration& configuration, const GenerationStatistics& statistics)
    {
        output << std::fixed << std::setprecision(3);
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ configuration.Width, configuration.Height });
        output << "\n## Configuration\n";
        output << "Width: " << configuration.Width << '\n';
        output << "Height: " << configuration.Height << '\n';
        output << "Aspect ratio: " << scaleTraits.AspectRatio << '\n';
        output << "Scale tier: " << PixelShipGenerator::getGenerationScaleTierName(scaleTraits.Tier) << '\n';
        output << "Horizontal capacity: " << scaleTraits.HorizontalCapacity << "%\n";
        output << "Longitudinal capacity: " << scaleTraits.LongitudinalCapacity << "%\n";
        output << "Major-feature capacity: " << scaleTraits.MajorFeatureCapacity << "%\n";
        output << "Detail complexity: " << scaleTraits.DetailComplexity << "%\n";
        output << "Shading complexity: " << scaleTraits.ShadingComplexity << "%\n";
        output << "Attachment complexity: " << scaleTraits.AttachmentComplexity << "%\n";
        output << "Animation complexity: " << scaleTraits.AnimationComplexity << "%\n";
        output << "Style: " << styleName(configuration.Style) << '\n';
        output << "Faction: " << factionName(configuration.Faction) << '\n';
        output << "Attachments: " << (configuration.AttachmentsEnabled ? "ON" : "OFF") << '\n';
        output << "Detail density: " << configuration.DetailDensity << '\n';
        output << "Asymmetric detail chance: " << configuration.AsymmetricDetailChance << '\n';
        output << "Samples: " << configuration.Samples << '\n';
        output << "Diagnostic seed: " << configuration.DiagnosticSeed << '\n';

        output << "\n## Generation\n";
        output << "Requested: " << statistics.RequestedGenerations << '\n';
        output << "Successful: " << statistics.SuccessfulGenerations << '\n';
        output << "Failed: " << statistics.FailedGenerations << " (" << percentage(statistics.FailedGenerations, statistics.RequestedGenerations) << "%)\n";
        output << "Average hull attempts: " << statistics.HullAttempts.average() << '\n';
        output << "Hull attempts min/max: " << statistics.HullAttempts.Minimum << " / " << statistics.HullAttempts.Maximum << '\n';
        output << "First-attempt success: " << percentage(statistics.FirstAttemptSuccessCount, statistics.RequestedGenerations) << "%\n";
        output << "Hull validation rejections: " << statistics.HullValidationRejectionCount << '\n';

        output << "\n## Macro Asymmetry\n";
        output << "Planned: " << statistics.MacroAsymmetryPlannedCount << " (" << percentage(statistics.MacroAsymmetryPlannedCount, statistics.SuccessfulGenerations) << "%)\n";
        output << "Fulfilled: " << statistics.MacroAsymmetryFulfilledCount << " (" << percentage(statistics.MacroAsymmetryFulfilledCount, statistics.MacroAsymmetryPlannedCount) << "% of plans)\n";
        output << "Rejected/fallback: " << statistics.MacroAsymmetryRejectedCount << '\n';
        output << "Average balance score: " << statistics.MacroAsymmetryBalanceScore.average() << "%\n";

        output << "\n## Visual Hierarchy\n";
        output << "Average reserved complexity: " << statistics.VisualHierarchyReservedComplexity.average() << '\n';
        output << "Fallback rate: " << percentage(statistics.VisualHierarchyFallbackCount, statistics.SuccessfulGenerations) << "%\n";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END); ++index)
        {
            const auto anchor = static_cast<PixelShipGenerator::ShipVisualAnchorType>(index);
            output << "  Primary " << PixelShipGenerator::getShipVisualAnchorTypeName(anchor) << ": " << percentage(statistics.PrimaryVisualAnchorCounts[index], statistics.SuccessfulGenerations) << "%";
            output << ", secondary " << percentage(statistics.SecondaryVisualAnchorCounts[index], statistics.SuccessfulGenerations) << "%\n";
        }
        for (const auto& entry : statistics.VisualAnchorCombinationFrequencies)
        {
            output << "  Pair " << entry.first << ": " << percentage(entry.second, statistics.SuccessfulGenerations) << "%\n";
        }

        output << "\n## Material Composition\n";
        output << "Average material zones: " << statistics.MaterialZoneCount.average() << '\n';
        output << "Average secondary-structure pixels: " << statistics.MaterialSecondaryHullPixelCount.average() << '\n';
        output << "Average mechanical-zone pixels: " << statistics.MaterialMechanicalPixelCount.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END); ++index)
        {
            output << "  Zone " << PixelShipGenerator::getShipMaterialZoneTypeName(static_cast<PixelShipGenerator::ShipMaterialZoneType>(index)) << ": " << percentage(statistics.MaterialZoneTypeCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
        }

        output << "\n## Procedural Livery\n";
        output << "Average livery markings: " << statistics.LiveryMarkingCount.average() << '\n';
        output << "Average primary-marking pixels: " << statistics.LiveryPrimaryPixelCount.average() << '\n';
        output << "Average supporting-marking pixels: " << statistics.LiverySecondaryPixelCount.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipLiveryType::SHIP_LIVERY_TYPE_END); ++index)
        {
            output << "  Marking " << PixelShipGenerator::getShipLiveryTypeName(static_cast<PixelShipGenerator::ShipLiveryType>(index)) << ": " << percentage(statistics.LiveryTypeCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
        }

        output << "\n## Detail Motif Grammar\n";
        output << "Average primary motif occurrences: " << statistics.DetailMotifPrimaryOccurrences.average() << '\n';
        output << "Average secondary motif occurrences: " << statistics.DetailMotifSecondaryOccurrences.average() << '\n';
        const double motifAttempts = statistics.DetailMotifPrimaryOccurrences.Sum + statistics.DetailMotifSecondaryOccurrences.Sum + statistics.DetailMotifRejectedPlacements.Sum;
        output << "Motif placement rejection rate: " << (motifAttempts <= 0.0 ? 0.0 : statistics.DetailMotifRejectedPlacements.Sum * 100.0 / motifAttempts) << "%\n";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::ShipDetailMotifType>(index);
            output << "  Primary " << PixelShipGenerator::getShipDetailMotifTypeName(type) << ": " << percentage(statistics.PrimaryDetailMotifCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
            output << "  Secondary " << PixelShipGenerator::getShipDetailMotifTypeName(type) << ": " << percentage(statistics.SecondaryDetailMotifCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
        }

        output << "\n## Complexity Budget\n";
        output << "Average initial budget: " << statistics.ComplexityInitialBudget.average() << '\n';
        output << "Average consumed budget: " << statistics.ComplexityConsumedBudget.average() << '\n';
        output << "Average unused budget: " << statistics.ComplexityUnusedBudget.average() << '\n';
        output << "Average utilization: " << statistics.ComplexityUtilizationPercent.average() << "%\n";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END); ++index)
        {
            const auto category = static_cast<PixelShipGenerator::GenerationComplexityCategory>(index);
            output << std::left << std::setw(18) << PixelShipGenerator::getGenerationComplexityCategoryName(category) << std::right
                << " allocation " << statistics.ComplexityCategoryAllocations[index].average()
                << ", consumed " << statistics.ComplexityCategoryConsumed[index].average() << '\n';
        }

        output << "\n## Semantic Spatial Load\n";
        output << "Average regional utilization: " << statistics.SpatialAverageUtilizationPercent.average() << "%\n";
        output << "Average spatial rejections: " << statistics.SpatialOverloadRejections.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            const auto region = static_cast<PixelShipGenerator::GenerationSpatialRegion>(index);
            output << std::left << std::setw(20) << PixelShipGenerator::getGenerationSpatialRegionName(region) << std::right
                << " load " << statistics.SpatialRegionLoadPercent[index].average() << "%"
                << ", dominant " << statistics.SpatialRegionDominantCount[index].average()
                << ", rejects " << statistics.SpatialRegionRejectionCount[index].average() << '\n';
        }

        output << "\n## Hull\n";
        output << "Average occupied width: " << statistics.HullOccupiedWidth.average() << " px (" << statistics.HullNormalizedWidth.average() * 100.0 << "% of canvas width)\n";
        output << "Average occupied height: " << statistics.HullOccupiedHeight.average() << " px (" << statistics.HullNormalizedHeight.average() * 100.0 << "% of canvas height)\n";
        output << "Average hull pixels: " << statistics.HullPixelCount.average() << '\n';
        output << "Hull pixels min/max: " << statistics.HullPixelCount.Minimum << " / " << statistics.HullPixelCount.Maximum << '\n';
        output << "Average canvas hull density: " << statistics.HullCanvasDensity.average() * 100.0 << "%\n";
        output << "Average hull-bounds fill density: " << statistics.HullBoundingFillDensity.average() * 100.0 << "%\n";
        output << "Average articulation events: " << statistics.SilhouetteArticulationCount.average() << '\n';
        output << "Average shoulder prominence: " << statistics.SilhouetteShoulderProminencePercent.average() << "%\n";
        output << "Average interior contraction: " << statistics.SilhouetteInteriorContractionPercent.average() << "%\n";
        output << "Average nose/rear taper: " << statistics.SilhouetteNoseTaperPercent.average() << "% / " << statistics.SilhouetteRearTaperPercent.average() << "%\n";
        output << "Average longest stable width run: " << statistics.SilhouetteLongestStableRunPercent.average() << "% of occupied height\n";
        output << "Average near-maximum rows: " << statistics.SilhouetteNearMaximumRowPercent.average() << "% of occupied height\n";
        output << "Average unused margins T/B/L/R: " << statistics.SilhouetteTopUnusedMargin.average() << " / " << statistics.SilhouetteBottomUnusedMargin.average() << " / " << statistics.SilhouetteLeftUnusedMargin.average() << " / " << statistics.SilhouetteRightUnusedMargin.average() << " px\n";
        output << "Average silhouette guidance applications: " << statistics.SilhouetteGuidanceAppliedCount.average() << '\n';
        output << "Average structural negative-space features: " << statistics.StructuralNegativeSpaceCount.average() << '\n';
        output << "Average reserved negative-space pixels: " << statistics.StructuralNegativeSpacePixelCount.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END); ++index)
        {
            output << "  Void " << PixelShipGenerator::getShipStructuralNegativeSpaceTypeName(static_cast<PixelShipGenerator::ShipStructuralNegativeSpaceType>(index)) << ": " << percentage(statistics.StructuralNegativeSpaceTypeCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
        }
        for (uint32_t index = 1u; index < static_cast<uint32_t>(PixelShipGenerator::SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END); ++index)
        {
            const auto reason = static_cast<PixelShipGenerator::SilhouetteValidationFailureReason>(index);
            output << "  Retry " << PixelShipGenerator::getSilhouetteValidationFailureReasonName(reason) << ": " << statistics.SilhouetteValidationFailureCounts[index] << '\n';
        }

        output << "\n## Wings\n";
        output << "Average maximum span: " << statistics.WingMaximumSpan.average() << " px\n";
        output << "Average maximum extension: " << statistics.WingMaximumExtension.average() << " px\n";
        output << "Average root thickness: " << statistics.WingRootThickness.average() << " px\n";
        output << "Average wing/root/outer pixels: " << statistics.WingPixelCount.average() << " / " << statistics.WingRootPixelCount.average() << " / " << statistics.OuterWingPixelCount.average() << '\n';
        output << "Average longitudinal start/end: " << statistics.WingStartNormalizedY.average() * 100.0 << "% / " << statistics.WingEndNormalizedY.average() * 100.0 << "% of canvas height\n";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::WingShapeType::WING_SHAPE_TYPE_END); ++index)
        {
            output << "  Wing shape " << getWingShapeName(static_cast<PixelShipGenerator::WingShapeType>(index)) << ": " << percentage(statistics.WingShapeCounts[index], statistics.SuccessfulGenerations) << "%\n";
        }

        output << "\n## Hull Layers\n";
        output << "Average layers per ship: " << statistics.HullLayerCount.average() << '\n';
        output << "Average layer pixels: " << statistics.HullLayerPixelCount.average() << '\n';
        output << "Average layer placement rejections: " << statistics.HullLayerPlacementRejections.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::ShipHullLayerType>(index);
            output << std::left << std::setw(25) << getHullLayerName(type) << std::right << statistics.HullLayerTypeCounts[index] << " (" << percentage(statistics.HullLayerTypeCounts[index], statistics.SuccessfulGenerations) << "% of ships)\n";
        }

        output << "\n## Central Core Treatment\n";
        output << "Average treatments per ship: " << statistics.CoreTreatmentCount.average() << '\n';
        output << "Average core-region pixels: " << statistics.CoreRegionPixelCount.average() << '\n';
        output << "Average raised/recessed pixels: " << statistics.CoreRaisedPixelCount.average() << " / " << statistics.CoreRecessedPixelCount.average() << '\n';
        output << "Average material/luminous pixels: " << statistics.CoreSecondaryMaterialPixelCount.average() << " / " << statistics.CoreLuminousPixelCount.average() << '\n';
        output << "Average core complexity cost: " << statistics.CoreTreatmentComplexityCost.average() << '\n';
        output << "Average core placement rejections: " << statistics.CoreTreatmentPlacementRejections.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::ShipCoreTreatmentType>(index);
            output << "  " << PixelShipGenerator::getShipCoreTreatmentTypeName(type) << ": " << percentage(statistics.CoreTreatmentTypeCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
        }

        output << "\n## Advanced Silhouette Modifiers\n";
        output << "Average accepted modifiers per ship: " << statistics.HullModifierCount.average() << '\n';
        const auto zeroModifierIterator = statistics.HullModifierCountFrequencies.find(0u);
        const uint64_t zeroModifierCount = zeroModifierIterator == statistics.HullModifierCountFrequencies.end() ? 0u : zeroModifierIterator->second;
        output << "Ships with zero modifiers: " << percentage(zeroModifierCount, statistics.SuccessfulGenerations) << "%\n";

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::HullModifierType>(index);
            output << std::left << std::setw(25) << getHullModifierName(type) << std::right << " accepted " << percentage(statistics.HullModifierOccurrenceCounts[index], statistics.SuccessfulGenerations) << "% | attempts " << statistics.HullModifierAttemptCounts[index] << " | rejected " << statistics.HullModifierRejectionCounts[index] << " (" << percentage(statistics.HullModifierRejectionCounts[index], statistics.HullModifierAttemptCounts[index]) << "%)\n";
        }

        std::vector<std::pair<std::string, uint64_t>> combinations(statistics.HullModifierCombinationFrequencies.begin(), statistics.HullModifierCombinationFrequencies.end());
        std::sort(combinations.begin(), combinations.end(), [](const auto& first, const auto& second) { return first.second > second.second || (first.second == second.second && first.first < second.first); });
        output << "Top modifier combinations:\n";

        for (std::size_t index = 0u; index < std::min<std::size_t>(5u, combinations.size()); ++index)
        {
            output << "  " << combinations[index].first << ": " << combinations[index].second << " (" << percentage(combinations[index].second, statistics.SuccessfulGenerations) << "%)\n";
        }

        output << "\n## Cockpit / Engines\n";
        output << "Average cockpit placement attempts: " << statistics.CockpitPlacementAttempts.average() << '\n';
        output << "Cockpit placement failures: " << statistics.CockpitPlacementFailureCount << " (" << percentage(statistics.CockpitPlacementFailureCount, statistics.SuccessfulGenerations) << "%)\n";
        output << "Average cockpit pixels: " << statistics.CockpitPixelCount.average() << " (glass " << statistics.CockpitGlassPixelCount.average() << ", frame " << statistics.CockpitFramePixelCount.average() << ", base " << statistics.CockpitBasePixelCount.average() << ")\n";
        output << "Average cockpit width/height: " << statistics.CockpitNormalizedWidth.average() * 100.0 << "% / " << statistics.CockpitNormalizedHeight.average() * 100.0 << "% of canvas\n";
        output << "Average cockpit upper-stage pixels: " << statistics.CockpitUpperSectionPixelCount.average() << '\n';
        output << "Average cockpit complexity cost: " << statistics.CockpitComplexityCost.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::CockpitSizeClass::COCKPIT_SIZE_CLASS_END); ++index)
        {
            output << "  Cockpit size " << PixelShipGenerator::getCockpitSizeClassName(static_cast<PixelShipGenerator::CockpitSizeClass>(index)) << ": " << percentage(statistics.CockpitSizeCounts[index], statistics.CockpitPlacementSuccessCount) << "%\n";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::CockpitShapeType::COCKPIT_SHAPE_TYPE_END); ++index)
        {
            output << "  Cockpit shape " << PixelShipGenerator::getCockpitShapeTypeName(static_cast<PixelShipGenerator::CockpitShapeType>(index)) << ": " << percentage(statistics.CockpitShapeCounts[index], statistics.CockpitPlacementSuccessCount) << "%\n";
        }
        output << "\n## Major Features / Weapons\n";
        output << "Average major features/pixels/rejections: " << statistics.MajorFeatureCount.average() << " / " << statistics.MajorFeaturePixelCount.average() << " / " << statistics.MajorFeaturePlacementRejections.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END); ++index)
        {
            output << "  Major feature " << getMajorFeatureName(static_cast<PixelShipGenerator::ShipMajorFeatureType>(index)) << ": " << percentage(statistics.MajorFeatureTypeCounts[index], statistics.SuccessfulGenerations) << "% of ships\n";
        }
        const uint64_t totalWeaponUnits = std::accumulate(statistics.WeaponTypeCounts.begin(), statistics.WeaponTypeCounts.end(), uint64_t(0u));
        output << "Average weapon hardpoints/weapons/pixels/rejections: " << statistics.WeaponHardpointCount.average() << " / " << statistics.WeaponCount.average() << " / " << statistics.WeaponPixelCount.average() << " / " << statistics.WeaponPlacementRejections.average() << '\n';
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponType::SHIP_WEAPON_TYPE_END); ++index)
        {
            output << "  Weapon type " << getWeaponTypeName(static_cast<PixelShipGenerator::ShipWeaponType>(index)) << ": " << percentage(statistics.WeaponTypeCounts[index], totalWeaponUnits) << "% of weapon units\n";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponHardpointRegion::SHIP_WEAPON_HARDPOINT_REGION_END); ++index)
        {
            output << "  Weapon region " << getWeaponRegionName(static_cast<PixelShipGenerator::ShipWeaponHardpointRegion>(index)) << ": " << percentage(statistics.WeaponRegionCounts[index], totalWeaponUnits) << "% of weapon units\n";
        }

        output << "Average engine count: " << statistics.EngineCount.average() << '\n';
        output << "Ships with zero engines: " << percentage(statistics.ZeroEngineCount, statistics.SuccessfulGenerations) << "%\n";
        output << "Average engine housing width: " << statistics.EngineHousingWidth.average() << '\n';
        output << "Average nozzle width: " << statistics.EngineNozzleWidth.average() << '\n';
        output << "Average exhaust length: " << statistics.EngineExhaustLength.average() << '\n';
        const uint64_t totalEngineUnits = std::accumulate(statistics.EngineSizeCounts.begin(), statistics.EngineSizeCounts.end(), uint64_t(0u));
        output << "Nacelle-style engine units: " << percentage(statistics.NacelleEngineCount, totalEngineUnits) << "%\n";

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::EngineLayoutType::ENGINE_LAYOUT_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::EngineLayoutType>(index);
            output << std::left << std::setw(25) << getEngineLayoutName(type) << std::right << statistics.EngineLayoutCounts[index] << " (" << percentage(statistics.EngineLayoutCounts[index], statistics.SuccessfulGenerations) << "% of ships)\n";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::EngineSizeClass>(index);
            output << std::left << std::setw(25) << getEngineSizeName(type) << std::right << statistics.EngineSizeCounts[index] << " (" << percentage(statistics.EngineSizeCounts[index], totalEngineUnits) << "% of engine units)\n";
        }

        output << "\n## Attachments\n";
        output << "Average attachment placements: " << statistics.AttachmentCount.average() << '\n';
        output << "Average attachment groups: " << statistics.AttachmentGroupCount.average() << '\n';
        output << "Ships with zero attachments: " << percentage(statistics.ZeroAttachmentCount, statistics.SuccessfulGenerations) << "%\n";
        output << "Placement attempts: " << statistics.AttachmentPlacementAttemptCount << '\n';
        output << "Placement failures: " << statistics.AttachmentPlacementFailureCount << " (" << percentage(statistics.AttachmentPlacementFailureCount, statistics.AttachmentPlacementAttemptCount) << "% of attempts)\n";
        output << "Symmetric-pair attempts: " << statistics.AttachmentSymmetricPairAttemptCount << '\n';
        output << "Symmetric-pair fallbacks: " << statistics.AttachmentSymmetricPairFailureCount << " (" << percentage(statistics.AttachmentSymmetricPairFailureCount, statistics.AttachmentSymmetricPairAttemptCount) << "% of pair attempts)\n";
        const uint64_t totalAttachmentPlacements = statistics.SymmetricAttachmentPlacementCount + statistics.AsymmetricAttachmentPlacementCount;
        output << "Symmetric attachment placements: " << percentage(statistics.SymmetricAttachmentPlacementCount, totalAttachmentPlacements) << "%\n";

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::ShipAttachmentType>(index);
            output << std::left << std::setw(20) << getAttachmentTypeName(type) << std::right << statistics.AttachmentTypeCounts[index] << " (" << percentage(statistics.AttachmentTypeCounts[index], totalAttachmentPlacements) << "% of placements)\n";
        }

        output << "\n## Surface Details\n";
        output << "Average accent patterns: " << statistics.AccentPatternCount.average() << '\n';
        output << "Average mechanical patterns: " << statistics.MechanicalPatternCount.average() << '\n';
        output << "Average light patterns: " << statistics.LightPatternCount.average() << '\n';
        output << "Average detail-mask pixels: " << statistics.DetailMaskPixelCount.average() << '\n';
        output << "Average detail-mask canvas density: " << statistics.DetailMaskCanvasDensity.average() * 100.0 << "%\n";
        const uint64_t totalSupplementalDetails = std::accumulate(statistics.SupplementalSurfaceDetailCounts.begin(), statistics.SupplementalSurfaceDetailCounts.end(), uint64_t(0u));

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::SupplementalSurfaceDetailType>(index);
            output << std::left << std::setw(22) << getSupplementalDetailTypeName(type) << std::right << statistics.SupplementalSurfaceDetailCounts[index] << " (" << percentage(statistics.SupplementalSurfaceDetailCounts[index], totalSupplementalDetails) << "% of supplemental details)\n";
        }
    }

    void writeGenerationStatisticsCsvHeader(std::ostream& output)
    {
        output << "width,height,aspect_ratio,scale_tier,horizontal_capacity,longitudinal_capacity,major_feature_capacity,detail_complexity,shading_complexity,attachment_complexity,animation_complexity,average_complexity_budget,average_complexity_consumed,average_complexity_unused,average_complexity_utilization_percent,style,faction,attachments_enabled,samples,successful,failed,average_attempts,first_attempt_success_percent,hull_validation_rejections,normalized_hull_width,normalized_hull_height,hull_canvas_density,average_hull_pixels,average_silhouette_articulation,average_shoulder_prominence_percent,average_interior_contraction_percent,average_nose_taper_percent,average_rear_taper_percent,average_stable_width_run_percent,average_near_maximum_row_percent,average_top_unused_margin,average_bottom_unused_margin,average_left_unused_margin,average_right_unused_margin,average_silhouette_guidance,average_structural_negative_space_count,average_structural_negative_space_pixels,average_wing_span,average_wing_extension,average_wing_root_thickness,average_wing_pixels,average_wing_root_pixels,average_outer_wing_pixels,average_wing_start_normalized_y,average_wing_end_normalized_y,average_modifiers,modifier_rejection_percent,average_cockpit_attempts,cockpit_failure_percent,average_cockpit_pixels,average_cockpit_normalized_width,average_cockpit_normalized_height,average_cockpit_glass_pixels,average_cockpit_frame_pixels,average_cockpit_base_pixels,average_cockpit_upper_pixels,average_cockpit_complexity_cost,average_major_feature_count,average_major_feature_pixels,average_major_feature_rejections,average_weapon_hardpoints,average_weapon_count,average_weapon_pixels,average_weapon_rejections,average_engine_count,zero_engine_percent,average_engine_housing_width,average_engine_nozzle_width,average_exhaust_length,nacelle_engine_percent,average_attachment_count,zero_attachment_percent,attachment_failure_percent,symmetric_attachment_percent,average_detail_patterns,detail_canvas_density";

        output << ",average_hull_layer_count,average_hull_layer_pixels,average_hull_layer_rejections,average_core_treatment_count,average_core_region_pixels,average_core_raised_pixels,average_core_recessed_pixels,average_core_material_pixels,average_core_luminous_pixels,average_core_complexity_cost,average_core_rejections";
        output << ",average_spatial_utilization_percent,average_spatial_rejections,macro_asymmetry_planned_percent,macro_asymmetry_fulfilled_percent,macro_asymmetry_balance_score";
        output << ",average_visual_hierarchy_reserved_complexity,visual_hierarchy_fallback_percent";
        output << ",average_material_zone_count,average_material_secondary_pixels,average_material_mechanical_pixels";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END); ++index)
        {
            output << ",material_zone_" << PixelShipGenerator::getShipMaterialZoneTypeName(static_cast<PixelShipGenerator::ShipMaterialZoneType>(index)) << "_ship_percent";
        }
        output << ",average_livery_marking_count,average_livery_primary_pixels,average_livery_secondary_pixels";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipLiveryType::SHIP_LIVERY_TYPE_END); ++index)
        {
            output << ",livery_" << PixelShipGenerator::getShipLiveryTypeName(static_cast<PixelShipGenerator::ShipLiveryType>(index)) << "_ship_percent";
        }
        output << ",average_primary_detail_motif_occurrences,average_secondary_detail_motif_occurrences,average_detail_motif_rejected_placements";
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END); ++index)
        {
            const auto type = static_cast<PixelShipGenerator::ShipDetailMotifType>(index);
            output << ",primary_detail_motif_" << PixelShipGenerator::getShipDetailMotifTypeName(type) << "_percent";
            output << ",secondary_detail_motif_" << PixelShipGenerator::getShipDetailMotifTypeName(type) << "_percent";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END); ++index)
        {
            const auto anchor = static_cast<PixelShipGenerator::ShipVisualAnchorType>(index);
            output << ",primary_anchor_" << PixelShipGenerator::getShipVisualAnchorTypeName(anchor) << "_percent";
            output << ",secondary_anchor_" << PixelShipGenerator::getShipVisualAnchorTypeName(anchor) << "_percent";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END); ++index)
        {
            output << ",negative_space_" << PixelShipGenerator::getShipStructuralNegativeSpaceTypeName(static_cast<PixelShipGenerator::ShipStructuralNegativeSpaceType>(index)) << "_ship_percent";
        }
        for (uint32_t index = 1u; index < static_cast<uint32_t>(PixelShipGenerator::SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END); ++index)
        {
            output << ",silhouette_retry_" << PixelShipGenerator::getSilhouetteValidationFailureReasonName(static_cast<PixelShipGenerator::SilhouetteValidationFailureReason>(index));
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            const auto region = static_cast<PixelShipGenerator::GenerationSpatialRegion>(index);
            output << ",spatial_" << PixelShipGenerator::getGenerationSpatialRegionName(region) << "_load_percent";
            output << ",spatial_" << PixelShipGenerator::getGenerationSpatialRegionName(region) << "_dominant";
            output << ",spatial_" << PixelShipGenerator::getGenerationSpatialRegionName(region) << "_rejections";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END); ++index)
        {
            const auto category = static_cast<PixelShipGenerator::GenerationComplexityCategory>(index);
            output << ",complexity_" << PixelShipGenerator::getGenerationComplexityCategoryName(category) << "_allocation";
            output << ",complexity_" << PixelShipGenerator::getGenerationComplexityCategoryName(category) << "_consumed";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            output << ",modifier_" << getHullModifierName(static_cast<PixelShipGenerator::HullModifierType>(index)) << "_ship_percent";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::WingShapeType::WING_SHAPE_TYPE_END); ++index)
        {
            output << ",wing_shape_" << getWingShapeName(static_cast<PixelShipGenerator::WingShapeType>(index)) << "_ship_percent";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::CockpitSizeClass::COCKPIT_SIZE_CLASS_END); ++index)
        {
            output << ",cockpit_size_" << PixelShipGenerator::getCockpitSizeClassName(static_cast<PixelShipGenerator::CockpitSizeClass>(index)) << "_ship_percent";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::CockpitShapeType::COCKPIT_SHAPE_TYPE_END); ++index)
        {
            output << ",cockpit_shape_" << PixelShipGenerator::getCockpitShapeTypeName(static_cast<PixelShipGenerator::CockpitShapeType>(index)) << "_ship_percent";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END); ++index)
        {
            output << ",major_feature_" << getMajorFeatureName(static_cast<PixelShipGenerator::ShipMajorFeatureType>(index)) << "_ship_percent";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponType::SHIP_WEAPON_TYPE_END); ++index)
        {
            output << ",weapon_type_" << getWeaponTypeName(static_cast<PixelShipGenerator::ShipWeaponType>(index)) << "_unit_percent";
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponHardpointRegion::SHIP_WEAPON_HARDPOINT_REGION_END); ++index)
        {
            output << ",weapon_region_" << getWeaponRegionName(static_cast<PixelShipGenerator::ShipWeaponHardpointRegion>(index)) << "_unit_percent";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::EngineLayoutType::ENGINE_LAYOUT_TYPE_END); ++index)
        {
            output << ",engine_layout_" << getEngineLayoutName(static_cast<PixelShipGenerator::EngineLayoutType>(index)) << "_ship_percent";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END); ++index)
        {
            output << ",engine_size_" << getEngineSizeName(static_cast<PixelShipGenerator::EngineSizeClass>(index)) << "_unit_percent";
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            output << ",attachment_" << getAttachmentTypeName(static_cast<PixelShipGenerator::ShipAttachmentType>(index)) << "_percent";
        }

        output << '\n';
    }

    void writeGenerationStatisticsCsvRow(std::ostream& output, const DiagnosticGenerationConfiguration& configuration, const GenerationStatistics& statistics)
    {
        const uint64_t totalModifierAttempts = std::accumulate(statistics.HullModifierAttemptCounts.begin(), statistics.HullModifierAttemptCounts.end(), uint64_t(0u));
        const uint64_t totalModifierRejections = std::accumulate(statistics.HullModifierRejectionCounts.begin(), statistics.HullModifierRejectionCounts.end(), uint64_t(0u));
        const uint64_t totalAttachmentPlacements = statistics.SymmetricAttachmentPlacementCount + statistics.AsymmetricAttachmentPlacementCount;
        const uint64_t totalEngineUnits = std::accumulate(statistics.EngineSizeCounts.begin(), statistics.EngineSizeCounts.end(), uint64_t(0u));
        const double averageDetailPatterns = statistics.AccentPatternCount.average() + statistics.MechanicalPatternCount.average() + statistics.LightPatternCount.average();
        output << std::fixed << std::setprecision(6);
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ configuration.Width, configuration.Height });
        output << configuration.Width << ',' << configuration.Height << ',' << scaleTraits.AspectRatio << ',' << PixelShipGenerator::getGenerationScaleTierName(scaleTraits.Tier) << ',' << scaleTraits.HorizontalCapacity << ',' << scaleTraits.LongitudinalCapacity << ',' << scaleTraits.MajorFeatureCapacity << ',' << scaleTraits.DetailComplexity << ',' << scaleTraits.ShadingComplexity << ',' << scaleTraits.AttachmentComplexity << ',' << scaleTraits.AnimationComplexity << ',' << statistics.ComplexityInitialBudget.average() << ',' << statistics.ComplexityConsumedBudget.average() << ',' << statistics.ComplexityUnusedBudget.average() << ',' << statistics.ComplexityUtilizationPercent.average() << ',' << styleName(configuration.Style) << ',' << factionName(configuration.Faction) << ',' << (configuration.AttachmentsEnabled ? 1 : 0) << ',' << configuration.Samples << ',' << statistics.SuccessfulGenerations << ',' << statistics.FailedGenerations << ',' << statistics.HullAttempts.average() << ',' << percentage(statistics.FirstAttemptSuccessCount, statistics.RequestedGenerations) << ',' << statistics.HullValidationRejectionCount << ',' << statistics.HullNormalizedWidth.average() << ',' << statistics.HullNormalizedHeight.average() << ',' << statistics.HullCanvasDensity.average() << ',' << statistics.HullPixelCount.average() << ',' << statistics.SilhouetteArticulationCount.average() << ',' << statistics.SilhouetteShoulderProminencePercent.average() << ',' << statistics.SilhouetteInteriorContractionPercent.average() << ',' << statistics.SilhouetteNoseTaperPercent.average() << ',' << statistics.SilhouetteRearTaperPercent.average() << ',' << statistics.SilhouetteLongestStableRunPercent.average() << ',' << statistics.SilhouetteNearMaximumRowPercent.average() << ',' << statistics.SilhouetteTopUnusedMargin.average() << ',' << statistics.SilhouetteBottomUnusedMargin.average() << ',' << statistics.SilhouetteLeftUnusedMargin.average() << ',' << statistics.SilhouetteRightUnusedMargin.average() << ',' << statistics.SilhouetteGuidanceAppliedCount.average() << ',' << statistics.StructuralNegativeSpaceCount.average() << ',' << statistics.StructuralNegativeSpacePixelCount.average() << ',' << statistics.WingMaximumSpan.average() << ',' << statistics.WingMaximumExtension.average() << ',' << statistics.WingRootThickness.average() << ',' << statistics.WingPixelCount.average() << ',' << statistics.WingRootPixelCount.average() << ',' << statistics.OuterWingPixelCount.average() << ',' << statistics.WingStartNormalizedY.average() << ',' << statistics.WingEndNormalizedY.average() << ',' << statistics.HullModifierCount.average() << ',' << percentage(totalModifierRejections, totalModifierAttempts) << ',' << statistics.CockpitPlacementAttempts.average() << ',' << percentage(statistics.CockpitPlacementFailureCount, statistics.SuccessfulGenerations) << ',' << statistics.CockpitPixelCount.average() << ',' << statistics.CockpitNormalizedWidth.average() << ',' << statistics.CockpitNormalizedHeight.average() << ',' << statistics.CockpitGlassPixelCount.average() << ',' << statistics.CockpitFramePixelCount.average() << ',' << statistics.CockpitBasePixelCount.average() << ',' << statistics.CockpitUpperSectionPixelCount.average() << ',' << statistics.CockpitComplexityCost.average() << ',' << statistics.MajorFeatureCount.average() << ',' << statistics.MajorFeaturePixelCount.average() << ',' << statistics.MajorFeaturePlacementRejections.average() << ',' << statistics.WeaponHardpointCount.average() << ',' << statistics.WeaponCount.average() << ',' << statistics.WeaponPixelCount.average() << ',' << statistics.WeaponPlacementRejections.average() << ',' << statistics.EngineCount.average() << ',' << percentage(statistics.ZeroEngineCount, statistics.SuccessfulGenerations) << ',' << statistics.EngineHousingWidth.average() << ',' << statistics.EngineNozzleWidth.average() << ',' << statistics.EngineExhaustLength.average() << ',' << percentage(statistics.NacelleEngineCount, totalEngineUnits) << ',' << statistics.AttachmentCount.average() << ',' << percentage(statistics.ZeroAttachmentCount, statistics.SuccessfulGenerations) << ',' << percentage(statistics.AttachmentPlacementFailureCount, statistics.AttachmentPlacementAttemptCount) << ',' << percentage(statistics.SymmetricAttachmentPlacementCount, totalAttachmentPlacements) << ',' << averageDetailPatterns << ',' << statistics.DetailMaskCanvasDensity.average();

        output << ',' << statistics.HullLayerCount.average() << ',' << statistics.HullLayerPixelCount.average() << ',' << statistics.HullLayerPlacementRejections.average();
        output << ',' << statistics.CoreTreatmentCount.average() << ',' << statistics.CoreRegionPixelCount.average() << ',' << statistics.CoreRaisedPixelCount.average() << ',' << statistics.CoreRecessedPixelCount.average() << ',' << statistics.CoreSecondaryMaterialPixelCount.average() << ',' << statistics.CoreLuminousPixelCount.average() << ',' << statistics.CoreTreatmentComplexityCost.average() << ',' << statistics.CoreTreatmentPlacementRejections.average();
        output << ',' << statistics.SpatialAverageUtilizationPercent.average() << ',' << statistics.SpatialOverloadRejections.average() << ',' << percentage(statistics.MacroAsymmetryPlannedCount, statistics.SuccessfulGenerations) << ',' << percentage(statistics.MacroAsymmetryFulfilledCount, statistics.MacroAsymmetryPlannedCount) << ',' << statistics.MacroAsymmetryBalanceScore.average();
        output << ',' << statistics.VisualHierarchyReservedComplexity.average() << ',' << percentage(statistics.VisualHierarchyFallbackCount, statistics.SuccessfulGenerations);
        output << ',' << statistics.MaterialZoneCount.average() << ',' << statistics.MaterialSecondaryHullPixelCount.average() << ',' << statistics.MaterialMechanicalPixelCount.average();
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.MaterialZoneTypeCounts[index], statistics.SuccessfulGenerations);
        }
        output << ',' << statistics.LiveryMarkingCount.average() << ',' << statistics.LiveryPrimaryPixelCount.average() << ',' << statistics.LiverySecondaryPixelCount.average();
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipLiveryType::SHIP_LIVERY_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.LiveryTypeCounts[index], statistics.SuccessfulGenerations);
        }
        output << ',' << statistics.DetailMotifPrimaryOccurrences.average() << ',' << statistics.DetailMotifSecondaryOccurrences.average() << ',' << statistics.DetailMotifRejectedPlacements.average();
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.PrimaryDetailMotifCounts[index], statistics.SuccessfulGenerations);
            output << ',' << percentage(statistics.SecondaryDetailMotifCounts[index], statistics.SuccessfulGenerations);
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.PrimaryVisualAnchorCounts[index], statistics.SuccessfulGenerations);
            output << ',' << percentage(statistics.SecondaryVisualAnchorCounts[index], statistics.SuccessfulGenerations);
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.StructuralNegativeSpaceTypeCounts[index], statistics.SuccessfulGenerations);
        }
        for (uint32_t index = 1u; index < static_cast<uint32_t>(PixelShipGenerator::SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END); ++index)
        {
            output << ',' << statistics.SilhouetteValidationFailureCounts[index];
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            output << ',' << statistics.SpatialRegionLoadPercent[index].average();
            output << ',' << statistics.SpatialRegionDominantCount[index].average();
            output << ',' << statistics.SpatialRegionRejectionCount[index].average();
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END); ++index)
        {
            output << ',' << statistics.ComplexityCategoryAllocations[index].average();
            output << ',' << statistics.ComplexityCategoryConsumed[index].average();
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.HullModifierOccurrenceCounts[index], statistics.SuccessfulGenerations);
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::WingShapeType::WING_SHAPE_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.WingShapeCounts[index], statistics.SuccessfulGenerations);
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::CockpitSizeClass::COCKPIT_SIZE_CLASS_END); ++index)
        {
            output << ',' << percentage(statistics.CockpitSizeCounts[index], statistics.CockpitPlacementSuccessCount);
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::CockpitShapeType::COCKPIT_SHAPE_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.CockpitShapeCounts[index], statistics.CockpitPlacementSuccessCount);
        }

        const uint64_t totalWeaponUnitsForCsv = std::accumulate(statistics.WeaponTypeCounts.begin(), statistics.WeaponTypeCounts.end(), uint64_t(0u));
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.MajorFeatureTypeCounts[index], statistics.SuccessfulGenerations);
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponType::SHIP_WEAPON_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.WeaponTypeCounts[index], totalWeaponUnitsForCsv);
        }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponHardpointRegion::SHIP_WEAPON_HARDPOINT_REGION_END); ++index)
        {
            output << ',' << percentage(statistics.WeaponRegionCounts[index], totalWeaponUnitsForCsv);
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::EngineLayoutType::ENGINE_LAYOUT_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.EngineLayoutCounts[index], statistics.SuccessfulGenerations);
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END); ++index)
        {
            output << ',' << percentage(statistics.EngineSizeCounts[index], totalEngineUnits);
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            output << ',' << percentage(statistics.AttachmentTypeCounts[index], totalAttachmentPlacements);
        }

        output << '\n';
    }
}
