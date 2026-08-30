#include "ShipFactionProfile.h"

#include <stdexcept>

namespace PixelShipGenerator
{
    uint32_t ShipFactionWeaponWeightMultipliers::getWeightPercent(ShipWeaponType type) const
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

    uint32_t ShipFactionMajorFeatureWeightMultipliers::getWeightPercent(ShipMajorFeatureType type) const
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

    uint32_t ShipFactionCockpitSizeWeightMultipliers::getWeightPercent(CockpitSizeClass sizeClass) const
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

    uint32_t ShipFactionCockpitShapeWeightMultipliers::getWeightPercent(CockpitShapeType shapeType) const
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

    uint32_t ShipFactionNegativeSpaceWeightMultipliers::getWeightPercent(ShipStructuralNegativeSpaceType type) const
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

    int32_t ShipFactionCoreTreatmentWeightOffsets::getOffset(ShipCoreTreatmentType type) const
    {
        switch (type)
        {
        case ShipCoreTreatmentType::CENTRAL_SPINE: return CentralSpine;
        case ShipCoreTreatmentType::COCKPIT_SURROUND: return CockpitSurround;
        case ShipCoreTreatmentType::RAISED_CORE_PLATE: return RaisedCorePlate;
        case ShipCoreTreatmentType::LATERAL_RECESSES: return LateralRecesses;
        case ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND: return LongitudinalArmorBand;
        case ShipCoreTreatmentType::CORE_CHANNEL: return CoreChannel;
        default: return 0;
        }
    }

    const ShipFactionHullLayerWeightAdjustment& ShipFactionHullLayerWeightAdjustments::getAdjustment(ShipHullLayerType type) const
    {
        switch (type)
        {
        case ShipHullLayerType::CENTRAL_DORSAL_PLATE: return CentralDorsalPlate;
        case ShipHullLayerType::FORWARD_ARMOR: return ForwardArmor;
        case ShipHullLayerType::WING_ARMOR: return WingArmor;
        case ShipHullLayerType::SHOULDER_ARMOR: return ShoulderArmor;
        case ShipHullLayerType::REAR_ENGINE_COVER: return RearEngineCover;
        default: return CentralDorsalPlate;
        }
    }

    uint32_t ShipFactionMaterialZoneWeightMultipliers::getWeightPercent(ShipMaterialZoneType type) const
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

    uint32_t ShipFactionLiveryWeightMultipliers::getWeightPercent(ShipLiveryType type) const
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

    uint32_t ShipFactionVisualAnchorWeightMultipliers::getWeightPercent(ShipVisualAnchorType type) const
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

    namespace
    {
        ShipFactionProfile createFrontierFactionProfile()
        {
            ShipFactionProfile result;

            result.Palette.HullHue = { 185u, 215u };
            result.Palette.HullSaturation = { 12u, 28u };
            result.Palette.HullValue = { 42u, 58u };
            result.Palette.Accent.HueOffset = { -35, 25 };
            result.Palette.Accent.Saturation = { 20u, 45u };
            result.Palette.Accent.Value = { 45u, 68u };
            result.Palette.Cockpit.HueOffset = { -5, 25 };
            result.Palette.Cockpit.Saturation = { 50u, 75u };
            result.Palette.Cockpit.Value = { 50u, 72u };
            result.Palette.Light.HueOffset = { -55, -20 };
            result.Palette.Light.Saturation = { 55u, 80u };
            result.Palette.Light.Value = { 70u, 92u };
            result.Palette.Exhaust.HueOffset = { 150, 180 };
            result.Palette.Exhaust.Saturation = { 70u, 95u };
            result.Palette.Exhaust.Value = { 85u, 100u };
            result.Palette.MechanicalSaturation = { 5u, 18u };
            result.Palette.MechanicalValue = { 24u, 40u };

            result.SurfaceDetails.DetailDensityPercent = 115u;
            result.SurfaceDetails.MechanicalPatternCountPercent = 135u;
            result.SurfaceDetails.LightPatternCountPercent = 70u;
            result.SurfaceDetails.AccentPanelWeightPercent = 135u;
            result.SurfaceDetails.AccentStripeWeightPercent = 70u;
            result.SurfaceDetails.AccentArmorWeightPercent = 110u;
            result.SurfaceDetails.HorizontalVentChancePercent = 145u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam = 160u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking = 45u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure = 170u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif = 40u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents = 135u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank = 120u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights = 75u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights = 70u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams = 95u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes = 110u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot = 135u;
            result.SurfaceDetails.MotifRepeatPercent = 92u;
            result.SurfaceDetails.AsymmetricDetailChanceOffset = 8;

            result.Attachments.WeightMultipliersPercent.WeaponMount = 90u;
            result.Attachments.WeightMultipliersPercent.SensorArray = 110u;
            result.Attachments.WeightMultipliersPercent.AuxiliaryPod = 150u;
            result.Attachments.WeightMultipliersPercent.Radiator = 145u;
            result.Attachments.WeightMultipliersPercent.ArmorFin = 95u;
            result.Attachments.WeightMultipliersPercent.TechnologyNode = 45u;
            result.Attachments.AttachmentChancePercent = 105u;
            result.Attachments.SymmetryChanceOffset = -5;

            result.Weapons.ChancePercent = 100u;
            result.Weapons.SymmetryChanceOffset = -15;
            result.Weapons.WeightMultipliersPercent.SingleCannon = 125u;
            result.Weapons.WeightMultipliersPercent.TwinCannon = 85u;
            result.Weapons.WeightMultipliersPercent.CompactTurret = 90u;
            result.Weapons.WeightMultipliersPercent.RailWeapon = 70u;
            result.Weapons.WeightMultipliersPercent.WeaponPod = 135u;
            result.Weapons.EmissiveChance = 10u;

            result.Engines.LayoutWeightMultipliersPercent.Central = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Twin = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Quad = 100u;
            result.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary = 100u;
            result.Engines.LayoutWeightMultipliersPercent.WideBank = 100u;
            result.Engines.SizeWeightMultipliersPercent.Small = 100u;
            result.Engines.SizeWeightMultipliersPercent.Medium = 100u;
            result.Engines.SizeWeightMultipliersPercent.Large = 100u;
            result.Engines.NacelleChancePercent = 100u;
            result.Engines.ExternalHeightPercent = 100u;

            result.MajorFeatures.ChancePercent = 110u;
            result.MajorFeatures.WeightMultipliersPercent.CentralSpine = 80u;
            result.MajorFeatures.WeightMultipliersPercent.ArmorPlate = 120u;
            result.MajorFeatures.WeightMultipliersPercent.RecessedBay = 130u;
            result.MajorFeatures.WeightMultipliersPercent.VentBank = 130u;
            result.MajorFeatures.WeightMultipliersPercent.WingPlate = 100u;
            result.MajorFeatures.WeightMultipliersPercent.TechCore = 50u;

            result.Cockpit.SizeWeightMultipliersPercent.Compact = 100u;
            result.Cockpit.SizeWeightMultipliersPercent.Standard = 100u;
            result.Cockpit.SizeWeightMultipliersPercent.Large = 112u;
            result.Cockpit.SizeWeightMultipliersPercent.Massive = 112u;
            result.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck = 118u;
            result.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge = 118u;

            result.Hull.NegativeSpaceChancePercent = 110u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel = 95u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.RearFork = 115u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap = 95u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay = 135u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel = 135u;
            result.Hull.PreferAlternateArticulationOrder = true;

            result.CoreTreatment.ChancePercent = 104u;
            result.CoreTreatment.WeightOffsets.RaisedCorePlate = 4;
            result.CoreTreatment.WeightOffsets.LateralRecesses = 4;
            result.CoreTreatment.CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::NONE;

            result.HullLayers.ChancePercent = 108u;
            result.HullLayers.WeightAdjustments.ForwardArmor.Offset = 20;
            result.HullLayers.WeightAdjustments.WingArmor.Offset = 15;
            result.HullLayers.WeightAdjustments.RearEngineCover.Offset = 15;

            result.Materials.ZoneWeightMultipliersPercent.WingSurface = 100u;
            result.Materials.ZoneWeightMultipliersPercent.ShoulderSurface = 100u;
            result.Materials.ZoneWeightMultipliersPercent.AxialBand = 100u;
            result.Materials.ZoneWeightMultipliersPercent.RearMechanical = 135u;
            result.Materials.ZoneWeightMultipliersPercent.CockpitCollar = 100u;
            result.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 135u;

            result.Livery.ChancePercent = 96u;
            result.Livery.WeightMultipliersPercent.CenterStripe = 115u;
            result.Livery.WeightMultipliersPercent.DoubleCenterStripe = 92u;
            result.Livery.WeightMultipliersPercent.WingBand = 92u;
            result.Livery.WeightMultipliersPercent.ShoulderBlock = 135u;
            result.Livery.WeightMultipliersPercent.NoseBand = 115u;
            result.Livery.WeightMultipliersPercent.Chevron = 92u;
            result.Livery.WeightMultipliersPercent.IdPanel = 135u;
            result.Livery.WeightMultipliersPercent.GeometricInsignia = 92u;
            result.Livery.AsymmetricChanceOffset = 20;
            result.Livery.AsymmetricChanceDivisor = 1u;
            result.Livery.AllowAsymmetricGeometricInsignia = true;

            result.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Wings = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Engines = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore = 80u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace = 125u;

            result.MacroAsymmetry.ChancePercent = 125u;
            result.Complexity.TotalBudgetPercent = 106u;
            result.Complexity.LegacyCategoryOffsets.HullLayer = 2;
            result.Complexity.LegacyCategoryOffsets.MajorFeature = -2;
            result.Complexity.LegacyCategoryOffsets.Attachment = 3;
            result.Complexity.LegacyCategoryOffsets.Detail = 2;
            result.Complexity.CategoryOffsets.CockpitStructure = 1;
            result.Complexity.CategoryOffsets.HullLayer = 2;
            result.Complexity.CategoryOffsets.MajorFeature = -2;
            result.Complexity.CategoryOffsets.Attachment = 3;
            result.Complexity.CategoryOffsets.Detail = 2;

            result.Finish.WeaponBodyRole = ShipFactionPaintColorRole::MECHANICAL_BASE;

            result.Animation.Idle.EngineMechanicalChanceOffset = 2u;
            result.Animation.Idle.EngineMechanicalChanceMaximum = 100u;
            result.Animation.Idle.WeaponMechanicalChanceOffset = 10u;
            result.Animation.Idle.WeaponMechanicalChanceMaximum = 100u;
            result.Animation.Idle.IrregularEngineCycle = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.AsynchronousEngines = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.RandomizeSymmetricWeaponAlternatePhase = true;
            result.Animation.LateralMovement.Staggered = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LateralMovement.Synchronized = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LongitudinalMovement.Staggered = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LongitudinalMovement.Synchronized = ShipFactionAnimationBooleanOverride::DISABLE;

            return result;
        }

