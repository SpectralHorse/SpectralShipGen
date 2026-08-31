#pragma once

#include <SpectralShipGen/ShipAnimationTraits.h>
#include <SpectralShipGen/ShipAttachmentProfile.h>
#include <SpectralShipGen/ShipCockpitType.h>
#include <SpectralShipGen/ShipCoreTreatmentType.h>
#include <SpectralShipGen/ShipHullLayerType.h>
#include <SpectralShipGen/ShipSurfaceDetailProfile.h>
#include <SpectralShipGen/ShipMajorFeatureType.h>
#include <SpectralShipGen/ShipStructuralNegativeSpaceType.h>
#include <SpectralShipGen/ShipVisualAnchorType.h>
#include <SpectralShipGen/ShipMaterialZoneType.h>
#include <SpectralShipGen/ShipLiveryType.h>
#include <SpectralShipGen/ShipWeaponType.h>

#include <array>
#include <cstdint>

namespace SpectralShipGen
{

    enum class ShipStyle : uint32_t
    {
        SLEEK = 0,
        FIGHTER,
        HEAVY,
        INDUSTRIAL,
        SPEARHEAD,
        DELTA,
        SHIP_STYLE_END
    };

    struct UIntRange
    {
        uint32_t Min = 0;
        uint32_t Max = 0;
    };


    struct ShipMajorFeatureWeights
    {
        uint32_t CentralSpine = 60u;
        uint32_t ArmorPlate = 70u;
        uint32_t RecessedBay = 50u;
        uint32_t VentBank = 55u;
        uint32_t WingPlate = 65u;
        uint32_t TechCore = 45u;

        uint32_t getWeight(ShipMajorFeatureType type) const
        {
            switch (type)
            {
            case ShipMajorFeatureType::CENTRAL_SPINE: return CentralSpine;
            case ShipMajorFeatureType::ARMOR_PLATE: return ArmorPlate;
            case ShipMajorFeatureType::RECESSED_BAY: return RecessedBay;
            case ShipMajorFeatureType::VENT_BANK: return VentBank;
            case ShipMajorFeatureType::WING_PLATE: return WingPlate;
            case ShipMajorFeatureType::TECH_CORE: return TechCore;
            default: return 0u;
            }
        }
    };

    struct ShipWeaponWeights
    {
        uint32_t SingleCannon = 80u;
        uint32_t TwinCannon = 90u;
        uint32_t CompactTurret = 70u;
        uint32_t RailWeapon = 75u;
        uint32_t WeaponPod = 65u;

        uint32_t getWeight(ShipWeaponType type) const
        {
            switch (type)
            {
            case ShipWeaponType::SINGLE_CANNON: return SingleCannon;
            case ShipWeaponType::TWIN_CANNON: return TwinCannon;
            case ShipWeaponType::COMPACT_TURRET: return CompactTurret;
            case ShipWeaponType::RAIL_WEAPON: return RailWeapon;
            case ShipWeaponType::WEAPON_POD: return WeaponPod;
            default: return 0u;
            }
        }
    };

    struct ShipWeaponHardpointWeights
    {
        uint32_t CentralNose = 100u;
        uint32_t ForwardFuselageSide = 100u;
        uint32_t WingRoot = 100u;
        uint32_t OuterWing = 100u;
        uint32_t ForwardShoulder = 100u;
        uint32_t CentralBody = 100u;

        uint32_t getWeight(ShipWeaponHardpointRegion region) const
        {
            switch (region)
            {
            case ShipWeaponHardpointRegion::CENTRAL_NOSE: return CentralNose;
            case ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE: return ForwardFuselageSide;
            case ShipWeaponHardpointRegion::WING_ROOT: return WingRoot;
            case ShipWeaponHardpointRegion::OUTER_WING: return OuterWing;
            case ShipWeaponHardpointRegion::FORWARD_SHOULDER: return ForwardShoulder;
            case ShipWeaponHardpointRegion::CENTRAL_BODY: return CentralBody;
            default: return 0u;
            }
        }
    };


    struct ShipCockpitSizeWeights
    {
        uint32_t Compact = 35u;
        uint32_t Standard = 70u;
        uint32_t Large = 45u;
        uint32_t Massive = 8u;

