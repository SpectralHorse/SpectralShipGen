#pragma once

#include <cstdint>

#include "ShipAttachmentProfile.h"
#include "ShipCockpitType.h"
#include "ShipCoreTreatmentType.h"
#include "ShipFactionPaletteProfile.h"
#include "ShipFactionType.h"
#include "ShipHullLayerType.h"
#include "ShipLiveryType.h"
#include "ShipMajorFeatureType.h"
#include "ShipMaterialZoneType.h"
#include "ShipStructuralNegativeSpaceType.h"
#include "ShipSurfaceDetailProfile.h"
#include "ShipVisualAnchorType.h"
#include "ShipWeaponType.h"

namespace PixelShipGenerator
{
    // Exact rational scale used where historical faction behavior is expressed
    // as integer arithmetic such as 4/5 or 2/3. Denominator must be non-zero.
    struct ShipFactionValueScale
    {
        uint32_t Numerator = 1u;
        uint32_t Denominator = 1u;
    };

    struct ShipFactionWeaponWeightMultipliers
    {
        uint32_t SingleCannon = 100u;
        uint32_t TwinCannon = 100u;
        uint32_t CompactTurret = 100u;
        uint32_t RailWeapon = 100u;
        uint32_t WeaponPod = 100u;

        uint32_t getWeightPercent(ShipWeaponType type) const;
    };

    struct ShipFactionWeaponProfile
    {
        // Multiplies the structural profile's large-weapon generation chance.
        uint32_t ChancePercent = 100u;
        // Added to the structural large-weapon symmetry probability.
        int32_t SymmetryChanceOffset = 0;
        ShipFactionWeaponWeightMultipliers WeightMultipliersPercent;
        // Independent per-weapon probability.
        uint32_t EmissiveChance = 0u;
    };

    struct ShipFactionEngineLayoutWeightMultipliers
    {
        uint32_t Central = 100u;
        uint32_t Twin = 100u;
        uint32_t Quad = 100u;
        uint32_t CentralAuxiliary = 100u;
        uint32_t WideBank = 100u;
    };

    struct ShipFactionEngineSizeWeightMultipliers
    {
        uint32_t Small = 100u;
        uint32_t Medium = 100u;
        uint32_t Large = 100u;
    };

    struct ShipFactionEngineProfile
    {
        // Relative selection-weight multipliers composed with structural engine weights.
        ShipFactionEngineLayoutWeightMultipliers LayoutWeightMultipliersPercent;
        ShipFactionEngineSizeWeightMultipliers SizeWeightMultipliersPercent;
        // Percent multipliers applied to structural/profile-derived engine tendencies.
        uint32_t NacelleChancePercent = 100u;
        uint32_t ExternalHeightPercent = 100u;
    };

    struct ShipFactionMajorFeatureWeightMultipliers
    {
        uint32_t CentralSpine = 100u;
        uint32_t ArmorPlate = 100u;
        uint32_t RecessedBay = 100u;
        uint32_t VentBank = 100u;
        uint32_t WingPlate = 100u;
        uint32_t TechCore = 100u;

        uint32_t getWeightPercent(ShipMajorFeatureType type) const;
    };

    struct ShipFactionMajorFeatureProfile
    {
        // Multiplies the structural Major Feature chance.
        uint32_t ChancePercent = 100u;
        // Relative selection-weight multipliers; 100 preserves structural intent.
        ShipFactionMajorFeatureWeightMultipliers WeightMultipliersPercent;
    };

    struct ShipFactionCockpitSizeWeightMultipliers
    {
        uint32_t Compact = 100u;
        uint32_t Standard = 100u;
        uint32_t Large = 100u;
        uint32_t Massive = 100u;

        uint32_t getWeightPercent(CockpitSizeClass sizeClass) const;
    };

    struct ShipFactionCockpitShapeWeightMultipliers
    {
        uint32_t CompactCanopy = 100u;
        uint32_t ElongatedCanopy = 100u;
        uint32_t WideCommandDeck = 100u;
        uint32_t SplitCanopy = 100u;
        uint32_t DorsalBridge = 100u;
        uint32_t LayeredBridge = 100u;

        uint32_t getWeightPercent(CockpitShapeType shapeType) const;
    };

    struct ShipFactionCockpitProfile
    {
        // Relative multipliers composed with structural cockpit size/shape weights.
        ShipFactionCockpitSizeWeightMultipliers SizeWeightMultipliersPercent;
        ShipFactionCockpitShapeWeightMultipliers ShapeWeightMultipliersPercent;
    };

    struct ShipFactionNegativeSpaceWeightMultipliers
    {
        uint32_t WingChannel = 100u;
        uint32_t RearFork = 100u;
        uint32_t ShoulderGap = 100u;
        uint32_t OpenFrameBay = 100u;
        uint32_t NacelleChannel = 100u;