        ShipFactionProfile createMilitaryFactionProfile()
        {
            ShipFactionProfile result;

            result.Palette.HullHue = { 80u, 135u };
            result.Palette.HullSaturation = { 8u, 24u };
            result.Palette.HullValue = { 34u, 50u };
            result.Palette.Accent.HueOffset = { 230, 300 };
            result.Palette.Accent.Saturation = { 55u, 85u };
            result.Palette.Accent.Value = { 65u, 90u };
            result.Palette.Cockpit.HueOffset = { 70, 130 };
            result.Palette.Cockpit.Saturation = { 45u, 70u };
            result.Palette.Cockpit.Value = { 50u, 72u };
            result.Palette.Light.HueOffset = { -110, -60 };
            result.Palette.Light.Saturation = { 70u, 95u };
            result.Palette.Light.Value = { 75u, 100u };
            result.Palette.Exhaust.HueOffset = { -100, -50 };
            result.Palette.Exhaust.Saturation = { 75u, 100u };
            result.Palette.Exhaust.Value = { 88u, 100u };
            result.Palette.MechanicalSaturation = { 4u, 14u };
            result.Palette.MechanicalValue = { 20u, 34u };

            result.SurfaceDetails.DetailDensityPercent = 90u;
            result.SurfaceDetails.MechanicalPatternCountPercent = 80u;
            result.SurfaceDetails.LightPatternCountPercent = 55u;
            result.SurfaceDetails.AccentPanelWeightPercent = 110u;
            result.SurfaceDetails.AccentStripeWeightPercent = 175u;
            result.SurfaceDetails.AccentArmorWeightPercent = 120u;
            result.SurfaceDetails.HorizontalVentChancePercent = 65u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam = 150u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking = 85u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure = 35u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif = 70u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents = 125u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank = 115u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights = 120u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights = 110u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams = 130u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes = 100u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot = 110u;
            result.SurfaceDetails.MotifRepeatPercent = 112u;
            result.SurfaceDetails.AsymmetricDetailChanceOffset = -8;

            result.Attachments.WeightMultipliersPercent.WeaponMount = 155u;
            result.Attachments.WeightMultipliersPercent.SensorArray = 95u;
            result.Attachments.WeightMultipliersPercent.AuxiliaryPod = 80u;
            result.Attachments.WeightMultipliersPercent.Radiator = 55u;
            result.Attachments.WeightMultipliersPercent.ArmorFin = 150u;
            result.Attachments.WeightMultipliersPercent.TechnologyNode = 45u;
            result.Attachments.AttachmentChancePercent = 105u;
            result.Attachments.SymmetryChanceOffset = 15;

            result.Weapons.ChancePercent = 120u;
            result.Weapons.SymmetryChanceOffset = 15;
            result.Weapons.WeightMultipliersPercent.SingleCannon = 120u;
            result.Weapons.WeightMultipliersPercent.TwinCannon = 145u;
            result.Weapons.WeightMultipliersPercent.CompactTurret = 105u;
            result.Weapons.WeightMultipliersPercent.RailWeapon = 110u;
            result.Weapons.WeightMultipliersPercent.WeaponPod = 115u;
            result.Weapons.EmissiveChance = 20u;

            result.Engines.LayoutWeightMultipliersPercent.Central = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Twin = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Quad = 100u;
            result.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary = 100u;
            result.Engines.LayoutWeightMultipliersPercent.WideBank = 100u;
            result.Engines.SizeWeightMultipliersPercent.Small = 100u;
            result.Engines.SizeWeightMultipliersPercent.Medium = 100u;
            result.Engines.SizeWeightMultipliersPercent.Large = 100u;
            result.Engines.NacelleChancePercent = 100u;
            result.Engines.ExternalHeightPercent = 100u;

            result.MajorFeatures.ChancePercent = 100u;
            result.MajorFeatures.WeightMultipliersPercent.CentralSpine = 100u;
            result.MajorFeatures.WeightMultipliersPercent.ArmorPlate = 130u;
            result.MajorFeatures.WeightMultipliersPercent.RecessedBay = 90u;
            result.MajorFeatures.WeightMultipliersPercent.VentBank = 110u;
            result.MajorFeatures.WeightMultipliersPercent.WingPlate = 120u;
            result.MajorFeatures.WeightMultipliersPercent.TechCore = 70u;

            result.Cockpit.SizeWeightMultipliersPercent.Compact = 102u;
            result.Cockpit.SizeWeightMultipliersPercent.Standard = 112u;
            result.Cockpit.SizeWeightMultipliersPercent.Large = 102u;
            result.Cockpit.SizeWeightMultipliersPercent.Massive = 88u;
            result.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck = 120u;
            result.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy = 88u;
            result.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge = 120u;
            result.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge = 100u;

            result.Hull.NegativeSpaceChancePercent = 95u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel = 120u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.RearFork = 100u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap = 115u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay = 75u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel = 100u;
            result.Hull.PreferAlternateArticulationOrder = false;

            result.CoreTreatment.ChancePercent = 106u;
            result.CoreTreatment.WeightOffsets.CockpitSurround = 5;
            result.CoreTreatment.WeightOffsets.LongitudinalArmorBand = 5;
            result.CoreTreatment.CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::NONE;

            result.HullLayers.ChancePercent = 105u;
            result.HullLayers.WeightAdjustments.ForwardArmor.Offset = 20;
            result.HullLayers.WeightAdjustments.ShoulderArmor.Offset = 15;

            result.Materials.ZoneWeightMultipliersPercent.WingSurface = 125u;
            result.Materials.ZoneWeightMultipliersPercent.ShoulderSurface = 125u;
            result.Materials.ZoneWeightMultipliersPercent.AxialBand = 95u;
            result.Materials.ZoneWeightMultipliersPercent.RearMechanical = 95u;
            result.Materials.ZoneWeightMultipliersPercent.CockpitCollar = 95u;
            result.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 125u;

            result.Livery.ChancePercent = 118u;
            result.Livery.WeightMultipliersPercent.CenterStripe = 105u;
            result.Livery.WeightMultipliersPercent.DoubleCenterStripe = 105u;
            result.Livery.WeightMultipliersPercent.WingBand = 135u;
            result.Livery.WeightMultipliersPercent.ShoulderBlock = 135u;
            result.Livery.WeightMultipliersPercent.NoseBand = 135u;
            result.Livery.WeightMultipliersPercent.Chevron = 105u;
            result.Livery.WeightMultipliersPercent.IdPanel = 105u;
            result.Livery.WeightMultipliersPercent.GeometricInsignia = 90u;
            result.Livery.AsymmetricChanceOffset = 0;
            result.Livery.AsymmetricChanceDivisor = 3u;
            result.Livery.AllowAsymmetricGeometricInsignia = false;

            result.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Wings = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Engines = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry = 70u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace = 100u;

            result.MacroAsymmetry.ChancePercent = 65u;
            result.Complexity.TotalBudgetPercent = 98u;
            result.Complexity.LegacyCategoryOffsets.HullLayer = 2;
            result.Complexity.LegacyCategoryOffsets.MajorFeature = 1;
            result.Complexity.LegacyCategoryOffsets.LargeWeapon = 2;
            result.Complexity.LegacyCategoryOffsets.Detail = -2;
            result.Complexity.CategoryOffsets.CockpitStructure = 2;
            result.Complexity.CategoryOffsets.HullLayer = 2;
            result.Complexity.CategoryOffsets.MajorFeature = 1;
            result.Complexity.CategoryOffsets.LargeWeapon = 2;
            result.Complexity.CategoryOffsets.Detail = -2;

            result.Finish.WeaponBodyRole = ShipFactionPaintColorRole::HULL_SECONDARY;

            result.Animation.Idle.ExhaustAmplitudeScale = { 4u, 5u };
            result.Animation.Idle.SynchronizeEngines = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.AsynchronousEngines = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LateralMovement.Synchronized = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LateralMovement.Staggered = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LateralMovement.ResponseStrengthScale = { 9u, 10u };
            result.Animation.LongitudinalMovement.Synchronized = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LongitudinalMovement.Staggered = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LongitudinalMovement.ResponseStrengthScale = { 9u, 10u };
            result.Animation.Firing.DurationScale = { 9u, 10u };

            return result;
        }