        uint32_t getWeight(CockpitSizeClass sizeClass) const
        {
            switch (sizeClass)
            {
            case CockpitSizeClass::COMPACT: return Compact;
            case CockpitSizeClass::STANDARD: return Standard;
            case CockpitSizeClass::LARGE: return Large;
            case CockpitSizeClass::MASSIVE: return Massive;
            default: return 0u;
            }
        }
    };

    struct ShipCockpitShapeWeights
    {
        uint32_t CompactCanopy = 70u;
        uint32_t ElongatedCanopy = 80u;
        uint32_t WideCommandDeck = 55u;
        uint32_t SplitCanopy = 45u;
        uint32_t DorsalBridge = 45u;
        uint32_t LayeredBridge = 30u;

        uint32_t getWeight(CockpitShapeType shapeType) const
        {
            switch (shapeType)
            {
            case CockpitShapeType::COMPACT_CANOPY: return CompactCanopy;
            case CockpitShapeType::ELONGATED_CANOPY: return ElongatedCanopy;
            case CockpitShapeType::WIDE_COMMAND_DECK: return WideCommandDeck;
            case CockpitShapeType::SPLIT_CANOPY: return SplitCanopy;
            case CockpitShapeType::DORSAL_BRIDGE: return DorsalBridge;
            case CockpitShapeType::LAYERED_BRIDGE: return LayeredBridge;
            default: return 0u;
            }
        }
    };


    struct ShipStructuralNegativeSpaceWeights
    {
        uint32_t WingChannel = 60u;
        uint32_t RearFork = 50u;
        uint32_t ShoulderGap = 60u;
        uint32_t OpenFrameBay = 45u;
        uint32_t NacelleChannel = 55u;

        uint32_t getWeight(ShipStructuralNegativeSpaceType type) const
        {
            switch (type)
            {
            case ShipStructuralNegativeSpaceType::WING_CHANNEL: return WingChannel;
            case ShipStructuralNegativeSpaceType::REAR_FORK: return RearFork;
            case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return ShoulderGap;
            case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return OpenFrameBay;
            case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return NacelleChannel;
            default: return 0u;
            }
        }
    };

    struct ShipMaterialZoneWeights
    {
        uint32_t WingSurface = 70u;
        uint32_t ShoulderSurface = 70u;
        uint32_t AxialBand = 70u;
        uint32_t RearMechanical = 60u;
        uint32_t CockpitCollar = 65u;
        uint32_t HardpointSurround = 55u;

        uint32_t getWeight(ShipMaterialZoneType type) const
        {
            switch (type)
            {
            case ShipMaterialZoneType::WING_SURFACE: return WingSurface;
            case ShipMaterialZoneType::SHOULDER_SURFACE: return ShoulderSurface;
            case ShipMaterialZoneType::AXIAL_BAND: return AxialBand;
            case ShipMaterialZoneType::REAR_MECHANICAL: return RearMechanical;
            case ShipMaterialZoneType::COCKPIT_COLLAR: return CockpitCollar;
            case ShipMaterialZoneType::HARDPOINT_SURROUND: return HardpointSurround;
            default: return 0u;
            }
        }
    };



    struct ShipLiveryWeights
    {
        uint32_t CenterStripe = 80u;
        uint32_t DoubleCenterStripe = 55u;
        uint32_t WingBand = 75u;
        uint32_t ShoulderBlock = 70u;
        uint32_t NoseBand = 70u;
        uint32_t Chevron = 55u;
        uint32_t IdPanel = 60u;
        uint32_t GeometricInsignia = 45u;

        uint32_t getWeight(ShipLiveryType type) const
        {
            switch (type)
            {
            case ShipLiveryType::CENTER_STRIPE: return CenterStripe;
            case ShipLiveryType::DOUBLE_CENTER_STRIPE: return DoubleCenterStripe;
            case ShipLiveryType::WING_BAND: return WingBand;
            case ShipLiveryType::SHOULDER_BLOCK: return ShoulderBlock;
            case ShipLiveryType::NOSE_BAND: return NoseBand;
            case ShipLiveryType::CHEVRON: return Chevron;
            case ShipLiveryType::ID_PANEL: return IdPanel;
            case ShipLiveryType::GEOMETRIC_INSIGNIA: return GeometricInsignia;
            default: return 0u;
            }
        }
    };

