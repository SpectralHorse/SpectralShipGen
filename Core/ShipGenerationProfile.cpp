#include <SpectralShipGen/ShipGenerationProfile.h>

#include <stdexcept>

namespace SpectralShipGen
{
    namespace
    {
        ShipGenerationProfile createFighterProfile()
        {
            ShipGenerationProfile profile;
            profile.VisualAnchorWeights.Silhouette = 45u;
            profile.VisualAnchorWeights.Cockpit = 95u;
            profile.VisualAnchorWeights.Wings = 130u;
            profile.VisualAnchorWeights.Engines = 55u;
            profile.VisualAnchorWeights.Weapons = 145u;
            profile.VisualAnchorWeights.MajorFeature = 55u;
            profile.VisualAnchorWeights.HullLayers = 60u;
            profile.VisualAnchorWeights.CentralCore = 65u;
            profile.VisualAnchorWeights.MacroAsymmetry = 22u;
            profile.VisualAnchorWeights.NegativeSpace = 48u;
            profile.VisualSecondaryAnchorChance = 38u;


            // Most defaults are already for the fighter.
            profile.HullHorizontalPaddingPercent = { 7u, 12u };
            profile.HullModifierChance = 60u;
            profile.MaximumHullModifiers = 2u;
            profile.BroaderShouldersModifierWeight = 75u;
            profile.SideLobesModifierWeight = 45u;
            profile.SteppedWingModifierWeight = 80u;
            profile.NarrowWaistModifierWeight = 50u;
            profile.WingCutoutModifierWeight = 60u;
            profile.SplitNoseModifierWeight = 55u;
            profile.MinimumSilhouetteWidthPercent = 42u;
            profile.MinimumSilhouetteHeightPercent = 70u;
            profile.SilhouetteArticulationTarget = 2u;
            profile.SilhouetteMaximumStableRunPercent = 50u;
            profile.SilhouetteConvexFillTriggerPercent = 65u;
            profile.StructuralNegativeSpaceChance = 38u;
            profile.MaximumStructuralNegativeSpaceStructures = 1u;
            profile.StructuralNegativeSpaceWeights.WingChannel = 105u;
            profile.StructuralNegativeSpaceWeights.RearFork = 40u;
            profile.StructuralNegativeSpaceWeights.ShoulderGap = 100u;
            profile.StructuralNegativeSpaceWeights.OpenFrameBay = 28u;
            profile.StructuralNegativeSpaceWeights.NacelleChannel = 58u;

            profile.CockpitSizeWeights.Compact = 35u;
            profile.CockpitSizeWeights.Standard = 75u;
            profile.CockpitSizeWeights.Large = 60u;
            profile.CockpitSizeWeights.Massive = 10u;
            profile.CockpitShapeWeights.CompactCanopy = 90u;
            profile.CockpitShapeWeights.ElongatedCanopy = 85u;
            profile.CockpitShapeWeights.WideCommandDeck = 55u;
            profile.CockpitShapeWeights.SplitCanopy = 70u;
            profile.CockpitShapeWeights.DorsalBridge = 55u;
            profile.CockpitShapeWeights.LayeredBridge = 35u;

            profile.CentralEngineWeight = 2u;
            profile.TwinEngineWeight = 6u;
            profile.QuadEngineWeight = 2u;
            profile.CentralAuxiliaryEngineWeight = 2u;
            profile.EngineBankWeight = 1u;
            profile.SmallEngineSizeWeight = 2u;
            profile.MediumEngineSizeWeight = 6u;
            profile.LargeEngineSizeWeight = 2u;
            profile.EngineNacelleChance = 35u;

            profile.MajorFeatureChance = 65u;
            profile.MaximumMajorFeatures = 2u;
            profile.MajorFeatureScalePercent = 100u;
            profile.MajorFeatureWeights.CentralSpine = 55u;
            profile.MajorFeatureWeights.ArmorPlate = 90u;
            profile.MajorFeatureWeights.RecessedBay = 45u;
            profile.MajorFeatureWeights.VentBank = 70u;
            profile.MajorFeatureWeights.WingPlate = 90u;
            profile.MajorFeatureWeights.TechCore = 45u;

            profile.LargeWeaponChance = 75u;
            profile.MaximumLargeWeaponGroups = 2u;
            profile.LargeWeaponSymmetryChance = 85u;
            profile.LargeWeaponScalePercent = 100u;
            profile.LargeWeaponWeights.SingleCannon = 110u;
            profile.LargeWeaponWeights.TwinCannon = 145u;
            profile.LargeWeaponWeights.CompactTurret = 80u;
            profile.LargeWeaponWeights.RailWeapon = 105u;
            profile.LargeWeaponWeights.WeaponPod = 85u;

            profile.AttachmentWeights.WeaponMount = 150u;
            profile.AttachmentWeights.SensorArray = 90u;
            profile.AttachmentWeights.AuxiliaryPod = 70u;
            profile.AttachmentWeights.Radiator = 40u;
            profile.AttachmentWeights.ArmorFin = 100u;
            profile.AttachmentWeights.TechnologyNode = 45u;
            profile.AttachmentChance = 65u;
            profile.MaximumAttachmentGroups = 2u;
            profile.SymmetricAttachmentChance = 80u;
            profile.AttachmentSizePercent = 100u;

            profile.SupplementalDetailWeights.PanelSeam = 90u;
            profile.SupplementalDetailWeights.GeometricMarking = 70u;
            profile.SupplementalDetailWeights.MechanicalExposure = 60u;
            profile.SupplementalDetailWeights.RepeatingMotif = 40u;
            profile.DetailMotifChance = 92u;
            profile.SecondaryDetailMotifChance = 32u;
            profile.DetailMotifRepeatPercent = 105u;
            profile.DetailMotifWeights.PairedVents = 120u;
            profile.DetailMotifWeights.TripleVentBank = 85u;
            profile.DetailMotifWeights.PairedLights = 115u;
            profile.DetailMotifWeights.ThreeNodeLights = 90u;
            profile.DetailMotifWeights.ParallelSeams = 90u;
            profile.DetailMotifWeights.RepeatedDashes = 75u;
            profile.DetailMotifWeights.RecessedSlot = 75u;
            profile.MacroAsymmetryChance = 9u;

            profile.MaterialCompositionChance = 92u;
            profile.MaximumMaterialZones = 3u;
            profile.MaterialSecondaryContrastPercent = 110u;
            profile.MaterialZoneWeights.WingSurface = 145u;
            profile.MaterialZoneWeights.ShoulderSurface = 135u;
            profile.MaterialZoneWeights.AxialBand = 55u;
            profile.MaterialZoneWeights.RearMechanical = 75u;
            profile.MaterialZoneWeights.CockpitCollar = 100u;
            profile.MaterialZoneWeights.HardpointSurround = 130u;

            profile.LiveryChance = 68u;
            profile.MaximumLiveryMarkings = 2u;
            profile.SupportingLiveryChance = 32u;
            profile.LiveryAsymmetricChance = 8u;
            profile.MaximumLiveryCoveragePercent = 18u;
            profile.MaximumLiveryConnectedCoveragePercent = 10u;
            profile.LiveryWeights.CenterStripe = 70u;
            profile.LiveryWeights.DoubleCenterStripe = 45u;
            profile.LiveryWeights.WingBand = 145u;
            profile.LiveryWeights.ShoulderBlock = 130u;
            profile.LiveryWeights.NoseBand = 125u;
            profile.LiveryWeights.Chevron = 85u;
            profile.LiveryWeights.IdPanel = 70u;
            profile.LiveryWeights.GeometricInsignia = 45u;


            // Task 80: resolved post-generation animation semantics.
            profile.AnimationTraits.Idle.EnginePulseStrength = 2u;
            profile.AnimationTraits.Idle.ExhaustAmplitudePercent = 100u;
            profile.AnimationTraits.Idle.EngineMechanicalChance = 3u;
            profile.AnimationTraits.Idle.WeaponMechanicalChance = 65u;
            profile.AnimationTraits.Idle.VentActivityChance = 45u;
            profile.AnimationTraits.Idle.AsynchronousEngines = true;
            profile.AnimationTraits.LateralMovement.ResponseStrengthPercent = 100u;
            profile.AnimationTraits.LateralMovement.EngineTravelLimit = 2u;
            profile.AnimationTraits.LateralMovement.AttachmentTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.Responsive = true;
            profile.AnimationTraits.LongitudinalMovement.ResponseStrengthPercent = 100u;
            profile.AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent = 100u;
            profile.AnimationTraits.LongitudinalMovement.BrakingContractionPercent = 80u;
            profile.AnimationTraits.LongitudinalMovement.Responsive = true;
            profile.AnimationTraits.Firing.ResponseStrengthPercent = 105u;
            profile.AnimationTraits.Firing.Responsive = true;

            return profile;
        }