        ShipFactionProfile createAscendantFactionProfile()
        {
            ShipFactionProfile result;

            result.Palette.HullHue = { 185u, 235u };
            result.Palette.HullSaturation = { 8u, 20u };
            result.Palette.HullValue = { 70u, 88u };
            result.Palette.Accent.HueOffset = { 20, 90 };
            result.Palette.Accent.Saturation = { 35u, 65u };
            result.Palette.Accent.Value = { 70u, 95u };
            result.Palette.Cockpit.HueOffset = { 0, 55 };
            result.Palette.Cockpit.Saturation = { 35u, 60u };
            result.Palette.Cockpit.Value = { 70u, 90u };
            result.Palette.Light.HueOffset = { 40, 120 };
            result.Palette.Light.Saturation = { 55u, 85u };
            result.Palette.Light.Value = { 90u, 100u };
            result.Palette.Exhaust.HueOffset = { 20, 100 };
            result.Palette.Exhaust.Saturation = { 40u, 75u };
            result.Palette.Exhaust.Value = { 90u, 100u };
            result.Palette.MechanicalSaturation = { 5u, 15u };
            result.Palette.MechanicalValue = { 45u, 60u };

            result.SurfaceDetails.DetailDensityPercent = 65u;
            result.SurfaceDetails.MechanicalPatternCountPercent = 25u;
            result.SurfaceDetails.LightPatternCountPercent = 80u;
            result.SurfaceDetails.AccentPanelWeightPercent = 40u;
            result.SurfaceDetails.AccentStripeWeightPercent = 90u;
            result.SurfaceDetails.AccentArmorWeightPercent = 30u;
            result.SurfaceDetails.HorizontalVentChancePercent = 15u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam = 35u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking = 210u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure = 10u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif = 90u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents = 35u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank = 20u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights = 145u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights = 155u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams = 150u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes = 95u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot = 55u;
            result.SurfaceDetails.MotifRepeatPercent = 88u;
            result.SurfaceDetails.AsymmetricDetailChanceOffset = -6;

            result.Attachments.WeightMultipliersPercent.WeaponMount = 80u;
            result.Attachments.WeightMultipliersPercent.SensorArray = 130u;
            result.Attachments.WeightMultipliersPercent.AuxiliaryPod = 55u;
            result.Attachments.WeightMultipliersPercent.Radiator = 35u;
            result.Attachments.WeightMultipliersPercent.ArmorFin = 75u;
            result.Attachments.WeightMultipliersPercent.TechnologyNode = 200u;
            result.Attachments.AttachmentChancePercent = 100u;
            result.Attachments.SymmetryChanceOffset = 10;

            result.Weapons.ChancePercent = 85u;
            result.Weapons.SymmetryChanceOffset = 10;
            result.Weapons.WeightMultipliersPercent.SingleCannon = 60u;
            result.Weapons.WeightMultipliersPercent.TwinCannon = 80u;
            result.Weapons.WeightMultipliersPercent.CompactTurret = 125u;
            result.Weapons.WeightMultipliersPercent.RailWeapon = 165u;
            result.Weapons.WeightMultipliersPercent.WeaponPod = 55u;
            result.Weapons.EmissiveChance = 80u;

            result.Engines.LayoutWeightMultipliersPercent.Central = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Twin = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Quad = 100u;
            result.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary = 100u;
            result.Engines.LayoutWeightMultipliersPercent.WideBank = 100u;
            result.Engines.SizeWeightMultipliersPercent.Small = 100u;
            result.Engines.SizeWeightMultipliersPercent.Medium = 100u;
            result.Engines.SizeWeightMultipliersPercent.Large = 100u;
            result.Engines.NacelleChancePercent = 100u;
            result.Engines.ExternalHeightPercent = 100u;

            result.MajorFeatures.ChancePercent = 90u;
            result.MajorFeatures.WeightMultipliersPercent.CentralSpine = 130u;
            result.MajorFeatures.WeightMultipliersPercent.ArmorPlate = 80u;
            result.MajorFeatures.WeightMultipliersPercent.RecessedBay = 60u;
            result.MajorFeatures.WeightMultipliersPercent.VentBank = 50u;
            result.MajorFeatures.WeightMultipliersPercent.WingPlate = 90u;
            result.MajorFeatures.WeightMultipliersPercent.TechCore = 160u;

            result.Cockpit.SizeWeightMultipliersPercent.Compact = 100u;
            result.Cockpit.SizeWeightMultipliersPercent.Standard = 100u;
            result.Cockpit.SizeWeightMultipliersPercent.Large = 118u;
            result.Cockpit.SizeWeightMultipliersPercent.Massive = 92u;
            result.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy = 120u;
            result.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck = 90u;
            result.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge = 120u;
            result.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge = 100u;

            result.Hull.NegativeSpaceChancePercent = 88u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel = 135u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.RearFork = 85u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap = 125u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay = 65u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel = 80u;
            result.Hull.PreferAlternateArticulationOrder = false;

            result.CoreTreatment.ChancePercent = 94u;
            result.CoreTreatment.WeightOffsets.CentralSpine = 6;
            result.CoreTreatment.WeightOffsets.LateralRecesses = -5;
            result.CoreTreatment.WeightOffsets.CoreChannel = 8;
            result.CoreTreatment.CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::EXCEPT_EVERY_THIRD_ROW;

            result.HullLayers.ChancePercent = 88u;
            result.HullLayers.WeightAdjustments.CentralDorsalPlate.Offset = 35;
            result.HullLayers.WeightAdjustments.ForwardArmor.Offset = 10;
            result.HullLayers.WeightAdjustments.RearEngineCover.Scale = { 1u, 2u };

            result.Materials.ZoneWeightMultipliersPercent.WingSurface = 90u;
            result.Materials.ZoneWeightMultipliersPercent.ShoulderSurface = 90u;
            result.Materials.ZoneWeightMultipliersPercent.AxialBand = 130u;
            result.Materials.ZoneWeightMultipliersPercent.RearMechanical = 70u;
            result.Materials.ZoneWeightMultipliersPercent.CockpitCollar = 130u;
            result.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 90u;

            result.Livery.ChancePercent = 78u;
            result.Livery.WeightMultipliersPercent.CenterStripe = 125u;
            result.Livery.WeightMultipliersPercent.DoubleCenterStripe = 90u;
            result.Livery.WeightMultipliersPercent.WingBand = 90u;
            result.Livery.WeightMultipliersPercent.ShoulderBlock = 70u;
            result.Livery.WeightMultipliersPercent.NoseBand = 90u;
            result.Livery.WeightMultipliersPercent.Chevron = 125u;
            result.Livery.WeightMultipliersPercent.IdPanel = 70u;
            result.Livery.WeightMultipliersPercent.GeometricInsignia = 125u;
            result.Livery.AsymmetricChanceOffset = 0;
            result.Livery.AsymmetricChanceDivisor = 2u;
            result.Livery.AllowAsymmetricGeometricInsignia = true;

            result.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Wings = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Engines = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore = 125u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace = 90u;

            result.MacroAsymmetry.ChancePercent = 72u;
            result.Complexity.TotalBudgetPercent = 92u;
            result.Complexity.LegacyCategoryOffsets.Silhouette = -2;
            result.Complexity.LegacyCategoryOffsets.HullLayer = 1;
            result.Complexity.LegacyCategoryOffsets.MajorFeature = 3;
            result.Complexity.LegacyCategoryOffsets.Attachment = -2;
            result.Complexity.CategoryOffsets.Silhouette = -2;
            result.Complexity.CategoryOffsets.CockpitStructure = 2;
            result.Complexity.CategoryOffsets.HullLayer = 1;
            result.Complexity.CategoryOffsets.MajorFeature = 3;
            result.Complexity.CategoryOffsets.Attachment = -2;

            result.PaletteBehavior.SecondaryToneDirection = ShipFactionSecondaryToneDirection::DARKER;

            result.Finish.WeaponMuzzleRole = ShipFactionPaintColorRole::HULL_ACCENT_HIGHLIGHT;
            result.Finish.WeaponBodyRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CoreSecondaryMaterialRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CoreRaisedRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CoreLuminousRole = ShipFactionPaintColorRole::LIGHT_HIGHLIGHT;
            result.Finish.CentralDorsalPlateRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CockpitFrameRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.ForceAxialRidgeEdgeHighlight = true;

            result.Animation.Idle.TechPulseStrength = 2u;
            result.Animation.Idle.ExhaustAmplitudeScale = { 9u, 10u };
            result.Animation.Idle.EngineMechanicalChanceMaximum = 1u;
            result.Animation.Idle.WeaponMechanicalChanceScale = { 3u, 4u };
            result.Animation.Idle.WeaponMechanicalChanceMinimum = 20u;
            result.Animation.LateralMovement.ResponseStrengthScale = { 4u, 5u };
            result.Animation.LongitudinalMovement.ResponseStrengthScale = { 4u, 5u };
            result.Animation.Firing.ResponseStrengthScale = { 4u, 5u };
            result.Animation.Firing.MaximumPreFireExtensionLimit = 1u;

            return result;
        }