    struct ShipComplexityCategoryWeights
    {
        int32_t Silhouette = 13;
        int32_t CockpitStructure = 10;
        int32_t HullLayer = 14;
        int32_t MajorFeature = 17;
        int32_t LargeWeapon = 28;
        int32_t Attachment = 8;
        int32_t Detail = 10;

        std::array<int32_t, 7u> toArray() const
        {
            return { Silhouette, CockpitStructure, HullLayer, MajorFeature, LargeWeapon, Attachment, Detail };
        }
    };

    struct ShipSpatialCapacityBias
    {
        int32_t Nose = 0;
        int32_t FrontFuselage = 0;
        int32_t MidFuselage = 0;
        int32_t RearFuselage = 0;
        int32_t WingRoot = 0;
        int32_t OuterWing = 0;
    };

    struct ShipCoreTreatmentWeights
    {
        uint32_t CentralSpine = 22u;
        uint32_t CockpitSurround = 22u;
        uint32_t RaisedCorePlate = 18u;
        uint32_t LateralRecesses = 12u;
        uint32_t LongitudinalArmorBand = 16u;
        uint32_t CoreChannel = 10u;

        uint32_t getWeight(ShipCoreTreatmentType type) const
        {
            switch (type)
            {
            case ShipCoreTreatmentType::CENTRAL_SPINE: return CentralSpine;
            case ShipCoreTreatmentType::COCKPIT_SURROUND: return CockpitSurround;
            case ShipCoreTreatmentType::RAISED_CORE_PLATE: return RaisedCorePlate;
            case ShipCoreTreatmentType::LATERAL_RECESSES: return LateralRecesses;
            case ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND: return LongitudinalArmorBand;
            case ShipCoreTreatmentType::CORE_CHANNEL: return CoreChannel;
            default: return 0u;
            }
        }
    };

    struct ShipHullLayerWeights
    {
        uint32_t CentralDorsalPlate = 75u;
        uint32_t ForwardArmor = 85u;
        uint32_t WingArmor = 90u;
        uint32_t ShoulderArmor = 90u;
        uint32_t RearEngineCover = 55u;

        uint32_t getWeight(ShipHullLayerType type) const
        {
            switch (type)
            {
            case ShipHullLayerType::CENTRAL_DORSAL_PLATE: return CentralDorsalPlate;
            case ShipHullLayerType::FORWARD_ARMOR: return ForwardArmor;
            case ShipHullLayerType::WING_ARMOR: return WingArmor;
            case ShipHullLayerType::SHOULDER_ARMOR: return ShoulderArmor;
            case ShipHullLayerType::REAR_ENGINE_COVER: return RearEngineCover;
            default: return 0u;
            }
        }
    };

    struct ShipSilhouetteGuidanceWeights
    {
        uint32_t BroaderShoulders = 3u;
        uint32_t SideLobes = 1u;
        uint32_t SteppedWingExtension = 2u;
    };

    struct ShipMacroAsymmetryCategoryWeights
    {
        uint32_t HullLayer = 25u;
        uint32_t LargeWeapon = 50u;
        uint32_t Attachment = 25u;

        std::array<uint32_t, 3u> toArray() const { return { HullLayer, LargeWeapon, Attachment }; }
    };

    enum class ShipDetailMotifPlacementBias : uint32_t
    {
        DEFAULT = 0u,
        AXIAL,
        WING_SURFACE
    };

    enum class ShipDetailMotifOrientationBias : uint32_t
    {
        DEFAULT = 0u,
        LONGITUDINAL,
        LATERAL
    };

    enum class ShipHullSurfaceTone : uint32_t
    {
        BASE = 0u,
        SECONDARY,
        HIGHLIGHT
    };