        ShipGenerationProfile createSleekProfile()
        {
            ShipGenerationProfile profile;
            profile.VisualAnchorWeights.Silhouette = 150u;
            profile.VisualAnchorWeights.Cockpit = 120u;
            profile.VisualAnchorWeights.Wings = 40u;
            profile.VisualAnchorWeights.Engines = 55u;
            profile.VisualAnchorWeights.Weapons = 35u;
            profile.VisualAnchorWeights.MajorFeature = 45u;
            profile.VisualAnchorWeights.HullLayers = 45u;
            profile.VisualAnchorWeights.CentralCore = 120u;
            profile.VisualAnchorWeights.MacroAsymmetry = 8u;
            profile.VisualAnchorWeights.NegativeSpace = 20u;
            profile.VisualSecondaryAnchorChance = 24u;


            profile.HullHorizontalPaddingPercent = { 8u, 13u };
            profile.NoseEndPercent = { 20u, 28u };
            profile.UpperFuselageEndPercent = { 38u, 45u };
            profile.MainBodyEndPercent = { 52u, 60u };
            profile.RearFuselageStartPercent = { 88u, 93u };

            profile.NoseWidthPercent = { 8u, 14u };
            profile.UpperFuselageWidthPercent = { 20u, 34u };
            profile.MainBodyWidthPercent = { 34u, 52u };
            profile.RearFuselageWidthPercent = { 26u, 42u };
            profile.RearWidthPercent = { 18u, 32u };

            profile.NoWingWeight = 30u;
            profile.SmallWingWeight = 35u;
            profile.SweptWingWeight = 30u;
            profile.BroadWingWeight = 5u;

            profile.SmallWingIncreasePercent = { 5u, 10u };
            profile.SweptWingWidthPercent = { 60u, 78u };
            profile.BroadWingWidthPercent = { 70u, 86u };

            profile.HullModifierChance = 55u;
            profile.MaximumHullModifiers = 2u;
            profile.BroaderShouldersModifierWeight = 30u;
            profile.SideLobesModifierWeight = 15u;
            profile.SteppedWingModifierWeight = 30u;
            profile.NarrowWaistModifierWeight = 100u;
            profile.WingCutoutModifierWeight = 45u;
            profile.SplitNoseModifierWeight = 85u;
            profile.MinimumSilhouetteWidthPercent = 28u;
            profile.MinimumSilhouetteHeightPercent = 72u;
            profile.SilhouetteArticulationTarget = 2u;
            profile.SilhouetteMaximumStableRunPercent = 60u;
            profile.SilhouetteConvexFillTriggerPercent = 60u;
            profile.StructuralNegativeSpaceChance = 12u;
            profile.MaximumStructuralNegativeSpaceStructures = 1u;
            profile.StructuralNegativeSpaceWeights.WingChannel = 90u;
            profile.StructuralNegativeSpaceWeights.RearFork = 32u;
            profile.StructuralNegativeSpaceWeights.ShoulderGap = 35u;
            profile.StructuralNegativeSpaceWeights.OpenFrameBay = 5u;
            profile.StructuralNegativeSpaceWeights.NacelleChannel = 18u;

            profile.CockpitStartPercent = { 16u, 34u };
            profile.CockpitHeightPercent = 125u;
            profile.CockpitWidthPercent = 110u;
            profile.MaximumCockpitHullPercent = 20u;
            profile.CockpitSizeWeights.Compact = 25u;
            profile.CockpitSizeWeights.Standard = 90u;
            profile.CockpitSizeWeights.Large = 65u;
            profile.CockpitSizeWeights.Massive = 6u;
            profile.CockpitShapeWeights.CompactCanopy = 55u;
            profile.CockpitShapeWeights.ElongatedCanopy = 145u;
            profile.CockpitShapeWeights.WideCommandDeck = 25u;
            profile.CockpitShapeWeights.SplitCanopy = 50u;
            profile.CockpitShapeWeights.DorsalBridge = 45u;
            profile.CockpitShapeWeights.LayeredBridge = 40u;

            profile.CentralEngineWeight = 6u;
            profile.TwinEngineWeight = 6u;
            profile.QuadEngineWeight = 1u;
            profile.CentralAuxiliaryEngineWeight = 1u;
            profile.EngineBankWeight = 0u;
            profile.SmallEngineSizeWeight = 2u;
            profile.MediumEngineSizeWeight = 6u;
            profile.LargeEngineSizeWeight = 3u;
            profile.EngineNacelleChance = 20u;

            profile.MajorFeatureChance = 45u;
            profile.MaximumMajorFeatures = 2u;
            profile.MajorFeatureScalePercent = 90u;
            profile.MajorFeatureWeights.CentralSpine = 100u;
            profile.MajorFeatureWeights.ArmorPlate = 40u;
            profile.MajorFeatureWeights.RecessedBay = 25u;
            profile.MajorFeatureWeights.VentBank = 15u;
            profile.MajorFeatureWeights.WingPlate = 70u;
            profile.MajorFeatureWeights.TechCore = 80u;

            profile.LargeWeaponChance = 35u;
            profile.MaximumLargeWeaponGroups = 1u;
            profile.LargeWeaponSymmetryChance = 90u;
            profile.LargeWeaponScalePercent = 90u;
            profile.LargeWeaponWeights.SingleCannon = 75u;
            profile.LargeWeaponWeights.TwinCannon = 80u;
            profile.LargeWeaponWeights.CompactTurret = 55u;
            profile.LargeWeaponWeights.RailWeapon = 145u;
            profile.LargeWeaponWeights.WeaponPod = 20u;

            profile.DetailDensityPercent = 70u;
            profile.MechanicalPatternCountPercent = 70u;

            profile.AccentPanelWeight = 25u;
            profile.AccentStripeWeight = 55u;
            profile.AccentArmorWeight = 20u;

            profile.HorizontalVentChance = 45u;

            // Attachments
            profile.AttachmentWeights.WeaponMount = 90u;
            profile.AttachmentWeights.SensorArray = 100u;
            profile.AttachmentWeights.AuxiliaryPod = 25u;
            profile.AttachmentWeights.Radiator = 15u;
            profile.AttachmentWeights.ArmorFin = 40u;
            profile.AttachmentWeights.TechnologyNode = 70u;
            profile.AttachmentChance = 50u;
            profile.MaximumAttachmentGroups = 1u;
            profile.SymmetricAttachmentChance = 85u;
            profile.AttachmentSizePercent = 80u;

            // Coloring
            profile.PaletteHullValueOffset = 5;
            profile.PaletteContrastPercent = 105u;
            profile.PaletteHullSaturationPercent = 90u;
            profile.PaletteAccentSaturationPercent = 110u;
            profile.PaletteEmissiveValuePercent = 110u;
            profile.SecondaryHullToneCoveragePercent = 4u;

            profile.SupplementalDetailWeights.PanelSeam = 55u;
            profile.SupplementalDetailWeights.GeometricMarking = 100u;
            profile.SupplementalDetailWeights.MechanicalExposure = 25u;
            profile.SupplementalDetailWeights.RepeatingMotif = 50u;
            profile.DetailMotifChance = 78u;
            profile.SecondaryDetailMotifChance = 18u;
            profile.DetailMotifRepeatPercent = 78u;
            profile.DetailMotifWeights.PairedVents = 45u;
            profile.DetailMotifWeights.TripleVentBank = 25u;
            profile.DetailMotifWeights.PairedLights = 115u;
            profile.DetailMotifWeights.ThreeNodeLights = 70u;
            profile.DetailMotifWeights.ParallelSeams = 135u;
            profile.DetailMotifWeights.RepeatedDashes = 80u;
            profile.DetailMotifWeights.RecessedSlot = 45u;
            profile.MacroAsymmetryChance = 5u;

            profile.MaterialCompositionChance = 72u;
            profile.MaximumMaterialZones = 2u;
            profile.MaterialSecondaryContrastPercent = 88u;
            profile.MaterialZoneWeights.WingSurface = 45u;
            profile.MaterialZoneWeights.ShoulderSurface = 45u;
            profile.MaterialZoneWeights.AxialBand = 135u;
            profile.MaterialZoneWeights.RearMechanical = 35u;
            profile.MaterialZoneWeights.CockpitCollar = 130u;
            profile.MaterialZoneWeights.HardpointSurround = 35u;

            profile.LiveryChance = 38u;
            profile.MaximumLiveryMarkings = 2u;
            profile.SupportingLiveryChance = 18u;
            profile.LiveryAsymmetricChance = 4u;
            profile.MaximumLiveryCoveragePercent = 12u;
            profile.MaximumLiveryConnectedCoveragePercent = 7u;
            profile.LiveryWeights.CenterStripe = 140u;
            profile.LiveryWeights.DoubleCenterStripe = 95u;
            profile.LiveryWeights.WingBand = 45u;
            profile.LiveryWeights.ShoulderBlock = 35u;
            profile.LiveryWeights.NoseBand = 100u;
            profile.LiveryWeights.Chevron = 60u;
            profile.LiveryWeights.IdPanel = 30u;
            profile.LiveryWeights.GeometricInsignia = 35u;


            // Task 79: static behavior formerly hidden behind ShipStyle.
            profile.ComplexityBudgetPercent = 82u;
            profile.ComplexityCategoryWeights = { 14, 10, 13, 18, 22, 8, 15 };
            profile.SilhouetteGuidanceEnabled = false;
            profile.CleanAxialTaperArticulationExemption = true;
            profile.CoreTreatmentChance = 58u;
            profile.MaximumCoreTreatments = 2u;
            profile.CoreTreatmentWeights = { 28u, 22u, 12u, 8u, 20u, 10u };
            profile.HullLayerChance = 48u;
            profile.MaximumHullLayers = 2u;
            profile.HullLayerWeights = { 105u, 90u, 45u, 35u, 45u };
            profile.WeaponHierarchyUsesWingRoot = false;
            profile.MaterialWingSurfaceUsesFullWing = false;
            profile.CoreRaisedSurfaceTone = ShipHullSurfaceTone::HIGHLIGHT;
            profile.CentralDorsalPlateTone = ShipHullSurfaceTone::HIGHLIGHT;
            profile.AxialRidgeUsesEdgeHighlight = true;
            profile.MacroAsymmetryCategoryWeights = { 32u, 28u, 40u };

            // Task 80: resolved post-generation animation semantics.
            profile.AnimationTraits.Idle.EnginePulseStrength = 1u;
            profile.AnimationTraits.Idle.ExhaustAmplitudePercent = 65u;
            profile.AnimationTraits.Idle.EngineMechanicalChance = 1u;
            profile.AnimationTraits.Idle.WeaponMechanicalChance = 30u;
            profile.AnimationTraits.Idle.VentActivityChance = 30u;
            profile.AnimationTraits.LateralMovement.ResponseStrengthPercent = 65u;
            profile.AnimationTraits.LateralMovement.EngineTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.AttachmentTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.Responsive = true;
            profile.AnimationTraits.LongitudinalMovement.ResponseStrengthPercent = 70u;
            profile.AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent = 75u;
            profile.AnimationTraits.LongitudinalMovement.BrakingContractionPercent = 65u;
            profile.AnimationTraits.LongitudinalMovement.Responsive = true;
            profile.AnimationTraits.Firing.ResponseStrengthPercent = 75u;
            profile.AnimationTraits.Firing.MaximumRecoilLimit = 1u;
            profile.AnimationTraits.Firing.Responsive = true;

            return profile;
        }