        ShipFactionProfile createXenoFactionProfile()
        {
            ShipFactionProfile result;

            result.Palette.HullHue = { 250u, 340u };
            result.Palette.HullSaturation = { 35u, 65u };
            result.Palette.HullValue = { 40u, 65u };
            result.Palette.Accent.HueOffset = { 80, 160 };
            result.Palette.Accent.Saturation = { 55u, 85u };
            result.Palette.Accent.Value = { 55u, 85u };
            result.Palette.Cockpit.HueOffset = { 120, 220 };
            result.Palette.Cockpit.Saturation = { 45u, 75u };
            result.Palette.Cockpit.Value = { 45u, 75u };
            result.Palette.Light.HueOffset = { 80, 220 };
            result.Palette.Light.Saturation = { 70u, 95u };
            result.Palette.Light.Value = { 85u, 100u };
            result.Palette.Exhaust.HueOffset = { 120, 240 };
            result.Palette.Exhaust.Saturation = { 60u, 90u };
            result.Palette.Exhaust.Value = { 80u, 100u };
            result.Palette.MechanicalSaturation = { 20u, 40u };
            result.Palette.MechanicalValue = { 20u, 40u };

            result.SurfaceDetails.DetailDensityPercent = 95u;
            result.SurfaceDetails.MechanicalPatternCountPercent = 50u;
            result.SurfaceDetails.LightPatternCountPercent = 105u;
            result.SurfaceDetails.AccentPanelWeightPercent = 35u;
            result.SurfaceDetails.AccentStripeWeightPercent = 45u;
            result.SurfaceDetails.AccentArmorWeightPercent = 55u;
            result.SurfaceDetails.HorizontalVentChancePercent = 25u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam = 45u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking = 180u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure = 45u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif = 260u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents = 55u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank = 45u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights = 90u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights = 150u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams = 65u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes = 165u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot = 130u;
            result.SurfaceDetails.MotifRepeatPercent = 106u;
            result.SurfaceDetails.AsymmetricDetailChanceOffset = 18;

            result.Attachments.WeightMultipliersPercent.WeaponMount = 95u;
            result.Attachments.WeightMultipliersPercent.SensorArray = 115u;
            result.Attachments.WeightMultipliersPercent.AuxiliaryPod = 45u;
            result.Attachments.WeightMultipliersPercent.Radiator = 40u;
            result.Attachments.WeightMultipliersPercent.ArmorFin = 115u;
            result.Attachments.WeightMultipliersPercent.TechnologyNode = 225u;
            result.Attachments.AttachmentChancePercent = 100u;
            result.Attachments.SymmetryChanceOffset = -20;

            result.Weapons.ChancePercent = 100u;
            result.Weapons.SymmetryChanceOffset = -5;
            result.Weapons.WeightMultipliersPercent.SingleCannon = 75u;
            result.Weapons.WeightMultipliersPercent.TwinCannon = 95u;
            result.Weapons.WeightMultipliersPercent.CompactTurret = 135u;
            result.Weapons.WeightMultipliersPercent.RailWeapon = 145u;
            result.Weapons.WeightMultipliersPercent.WeaponPod = 115u;
            result.Weapons.EmissiveChance = 65u;

            result.Engines.LayoutWeightMultipliersPercent.Central = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Twin = 100u;
            result.Engines.LayoutWeightMultipliersPercent.Quad = 100u;
            result.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary = 100u;
            result.Engines.LayoutWeightMultipliersPercent.WideBank = 100u;
            result.Engines.SizeWeightMultipliersPercent.Small = 100u;
            result.Engines.SizeWeightMultipliersPercent.Medium = 100u;
            result.Engines.SizeWeightMultipliersPercent.Large = 100u;
            result.Engines.NacelleChancePercent = 100u;
            result.Engines.ExternalHeightPercent = 100u;

            result.MajorFeatures.ChancePercent = 100u;
            result.MajorFeatures.WeightMultipliersPercent.CentralSpine = 90u;
            result.MajorFeatures.WeightMultipliersPercent.ArmorPlate = 70u;
            result.MajorFeatures.WeightMultipliersPercent.RecessedBay = 70u;
            result.MajorFeatures.WeightMultipliersPercent.VentBank = 80u;
            result.MajorFeatures.WeightMultipliersPercent.WingPlate = 110u;
            result.MajorFeatures.WeightMultipliersPercent.TechCore = 160u;

            result.Cockpit.SizeWeightMultipliersPercent.Compact = 96u;
            result.Cockpit.SizeWeightMultipliersPercent.Standard = 96u;
            result.Cockpit.SizeWeightMultipliersPercent.Large = 115u;
            result.Cockpit.SizeWeightMultipliersPercent.Massive = 115u;
            result.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy = 96u;
            result.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy = 96u;
            result.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck = 96u;
            result.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy = 135u;
            result.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge = 96u;
            result.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge = 135u;

            result.Hull.NegativeSpaceChancePercent = 105u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel = 90u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.RearFork = 110u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap = 125u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay = 125u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel = 90u;
            result.Hull.PreferAlternateArticulationOrder = true;

            result.CoreTreatment.ChancePercent = 100u;
            result.CoreTreatment.WeightOffsets.LateralRecesses = 5;
            result.CoreTreatment.WeightOffsets.CoreChannel = 5;
            result.CoreTreatment.CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::EXCEPT_EVERY_THIRD_ROW;

            result.HullLayers.ChancePercent = 100u;
            result.HullLayers.WeightAdjustments.CentralDorsalPlate.Offset = 15;
            result.HullLayers.WeightAdjustments.WingArmor.Offset = 20;

            result.Materials.ZoneWeightMultipliersPercent.WingSurface = 125u;
            result.Materials.ZoneWeightMultipliersPercent.ShoulderSurface = 100u;
            result.Materials.ZoneWeightMultipliersPercent.AxialBand = 125u;
            result.Materials.ZoneWeightMultipliersPercent.RearMechanical = 100u;
            result.Materials.ZoneWeightMultipliersPercent.CockpitCollar = 100u;
            result.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 100u;

            result.Livery.ChancePercent = 98u;
            result.Livery.WeightMultipliersPercent.CenterStripe = 90u;
            result.Livery.WeightMultipliersPercent.DoubleCenterStripe = 135u;
            result.Livery.WeightMultipliersPercent.WingBand = 90u;
            result.Livery.WeightMultipliersPercent.ShoulderBlock = 90u;
            result.Livery.WeightMultipliersPercent.NoseBand = 90u;
            result.Livery.WeightMultipliersPercent.Chevron = 135u;
            result.Livery.WeightMultipliersPercent.IdPanel = 90u;
            result.Livery.WeightMultipliersPercent.GeometricInsignia = 135u;
            result.Livery.AsymmetricChanceOffset = 15;
            result.Livery.AsymmetricChanceDivisor = 1u;
            result.Livery.AllowAsymmetricGeometricInsignia = true;

            result.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette = 120u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Wings = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Engines = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature = 120u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace = 120u;

            result.MacroAsymmetry.ChancePercent = 135u;
            result.Complexity.TotalBudgetPercent = 102u;
            result.Complexity.LegacyCategoryOffsets.HullLayer = 1;
            result.Complexity.LegacyCategoryOffsets.MajorFeature = 1;
            result.Complexity.LegacyCategoryOffsets.Attachment = 1;
            result.Complexity.CategoryOffsets.CockpitStructure = 1;
            result.Complexity.CategoryOffsets.HullLayer = 1;
            result.Complexity.CategoryOffsets.MajorFeature = 1;
            result.Complexity.CategoryOffsets.Attachment = 1;

            result.Finish.WeaponMuzzleRole = ShipFactionPaintColorRole::HULL_ACCENT_HIGHLIGHT;
            result.Finish.WeaponBodyRole = ShipFactionPaintColorRole::HULL_ACCENT;
            result.Finish.WeaponRaisedHighlightRole = ShipFactionPaintColorRole::HULL_ACCENT_HIGHLIGHT;
            result.Finish.CoreLuminousRole = ShipFactionPaintColorRole::HULL_ACCENT;
            result.Finish.CoreLuminousHighlightRole = ShipFactionPaintColorRole::HULL_ACCENT_HIGHLIGHT;

            result.Animation.Idle.AlternateEnginePhases = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.AsynchronousEngines = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.AlternateWeaponPhases = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.TechPulseStrength = 2u;
            result.Animation.Idle.AlternateTechCorePhases = true;
            result.Animation.LateralMovement.Staggered = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LateralMovement.Synchronized = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LongitudinalMovement.Staggered = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LongitudinalMovement.Synchronized = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LongitudinalMovement.MinimumExhaustVariationLimit = 1u;

            return result;
        }

