#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "GenerationComplexityBudget.h"
#include "GenerationScaleTraits.h"
#include "GenerationSpatialBudget.h"
#include "MacroAsymmetry.h"
#include "PixelMask.h"
#include "ShipSurfaceDetailProfile.h"
#include "ShipCockpitType.h"
#include "ShipCoreTreatmentType.h"
#include "ShipDetailMotifType.h"
#include "ShipMajorFeatureType.h"
#include "ShipStructuralNegativeSpaceType.h"
#include "ShipVisualAnchorType.h"
#include "ShipMaterialZoneType.h"
#include "ShipLiveryType.h"
#include "ShipHullLayerType.h"
#include "ShipWingShapeType.h"
#include "ShipWeaponType.h"
#include "SilhouetteQualityMetrics.h"

namespace PixelShipGenerator
{
    enum class HullModifierType : uint32_t
    {
        BROADER_SHOULDERS = 0u,
        SIDE_LOBES,
        STEPPED_WING_EXTENSION,
        NARROW_WAIST,
        WING_CUTOUT,
        SPLIT_NOSE,
        HULL_MODIFIER_TYPE_END
    };


    enum class EngineLayoutType : uint32_t
    {
        CENTRAL = 0u,
        TWIN,
        QUAD,
        CENTRAL_AUXILIARY,
        WIDE_BANK,
        ENGINE_LAYOUT_TYPE_END
    };

    enum class EngineSizeClass : uint32_t
    {
        SMALL = 0u,
        MEDIUM,
        LARGE,
        ENGINE_SIZE_CLASS_END
    };

    struct EngineUnitDebugInfo
    {
        EngineSizeClass SizeClass = EngineSizeClass::SMALL;
        uint32_t HousingStartX = 0u;
        uint32_t HousingWidth = 0u;
        uint32_t NozzleStartX = 0u;
        uint32_t NozzleWidth = 0u;
        uint32_t RootStartY = 0u;
        uint32_t NozzleY = 0u;
        uint32_t ExhaustStartY = 0u;
        uint32_t ExhaustLength = 0u;
        bool Nacelle = false;
    };

    struct WeaponUnitDebugInfo
    {
        ShipWeaponType Type = ShipWeaponType::SINGLE_CANNON;
        ShipWeaponHardpointRegion Region = ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE;
        uint32_t AnchorX = 0u;
        uint32_t AnchorY = 0u;
        uint32_t BodyMinX = 0u;
        uint32_t BodyMaxX = 0u;
        uint32_t BodyMinY = 0u;
        uint32_t BodyMaxY = 0u;
        uint32_t BarrelMinX = 0u;
        uint32_t BarrelMaxX = 0u;
        uint32_t BarrelMinY = 0u;
        uint32_t BarrelMaxY = 0u;
        uint32_t MuzzleX = 0u;
        uint32_t MuzzleY = 0u;
        uint32_t SymmetryGroup = 0u;
        bool MovableBarrel = false;
        bool Emissive = false;
    };

    enum class ShipGenerationDebugStageType : uint32_t
    {
        BASE_HULL = 0u,
        CLEANED_BASE_HULL,
        AFTER_ADDITIVE_MODIFIERS,
        AFTER_SUBTRACTIVE_MODIFIERS,
        FINAL_HULL
    };

    struct ShipGenerationDebugStage
    {
        ShipGenerationDebugStage(ShipGenerationDebugStageType type, const PixelMask& hullMask) : Type(type), HullMask(hullMask) {}

        ShipGenerationDebugStageType Type = ShipGenerationDebugStageType::BASE_HULL;
        PixelMask HullMask;
    };

