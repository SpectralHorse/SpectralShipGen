#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>

#include "GeneratedShip.h"
#include "ShipAttachment.h"
#include "ShipFactionType.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationProfile.h"

namespace PixelShipGeneratorDiagnostics
{
    struct NumericStatistics
    {
        void add(double value);
        double average() const;

        uint64_t Count = 0u;
        double Sum = 0.0;
        double Minimum = 0.0;
        double Maximum = 0.0;
    };

    struct DiagnosticGenerationConfiguration
    {
        uint32_t Width = 44u;
        uint32_t Height = 44u;
        PixelShipGenerator::ShipStyle Style = PixelShipGenerator::ShipStyle::FIGHTER;
        PixelShipGenerator::ShipFactionType Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
        uint32_t DetailDensity = 50u;
        uint32_t AsymmetricDetailChance = 10u;
        bool AttachmentsEnabled = true;
        uint64_t Samples = 1000u;
        uint64_t DiagnosticSeed = 0x6A09E667F3BCC909ull;
    };

    struct GenerationStatistics
    {
        uint64_t RequestedGenerations = 0u;
        uint64_t SuccessfulGenerations = 0u;
        uint64_t FailedGenerations = 0u;
        uint64_t FirstAttemptSuccessCount = 0u;
        uint64_t HullValidationRejectionCount = 0u;
        NumericStatistics HullAttempts;
        NumericStatistics HullOccupiedWidth;
        NumericStatistics HullOccupiedHeight;
        NumericStatistics HullBoundingArea;
        NumericStatistics HullPixelCount;
        NumericStatistics HullNormalizedWidth;
        NumericStatistics HullNormalizedHeight;
        NumericStatistics HullCanvasDensity;
        NumericStatistics HullBoundingFillDensity;
        NumericStatistics SilhouetteArticulationCount;
        NumericStatistics SilhouetteShoulderProminencePercent;
        NumericStatistics SilhouetteInteriorContractionPercent;
        NumericStatistics SilhouetteNoseTaperPercent;
        NumericStatistics SilhouetteRearTaperPercent;
        NumericStatistics SilhouetteLongestStableRunPercent;
        NumericStatistics SilhouetteNearMaximumRowPercent;
        NumericStatistics SilhouetteTopUnusedMargin;
        NumericStatistics SilhouetteBottomUnusedMargin;
        NumericStatistics SilhouetteLeftUnusedMargin;
        NumericStatistics SilhouetteRightUnusedMargin;
        NumericStatistics SilhouetteGuidanceAppliedCount;
        NumericStatistics StructuralNegativeSpaceCount;
        NumericStatistics StructuralNegativeSpacePixelCount;
        NumericStatistics MaterialZoneCount;
        NumericStatistics MaterialSecondaryHullPixelCount;
        NumericStatistics MaterialMechanicalPixelCount;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END)> MaterialZoneTypeCounts = {};
        NumericStatistics LiveryMarkingCount;
        NumericStatistics LiveryPrimaryPixelCount;
        NumericStatistics LiverySecondaryPixelCount;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipLiveryType::SHIP_LIVERY_TYPE_END)> LiveryTypeCounts = {};
        NumericStatistics DetailMotifPrimaryOccurrences;
        NumericStatistics DetailMotifSecondaryOccurrences;
        NumericStatistics DetailMotifRejectedPlacements;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END)> PrimaryDetailMotifCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END)> SecondaryDetailMotifCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END)> StructuralNegativeSpaceTypeCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END)> SilhouetteValidationFailureCounts = {};
        NumericStatistics WingMaximumSpan;
        NumericStatistics WingMaximumExtension;
        NumericStatistics WingRootThickness;
        NumericStatistics WingPixelCount;
        NumericStatistics WingRootPixelCount;
        NumericStatistics OuterWingPixelCount;
        NumericStatistics WingStartNormalizedY;
        NumericStatistics WingEndNormalizedY;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::WingShapeType::WING_SHAPE_TYPE_END)> WingShapeCounts = {};
        NumericStatistics ComplexityInitialBudget;
        NumericStatistics ComplexityConsumedBudget;
        NumericStatistics ComplexityUnusedBudget;
        NumericStatistics ComplexityUtilizationPercent;
        NumericStatistics VisualHierarchyReservedComplexity;
        uint64_t VisualHierarchyFallbackCount = 0u;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)> PrimaryVisualAnchorCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)> SecondaryVisualAnchorCounts = {};
        std::map<std::string, uint64_t> VisualAnchorCombinationFrequencies;
        std::array<NumericStatistics, PixelShipGenerator::GenerationComplexityBudget::CategoryCount> ComplexityCategoryAllocations;
        std::array<NumericStatistics, PixelShipGenerator::GenerationComplexityBudget::CategoryCount> ComplexityCategoryConsumed;
        NumericStatistics SpatialAverageUtilizationPercent;
        NumericStatistics SpatialOverloadRejections;
        uint64_t MacroAsymmetryPlannedCount = 0u;
        uint64_t MacroAsymmetryFulfilledCount = 0u;
        uint64_t MacroAsymmetryRejectedCount = 0u;
        NumericStatistics MacroAsymmetryBalanceScore;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::MacroAsymmetryCategory::MACRO_ASYMMETRY_CATEGORY_END)> MacroAsymmetryCategoryCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::MacroAsymmetrySide::MACRO_ASYMMETRY_SIDE_END)> MacroAsymmetrySideCounts = {};
        std::array<NumericStatistics, PixelShipGenerator::GenerationSpatialBudget::RegionCount> SpatialRegionLoadPercent;
        std::array<NumericStatistics, PixelShipGenerator::GenerationSpatialBudget::RegionCount> SpatialRegionDominantCount;
        std::array<NumericStatistics, PixelShipGenerator::GenerationSpatialBudget::RegionCount> SpatialRegionRejectionCount;
        NumericStatistics HullModifierCount;
        NumericStatistics HullLayerCount;
        NumericStatistics HullLayerPixelCount;
        NumericStatistics HullLayerPlacementRejections;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)> HullLayerTypeCounts = {};
        NumericStatistics CoreTreatmentCount;
        NumericStatistics CoreRegionPixelCount;
        NumericStatistics CoreRaisedPixelCount;
        NumericStatistics CoreRecessedPixelCount;
        NumericStatistics CoreSecondaryMaterialPixelCount;
        NumericStatistics CoreLuminousPixelCount;
        NumericStatistics CoreTreatmentComplexityCost;
        NumericStatistics CoreTreatmentPlacementRejections;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)> CoreTreatmentTypeCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END)> HullModifierOccurrenceCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END)> HullModifierAttemptCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::HullModifierType::HULL_MODIFIER_TYPE_END)> HullModifierRejectionCounts = {};
        std::map<uint32_t, uint64_t> HullModifierCountFrequencies;
        std::map<std::string, uint64_t> HullModifierCombinationFrequencies;
        NumericStatistics CockpitPlacementAttempts;
        uint64_t CockpitPlacementSuccessCount = 0u;
        uint64_t CockpitPlacementFailureCount = 0u;
        NumericStatistics CockpitPixelCount;
        NumericStatistics CockpitNormalizedWidth;
        NumericStatistics CockpitNormalizedHeight;
        NumericStatistics CockpitGlassPixelCount;
        NumericStatistics CockpitFramePixelCount;
        NumericStatistics CockpitBasePixelCount;
        NumericStatistics CockpitUpperSectionPixelCount;
        NumericStatistics CockpitComplexityCost;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::CockpitSizeClass::COCKPIT_SIZE_CLASS_END)> CockpitSizeCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::CockpitShapeType::COCKPIT_SHAPE_TYPE_END)> CockpitShapeCounts = {};
        NumericStatistics MajorFeatureCount;
        NumericStatistics MajorFeaturePixelCount;
        NumericStatistics MajorFeaturePlacementRejections;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> MajorFeatureTypeCounts = {};
        NumericStatistics WeaponHardpointCount;
        NumericStatistics WeaponCount;
        NumericStatistics WeaponPixelCount;
        NumericStatistics WeaponPlacementRejections;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipWeaponType::SHIP_WEAPON_TYPE_END)> WeaponTypeCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipWeaponHardpointRegion::SHIP_WEAPON_HARDPOINT_REGION_END)> WeaponRegionCounts = {};
        NumericStatistics EngineCount;
        uint64_t ZeroEngineCount = 0u;
        NumericStatistics EngineHousingWidth;
        NumericStatistics EngineNozzleWidth;
        NumericStatistics EngineExhaustLength;
        uint64_t NacelleEngineCount = 0u;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::EngineLayoutType::ENGINE_LAYOUT_TYPE_END)> EngineLayoutCounts = {};
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END)> EngineSizeCounts = {};
        NumericStatistics AttachmentCount;
        NumericStatistics AttachmentGroupCount;
        uint64_t ZeroAttachmentCount = 0u;
        uint64_t AttachmentPlacementAttemptCount = 0u;
        uint64_t AttachmentPlacementFailureCount = 0u;
        uint64_t AttachmentSymmetricPairAttemptCount = 0u;
        uint64_t AttachmentSymmetricPairFailureCount = 0u;
        uint64_t SymmetricAttachmentPlacementCount = 0u;
        uint64_t AsymmetricAttachmentPlacementCount = 0u;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END)> AttachmentTypeCounts = {};
        NumericStatistics AccentPatternCount;
        NumericStatistics MechanicalPatternCount;
        NumericStatistics LightPatternCount;
        NumericStatistics DetailMaskPixelCount;
        NumericStatistics DetailMaskCanvasDensity;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END)> SupplementalSurfaceDetailCounts = {};

        void recordSuccess(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo, const DiagnosticGenerationConfiguration& configuration);
        void recordFailure(const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo);
        uint64_t deterministicSignature() const;
    };

    uint64_t deriveDiagnosticSampleSeed(uint64_t diagnosticSeed, uint64_t sampleIndex);
    GenerationStatistics collectGenerationStatistics(const DiagnosticGenerationConfiguration& configuration);
    void printGenerationStatistics(std::ostream& output, const DiagnosticGenerationConfiguration& configuration, const GenerationStatistics& statistics);
    void writeGenerationStatisticsCsvHeader(std::ostream& output);
    void writeGenerationStatisticsCsvRow(std::ostream& output, const DiagnosticGenerationConfiguration& configuration, const GenerationStatistics& statistics);
}