        ShipFactionProfile createCorporateFactionProfile()
        {
            ShipFactionProfile result;

            // Commercial finishes stay near-neutral so a single disciplined
            // identification color reads clearly as a manufactured brand cue.
            result.Palette.HullHue = { 175u, 245u };
            result.Palette.HullSaturation = { 4u, 18u };
            result.Palette.HullValue = { 48u, 82u };
            result.Palette.Accent.HueOffset = { 70, 285 };
            result.Palette.Accent.Saturation = { 70u, 95u };
            result.Palette.Accent.Value = { 68u, 96u };
            result.Palette.Cockpit.HueOffset = { -10, 45 };
            result.Palette.Cockpit.Saturation = { 35u, 62u };
            result.Palette.Cockpit.Value = { 64u, 90u };
            result.Palette.Light.HueOffset = { 65, 160 };
            result.Palette.Light.Saturation = { 58u, 86u };
            result.Palette.Light.Value = { 86u, 100u };
            result.Palette.Exhaust.HueOffset = { 140, 205 };
            result.Palette.Exhaust.Saturation = { 55u, 82u };
            result.Palette.Exhaust.Value = { 90u, 100u };
            result.Palette.MechanicalSaturation = { 2u, 12u };
            result.Palette.MechanicalValue = { 20u, 36u };

            result.SurfaceDetails.DetailDensityPercent = 92u;
            result.SurfaceDetails.MechanicalPatternCountPercent = 58u;
            result.SurfaceDetails.LightPatternCountPercent = 72u;
            result.SurfaceDetails.AccentPanelWeightPercent = 70u;
            result.SurfaceDetails.AccentStripeWeightPercent = 220u;
            result.SurfaceDetails.AccentArmorWeightPercent = 72u;
            result.SurfaceDetails.HorizontalVentChancePercent = 72u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam = 190u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking = 135u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure = 12u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif = 115u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.IdentificationMarking = 185u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.LuminousChannel = 18u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents = 90u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank = 75u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights = 140u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights = 115u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams = 165u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes = 135u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot = 120u;
            result.SurfaceDetails.MotifRepeatPercent = 122u;
            result.SurfaceDetails.AsymmetricDetailChanceOffset = -14;

            result.Attachments.WeightMultipliersPercent.WeaponMount = 110u;
            result.Attachments.WeightMultipliersPercent.SensorArray = 120u;
            result.Attachments.WeightMultipliersPercent.AuxiliaryPod = 135u;
            result.Attachments.WeightMultipliersPercent.Radiator = 55u;
            result.Attachments.WeightMultipliersPercent.ArmorFin = 82u;
            result.Attachments.WeightMultipliersPercent.TechnologyNode = 95u;
            result.Attachments.AttachmentChancePercent = 92u;
            result.Attachments.SymmetryChanceOffset = 20;

            result.Weapons.ChancePercent = 108u;
            result.Weapons.SymmetryChanceOffset = 24;
            result.Weapons.WeightMultipliersPercent.SingleCannon = 82u;
            result.Weapons.WeightMultipliersPercent.TwinCannon = 142u;
            result.Weapons.WeightMultipliersPercent.CompactTurret = 100u;
            result.Weapons.WeightMultipliersPercent.RailWeapon = 88u;
            result.Weapons.WeightMultipliersPercent.WeaponPod = 155u;
            result.Weapons.EmissiveChance = 28u;

            result.Engines.LayoutWeightMultipliersPercent.Central = 78u;
            result.Engines.LayoutWeightMultipliersPercent.Twin = 132u;
            result.Engines.LayoutWeightMultipliersPercent.Quad = 112u;
            result.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary = 108u;
            result.Engines.LayoutWeightMultipliersPercent.WideBank = 118u;
            result.Engines.SizeWeightMultipliersPercent.Small = 55u;
            result.Engines.SizeWeightMultipliersPercent.Medium = 170u;
            result.Engines.SizeWeightMultipliersPercent.Large = 70u;
            result.Engines.NacelleChancePercent = 180u;
            result.Engines.ExternalHeightPercent = 92u;

            result.MajorFeatures.ChancePercent = 98u;
            result.MajorFeatures.WeightMultipliersPercent.CentralSpine = 100u;
            result.MajorFeatures.WeightMultipliersPercent.ArmorPlate = 135u;
            result.MajorFeatures.WeightMultipliersPercent.RecessedBay = 115u;
            result.MajorFeatures.WeightMultipliersPercent.VentBank = 68u;
            result.MajorFeatures.WeightMultipliersPercent.WingPlate = 122u;
            result.MajorFeatures.WeightMultipliersPercent.TechCore = 95u;

            result.Cockpit.SizeWeightMultipliersPercent.Compact = 88u;
            result.Cockpit.SizeWeightMultipliersPercent.Standard = 112u;
            result.Cockpit.SizeWeightMultipliersPercent.Large = 122u;
            result.Cockpit.SizeWeightMultipliersPercent.Massive = 96u;
            result.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy = 100u;
            result.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy = 125u;
            result.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck = 165u;
            result.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy = 70u;
            result.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge = 92u;
            result.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge = 100u;

            result.Hull.NegativeSpaceChancePercent = 92u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel = 125u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.RearFork = 75u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap = 140u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay = 95u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel = 115u;
            result.Hull.PreferAlternateArticulationOrder = false;

            result.CoreTreatment.ChancePercent = 108u;
            result.CoreTreatment.WeightOffsets.CockpitSurround = 9;
            result.CoreTreatment.WeightOffsets.RaisedCorePlate = 7;
            result.CoreTreatment.WeightOffsets.LateralRecesses = -3;
            result.CoreTreatment.WeightOffsets.LongitudinalArmorBand = 10;
            result.CoreTreatment.CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::NONE;

            result.HullLayers.ChancePercent = 106u;
            result.HullLayers.WeightAdjustments.CentralDorsalPlate.Offset = 18;
            result.HullLayers.WeightAdjustments.ForwardArmor.Offset = 22;
            result.HullLayers.WeightAdjustments.ShoulderArmor.Offset = 18;
            result.HullLayers.WeightAdjustments.RearEngineCover.Offset = 8;

            result.Materials.ZoneWeightMultipliersPercent.WingSurface = 135u;
            result.Materials.ZoneWeightMultipliersPercent.ShoulderSurface = 135u;
            result.Materials.ZoneWeightMultipliersPercent.AxialBand = 100u;
            result.Materials.ZoneWeightMultipliersPercent.RearMechanical = 85u;
            result.Materials.ZoneWeightMultipliersPercent.CockpitCollar = 135u;
            result.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 100u;

            result.Livery.ChancePercent = 145u;
            result.Livery.WeightMultipliersPercent.CenterStripe = 150u;
            result.Livery.WeightMultipliersPercent.DoubleCenterStripe = 150u;
            result.Livery.WeightMultipliersPercent.WingBand = 125u;
            result.Livery.WeightMultipliersPercent.ShoulderBlock = 125u;
            result.Livery.WeightMultipliersPercent.NoseBand = 110u;
            result.Livery.WeightMultipliersPercent.Chevron = 110u;
            result.Livery.WeightMultipliersPercent.IdPanel = 150u;
            result.Livery.WeightMultipliersPercent.GeometricInsignia = 150u;
            result.Livery.AsymmetricChanceOffset = 28;
            result.Livery.AsymmetricChanceDivisor = 1u;
            result.Livery.AllowAsymmetricGeometricInsignia = true;

            result.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit = 120u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Wings = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Engines = 120u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers = 120u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry = 65u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace = 100u;

            result.MacroAsymmetry.ChancePercent = 58u;
            result.Complexity.TotalBudgetPercent = 98u;
            result.Complexity.LegacyCategoryOffsets.HullLayer = 2;
            result.Complexity.LegacyCategoryOffsets.LargeWeapon = 1;
            result.Complexity.LegacyCategoryOffsets.Attachment = -1;
            result.Complexity.LegacyCategoryOffsets.Detail = 1;
            result.Complexity.CategoryOffsets.CockpitStructure = 2;
            result.Complexity.CategoryOffsets.HullLayer = 2;
            result.Complexity.CategoryOffsets.LargeWeapon = 1;
            result.Complexity.CategoryOffsets.Attachment = -1;
            result.Complexity.CategoryOffsets.Detail = 1;

            result.PaletteBehavior.HullValueMode = ShipFactionHullValueMode::ALTERNATING_BRIGHT_DARK_RANGES;
            result.PaletteBehavior.BrightHullValue = { 68u, 84u };
            result.PaletteBehavior.DarkHullValue = { 32u, 48u };
            result.PaletteBehavior.SecondaryToneDirection = ShipFactionSecondaryToneDirection::CONTRAST_FROM_HULL_MIDPOINT;
            result.PaletteBehavior.MinimumAccentHueDistance = 65u;
            result.PaletteBehavior.AccentHueSeparationShiftA = 120u;
            result.PaletteBehavior.AccentHueSeparationShiftB = 210u;

            result.Finish.WeaponBodyRole = ShipFactionPaintColorRole::HULL_SECONDARY;
            result.Finish.CoreSecondaryMaterialRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CoreRaisedRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CentralDorsalPlateRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CockpitBaseRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.CockpitFrameRole = ShipFactionPaintColorRole::HULL_HIGHLIGHT;
            result.Finish.ForceAxialRidgeEdgeHighlight = true;

            result.Animation.Idle.ExhaustAmplitudeScale = { 3u, 4u };
            result.Animation.Idle.EngineMechanicalChanceMaximum = 2u;
            result.Animation.Idle.WeaponMechanicalChanceScale = { 4u, 5u };
            result.Animation.Idle.WeaponMechanicalChanceMinimum = 24u;
            result.Animation.Idle.VentActivityChanceScale = { 2u, 3u };
            result.Animation.Idle.SynchronizeEngines = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.AsynchronousEngines = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.Idle.AlternateEnginePhases = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.Idle.AlternateWeaponPhases = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LateralMovement.Synchronized = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LateralMovement.Staggered = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LateralMovement.ResponseStrengthScale = { 4u, 5u };
            result.Animation.LongitudinalMovement.Synchronized = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LongitudinalMovement.Staggered = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LongitudinalMovement.ResponseStrengthScale = { 4u, 5u };
            result.Animation.Firing.ResponseStrengthScale = { 9u, 10u };

            return result;
        }