    struct ShipVisualAnchorWeights
    {
        uint32_t Silhouette = 70u;
        uint32_t Cockpit = 70u;
        uint32_t Wings = 70u;
        uint32_t Engines = 70u;
        uint32_t Weapons = 70u;
        uint32_t MajorFeature = 70u;
        uint32_t HullLayers = 70u;
        uint32_t CentralCore = 70u;
        uint32_t MacroAsymmetry = 35u;
        uint32_t NegativeSpace = 35u;

        uint32_t getWeight(ShipVisualAnchorType type) const
        {
            switch (type)
            {
            case ShipVisualAnchorType::SILHOUETTE: return Silhouette;
            case ShipVisualAnchorType::COCKPIT: return Cockpit;
            case ShipVisualAnchorType::WINGS: return Wings;
            case ShipVisualAnchorType::ENGINES: return Engines;
            case ShipVisualAnchorType::WEAPONS: return Weapons;
            case ShipVisualAnchorType::MAJOR_FEATURE: return MajorFeature;
            case ShipVisualAnchorType::HULL_LAYERS: return HullLayers;
            case ShipVisualAnchorType::CENTRAL_CORE: return CentralCore;
            case ShipVisualAnchorType::MACRO_ASYMMETRY: return MacroAsymmetry;
            case ShipVisualAnchorType::NEGATIVE_SPACE: return NegativeSpace;
            default: return 0u;
            }
        }
    };

    struct ShipGenerationProfile
    {
        // Public field semantics:
        // - *Chance values are probabilities and use 0..100.
        // - *Weight values are non-negative relative selection weights; their
        //   absolute magnitude is not a probability.
        // - scale/size/strength *Percent values are multipliers where 100 is
        //   baseline and values above 100 may be valid.
        // - UIntRange values are inclusive and must satisfy Min <= Max.
        // - count/maximum fields are discrete limits; signed offsets/biases may
        //   intentionally be negative or positive.
        // validateShipGenerationProfile() defines the complete safety contract.

        // Post-generation behavior resolved with the structural preset.
        // Animation consumes these semantics rather than built-in ShipStyle identity.
        ShipAnimationTraits AnimationTraits;

        ShipVisualAnchorWeights VisualAnchorWeights;
        uint32_t VisualSecondaryAnchorChance = 32u;
        bool VisualHierarchyEnabled = true;
        bool HullLayerHierarchyUsesWingRoot = false;
        bool WeaponHierarchyUsesWingRoot = true;

        // Global complexity/spatial capacity policy. Built-in styles resolve
        // these once as preset data; downstream static stages consume them
        // without consulting ShipStyle identity.
        uint32_t ComplexityBudgetPercent = 100u;
        ShipComplexityCategoryWeights ComplexityCategoryWeights;
        ShipSpatialCapacityBias SpatialCapacityBias;

        // Canvas-relative longitudinal occupancy. Existing styles keep the
        // original 8-13% padding range; styles can bias toward longer/shorter
        // occupied silhouettes without requiring a particular aspect ratio.
        UIntRange HullVerticalPaddingPercent = { 8u, 13u };
        UIntRange HullHorizontalPaddingPercent = { 8u, 13u };

        UIntRange NoseEndPercent = { 14u, 20u };
        UIntRange UpperFuselageEndPercent = { 31u, 39u };
        UIntRange MainBodyEndPercent = { 48u, 56u };
        UIntRange RearFuselageStartPercent = { 87u, 91u };

        UIntRange NoseWidthPercent = { 10u, 18u };
        UIntRange UpperFuselageWidthPercent = { 25u, 45u };
        UIntRange MainBodyWidthPercent = { 42u, 68u };
        UIntRange RearFuselageWidthPercent = { 30u, 55u };
        UIntRange RearWidthPercent = { 20u, 42u };

        uint32_t NoWingWeight = 25u;
        uint32_t SmallWingWeight = 25u;
        uint32_t SweptWingWeight = 30u;
        uint32_t BroadWingWeight = 20u;

        UIntRange SmallWingIncreasePercent = { 8u, 16u };
        UIntRange SweptWingWidthPercent = { 72u, 96u };
        UIntRange BroadWingWidthPercent = { 82u, 100u };
        int32_t WingLongitudinalOffsetPercent = 0;
        uint32_t WingRootLengthPercent = 100u;
        uint32_t WingRootWidthPercent = 100u;
        uint32_t SmallWingTaperPercent = 45u;
        uint32_t SweptWingTaperPercent = 38u;
        uint32_t BroadWingTaperPercent = 62u;