        ShipGenerationProfile createHeavyProfile()
        {
            ShipGenerationProfile profile;
            profile.VisualAnchorWeights.Silhouette = 55u;
            profile.VisualAnchorWeights.Cockpit = 65u;
            profile.VisualAnchorWeights.Wings = 55u;
            profile.VisualAnchorWeights.Engines = 125u;
            profile.VisualAnchorWeights.Weapons = 130u;
            profile.VisualAnchorWeights.MajorFeature = 75u;
            profile.VisualAnchorWeights.HullLayers = 150u;
            profile.VisualAnchorWeights.CentralCore = 120u;
            profile.VisualAnchorWeights.MacroAsymmetry = 28u;
            profile.VisualAnchorWeights.NegativeSpace = 28u;
            profile.VisualSecondaryAnchorChance = 42u;


            profile.HullHorizontalPaddingPercent = { 7u, 11u };
            profile.NoseEndPercent = { 10u, 16u };
            profile.UpperFuselageEndPercent = { 26u, 34u };
            profile.MainBodyEndPercent = { 42u, 50u };
            profile.RearFuselageStartPercent = { 84u, 90u };

            profile.NoseWidthPercent = { 14u, 22u };
            profile.UpperFuselageWidthPercent = { 42u, 60u };
            profile.MainBodyWidthPercent = { 62u, 84u };
            profile.RearFuselageWidthPercent = { 50u, 72u };
            profile.RearWidthPercent = { 38u, 58u };

            profile.NoWingWeight = 10u;
            profile.SmallWingWeight = 15u;
            profile.SweptWingWeight = 25u;
            profile.BroadWingWeight = 50u;

            profile.SmallWingIncreasePercent = { 12u, 22u };
            profile.SweptWingWidthPercent = { 84u, 100u };
            profile.BroadWingWidthPercent = { 92u, 100u };

            profile.HullModifierChance = 80u;
            profile.MaximumHullModifiers = 3u;
            profile.BroaderShouldersModifierWeight = 120u;
            profile.SideLobesModifierWeight = 125u;
            profile.SteppedWingModifierWeight = 90u;
            profile.NarrowWaistModifierWeight = 20u;
            profile.WingCutoutModifierWeight = 55u;
            profile.SplitNoseModifierWeight = 25u;
            profile.MinimumSilhouetteWidthPercent = 56u;
            profile.MinimumSilhouetteHeightPercent = 70u;
            profile.SilhouetteArticulationTarget = 2u;
            profile.SilhouetteMaximumStableRunPercent = 54u;
            profile.SilhouetteConvexFillTriggerPercent = 72u;
            profile.StructuralNegativeSpaceChance = 24u;
            profile.MaximumStructuralNegativeSpaceStructures = 1u;
            profile.StructuralNegativeSpaceWeights.WingChannel = 38u;
            profile.StructuralNegativeSpaceWeights.RearFork = 45u;
            profile.StructuralNegativeSpaceWeights.ShoulderGap = 68u;
            profile.StructuralNegativeSpaceWeights.OpenFrameBay = 62u;
            profile.StructuralNegativeSpaceWeights.NacelleChannel = 50u;

            profile.CockpitStartPercent = { 12u, 30u };
            profile.CockpitHeightPercent = 95u;
            profile.CockpitWidthPercent = 115u;
            profile.MaximumCockpitHullPercent = 22u;
            profile.CockpitSizeWeights.Compact = 15u;
            profile.CockpitSizeWeights.Standard = 55u;
            profile.CockpitSizeWeights.Large = 100u;
            profile.CockpitSizeWeights.Massive = 38u;
            profile.CockpitShapeWeights.CompactCanopy = 35u;
            profile.CockpitShapeWeights.ElongatedCanopy = 45u;
            profile.CockpitShapeWeights.WideCommandDeck = 130u;
            profile.CockpitShapeWeights.SplitCanopy = 40u;
            profile.CockpitShapeWeights.DorsalBridge = 105u;
            profile.CockpitShapeWeights.LayeredBridge = 95u;

            profile.CentralEngineWeight = 1u;
            profile.TwinEngineWeight = 5u;
            profile.QuadEngineWeight = 5u;
            profile.CentralAuxiliaryEngineWeight = 4u;
            profile.EngineBankWeight = 4u;
            profile.SmallEngineSizeWeight = 1u;
            profile.MediumEngineSizeWeight = 4u;
            profile.LargeEngineSizeWeight = 7u;
            profile.EngineNacelleChance = 65u;

            profile.MajorFeatureChance = 85u;
            profile.MaximumMajorFeatures = 3u;
            profile.MajorFeatureScalePercent = 125u;
            profile.MajorFeatureWeights.CentralSpine = 45u;
            profile.MajorFeatureWeights.ArmorPlate = 130u;
            profile.MajorFeatureWeights.RecessedBay = 80u;
            profile.MajorFeatureWeights.VentBank = 80u;
            profile.MajorFeatureWeights.WingPlate = 120u;
            profile.MajorFeatureWeights.TechCore = 30u;

            profile.LargeWeaponChance = 82u;
            profile.MaximumLargeWeaponGroups = 2u;
            profile.LargeWeaponSymmetryChance = 82u;
            profile.LargeWeaponScalePercent = 115u;
            profile.LargeWeaponWeights.SingleCannon = 155u;
            profile.LargeWeaponWeights.TwinCannon = 120u;
            profile.LargeWeaponWeights.CompactTurret = 95u;
            profile.LargeWeaponWeights.RailWeapon = 80u;
            profile.LargeWeaponWeights.WeaponPod = 150u;

            profile.DetailDensityPercent = 130u;
            profile.MechanicalPatternCountPercent = 120u;

            profile.AccentPanelWeight = 30u;
            profile.AccentStripeWeight = 10u;
            profile.AccentArmorWeight = 60u;

            profile.HorizontalVentChance = 55u;

            // Attachments
            profile.AttachmentWeights.WeaponMount = 130u;
            profile.AttachmentWeights.SensorArray = 45u;
            profile.AttachmentWeights.AuxiliaryPod = 100u;
            profile.AttachmentWeights.Radiator = 50u;
            profile.AttachmentWeights.ArmorFin = 160u;
            profile.AttachmentWeights.TechnologyNode = 35u;
            profile.AttachmentChance = 80u;
            profile.MaximumAttachmentGroups = 3u;
            profile.SymmetricAttachmentChance = 85u;
            profile.AttachmentSizePercent = 120u;

            // Coloring
            profile.PaletteHullValueOffset = -8;
            profile.PaletteContrastPercent = 115u;
            profile.PaletteHullSaturationPercent = 90u;
            profile.PaletteAccentSaturationPercent = 115u;
            profile.PaletteEmissiveValuePercent = 95u;
            profile.SecondaryHullToneCoveragePercent = 10u;

            profile.SupplementalDetailWeights.PanelSeam = 110u;
            profile.SupplementalDetailWeights.GeometricMarking = 50u;
            profile.SupplementalDetailWeights.MechanicalExposure = 85u;
            profile.SupplementalDetailWeights.RepeatingMotif = 40u;
            profile.DetailMotifChance = 94u;
            profile.SecondaryDetailMotifChance = 30u;
            profile.DetailMotifRepeatPercent = 110u;
            profile.DetailMotifWeights.PairedVents = 120u;
            profile.DetailMotifWeights.TripleVentBank = 155u;
            profile.DetailMotifWeights.PairedLights = 60u;
            profile.DetailMotifWeights.ThreeNodeLights = 45u;
            profile.DetailMotifWeights.ParallelSeams = 135u;
            profile.DetailMotifWeights.RepeatedDashes = 45u;
            profile.DetailMotifWeights.RecessedSlot = 125u;
            profile.MacroAsymmetryChance = 14u;

            profile.MaterialCompositionChance = 94u;
            profile.MaximumMaterialZones = 3u;
            profile.MaterialSecondaryContrastPercent = 118u;
            profile.MaterialZoneWeights.WingSurface = 90u;
            profile.MaterialZoneWeights.ShoulderSurface = 145u;
            profile.MaterialZoneWeights.AxialBand = 90u;
            profile.MaterialZoneWeights.RearMechanical = 120u;
            profile.MaterialZoneWeights.CockpitCollar = 70u;
            profile.MaterialZoneWeights.HardpointSurround = 110u;

            profile.LiveryChance = 60u;
            profile.MaximumLiveryMarkings = 2u;
            profile.SupportingLiveryChance = 28u;
            profile.LiveryAsymmetricChance = 8u;
            profile.MaximumLiveryCoveragePercent = 20u;
            profile.MaximumLiveryConnectedCoveragePercent = 11u;
            profile.LiveryWeights.CenterStripe = 55u;
            profile.LiveryWeights.DoubleCenterStripe = 40u;
            profile.LiveryWeights.WingBand = 85u;
            profile.LiveryWeights.ShoulderBlock = 145u;
            profile.LiveryWeights.NoseBand = 70u;
            profile.LiveryWeights.Chevron = 45u;
            profile.LiveryWeights.IdPanel = 65u;
            profile.LiveryWeights.GeometricInsignia = 40u;


            // Task 79: static behavior formerly hidden behind ShipStyle.
            profile.ComplexityBudgetPercent = 108u;
            profile.ComplexityCategoryWeights = { 13, 15, 20, 24, 19, 5, 4 };
            profile.SilhouetteGuidanceWeights = { 3u, 2u, 1u };
            profile.CoreTreatmentChance = 82u;
            profile.CoreTreatmentWeights = { 18u, 16u, 30u, 14u, 16u, 6u };
            profile.HullLayerChance = 78u;
            profile.HullLayerWeights = { 85u, 110u, 90u, 105u, 90u };
            profile.HullLayerHierarchyUsesWingRoot = true;
            profile.WeaponHierarchyUsesWingRoot = false;
            profile.CoreRaisedSurfaceTone = ShipHullSurfaceTone::BASE;
            profile.MacroAsymmetryCategoryWeights = { 50u, 35u, 15u };

            // Task 80: resolved post-generation animation semantics.
            profile.AnimationTraits.Idle.EnginePulseStrength = 2u;
            profile.AnimationTraits.Idle.ExhaustAmplitudePercent = 100u;
            profile.AnimationTraits.Idle.EngineMechanicalChance = 4u;
            profile.AnimationTraits.Idle.WeaponMechanicalChance = 50u;
            profile.AnimationTraits.Idle.VentActivityChance = 50u;
            profile.AnimationTraits.Idle.SlowMechanicalCycle = true;
            profile.AnimationTraits.LateralMovement.ResponseStrengthPercent = 65u;
            profile.AnimationTraits.LateralMovement.EngineTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.AttachmentTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.HeavyResponse = true;
            profile.AnimationTraits.LongitudinalMovement.ResponseStrengthPercent = 65u;
            profile.AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent = 70u;
            profile.AnimationTraits.LongitudinalMovement.BrakingContractionPercent = 60u;
            profile.AnimationTraits.LongitudinalMovement.HeavyResponse = true;
            profile.AnimationTraits.Firing.ResponseStrengthPercent = 115u;
            profile.AnimationTraits.Firing.DurationAdditionMilliseconds = 55u;
            profile.AnimationTraits.Firing.HeavyResponse = true;

            return profile;
        }