        ShipFactionProfile createRelicFactionProfile()
        {
            ShipFactionProfile result;

            // Relic hulls are dark and subdued; sparse emissive roles provide
            // the high-contrast "internally alive" read instead of bright armor.
            result.Palette.HullHue = { 205u, 330u };
            result.Palette.HullSaturation = { 10u, 34u };
            result.Palette.HullValue = { 24u, 46u };
            result.Palette.Accent.HueOffset = { -30, 45 };
            result.Palette.Accent.Saturation = { 18u, 45u };
            result.Palette.Accent.Value = { 32u, 58u };
            result.Palette.Cockpit.HueOffset = { 90, 180 };
            result.Palette.Cockpit.Saturation = { 46u, 76u };
            result.Palette.Cockpit.Value = { 46u, 72u };
            result.Palette.Light.HueOffset = { 95, 205 };
            result.Palette.Light.Saturation = { 76u, 100u };
            result.Palette.Light.Value = { 92u, 100u };
            result.Palette.Exhaust.HueOffset = { 90, 190 };
            result.Palette.Exhaust.Saturation = { 62u, 92u };
            result.Palette.Exhaust.Value = { 78u, 96u };
            result.Palette.MechanicalSaturation = { 8u, 24u };
            result.Palette.MechanicalValue = { 12u, 28u };

            result.SurfaceDetails.DetailDensityPercent = 72u;
            result.SurfaceDetails.MechanicalPatternCountPercent = 30u;
            result.SurfaceDetails.LightPatternCountPercent = 118u;
            result.SurfaceDetails.AccentPanelWeightPercent = 32u;
            result.SurfaceDetails.AccentStripeWeightPercent = 45u;
            result.SurfaceDetails.AccentArmorWeightPercent = 82u;
            result.SurfaceDetails.HorizontalVentChancePercent = 18u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam = 72u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking = 155u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure = 12u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif = 230u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.IdentificationMarking = 8u;
            result.SurfaceDetails.SupplementalWeightMultipliersPercent.LuminousChannel = 250u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents = 25u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank = 20u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights = 95u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights = 145u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams = 125u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes = 80u;
            result.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot = 155u;
            result.SurfaceDetails.MotifRepeatPercent = 78u;
            result.SurfaceDetails.AsymmetricDetailChanceOffset = -2;
            result.SurfaceDetails.LuminousChannelCoreRegionBiasChance = 72u;

            result.Attachments.WeightMultipliersPercent.WeaponMount = 70u;
            result.Attachments.WeightMultipliersPercent.SensorArray = 72u;
            result.Attachments.WeightMultipliersPercent.AuxiliaryPod = 42u;
            result.Attachments.WeightMultipliersPercent.Radiator = 28u;
            result.Attachments.WeightMultipliersPercent.ArmorFin = 105u;
            result.Attachments.WeightMultipliersPercent.TechnologyNode = 175u;
            result.Attachments.AttachmentChancePercent = 72u;
            result.Attachments.SymmetryChanceOffset = 4;

            result.Weapons.ChancePercent = 94u;
            result.Weapons.SymmetryChanceOffset = 4;
            result.Weapons.WeightMultipliersPercent.SingleCannon = 58u;
            result.Weapons.WeightMultipliersPercent.TwinCannon = 78u;
            result.Weapons.WeightMultipliersPercent.CompactTurret = 145u;
            result.Weapons.WeightMultipliersPercent.RailWeapon = 175u;
            result.Weapons.WeightMultipliersPercent.WeaponPod = 62u;
            result.Weapons.EmissiveChance = 76u;

            result.Engines.LayoutWeightMultipliersPercent.Central = 148u;
            result.Engines.LayoutWeightMultipliersPercent.Twin = 82u;
            result.Engines.LayoutWeightMultipliersPercent.Quad = 55u;
            result.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary = 138u;
            result.Engines.LayoutWeightMultipliersPercent.WideBank = 68u;
            result.Engines.SizeWeightMultipliersPercent.Small = 15u;
            result.Engines.SizeWeightMultipliersPercent.Medium = 55u;
            result.Engines.SizeWeightMultipliersPercent.Large = 330u;
            result.Engines.NacelleChancePercent = 25u;
            result.Engines.ExternalHeightPercent = 60u;

            result.MajorFeatures.ChancePercent = 112u;
            result.MajorFeatures.WeightMultipliersPercent.CentralSpine = 175u;
            result.MajorFeatures.WeightMultipliersPercent.ArmorPlate = 150u;
            result.MajorFeatures.WeightMultipliersPercent.RecessedBay = 120u;
            result.MajorFeatures.WeightMultipliersPercent.VentBank = 38u;
            result.MajorFeatures.WeightMultipliersPercent.WingPlate = 72u;
            result.MajorFeatures.WeightMultipliersPercent.TechCore = 185u;

            result.Cockpit.SizeWeightMultipliersPercent.Compact = 68u;
            result.Cockpit.SizeWeightMultipliersPercent.Standard = 92u;
            result.Cockpit.SizeWeightMultipliersPercent.Large = 126u;
            result.Cockpit.SizeWeightMultipliersPercent.Massive = 138u;
            result.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy = 45u;
            result.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy = 50u;
            result.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck = 105u;
            result.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy = 110u;
            result.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge = 300u;
            result.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge = 450u;

            result.Hull.NegativeSpaceChancePercent = 115u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel = 78u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.RearFork = 135u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap = 95u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay = 155u;
            result.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel = 115u;
            result.Hull.PreferAlternateArticulationOrder = true;

            result.CoreTreatment.ChancePercent = 128u;
            result.CoreTreatment.WeightOffsets.CentralSpine = 16;
            result.CoreTreatment.WeightOffsets.CockpitSurround = -4;
            result.CoreTreatment.WeightOffsets.RaisedCorePlate = 20;
            result.CoreTreatment.WeightOffsets.CoreChannel = 22;
            result.CoreTreatment.CoreChannelLuminousPattern = ShipFactionCoreChannelLuminousPattern::EVERY_THIRD_ROW;

            result.HullLayers.ChancePercent = 124u;
            result.HullLayers.MaximumLayerCount = 2u;
            result.HullLayers.WeightAdjustments.CentralDorsalPlate.Offset = 55;
            result.HullLayers.WeightAdjustments.WingArmor.Scale = { 3u, 4u };
            result.HullLayers.WeightAdjustments.ShoulderArmor.Offset = 30;
            result.HullLayers.WeightAdjustments.RearEngineCover.Offset = 25;

            result.Materials.ZoneWeightMultipliersPercent.WingSurface = 90u;
            result.Materials.ZoneWeightMultipliersPercent.ShoulderSurface = 135u;
            result.Materials.ZoneWeightMultipliersPercent.AxialBand = 135u;
            result.Materials.ZoneWeightMultipliersPercent.RearMechanical = 135u;
            result.Materials.ZoneWeightMultipliersPercent.CockpitCollar = 90u;
            result.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 90u;

            result.Livery.ChancePercent = 78u;
            result.Livery.WeightMultipliersPercent.CenterStripe = 82u;
            result.Livery.WeightMultipliersPercent.DoubleCenterStripe = 82u;
            result.Livery.WeightMultipliersPercent.WingBand = 65u;
            result.Livery.WeightMultipliersPercent.ShoulderBlock = 82u;
            result.Livery.WeightMultipliersPercent.NoseBand = 82u;
            result.Livery.WeightMultipliersPercent.Chevron = 145u;
            result.Livery.WeightMultipliersPercent.IdPanel = 65u;
            result.Livery.WeightMultipliersPercent.GeometricInsignia = 145u;
            result.Livery.AsymmetricChanceOffset = 0;
            result.Livery.AsymmetricChanceDivisor = 1u;
            result.Livery.AllowAsymmetricGeometricInsignia = true;

            result.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Wings = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Engines = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons = 90u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature = 135u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers = 135u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore = 135u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry = 100u;
            result.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace = 100u;

            result.MacroAsymmetry.ChancePercent = 88u;
            result.Complexity.TotalBudgetPercent = 104u;
            result.Complexity.LegacyCategoryOffsets.HullLayer = 4;
            result.Complexity.LegacyCategoryOffsets.MajorFeature = 3;
            result.Complexity.LegacyCategoryOffsets.Attachment = -2;
            result.Complexity.LegacyCategoryOffsets.Detail = -3;
            result.Complexity.CategoryOffsets.CockpitStructure = 3;
            result.Complexity.CategoryOffsets.HullLayer = 5;
            result.Complexity.CategoryOffsets.MajorFeature = 4;
            result.Complexity.CategoryOffsets.Attachment = -2;
            result.Complexity.CategoryOffsets.Detail = -4;

            result.PaletteBehavior.SecondaryToneDirection = ShipFactionSecondaryToneDirection::LIGHTER;

            result.Finish.WeaponMuzzleRole = ShipFactionPaintColorRole::LIGHT_HIGHLIGHT;
            result.Finish.WeaponBodyRole = ShipFactionPaintColorRole::HULL_BASE;
            result.Finish.CoreSecondaryMaterialRole = ShipFactionPaintColorRole::HULL_BASE;
            result.Finish.CoreRaisedRole = ShipFactionPaintColorRole::HULL_BASE;
            result.Finish.CoreLuminousRole = ShipFactionPaintColorRole::LIGHT_HIGHLIGHT;
            result.Finish.CentralDorsalPlateRole = ShipFactionPaintColorRole::HULL_BASE;
            result.Finish.CockpitBaseRole = ShipFactionPaintColorRole::HULL_BASE;
            result.Finish.CockpitFrameRole = ShipFactionPaintColorRole::HULL_BASE;
            result.Finish.EngineHotCoreRole = ShipFactionPaintColorRole::LIGHT_HIGHLIGHT;
            result.Finish.EngineInteriorHighlightRole = ShipFactionPaintColorRole::LIGHT_BASE;

            result.Animation.Idle.EnginePulseStrengthMinimum = 1u;
            result.Animation.Idle.ExhaustAmplitudeScale = { 7u, 10u };
            result.Animation.Idle.EngineMechanicalChanceMaximum = 2u;
            result.Animation.Idle.WeaponMechanicalChanceScale = { 2u, 3u };
            result.Animation.Idle.WeaponMechanicalChanceMinimum = 18u;
            result.Animation.Idle.VentActivityChanceScale = { 1u, 2u };
            result.Animation.Idle.TechPulseStrength = 2u;
            result.Animation.Idle.SlowMechanicalCycle = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.Idle.IrregularEngineCycle = ShipFactionAnimationBooleanOverride::DISABLE;
            result.Animation.LateralMovement.HeavyResponse = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LateralMovement.ResponseStrengthScale = { 3u, 4u };
            result.Animation.LateralMovement.SquareTransitionInput = true;
            result.Animation.LongitudinalMovement.HeavyResponse = ShipFactionAnimationBooleanOverride::ENABLE;
            result.Animation.LongitudinalMovement.ResponseStrengthScale = { 3u, 4u };
            result.Animation.LongitudinalMovement.SquareTransitionInput = true;
            result.Animation.Firing.DurationAdditionMilliseconds = 65u;
            result.Animation.Firing.HeavyResponse = ShipFactionAnimationBooleanOverride::ENABLE;

            return result;
        }
    }

    const ShipFactionProfile& getShipFactionProfile(ShipFactionType faction)
    {
        static const ShipFactionProfile FrontierProfile = createFrontierFactionProfile();
        static const ShipFactionProfile MilitaryProfile = createMilitaryFactionProfile();
        static const ShipFactionProfile AscendantProfile = createAscendantFactionProfile();
        static const ShipFactionProfile XenoProfile = createXenoFactionProfile();
        static const ShipFactionProfile CorporateProfile = createCorporateFactionProfile();
        static const ShipFactionProfile RelicProfile = createRelicFactionProfile();

        switch (faction)
        {
        case ShipFactionType::FRONTIER: return FrontierProfile;
        case ShipFactionType::MILITARY: return MilitaryProfile;
        case ShipFactionType::ASCENDANT: return AscendantProfile;
        case ShipFactionType::XENO: return XenoProfile;
        case ShipFactionType::CORPORATE: return CorporateProfile;
        case ShipFactionType::RELIC: return RelicProfile;
        default:
            throw std::invalid_argument("Ship faction profile lookup requires a built-in ShipFactionType.");
        }
    }
}