        // Advanced silhouette modifiers
        uint32_t HullModifierChance = 60u;
        uint32_t MaximumHullModifiers = 2u;
        uint32_t BroaderShouldersModifierWeight = 60u;
        uint32_t SideLobesModifierWeight = 45u;
        uint32_t SteppedWingModifierWeight = 70u;
        uint32_t NarrowWaistModifierWeight = 50u;
        uint32_t WingCutoutModifierWeight = 60u;
        uint32_t SplitNoseModifierWeight = 55u;

        // Explicit macro-form expectations used by Task-56 silhouette guidance
        // and validation. They remain style-profile data rather than a global
        // aesthetic score.
        uint32_t MinimumSilhouetteWidthPercent = 44u;
        uint32_t MinimumSilhouetteHeightPercent = 70u;
        uint32_t SilhouetteArticulationTarget = 3u;
        uint32_t SilhouetteMaximumStableRunPercent = 50u;
        uint32_t SilhouetteConvexFillTriggerPercent = 65u;
        bool SilhouetteGuidanceEnabled = true;
        bool SilhouetteWeakArticulationGuidanceEnabled = true;
        ShipSilhouetteGuidanceWeights SilhouetteGuidanceWeights;
        bool SilhouetteProfileValidationEnabled = true;
        uint32_t TinySilhouetteExtraWidthRelaxationPercent = 0u;
        bool CleanAxialTaperArticulationExemption = false;
        bool WingWedgeArticulationExemption = false;

        // Deliberate structural voids cut during Hull generation. These use
        // the existing SILHOUETTE complexity budget and are preserved for
        // later placement stages through StructuralNegativeSpaceData.
        uint32_t StructuralNegativeSpaceChance = 28u;
        uint32_t MaximumStructuralNegativeSpaceStructures = 1u;
        uint32_t StructuralNegativeSpaceScalePercent = 100u;
        ShipStructuralNegativeSpaceWeights StructuralNegativeSpaceWeights;
        UIntRange RearForkStartPercent = { 68u, 80u };

        // Core-treatment and hull-layer vocabulary/geometry.
        uint32_t CoreTreatmentChance = 70u;
        uint32_t CoreRegionWidthBasePercent = 58u;
        uint32_t CoreRegionWidthHorizontalCapacityDivisor = 5u;
        uint32_t CoreRegionWidthMaximumPercent = 0u; // 0 = no additional cap
        uint32_t RaisedCorePlateWidthPercent = 100u;
        uint32_t MaximumCoreTreatments = 3u;
        ShipCoreTreatmentWeights CoreTreatmentWeights;

        uint32_t HullLayerChance = 62u;
        uint32_t MaximumHullLayers = 3u;
        ShipHullLayerWeights HullLayerWeights;

        UIntRange CockpitStartPercent = { 14u, 38u };
        uint32_t CockpitHeightPercent = 100u;
        uint32_t CockpitWidthPercent = 100u;
        uint32_t MaximumCockpitHullPercent = 16u;
        ShipCockpitSizeWeights CockpitSizeWeights;
        ShipCockpitShapeWeights CockpitShapeWeights;

        uint32_t CentralEngineWeight = 1u;
        uint32_t TwinEngineWeight = 1u;
        uint32_t QuadEngineWeight = 1u;
        uint32_t CentralAuxiliaryEngineWeight = 1u;
        uint32_t EngineBankWeight = 1u;
        uint32_t SmallEngineSizeWeight = 2u;
        uint32_t MediumEngineSizeWeight = 5u;
        uint32_t LargeEngineSizeWeight = 2u;
        uint32_t EngineNacelleChance = 35u;

        // Major structural surface features
        uint32_t MajorFeatureChance = 60u;
        uint32_t MaximumMajorFeatures = 2u;
        uint32_t MajorFeatureScalePercent = 100u;
        ShipMajorFeatureWeights MajorFeatureWeights;