        ShipGenerationProfile createIndustrialProfile()
        {
            ShipGenerationProfile profile;
            profile.VisualAnchorWeights.Silhouette = 50u;
            profile.VisualAnchorWeights.Cockpit = 45u;
            profile.VisualAnchorWeights.Wings = 55u;
            profile.VisualAnchorWeights.Engines = 150u;
            profile.VisualAnchorWeights.Weapons = 65u;
            profile.VisualAnchorWeights.MajorFeature = 140u;
            profile.VisualAnchorWeights.HullLayers = 70u;
            profile.VisualAnchorWeights.CentralCore = 65u;
            profile.VisualAnchorWeights.MacroAsymmetry = 105u;
            profile.VisualAnchorWeights.NegativeSpace = 145u;
            profile.VisualSecondaryAnchorChance = 46u;


            profile.HullHorizontalPaddingPercent = { 7u, 12u };
            profile.NoseEndPercent = { 12u, 18u };
            profile.UpperFuselageEndPercent = { 27u, 34u };
            profile.MainBodyEndPercent = { 42u, 50u };
            profile.RearFuselageStartPercent = { 82u, 88u };

            profile.NoseWidthPercent = { 14u, 22u };
            profile.UpperFuselageWidthPercent = { 40u, 58u };
            profile.MainBodyWidthPercent = { 58u, 78u };
            profile.RearFuselageWidthPercent = { 52u, 74u };
            profile.RearWidthPercent = { 40u, 62u };

            profile.NoWingWeight = 20u;
            profile.SmallWingWeight = 20u;
            profile.SweptWingWeight = 15u;
            profile.BroadWingWeight = 45u;

            profile.SmallWingIncreasePercent = { 10u, 18u };
            profile.SweptWingWidthPercent = { 76u, 94u };
            profile.BroadWingWidthPercent = { 88u, 100u };

            profile.HullModifierChance = 85u;
            profile.MaximumHullModifiers = 3u;
            profile.BroaderShouldersModifierWeight = 75u;
            profile.SideLobesModifierWeight = 85u;
            profile.SteppedWingModifierWeight = 130u;
            profile.NarrowWaistModifierWeight = 45u;
            profile.WingCutoutModifierWeight = 110u;
            profile.SplitNoseModifierWeight = 35u;
            profile.MinimumSilhouetteWidthPercent = 52u;
            profile.MinimumSilhouetteHeightPercent = 70u;
            profile.SilhouetteArticulationTarget = 3u;
            profile.SilhouetteMaximumStableRunPercent = 46u;
            profile.SilhouetteConvexFillTriggerPercent = 70u;
            profile.StructuralNegativeSpaceChance = 68u;
            profile.MaximumStructuralNegativeSpaceStructures = 2u;
            profile.StructuralNegativeSpaceWeights.WingChannel = 82u;
            profile.StructuralNegativeSpaceWeights.RearFork = 115u;
            profile.StructuralNegativeSpaceWeights.ShoulderGap = 88u;
            profile.StructuralNegativeSpaceWeights.OpenFrameBay = 140u;
            profile.StructuralNegativeSpaceWeights.NacelleChannel = 150u;

            profile.CockpitStartPercent = { 14u, 32u };
            profile.CockpitHeightPercent = 100u;
            profile.CockpitWidthPercent = 105u;
            profile.MaximumCockpitHullPercent = 21u;
            profile.CockpitSizeWeights.Compact = 20u;
            profile.CockpitSizeWeights.Standard = 60u;
            profile.CockpitSizeWeights.Large = 90u;
            profile.CockpitSizeWeights.Massive = 30u;
            profile.CockpitShapeWeights.CompactCanopy = 40u;
            profile.CockpitShapeWeights.ElongatedCanopy = 55u;
            profile.CockpitShapeWeights.WideCommandDeck = 95u;
            profile.CockpitShapeWeights.SplitCanopy = 80u;
            profile.CockpitShapeWeights.DorsalBridge = 85u;
            profile.CockpitShapeWeights.LayeredBridge = 130u;

            profile.CentralEngineWeight = 1u;
            profile.TwinEngineWeight = 3u;
            profile.QuadEngineWeight = 4u;
            profile.CentralAuxiliaryEngineWeight = 3u;
            profile.EngineBankWeight = 7u;
            profile.SmallEngineSizeWeight = 2u;
            profile.MediumEngineSizeWeight = 5u;
            profile.LargeEngineSizeWeight = 5u;
            profile.EngineNacelleChance = 75u;

            profile.MajorFeatureChance = 90u;
            profile.MaximumMajorFeatures = 3u;
            profile.MajorFeatureScalePercent = 115u;
            profile.MajorFeatureWeights.CentralSpine = 35u;
            profile.MajorFeatureWeights.ArmorPlate = 75u;
            profile.MajorFeatureWeights.RecessedBay = 120u;
            profile.MajorFeatureWeights.VentBank = 140u;
            profile.MajorFeatureWeights.WingPlate = 80u;
            profile.MajorFeatureWeights.TechCore = 45u;

            profile.LargeWeaponChance = 62u;
            profile.MaximumLargeWeaponGroups = 2u;
            profile.LargeWeaponSymmetryChance = 55u;
            profile.LargeWeaponScalePercent = 110u;
            profile.LargeWeaponWeights.SingleCannon = 110u;
            profile.LargeWeaponWeights.TwinCannon = 90u;
            profile.LargeWeaponWeights.CompactTurret = 100u;
            profile.LargeWeaponWeights.RailWeapon = 65u;
            profile.LargeWeaponWeights.WeaponPod = 130u;

            profile.DetailDensityPercent = 140u;
            profile.MechanicalPatternCountPercent = 180u;

            profile.AccentPanelWeight = 55u;
            profile.AccentStripeWeight = 10u;
            profile.AccentArmorWeight = 35u;

            profile.HorizontalVentChance = 80u;

            // Attachments
            profile.AttachmentWeights.WeaponMount = 55u;
            profile.AttachmentWeights.SensorArray = 110u;
            profile.AttachmentWeights.AuxiliaryPod = 150u;
            profile.AttachmentWeights.Radiator = 180u;
            profile.AttachmentWeights.ArmorFin = 70u;
            profile.AttachmentWeights.TechnologyNode = 35u;
            profile.AttachmentChance = 85u;
            profile.MaximumAttachmentGroups = 3u;
            profile.SymmetricAttachmentChance = 65u;
            profile.AttachmentSizePercent = 110u;

            // Coloring
            profile.PaletteHullValueOffset = -5;
            profile.PaletteContrastPercent = 95u;
            profile.PaletteHullSaturationPercent = 80u;
            profile.PaletteAccentSaturationPercent = 90u;
            profile.PaletteEmissiveValuePercent = 90u;
            profile.SecondaryHullToneCoveragePercent = 16u;

            profile.SupplementalDetailWeights.PanelSeam = 130u;
            profile.SupplementalDetailWeights.GeometricMarking = 40u;
            profile.SupplementalDetailWeights.MechanicalExposure = 140u;
            profile.SupplementalDetailWeights.RepeatingMotif = 35u;
            profile.DetailMotifChance = 97u;
            profile.SecondaryDetailMotifChance = 42u;
            profile.DetailMotifRepeatPercent = 125u;
            profile.DetailMotifWeights.PairedVents = 145u;
            profile.DetailMotifWeights.TripleVentBank = 175u;
            profile.DetailMotifWeights.PairedLights = 55u;
            profile.DetailMotifWeights.ThreeNodeLights = 45u;
            profile.DetailMotifWeights.ParallelSeams = 115u;
            profile.DetailMotifWeights.RepeatedDashes = 110u;
            profile.DetailMotifWeights.RecessedSlot = 160u;
            profile.MacroAsymmetryChance = 20u;

            profile.MaterialCompositionChance = 96u;
            profile.MaximumMaterialZones = 4u;
            profile.MaterialSecondaryContrastPercent = 122u;
            profile.MaterialZoneWeights.WingSurface = 80u;
            profile.MaterialZoneWeights.ShoulderSurface = 105u;
            profile.MaterialZoneWeights.AxialBand = 65u;
            profile.MaterialZoneWeights.RearMechanical = 180u;
            profile.MaterialZoneWeights.CockpitCollar = 55u;
            profile.MaterialZoneWeights.HardpointSurround = 155u;

            profile.LiveryChance = 70u;
            profile.MaximumLiveryMarkings = 2u;
            profile.SupportingLiveryChance = 38u;
            profile.LiveryAsymmetricChance = 25u;
            profile.MaximumLiveryCoveragePercent = 16u;
            profile.MaximumLiveryConnectedCoveragePercent = 9u;
            profile.LiveryWeights.CenterStripe = 45u;
            profile.LiveryWeights.DoubleCenterStripe = 30u;
            profile.LiveryWeights.WingBand = 70u;
            profile.LiveryWeights.ShoulderBlock = 110u;
            profile.LiveryWeights.NoseBand = 45u;
            profile.LiveryWeights.Chevron = 55u;
            profile.LiveryWeights.IdPanel = 135u;
            profile.LiveryWeights.GeometricInsignia = 55u;


            // Task 79: static behavior formerly hidden behind ShipStyle.
            profile.ComplexityBudgetPercent = 112u;
            profile.ComplexityCategoryWeights = { 12, 14, 17, 17, 14, 16, 10 };
            profile.SilhouetteGuidanceWeights = { 1u, 2u, 3u };
            profile.CoreTreatmentChance = 84u;
            profile.CoreTreatmentWeights = { 14u, 14u, 18u, 22u, 16u, 16u };
            profile.HullLayerChance = 72u;
            profile.HullLayerWeights = { 55u, 75u, 85u, 95u, 110u };
            profile.WeaponHierarchyUsesWingRoot = false;
            profile.MaterialWingSurfaceUsesFullWing = false;
            profile.MacroAsymmetryCategoryWeights = { 30u, 20u, 50u };

            // Task 80: resolved post-generation animation semantics.
            profile.AnimationTraits.Idle.EnginePulseStrength = 2u;
            profile.AnimationTraits.Idle.ExhaustAmplitudePercent = 90u;
            profile.AnimationTraits.Idle.EngineMechanicalChance = 8u;
            profile.AnimationTraits.Idle.WeaponMechanicalChance = 80u;
            profile.AnimationTraits.Idle.VentActivityChance = 90u;
            profile.AnimationTraits.Idle.AsynchronousEngines = true;
            profile.AnimationTraits.LateralMovement.ResponseStrengthPercent = 110u;
            profile.AnimationTraits.LateralMovement.EngineTravelLimit = 2u;
            profile.AnimationTraits.LateralMovement.AttachmentTravelLimit = 2u;
            profile.AnimationTraits.LateralMovement.Staggered = true;
            profile.AnimationTraits.LongitudinalMovement.ResponseStrengthPercent = 115u;
            profile.AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent = 100u;
            profile.AnimationTraits.LongitudinalMovement.BrakingContractionPercent = 100u;
            profile.AnimationTraits.LongitudinalMovement.ExhaustVariationLimit = 2u;
            profile.AnimationTraits.LongitudinalMovement.AttachmentTravelLimit = 2u;
            profile.AnimationTraits.LongitudinalMovement.BrakingTravelLimit = 2u;
            profile.AnimationTraits.LongitudinalMovement.Staggered = true;
            profile.AnimationTraits.Firing.ResponseStrengthPercent = 120u;
            profile.AnimationTraits.Firing.AdditionalRecoilLimit = 1u;
            profile.AnimationTraits.Firing.MinimumPreFireExtensionLimit = 1u;

            return profile;
        }