        uint32_t getWeightPercent(ShipStructuralNegativeSpaceType type) const;
    };

    struct ShipFactionHullProfile
    {
        // Multiplies the structural negative-space chance.
        uint32_t NegativeSpaceChancePercent = 100u;
        // Relative selection-weight multipliers for structural negative-space types.
        ShipFactionNegativeSpaceWeightMultipliers NegativeSpaceWeightMultipliersPercent;
        // Selects the existing alternate articulation-priority ordering. This is
        // a reusable structural-language preference, not a built-in identity flag.
        bool PreferAlternateArticulationOrder = false;
    };

    enum class ShipFactionCoreChannelLuminousPattern : uint32_t
    {
        NONE = 0u,
        EVERY_THIRD_ROW,
        EXCEPT_EVERY_THIRD_ROW,
        SHIP_FACTION_CORE_CHANNEL_LUMINOUS_PATTERN_END
    };

    struct ShipFactionCoreTreatmentWeightOffsets
    {
        int32_t CentralSpine = 0;
        int32_t CockpitSurround = 0;
        int32_t RaisedCorePlate = 0;
        int32_t LateralRecesses = 0;
        int32_t LongitudinalArmorBand = 0;
        int32_t CoreChannel = 0;

        int32_t getOffset(ShipCoreTreatmentType type) const;
    };

    struct ShipFactionCoreTreatmentProfile
    {
        // Multiplies the structural core-treatment chance.
        uint32_t ChancePercent = 100u;
        // Signed values are added to structural core-treatment selection weights.
        ShipFactionCoreTreatmentWeightOffsets WeightOffsets;
        // Semantic pattern override used only when the selected treatment supports it.
        ShipFactionCoreChannelLuminousPattern CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::NONE;
    };

    struct ShipFactionHullLayerWeightAdjustment
    {
        int32_t Offset = 0;
        ShipFactionValueScale Scale;
    };

    struct ShipFactionHullLayerWeightAdjustments
    {
        ShipFactionHullLayerWeightAdjustment CentralDorsalPlate;
        ShipFactionHullLayerWeightAdjustment ForwardArmor;
        ShipFactionHullLayerWeightAdjustment WingArmor;
        ShipFactionHullLayerWeightAdjustment ShoulderArmor;
        ShipFactionHullLayerWeightAdjustment RearEngineCover;

        const ShipFactionHullLayerWeightAdjustment& getAdjustment(ShipHullLayerType type) const;
    };

    struct ShipFactionHullLayerProfile
    {
        // Multiplies the structural hull-layer chance.
        uint32_t ChancePercent = 100u;
        // 0 means no faction-specific cap beyond the structural/scale-derived cap.
        uint32_t MaximumLayerCount = 0u;
        ShipFactionHullLayerWeightAdjustments WeightAdjustments;
    };

    struct ShipFactionMaterialZoneWeightMultipliers
    {
        uint32_t WingSurface = 100u;
        uint32_t ShoulderSurface = 100u;
        uint32_t AxialBand = 100u;
        uint32_t RearMechanical = 100u;
        uint32_t CockpitCollar = 100u;
        uint32_t HardpointSurround = 100u;

        uint32_t getWeightPercent(ShipMaterialZoneType type) const;
    };

    struct ShipFactionMaterialProfile
    {
        // Relative multipliers composed with structural material-zone weights.
        ShipFactionMaterialZoneWeightMultipliers ZoneWeightMultipliersPercent;
    };

    struct ShipFactionLiveryWeightMultipliers
    {
        uint32_t CenterStripe = 100u;
        uint32_t DoubleCenterStripe = 100u;
        uint32_t WingBand = 100u;
        uint32_t ShoulderBlock = 100u;
        uint32_t NoseBand = 100u;
        uint32_t Chevron = 100u;
        uint32_t IdPanel = 100u;
        uint32_t GeometricInsignia = 100u;

        uint32_t getWeightPercent(ShipLiveryType type) const;
    };

    struct ShipFactionLiveryProfile
    {
        // Multiplies the structural livery chance.
        uint32_t ChancePercent = 100u;
        // Relative selection-weight multipliers for livery types.
        ShipFactionLiveryWeightMultipliers WeightMultipliersPercent;
        // Applied after macro-asymmetry's existing structural bonus.
        int32_t AsymmetricChanceOffset = 0;
        uint32_t AsymmetricChanceDivisor = 1u;
        bool AllowAsymmetricGeometricInsignia = true;
    };

