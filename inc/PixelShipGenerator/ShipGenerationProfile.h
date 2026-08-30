#pragma once

#include "ShipAttachmentProfile.h"
#include "ShipCockpitType.h"
#include "ShipSurfaceDetailProfile.h"
#include "ShipMajorFeatureType.h"
#include "ShipStructuralNegativeSpaceType.h"
#include "ShipVisualAnchorType.h"
#include "ShipMaterialZoneType.h"
#include "ShipLiveryType.h"
#include "ShipWeaponType.h"

#include <cstdint>

namespace PixelShipGenerator
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
        ShipVisualAnchorWeights VisualAnchorWeights;
        uint32_t VisualSecondaryAnchorChance = 32u;

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

        // Deliberate structural voids cut during Hull generation. These use
        // the existing SILHOUETTE complexity budget and are preserved for
        // later placement stages through StructuralNegativeSpaceData.
        uint32_t StructuralNegativeSpaceChance = 28u;
        uint32_t MaximumStructuralNegativeSpaceStructures = 1u;
        uint32_t StructuralNegativeSpaceScalePercent = 100u;
        ShipStructuralNegativeSpaceWeights StructuralNegativeSpaceWeights;

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

        // Occasional deliberate component-level macro asymmetry.
        uint32_t MacroAsymmetryChance = 10u;

        // Detailing
        SupplementalSurfaceDetailWeights SupplementalDetailWeights;
    };

    const ShipGenerationProfile& getShipGenerationProfile(ShipStyle style);
}