        ShipGenerationProfile createSpearheadProfile()
        {
            ShipGenerationProfile profile;
            profile.VisualAnchorWeights.Silhouette = 1000u;
            profile.VisualAnchorWeights.Cockpit = 0u;
            profile.VisualAnchorWeights.Wings = 0u;
            profile.VisualAnchorWeights.Engines = 0u;
            profile.VisualAnchorWeights.Weapons = 0u;
            profile.VisualAnchorWeights.MajorFeature = 0u;
            profile.VisualAnchorWeights.HullLayers = 0u;
            profile.VisualAnchorWeights.CentralCore = 0u;
            profile.VisualAnchorWeights.MacroAsymmetry = 0u;
            profile.VisualAnchorWeights.NegativeSpace = 0u;
            profile.VisualSecondaryAnchorChance = 0u;


            // Long occupied silhouette with a narrow fuselage that reaches its
            // maximum breadth late. Wings provide secondary rather than primary
            // lateral mass.
            profile.HullVerticalPaddingPercent = { 5u, 9u };
            profile.HullHorizontalPaddingPercent = { 8u, 13u };
            profile.NoseEndPercent = { 27u, 34u };
            profile.UpperFuselageEndPercent = { 48u, 56u };
            profile.MainBodyEndPercent = { 64u, 72u };
            profile.RearFuselageStartPercent = { 91u, 95u };

            profile.NoseWidthPercent = { 5u, 10u };
            profile.UpperFuselageWidthPercent = { 14u, 24u };
            profile.MainBodyWidthPercent = { 28u, 42u };
            profile.RearFuselageWidthPercent = { 22u, 36u };
            profile.RearWidthPercent = { 14u, 26u };

            profile.NoWingWeight = 5u;
            profile.SmallWingWeight = 10u;
            profile.SweptWingWeight = 75u;
            profile.BroadWingWeight = 10u;
            profile.SmallWingIncreasePercent = { 5u, 10u };
            profile.SweptWingWidthPercent = { 55u, 72u };
            profile.BroadWingWidthPercent = { 65u, 82u };
            profile.WingLongitudinalOffsetPercent = 8;
            profile.WingRootLengthPercent = 75u;
            profile.WingRootWidthPercent = 72u;
            profile.SweptWingTaperPercent = 70u;
            profile.BroadWingTaperPercent = 72u;

            profile.HullModifierChance = 72u;
            profile.MaximumHullModifiers = 2u;
            profile.BroaderShouldersModifierWeight = 15u;
            profile.SideLobesModifierWeight = 5u;
            profile.SteppedWingModifierWeight = 25u;
            profile.NarrowWaistModifierWeight = 145u;
            profile.WingCutoutModifierWeight = 35u;
            profile.SplitNoseModifierWeight = 45u;
            profile.MinimumSilhouetteWidthPercent = 30u;
            profile.MinimumSilhouetteHeightPercent = 80u;
            profile.SilhouetteArticulationTarget = 2u;
            profile.SilhouetteMaximumStableRunPercent = 65u;
            profile.SilhouetteConvexFillTriggerPercent = 55u;
            profile.StructuralNegativeSpaceChance = 28u;
            profile.MaximumStructuralNegativeSpaceStructures = 1u;
            profile.StructuralNegativeSpaceWeights.WingChannel = 22u;
            profile.StructuralNegativeSpaceWeights.RearFork = 165u;
            profile.StructuralNegativeSpaceWeights.ShoulderGap = 12u;
            profile.StructuralNegativeSpaceWeights.OpenFrameBay = 18u;
            profile.StructuralNegativeSpaceWeights.NacelleChannel = 70u;

            profile.CockpitStartPercent = { 10u, 24u };
            profile.CockpitHeightPercent = 145u;
            profile.CockpitWidthPercent = 78u;
            profile.MaximumCockpitHullPercent = 22u;
            profile.CockpitSizeWeights.Compact = 10u;
            profile.CockpitSizeWeights.Standard = 70u;
            profile.CockpitSizeWeights.Large = 105u;
            profile.CockpitSizeWeights.Massive = 12u;
            profile.CockpitShapeWeights.CompactCanopy = 25u;
            profile.CockpitShapeWeights.ElongatedCanopy = 190u;
            profile.CockpitShapeWeights.WideCommandDeck = 8u;
            profile.CockpitShapeWeights.SplitCanopy = 25u;
            profile.CockpitShapeWeights.DorsalBridge = 115u;
            profile.CockpitShapeWeights.LayeredBridge = 75u;

            profile.CentralEngineWeight = 7u;
            profile.TwinEngineWeight = 7u;
            profile.QuadEngineWeight = 1u;
            profile.CentralAuxiliaryEngineWeight = 5u;
            profile.EngineBankWeight = 0u;
            profile.SmallEngineSizeWeight = 1u;
            profile.MediumEngineSizeWeight = 6u;
            profile.LargeEngineSizeWeight = 4u;
            profile.EngineNacelleChance = 25u;

            profile.MajorFeatureChance = 75u;
            profile.MaximumMajorFeatures = 2u;
            profile.MajorFeatureScalePercent = 110u;
            profile.MajorFeatureWeights.CentralSpine = 190u;
            profile.MajorFeatureWeights.ArmorPlate = 75u;
            profile.MajorFeatureWeights.RecessedBay = 35u;
            profile.MajorFeatureWeights.VentBank = 30u;
            profile.MajorFeatureWeights.WingPlate = 45u;
            profile.MajorFeatureWeights.TechCore = 80u;

            profile.LargeWeaponChance = 72u;
            profile.MaximumLargeWeaponGroups = 2u;
            profile.LargeWeaponSymmetryChance = 88u;
            profile.LargeWeaponScalePercent = 105u;
            profile.LargeWeaponWeights.SingleCannon = 100u;
            profile.LargeWeaponWeights.TwinCannon = 110u;
            profile.LargeWeaponWeights.CompactTurret = 30u;
            profile.LargeWeaponWeights.RailWeapon = 200u;
            profile.LargeWeaponWeights.WeaponPod = 20u;
            profile.LargeWeaponHardpointWeights.CentralNose = 190u;
            profile.LargeWeaponHardpointWeights.ForwardFuselageSide = 145u;
            profile.LargeWeaponHardpointWeights.WingRoot = 60u;
            profile.LargeWeaponHardpointWeights.OuterWing = 20u;
            profile.LargeWeaponHardpointWeights.ForwardShoulder = 135u;
            profile.LargeWeaponHardpointWeights.CentralBody = 125u;

            profile.DetailDensityPercent = 82u;
            profile.MechanicalPatternCountPercent = 72u;
            profile.AccentPanelWeight = 25u;
            profile.AccentStripeWeight = 55u;
            profile.AccentArmorWeight = 20u;
            profile.HorizontalVentChance = 42u;

            profile.AttachmentWeights.WeaponMount = 95u;
            profile.AttachmentWeights.SensorArray = 90u;
            profile.AttachmentWeights.AuxiliaryPod = 20u;
            profile.AttachmentWeights.Radiator = 10u;
            profile.AttachmentWeights.ArmorFin = 55u;
            profile.AttachmentWeights.TechnologyNode = 65u;
            profile.AttachmentChance = 48u;
            profile.MaximumAttachmentGroups = 1u;
            profile.SymmetricAttachmentChance = 88u;
            profile.AttachmentSizePercent = 82u;

            profile.PaletteHullValueOffset = 3;
            profile.PaletteContrastPercent = 108u;
            profile.PaletteHullSaturationPercent = 92u;
            profile.PaletteAccentSaturationPercent = 112u;
            profile.PaletteEmissiveValuePercent = 108u;
            profile.SecondaryHullToneCoveragePercent = 6u;

            profile.SupplementalDetailWeights.PanelSeam = 65u;
            profile.SupplementalDetailWeights.GeometricMarking = 95u;
            profile.SupplementalDetailWeights.MechanicalExposure = 30u;
            profile.SupplementalDetailWeights.RepeatingMotif = 45u;
            profile.DetailMotifChance = 82u;
            profile.SecondaryDetailMotifChance = 18u;
            profile.DetailMotifRepeatPercent = 88u;
            profile.DetailMotifWeights.PairedVents = 55u;
            profile.DetailMotifWeights.TripleVentBank = 40u;
            profile.DetailMotifWeights.PairedLights = 90u;
            profile.DetailMotifWeights.ThreeNodeLights = 60u;
            profile.DetailMotifWeights.ParallelSeams = 150u;
            profile.DetailMotifWeights.RepeatedDashes = 120u;
            profile.DetailMotifWeights.RecessedSlot = 65u;
            profile.MacroAsymmetryChance = 4u;

            profile.MaterialCompositionChance = 88u;
            profile.MaximumMaterialZones = 2u;
            profile.MaterialSecondaryContrastPercent = 92u;
            profile.MaterialZoneWeights.WingSurface = 25u;
            profile.MaterialZoneWeights.ShoulderSurface = 35u;
            profile.MaterialZoneWeights.AxialBand = 200u;
            profile.MaterialZoneWeights.RearMechanical = 90u;
            profile.MaterialZoneWeights.CockpitCollar = 110u;
            profile.MaterialZoneWeights.HardpointSurround = 65u;

            profile.LiveryChance = 55u;
            profile.MaximumLiveryMarkings = 2u;
            profile.SupportingLiveryChance = 18u;
            profile.LiveryAsymmetricChance = 3u;
            profile.MaximumLiveryCoveragePercent = 18u;
            profile.MaximumLiveryConnectedCoveragePercent = 10u;
            profile.LiveryWeights.CenterStripe = 170u;
            profile.LiveryWeights.DoubleCenterStripe = 120u;
            profile.LiveryWeights.WingBand = 25u;
            profile.LiveryWeights.ShoulderBlock = 30u;
            profile.LiveryWeights.NoseBand = 145u;
            profile.LiveryWeights.Chevron = 65u;
            profile.LiveryWeights.IdPanel = 25u;
            profile.LiveryWeights.GeometricInsignia = 25u;


            // Task 79: static behavior formerly hidden behind ShipStyle.
            profile.VisualHierarchyEnabled = false;
            profile.ComplexityBudgetPercent = 98u;
            profile.ComplexityCategoryWeights = { 15, 14, 18, 22, 24, 3, 4 };
            profile.SpatialCapacityBias = { 3, 4, 4, 2, -2, -4 };
            profile.SilhouetteGuidanceEnabled = false;
            profile.SilhouetteProfileValidationEnabled = false;
            profile.RearForkStartPercent = { 70u, 70u };
            profile.CoreTreatmentChance = 90u;
            profile.CoreRegionWidthBasePercent = 46u;
            profile.CoreRegionWidthHorizontalCapacityDivisor = 7u;
            profile.CoreRegionWidthMaximumPercent = 68u;
            profile.RaisedCorePlateWidthPercent = 72u;
            profile.MaximumCoreTreatments = 2u;
            profile.CoreTreatmentWeights = { 58u, 22u, 14u, 6u, 46u, 22u };
            profile.HullLayerChance = 68u;
            profile.MaximumHullLayers = 2u;
            profile.HullLayerWeights = { 165u, 105u, 35u, 45u, 55u };
            profile.WeaponHierarchyUsesWingRoot = false;
            profile.DetailMotifMirroringBonusPercent = 8u;
            profile.DetailMotifPlacementBias = ShipDetailMotifPlacementBias::AXIAL;
            profile.DetailMotifOrientationBias = ShipDetailMotifOrientationBias::LONGITUDINAL;
            profile.MaterialWingSurfaceUsesFullWing = false;
            profile.MaterialAxialBandWidthPercent = 42u;
            profile.CoreRaisedSurfaceTone = ShipHullSurfaceTone::HIGHLIGHT;
            profile.CentralDorsalPlateTone = ShipHullSurfaceTone::HIGHLIGHT;
            profile.AxialRidgeUsesEdgeHighlight = true;
            profile.MacroAsymmetryCategoryWeights = { 30u, 55u, 15u };
            profile.MacroAsymmetryOuterRegionChance = 30u;
            profile.MacroAsymmetryWingRootRegionChance = 35u;
            profile.MacroAsymmetryVisualWeightPercent = 80u;

            // Task 80: resolved post-generation animation semantics.
            profile.AnimationTraits.Idle.EnginePulseStrength = 2u;
            profile.AnimationTraits.Idle.ExhaustAmplitudePercent = 110u;
            profile.AnimationTraits.Idle.EngineMechanicalChance = 2u;
            profile.AnimationTraits.Idle.WeaponMechanicalChance = 45u;
            profile.AnimationTraits.Idle.VentActivityChance = 28u;
            profile.AnimationTraits.Idle.SynchronizeEngines = true;
            profile.AnimationTraits.LateralMovement.ResponseStrengthPercent = 70u;
            profile.AnimationTraits.LateralMovement.EngineTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.AttachmentTravelLimit = 1u;
            profile.AnimationTraits.LateralMovement.Synchronized = true;
            profile.AnimationTraits.LongitudinalMovement.ResponseStrengthPercent = 105u;
            profile.AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent = 100u;
            profile.AnimationTraits.LongitudinalMovement.BrakingContractionPercent = 75u;
            profile.AnimationTraits.LongitudinalMovement.Synchronized = true;
            profile.AnimationTraits.Firing.ResponseStrengthPercent = 105u;
            profile.AnimationTraits.Firing.RailWeaponAdditionalRecoilLimit = 1u;

            return profile;
        }