        // Large visible weapons
        uint32_t LargeWeaponChance = 45u;
        uint32_t MaximumLargeWeaponGroups = 1u;
        uint32_t LargeWeaponSymmetryChance = 70u;
        uint32_t LargeWeaponScalePercent = 100u;
        ShipWeaponWeights LargeWeaponWeights;
        ShipWeaponHardpointWeights LargeWeaponHardpointWeights;

        uint32_t DetailDensityPercent = 100u;
        uint32_t MechanicalPatternCountPercent = 100u;

        // Per-ship repeated small/medium detail vocabulary. Motif geometry is
        // owned by the existing DETAILS domain and replaces part of the
        // free-form detail density rather than adding more overall noise.
        uint32_t DetailMotifChance = 88u;
        uint32_t SecondaryDetailMotifChance = 30u;
        uint32_t DetailMotifRepeatPercent = 100u;
        ShipDetailMotifWeights DetailMotifWeights;
        uint32_t DetailMotifMirroringBonusPercent = 0u;
        ShipDetailMotifPlacementBias DetailMotifPlacementBias = ShipDetailMotifPlacementBias::DEFAULT;
        ShipDetailMotifOrientationBias DetailMotifOrientationBias = ShipDetailMotifOrientationBias::DEFAULT;

        uint32_t AccentPanelWeight = 40u;
        uint32_t AccentStripeWeight = 30u;
        uint32_t AccentArmorWeight = 30u;

        uint32_t HorizontalVentChance = 60u;

        // Attachments
        ShipAttachmentWeights AttachmentWeights;
        uint32_t AttachmentChance = 55u;
        uint32_t MaximumAttachmentGroups = 2u;
        uint32_t SymmetricAttachmentChance = 75u;
        uint32_t AttachmentSizePercent = 100u;

        // Broad semantic material composition. Geometry is derived from
        // structural/HULL intent, while Palette rerolls recolor these masks.
        uint32_t MaterialCompositionChance = 88u;
        uint32_t MaximumMaterialZones = 3u;
        uint32_t MaterialSecondaryContrastPercent = 100u;
        ShipMaterialZoneWeights MaterialZoneWeights;
        bool MaterialWingSurfaceUsesFullWing = true;
        uint32_t MaterialAxialBandWidthPercent = 30u;

        // Large procedural graphic markings. Geometry is owned by the DETAILS
        // domain, while Palette rerolls only recolor the generated masks.
        uint32_t LiveryChance = 52u;
        uint32_t MaximumLiveryMarkings = 2u;
        uint32_t SupportingLiveryChance = 28u;
        uint32_t LiveryAsymmetricChance = 12u;
        uint32_t MaximumLiveryCoveragePercent = 18u;
        uint32_t MaximumLiveryConnectedCoveragePercent = 10u;
        ShipLiveryWeights LiveryWeights;

        // Coloring
        int32_t PaletteHullValueOffset = 0;
        uint32_t PaletteContrastPercent = 100u;
        uint32_t PaletteHullSaturationPercent = 100u;
        uint32_t PaletteAccentSaturationPercent = 100u;
        uint32_t PaletteEmissiveValuePercent = 100u;
        uint32_t SecondaryHullToneCoveragePercent = 8u;
        ShipHullSurfaceTone CoreRaisedSurfaceTone = ShipHullSurfaceTone::SECONDARY;
        ShipHullSurfaceTone CentralDorsalPlateTone = ShipHullSurfaceTone::SECONDARY;
        bool AxialRidgeUsesEdgeHighlight = false;

        // Occasional deliberate component-level macro asymmetry.
        uint32_t MacroAsymmetryChance = 10u;
        ShipMacroAsymmetryCategoryWeights MacroAsymmetryCategoryWeights;
        uint32_t MacroAsymmetryOuterRegionChance = 62u;
        uint32_t MacroAsymmetryWingRootRegionChance = 60u;
        uint32_t MacroAsymmetryVisualWeightPercent = 100u;

        // Detailing
        SupplementalSurfaceDetailWeights SupplementalDetailWeights;
    };

    const ShipGenerationProfile& getShipGenerationProfile(ShipStyle style);
}