    struct ShipFactionVisualAnchorWeightMultipliers
    {
        uint32_t Silhouette = 100u;
        uint32_t Cockpit = 100u;
        uint32_t Wings = 100u;
        uint32_t Engines = 100u;
        uint32_t Weapons = 100u;
        uint32_t MajorFeature = 100u;
        uint32_t HullLayers = 100u;
        uint32_t CentralCore = 100u;
        uint32_t MacroAsymmetry = 100u;
        uint32_t NegativeSpace = 100u;

        uint32_t getWeightPercent(ShipVisualAnchorType type) const;
    };

    struct ShipFactionVisualHierarchyProfile
    {
        // Relative multipliers composed with structural visual-anchor weights.
        ShipFactionVisualAnchorWeightMultipliers AnchorWeightMultipliersPercent;
    };

    struct ShipFactionMacroAsymmetryProfile
    {
        // Multiplies the structural macro-asymmetry chance.
        uint32_t ChancePercent = 100u;
    };

    struct ShipFactionComplexityCategoryOffsets
    {
        int32_t Silhouette = 0;
        int32_t CockpitStructure = 0;
        int32_t HullLayer = 0;
        int32_t MajorFeature = 0;
        int32_t LargeWeapon = 0;
        int32_t Attachment = 0;
        int32_t Detail = 0;
    };

    struct ShipFactionComplexityProfile
    {
        // Multiplies the structural/scale-derived total complexity budget.
        uint32_t TotalBudgetPercent = 100u;
        // Signed offsets preserve the two existing budget category formulations.
        ShipFactionComplexityCategoryOffsets LegacyCategoryOffsets;
        ShipFactionComplexityCategoryOffsets CategoryOffsets;
    };

    enum class ShipFactionHullValueMode : uint32_t
    {
        PROFILE_RANGE = 0u,
        ALTERNATING_BRIGHT_DARK_RANGES,
        SHIP_FACTION_HULL_VALUE_MODE_END
    };

    enum class ShipFactionSecondaryToneDirection : uint32_t
    {
        RANDOM = 0u,
        DARKER,
        LIGHTER,
        CONTRAST_FROM_HULL_MIDPOINT,
        SHIP_FACTION_SECONDARY_TONE_DIRECTION_END
    };

    struct ShipFactionPaletteBehaviorProfile
    {
        // Relationship/selection semantics layered on top of Palette's HSV ranges.
        // These are not generic strength multipliers and may deliberately override
        // the normal tone-selection rule for a custom technological language.
        ShipFactionHullValueMode HullValueMode = ShipFactionHullValueMode::PROFILE_RANGE;
        PaletteUIntRange BrightHullValue = { 0u, 0u };
        PaletteUIntRange DarkHullValue = { 0u, 0u };
        ShipFactionSecondaryToneDirection SecondaryToneDirection = ShipFactionSecondaryToneDirection::RANDOM;
        uint32_t MinimumAccentHueDistance = 0u;
        int32_t AccentHueSeparationShiftA = 0;
        int32_t AccentHueSeparationShiftB = 0;
    };

    enum class ShipFactionPaintColorRole : uint32_t
    {
        PROFILE_DEFAULT = 0u,
        HULL_BASE,
        HULL_SECONDARY,
        HULL_HIGHLIGHT,
        HULL_ACCENT,
        HULL_ACCENT_HIGHLIGHT,
        MECHANICAL_BASE,
        ENGINE_BASE,
        ENGINE_HIGHLIGHT,
        ENGINE_HOT_CORE,
        LIGHT_BASE,
        LIGHT_HIGHLIGHT,
        SHIP_FACTION_PAINT_COLOR_ROLE_END
    };

    struct ShipFactionFinishProfile
    {
        // Semantic palette-role selections for existing painter operations. A role
        // changes which already-generated palette color is used; it does not add geometry.
        ShipFactionPaintColorRole WeaponMuzzleRole = ShipFactionPaintColorRole::ENGINE_HIGHLIGHT;
        ShipFactionPaintColorRole WeaponBodyRole = ShipFactionPaintColorRole::ENGINE_BASE;
        ShipFactionPaintColorRole WeaponRaisedHighlightRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
        ShipFactionPaintColorRole CoreSecondaryMaterialRole = ShipFactionPaintColorRole::HULL_SECONDARY;
        ShipFactionPaintColorRole CoreRaisedRole = ShipFactionPaintColorRole::PROFILE_DEFAULT;
        ShipFactionPaintColorRole CoreLuminousRole = ShipFactionPaintColorRole::LIGHT_BASE;
        ShipFactionPaintColorRole CoreLuminousHighlightRole = ShipFactionPaintColorRole::LIGHT_HIGHLIGHT;
        ShipFactionPaintColorRole CentralDorsalPlateRole = ShipFactionPaintColorRole::PROFILE_DEFAULT;
        ShipFactionPaintColorRole CockpitBaseRole = ShipFactionPaintColorRole::HULL_SECONDARY;
        ShipFactionPaintColorRole CockpitFrameRole = ShipFactionPaintColorRole::HULL_SECONDARY;
        ShipFactionPaintColorRole EngineHotCoreRole = ShipFactionPaintColorRole::ENGINE_HOT_CORE;
        ShipFactionPaintColorRole EngineInteriorHighlightRole = ShipFactionPaintColorRole::ENGINE_HIGHLIGHT;
        bool ForceAxialRidgeEdgeHighlight = false;
    };