        ShipGenerationProfile createDeltaProfile()
        {
            ShipGenerationProfile profile;
            profile.VisualAnchorWeights.Silhouette = 55u;
            profile.VisualAnchorWeights.Cockpit = 105u;
            profile.VisualAnchorWeights.Wings = 155u;
            profile.VisualAnchorWeights.Engines = 120u;
            profile.VisualAnchorWeights.Weapons = 45u;
            profile.VisualAnchorWeights.MajorFeature = 55u;
            profile.VisualAnchorWeights.HullLayers = 85u;
            profile.VisualAnchorWeights.CentralCore = 135u;
            profile.VisualAnchorWeights.MacroAsymmetry = 18u;
            profile.VisualAnchorWeights.NegativeSpace = 100u;
            profile.VisualSecondaryAnchorChance = 40u;


            // Shorter occupied body with very early lateral growth. The wing
            // roots and broad wings form the primary mass instead of merely
            // decorating a Heavy-like fuselage.
            profile.HullVerticalPaddingPercent = { 11u, 16u };
            profile.HullHorizontalPaddingPercent = { 8u, 13u };
            profile.NoseEndPercent = { 8u, 13u };
            profile.UpperFuselageEndPercent = { 18u, 25u };
            profile.MainBodyEndPercent = { 31u, 39u };
            profile.RearFuselageStartPercent = { 78u, 85u };

            profile.NoseWidthPercent = { 14u, 22u };
            profile.UpperFuselageWidthPercent = { 55u, 72u };
            profile.MainBodyWidthPercent = { 78u, 96u };
            profile.RearFuselageWidthPercent = { 72u, 94u };
            profile.RearWidthPercent = { 52u, 78u };

            profile.NoWingWeight = 2u;
            profile.SmallWingWeight = 3u;
            profile.SweptWingWeight = 15u;
            profile.BroadWingWeight = 80u;
            profile.SmallWingIncreasePercent = { 12u, 20u };
            profile.SweptWingWidthPercent = { 88u, 100u };
            profile.BroadWingWidthPercent = { 96u, 100u };
            profile.WingLongitudinalOffsetPercent = -8;
            profile.WingRootLengthPercent = 180u;
            profile.WingRootWidthPercent = 165u;
            profile.SweptWingTaperPercent = 34u;
            profile.BroadWingTaperPercent = 30u;

            profile.HullModifierChance = 82u;
            profile.MaximumHullModifiers = 2u;
            profile.BroaderShouldersModifierWeight = 175u;
            profile.SideLobesModifierWeight = 25u;
            profile.SteppedWingModifierWeight = 95u;
            profile.NarrowWaistModifierWeight = 12u;
            profile.WingCutoutModifierWeight = 42u;
            profile.SplitNoseModifierWeight = 8u;
            profile.MinimumSilhouetteWidthPercent = 68u;
            profile.MinimumSilhouetteHeightPercent = 64u;
            profile.SilhouetteArticulationTarget = 2u;
            profile.SilhouetteMaximumStableRunPercent = 58u;
            profile.SilhouetteConvexFillTriggerPercent = 80u;
            profile.StructuralNegativeSpaceChance = 62u;
            profile.MaximumStructuralNegativeSpaceStructures = 2u;
            profile.StructuralNegativeSpaceWeights.WingChannel = 165u;
            profile.StructuralNegativeSpaceWeights.RearFork = 48u;
            profile.StructuralNegativeSpaceWeights.ShoulderGap = 145u;
            profile.StructuralNegativeSpaceWeights.OpenFrameBay = 42u;
            profile.StructuralNegativeSpaceWeights.NacelleChannel = 120u;

            profile.CockpitStartPercent = { 12u, 28u };
            profile.CockpitHeightPercent = 90u;
            profile.CockpitWidthPercent = 150u;
            profile.MaximumCockpitHullPercent = 25u;
            profile.CockpitSizeWeights.Compact = 8u;
            profile.CockpitSizeWeights.Standard = 45u;
            profile.CockpitSizeWeights.Large = 120u;
            profile.CockpitSizeWeights.Massive = 48u;
            profile.CockpitShapeWeights.CompactCanopy = 20u;
            profile.CockpitShapeWeights.ElongatedCanopy = 25u;
            profile.CockpitShapeWeights.WideCommandDeck = 205u;
            profile.CockpitShapeWeights.SplitCanopy = 120u;
            profile.CockpitShapeWeights.DorsalBridge = 40u;
            profile.CockpitShapeWeights.LayeredBridge = 80u;

            profile.CentralEngineWeight = 1u;
            profile.TwinEngineWeight = 6u;
            profile.QuadEngineWeight = 7u;
            profile.CentralAuxiliaryEngineWeight = 2u;
            profile.EngineBankWeight = 8u;
            profile.SmallEngineSizeWeight = 1u;
            profile.MediumEngineSizeWeight = 5u;
            profile.LargeEngineSizeWeight = 5u;
            profile.EngineNacelleChance = 58u;

            profile.MajorFeatureChance = 84u;
            profile.MaximumMajorFeatures = 3u;
            profile.MajorFeatureScalePercent = 120u;
            profile.MajorFeatureWeights.CentralSpine = 40u;
            profile.MajorFeatureWeights.ArmorPlate = 145u;
            profile.MajorFeatureWeights.RecessedBay = 45u;
            profile.MajorFeatureWeights.VentBank = 45u;
            profile.MajorFeatureWeights.WingPlate = 195u;
            profile.MajorFeatureWeights.TechCore = 35u;

            profile.LargeWeaponChance = 78u;
            profile.MaximumLargeWeaponGroups = 2u;
            profile.LargeWeaponSymmetryChance = 90u;
            profile.LargeWeaponScalePercent = 110u;
            profile.LargeWeaponWeights.SingleCannon = 105u;
            profile.LargeWeaponWeights.TwinCannon = 175u;
            profile.LargeWeaponWeights.CompactTurret = 50u;
            profile.LargeWeaponWeights.RailWeapon = 45u;
            profile.LargeWeaponWeights.WeaponPod = 135u;
            profile.LargeWeaponHardpointWeights.CentralNose = 45u;
            profile.LargeWeaponHardpointWeights.ForwardFuselageSide = 75u;
            profile.LargeWeaponHardpointWeights.WingRoot = 185u;
            profile.LargeWeaponHardpointWeights.OuterWing = 220u;
            profile.LargeWeaponHardpointWeights.ForwardShoulder = 160u;
            profile.LargeWeaponHardpointWeights.CentralBody = 45u;

            profile.DetailDensityPercent = 105u;
            profile.MechanicalPatternCountPercent = 90u;
            profile.AccentPanelWeight = 35u;
            profile.AccentStripeWeight = 20u;
            profile.AccentArmorWeight = 45u;
            profile.HorizontalVentChance = 48u;

            profile.AttachmentWeights.WeaponMount = 125u;
            profile.AttachmentWeights.SensorArray = 55u;
            profile.AttachmentWeights.AuxiliaryPod = 70u;
            profile.AttachmentWeights.Radiator = 30u;
            profile.AttachmentWeights.ArmorFin = 130u;
            profile.AttachmentWeights.TechnologyNode = 35u;
            profile.AttachmentChance = 66u;
            profile.MaximumAttachmentGroups = 2u;
            profile.SymmetricAttachmentChance = 88u;
            profile.AttachmentSizePercent = 108u;

            profile.PaletteHullValueOffset = -3;
            profile.PaletteContrastPercent = 108u;
            profile.PaletteHullSaturationPercent = 92u;
            profile.PaletteAccentSaturationPercent = 108u;
            profile.PaletteEmissiveValuePercent = 100u;
            profile.SecondaryHullToneCoveragePercent = 13u;

            profile.SupplementalDetailWeights.PanelSeam = 105u;
            profile.SupplementalDetailWeights.GeometricMarking = 65u;
            profile.SupplementalDetailWeights.MechanicalExposure = 55u;
            profile.SupplementalDetailWeights.RepeatingMotif = 45u;
            profile.DetailMotifChance = 94u;
            profile.SecondaryDetailMotifChance = 34u;
            profile.DetailMotifRepeatPercent = 112u;
            profile.DetailMotifWeights.PairedVents = 95u;
            profile.DetailMotifWeights.TripleVentBank = 105u;
            profile.DetailMotifWeights.PairedLights = 110u;
            profile.DetailMotifWeights.ThreeNodeLights = 95u;
            profile.DetailMotifWeights.ParallelSeams = 130u;
            profile.DetailMotifWeights.RepeatedDashes = 100u;
            profile.DetailMotifWeights.RecessedSlot = 90u;
            profile.MacroAsymmetryChance = 11u;

            profile.MaterialCompositionChance = 96u;
            profile.MaximumMaterialZones = 3u;
            profile.MaterialSecondaryContrastPercent = 115u;
            profile.MaterialZoneWeights.WingSurface = 210u;
            profile.MaterialZoneWeights.ShoulderSurface = 190u;
            profile.MaterialZoneWeights.AxialBand = 60u;
            profile.MaterialZoneWeights.RearMechanical = 120u;
            profile.MaterialZoneWeights.CockpitCollar = 115u;
            profile.MaterialZoneWeights.HardpointSurround = 90u;

            profile.LiveryChance = 72u;
            profile.MaximumLiveryMarkings = 2u;
            profile.SupportingLiveryChance = 34u;
            profile.LiveryAsymmetricChance = 8u;
            profile.MaximumLiveryCoveragePercent = 14u;
            profile.MaximumLiveryConnectedCoveragePercent = 8u;
            profile.LiveryWeights.CenterStripe = 45u;
            profile.LiveryWeights.DoubleCenterStripe = 35u;
            profile.LiveryWeights.WingBand = 170u;
            profile.LiveryWeights.ShoulderBlock = 155u;
            profile.LiveryWeights.NoseBand = 50u;
            profile.LiveryWeights.Chevron = 55u;
            profile.LiveryWeights.IdPanel = 95u;
            profile.LiveryWeights.GeometricInsignia = 50u;


            // Task 79: static behavior formerly hidden behind ShipStyle.
            profile.ComplexityBudgetPercent = 106u;
            profile.ComplexityCategoryWeights = { 15, 14, 25, 18, 22, 3, 3 };
            profile.SpatialCapacityBias = { -2, 1, 2, 1, 5, 5 };
            profile.SilhouetteWeakArticulationGuidanceEnabled = false;
            profile.TinySilhouetteExtraWidthRelaxationPercent = 4u;
            profile.WingWedgeArticulationExemption = true;
            profile.CoreTreatmentChance = 90u;
            profile.CoreRegionWidthBasePercent = 72u;
            profile.CoreRegionWidthHorizontalCapacityDivisor = 5u;
            profile.CoreRegionWidthMaximumPercent = 92u;
            profile.RaisedCorePlateWidthPercent = 150u;
            profile.CoreTreatmentWeights = { 12u, 32u, 58u, 34u, 12u, 8u };
            profile.HullLayerChance = 84u;
            profile.HullLayerWeights = { 65u, 80u, 185u, 190u, 70u };
            profile.HullLayerHierarchyUsesWingRoot = true;
            profile.DetailMotifMirroringBonusPercent = 8u;
            profile.DetailMotifPlacementBias = ShipDetailMotifPlacementBias::WING_SURFACE;
            profile.DetailMotifOrientationBias = ShipDetailMotifOrientationBias::LATERAL;
            profile.MacroAsymmetryCategoryWeights = { 45u, 40u, 15u };
            profile.MacroAsymmetryOuterRegionChance = 78u;
            profile.MacroAsymmetryWingRootRegionChance = 76u;
            profile.MacroAsymmetryVisualWeightPercent = 90u;

            // Task 80: resolved post-generation animation semantics.
            profile.AnimationTraits.Idle.EnginePulseStrength = 2u;
            profile.AnimationTraits.Idle.ExhaustAmplitudePercent = 95u;
            profile.AnimationTraits.Idle.EngineMechanicalChance = 4u;
            profile.AnimationTraits.Idle.WeaponMechanicalChance = 60u;
            profile.AnimationTraits.Idle.VentActivityChance = 42u;
            profile.AnimationTraits.Idle.AlternateEnginePhases = true;
            profile.AnimationTraits.LateralMovement.ResponseStrengthPercent = 100u;
            profile.AnimationTraits.LateralMovement.EngineTravelLimit = 2u;
            profile.AnimationTraits.LateralMovement.AttachmentTravelLimit = 2u;
            profile.AnimationTraits.LateralMovement.HeavyResponse = true;
            profile.AnimationTraits.LongitudinalMovement.ResponseStrengthPercent = 95u;
            profile.AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent = 90u;
            profile.AnimationTraits.LongitudinalMovement.BrakingContractionPercent = 90u;
            profile.AnimationTraits.LongitudinalMovement.AttachmentTravelLimit = 2u;
            profile.AnimationTraits.LongitudinalMovement.BrakingTravelLimit = 2u;
            profile.AnimationTraits.LongitudinalMovement.HeavyResponse = true;
            profile.AnimationTraits.Firing.ResponseStrengthPercent = 95u;
            profile.AnimationTraits.Firing.DurationAdditionMilliseconds = 20u;

            return profile;
        }
    }

    const ShipGenerationProfile& getShipGenerationProfile(ShipStyle style)
    {
        static const ShipGenerationProfile FighterProfile = createFighterProfile();
        static const ShipGenerationProfile SleekProfile = createSleekProfile();
        static const ShipGenerationProfile HeavyProfile = createHeavyProfile();
        static const ShipGenerationProfile IndustrialProfile = createIndustrialProfile();
        static const ShipGenerationProfile SpearheadProfile = createSpearheadProfile();
        static const ShipGenerationProfile DeltaProfile = createDeltaProfile();

        switch (style)
        {
        case ShipStyle::SLEEK: return SleekProfile;
        case ShipStyle::HEAVY: return HeavyProfile;
        case ShipStyle::INDUSTRIAL: return IndustrialProfile;
        case ShipStyle::FIGHTER: return FighterProfile;
        case ShipStyle::SPEARHEAD: return SpearheadProfile;
        case ShipStyle::DELTA: return DeltaProfile;
        default: throw std::invalid_argument("getShipGenerationProfile requires a valid built-in ShipStyle.");
        }
    }
}