    struct ShipGenerationDebugInfo
    {
        GenerationScaleTraits ScaleTraits;
        uint32_t ComplexityInitialBudget = 0u;
        uint32_t ComplexityConsumedBudget = 0u;
        uint32_t ComplexityUnusedBudget = 0u;
        ShipVisualAnchorType PrimaryVisualAnchor = ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
        ShipVisualAnchorType SecondaryVisualAnchor = ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
        GenerationSpatialRegion VisualAnchorTargetRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        uint32_t VisualHierarchyReservedComplexity = 0u;
        bool VisualHierarchyFallbackOccurred = false;
        std::array<uint32_t, GenerationComplexityBudget::CategoryCount> ComplexityCategoryAllocations = {};
        std::array<uint32_t, GenerationComplexityBudget::CategoryCount> ComplexityCategoryConsumed = {};
        uint32_t SpatialOverloadRejectionCount = 0u;
        std::array<uint32_t, GenerationSpatialBudget::RegionCount> SpatialRegionAreas = {};
        std::array<uint32_t, GenerationSpatialBudget::RegionCount> SpatialRegionCapacities = {};
        std::array<uint32_t, GenerationSpatialBudget::RegionCount> SpatialRegionLoads = {};
        std::array<uint32_t, GenerationSpatialBudget::RegionCount> SpatialRegionDominantCounts = {};
        std::array<uint32_t, GenerationSpatialBudget::RegionCount> SpatialRegionRejections = {};
        uint32_t SpatialRegionMapWidth = 0u;
        uint32_t SpatialRegionMapHeight = 0u;
        std::vector<uint8_t> SpatialRegionMap;
        bool MacroAsymmetryPlanned = false;
        bool MacroAsymmetryFulfilled = false;
        bool MacroAsymmetryRejected = false;
        MacroAsymmetrySide MacroAsymmetryDominantSide = MacroAsymmetrySide::MACRO_ASYMMETRY_SIDE_END;
        MacroAsymmetryCategory MacroAsymmetryFeatureCategory = MacroAsymmetryCategory::MACRO_ASYMMETRY_CATEGORY_END;
        GenerationSpatialRegion MacroAsymmetryTargetRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        MacroAsymmetryBalanceStrategy MacroAsymmetryBalancingStrategy = MacroAsymmetryBalanceStrategy::MACRO_ASYMMETRY_BALANCE_STRATEGY_END;
        uint32_t MacroAsymmetryDesiredVisualWeight = 0u;
        uint32_t MacroAsymmetryActualVisualWeight = 0u;
        uint32_t MacroAsymmetryBalanceScore = 100u;
        PixelMask MacroAsymmetryMask;
        uint32_t HullGenerationAttemptCount = 0u;
        uint32_t HullValidationRejectionCount = 0u;
        uint32_t SilhouetteGuidanceAppliedCount = 0u;
        uint32_t MaterialZoneCount = 0u;
        uint32_t MaterialSecondaryHullPixelCount = 0u;
        uint32_t MaterialMechanicalPixelCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END)> MaterialZoneTypeCounts = {};
        PixelMask MaterialSecondaryHullMask;
        PixelMask MaterialMechanicalMask;
        uint32_t LiveryMarkingCount = 0u;
        uint32_t LiveryPrimaryPixelCount = 0u;
        uint32_t LiverySecondaryPixelCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(ShipLiveryType::SHIP_LIVERY_TYPE_END)> LiveryTypeCounts = {};
        PixelMask LiveryPrimaryMask;
        PixelMask LiverySecondaryMask;
        ShipDetailMotifType PrimaryDetailMotif = ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
        ShipDetailMotifType SecondaryDetailMotif = ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
        GenerationSpatialRegion PrimaryDetailMotifRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        GenerationSpatialRegion SecondaryDetailMotifRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        uint32_t PrimaryDetailMotifOccurrenceCount = 0u;
        uint32_t SecondaryDetailMotifOccurrenceCount = 0u;
        uint32_t DetailMotifRejectedPlacementCount = 0u;
        PixelMask PrimaryDetailMotifMask;
        PixelMask SecondaryDetailMotifMask;
        uint32_t StructuralNegativeSpaceCount = 0u;
        uint32_t StructuralNegativeSpaceAttemptCount = 0u;
        uint32_t StructuralNegativeSpaceSuccessCount = 0u;
        uint32_t StructuralNegativeSpacePixelCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END)> StructuralNegativeSpaceTypeCounts = {};
        PixelMask ReservedNegativeSpaceMask;
        SilhouetteQualityMetrics SilhouetteMetrics;
        SilhouetteValidationFailureReason LastSilhouetteValidationFailure = SilhouetteValidationFailureReason::NONE;
        std::array<uint32_t, static_cast<std::size_t>(SilhouetteValidationFailureReason::SILHOUETTE_VALIDATION_FAILURE_REASON_END)> SilhouetteValidationFailureCounts = {};
        WingShapeType WingShape = WingShapeType::NONE;
        uint32_t WingStartY = 0u;
        uint32_t WingEndY = 0u;
        uint32_t WingMaximumSpan = 0u;
        uint32_t WingMaximumExtension = 0u;
        uint32_t WingRootThickness = 0u;
        uint32_t WingPixelCount = 0u;
        uint32_t WingRootPixelCount = 0u;
        uint32_t OuterWingPixelCount = 0u;
        uint32_t CoreTreatmentCount = 0u;
        uint32_t CoreTreatmentComplexityCost = 0u;
        uint32_t CoreTreatmentPlacementRejectionCount = 0u;
        uint32_t CoreRegionPixelCount = 0u;
        uint32_t CoreRaisedPixelCount = 0u;
        uint32_t CoreRecessedPixelCount = 0u;
        uint32_t CoreSecondaryMaterialPixelCount = 0u;
        uint32_t CoreLuminousPixelCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)> CoreTreatmentTypeCounts = {};
        PixelMask CoreRegionMask;
        PixelMask CoreRaisedMask;
        PixelMask CoreRecessedMask;
        PixelMask CoreSecondaryMaterialMask;
        PixelMask CoreLuminousMask;
        uint32_t HullLayerCount = 0u;
        uint32_t HullLayerLowerCount = 0u;
        uint32_t HullLayerUpperCount = 0u;
        uint32_t HullLayerPlacementRejectionCount = 0u;
        uint32_t HullLayerPixelCount = 0u;
        uint32_t HullLayerLowerPixelCount = 0u;
        uint32_t HullLayerUpperPixelCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)> HullLayerTypeCounts = {};
        PixelMask HullLayerMask;
        PixelMask HullLayerUpperMask;
        uint32_t MajorFeatureCount = 0u;
        uint32_t MajorFeaturePlacementAttemptCount = 0u;
        uint32_t MajorFeaturePlacementRejectionCount = 0u;
        uint32_t MajorFeaturePixelCount = 0u;
        uint32_t MajorFeatureRaisedPixelCount = 0u;
        uint32_t MajorFeatureRecessedPixelCount = 0u;
        uint32_t MajorFeatureMechanicalPixelCount = 0u;
        uint32_t MajorFeatureEmissivePixelCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> MajorFeatureTypeCounts = {};
        uint32_t WeaponHardpointCount = 0u;
        uint32_t WeaponRequestedGroupCount = 0u;
        uint32_t WeaponRealizedGroupCount = 0u;
        uint32_t WeaponPlacementAttemptCount = 0u;
        uint32_t WeaponPlacementRejectionCount = 0u;
        uint32_t WeaponGenerationChanceSkipCount = 0u;
        uint32_t WeaponNoHardpointFailureCount = 0u;
        uint32_t WeaponTypeSelectionFailureCount = 0u;
        uint32_t WeaponCandidateGeometryFailureCount = 0u;
        uint32_t WeaponSemanticCollisionFailureCount = 0u;
        uint32_t WeaponConnectivityFailureCount = 0u;
        uint32_t WeaponFiringClearanceFailureCount = 0u;
        uint32_t WeaponSymmetryPairFailureCount = 0u;
        uint32_t WeaponSpatialBudgetRejectionCount = 0u;
        uint32_t WeaponComplexityBudgetRejectionCount = 0u;
        uint32_t WeaponCount = 0u;
        uint32_t WeaponPixelCount = 0u;
        uint32_t WeaponMovablePixelCount = 0u;
        uint32_t WeaponCoveragePermille = 0u;
        bool WeaponVisualAnchorOpportunity = false;
        bool WeaponVisualAnchorRealized = false;
        PixelMask WeaponOccupiedMask;
        std::array<uint32_t, static_cast<std::size_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END)> WeaponTypeCounts = {};
        std::vector<WeaponUnitDebugInfo> WeaponUnits;
        uint32_t EngineCount = 0u;
        EngineLayoutType EngineLayout = EngineLayoutType::ENGINE_LAYOUT_TYPE_END;
        std::vector<EngineUnitDebugInfo> EngineUnits;
        uint32_t CockpitPlacementAttemptCount = 0u;
        bool CockpitPlacementSucceeded = false;
        CockpitSizeClass CockpitSize = CockpitSizeClass::COMPACT;
        CockpitShapeType CockpitShape = CockpitShapeType::COMPACT_CANOPY;
        uint32_t CockpitPixelCount = 0u;
        uint32_t CockpitGlassPixelCount = 0u;
        uint32_t CockpitFramePixelCount = 0u;
        uint32_t CockpitBasePixelCount = 0u;
        uint32_t CockpitUpperSectionPixelCount = 0u;
        uint32_t CockpitComplexityCost = 0u;
        uint32_t AttachmentRequestedGroupCount = 0u;
        uint32_t AttachmentPlacedGroupCount = 0u;
        uint32_t AttachmentPlacementAttemptCount = 0u;
        uint32_t AttachmentPlacementFailureCount = 0u;
        uint32_t AttachmentSymmetricPairAttemptCount = 0u;
        uint32_t AttachmentSymmetricPairFailureCount = 0u;
        uint32_t AccentPatternCount = 0u;
        uint32_t MechanicalPatternCount = 0u;
        uint32_t LightPatternCount = 0u;
        std::array<uint32_t, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)> HullModifierAttemptCounts = {};
        std::array<uint32_t, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)> HullModifierRejectionCounts = {};
        std::array<uint32_t, static_cast<std::size_t>(SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END)> SupplementalSurfaceDetailCounts = {};
        std::vector<HullModifierType> AppliedHullModifiers;
        ResolvedSurfaceDetailProfile SurfaceDetailProfile;
        bool HasSurfaceDetailProfile = false;
        std::vector<ShipGenerationDebugStage> HullStages;
    };
}