    enum class ShipFactionAnimationBooleanOverride : uint32_t
    {
        INHERIT = 0u,
        ENABLE,
        DISABLE,
        SHIP_FACTION_ANIMATION_BOOLEAN_OVERRIDE_END
    };

    struct ShipFactionIdleAnimationProfile
    {
        int32_t EngineMechanicalChanceOffset = 0;
        uint32_t EngineMechanicalChanceMaximum = 0u;
        uint32_t EngineMechanicalChanceMinimum = 0u;
        uint32_t EnginePulseStrengthMinimum = 0u;
        ShipFactionValueScale ExhaustAmplitudeScale;
        int32_t WeaponMechanicalChanceOffset = 0;
        ShipFactionValueScale WeaponMechanicalChanceScale;
        uint32_t WeaponMechanicalChanceMinimum = 0u;
        uint32_t WeaponMechanicalChanceMaximum = 0u;
        ShipFactionValueScale VentActivityChanceScale;
        uint32_t TechPulseStrength = 0u;
        ShipFactionAnimationBooleanOverride SynchronizeEngines = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride AsynchronousEngines = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride AlternateEnginePhases = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride AlternateWeaponPhases = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride SlowMechanicalCycle = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride IrregularEngineCycle = ShipFactionAnimationBooleanOverride::INHERIT;
        bool RandomizeSymmetricWeaponAlternatePhase = false;
        bool AlternateTechCorePhases = false;
    };

    struct ShipFactionMovementAnimationProfile
    {
        ShipFactionValueScale ResponseStrengthScale;
        ShipFactionAnimationBooleanOverride Synchronized = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride Staggered = ShipFactionAnimationBooleanOverride::INHERIT;
        ShipFactionAnimationBooleanOverride HeavyResponse = ShipFactionAnimationBooleanOverride::INHERIT;
        bool SquareTransitionInput = false;
        uint32_t MinimumExhaustVariationLimit = 0u;
    };

    struct ShipFactionFiringAnimationProfile
    {
        ShipFactionValueScale DurationScale;
        int32_t DurationAdditionMilliseconds = 0;
        ShipFactionValueScale ResponseStrengthScale;
        uint32_t MaximumPreFireExtensionLimit = 0u;
        ShipFactionAnimationBooleanOverride HeavyResponse = ShipFactionAnimationBooleanOverride::INHERIT;
    };

    struct ShipFactionAnimationProfile
    {
        // Resolved faction response semantics for Task 85. Task 83 only declares
        // these values; current animators continue to use their compatibility path.
        ShipFactionIdleAnimationProfile Idle;
        ShipFactionMovementAnimationProfile LateralMovement;
        ShipFactionMovementAnimationProfile LongitudinalMovement;
        ShipFactionFiringAnimationProfile Firing;
    };

    struct ShipFactionProfile
    {
        ShipFactionPaletteProfile Palette;
        ShipFactionPaletteBehaviorProfile PaletteBehavior;
        ShipFactionSurfaceDetailProfile SurfaceDetails;
        ShipFactionAttachmentProfile Attachments;
        ShipFactionWeaponProfile Weapons;
        ShipFactionEngineProfile Engines;
        ShipFactionMajorFeatureProfile MajorFeatures;
        ShipFactionCockpitProfile Cockpit;
        ShipFactionHullProfile Hull;
        ShipFactionCoreTreatmentProfile CoreTreatment;
        ShipFactionHullLayerProfile HullLayers;
        ShipFactionMaterialProfile Materials;
        ShipFactionLiveryProfile Livery;
        ShipFactionVisualHierarchyProfile VisualHierarchy;
        ShipFactionMacroAsymmetryProfile MacroAsymmetry;
        ShipFactionComplexityProfile Complexity;
        ShipFactionFinishProfile Finish;
        ShipFactionAnimationProfile Animation;
    };

    // Returns an immutable canonical built-in faction profile. SHIP_FACTION_TYPE_END
    // is not a built-in preset and is rejected.
    const ShipFactionProfile& getShipFactionProfile(ShipFactionType faction);
}
