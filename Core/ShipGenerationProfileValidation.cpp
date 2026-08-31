#include <SpectralShipGen/ShipGenerationProfileValidation.h>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>

namespace SpectralShipGen
{
    namespace
    {
        using Result = ValidationResult;

        void addError(Result& result, const char* field, const char* message)
        {
            result.Errors.push_back({ field, message });
        }

        void validateRange(Result& result, const char* field, const UIntRange& range)
        {
            if (range.Min > range.Max)
            {
                addError(result, field, "range minimum must not exceed maximum");
            }
        }

        void validatePercentageRange(Result& result, const char* field, const UIntRange& range)
        {
            validateRange(result, field, range);
            if (range.Max > 100u)
            {
                addError(result, field, "canvas-relative percentage range must remain within 0..100");
            }
        }

        void validateProbability(Result& result, const char* field, uint32_t value)
        {
            if (value > 100u)
            {
                addError(result, field, "probability must remain within 0..100");
            }
        }

        void validatePercentageLimit(Result& result, const char* field, uint32_t value)
        {
            if (value > 100u)
            {
                addError(result, field, "percentage limit must remain within 0..100");
            }
        }

        void validateWeightGroup(Result& result, const char* field, std::initializer_list<uint32_t> weights, bool required)
        {
            uint64_t total = 0u;
            for (uint32_t weight : weights)
            {
                total += weight;
            }

            if (required && total == 0u)
            {
                addError(result, field, "at least one relative weight must be non-zero while this choice is active");
            }
            if (total > std::numeric_limits<uint32_t>::max())
            {
                addError(result, field, "relative-weight total exceeds the safe 32-bit selection range");
            }
        }

        void validateSignedWeights(Result& result, const char* field, const ShipComplexityCategoryWeights& weights)
        {
            for (int32_t weight : weights.toArray())
            {
                if (weight < 0)
                {
                    addError(result, field, "relative complexity weights must be non-negative");
                    return;
                }
            }
        }

        bool active(uint32_t chance, uint32_t maximumCount)
        {
            return chance > 0u && maximumCount > 0u;
        }
    }

    ValidationResult validateShipGenerationProfile(const ShipGenerationProfile& profile)
    {
        ValidationResult result;

        // UIntRange contracts. Canvas-relative ranges are bounded because they
        // are converted directly to image/profile indices or half-widths.
        validatePercentageRange(result, "HullVerticalPaddingPercent", profile.HullVerticalPaddingPercent);
        validatePercentageRange(result, "HullHorizontalPaddingPercent", profile.HullHorizontalPaddingPercent);
        validatePercentageRange(result, "NoseEndPercent", profile.NoseEndPercent);
        validatePercentageRange(result, "UpperFuselageEndPercent", profile.UpperFuselageEndPercent);
        validatePercentageRange(result, "MainBodyEndPercent", profile.MainBodyEndPercent);
        validatePercentageRange(result, "RearFuselageStartPercent", profile.RearFuselageStartPercent);
        validatePercentageRange(result, "NoseWidthPercent", profile.NoseWidthPercent);
        validatePercentageRange(result, "UpperFuselageWidthPercent", profile.UpperFuselageWidthPercent);
        validatePercentageRange(result, "MainBodyWidthPercent", profile.MainBodyWidthPercent);
        validatePercentageRange(result, "RearFuselageWidthPercent", profile.RearFuselageWidthPercent);
        validatePercentageRange(result, "RearWidthPercent", profile.RearWidthPercent);
        validateRange(result, "SmallWingIncreasePercent", profile.SmallWingIncreasePercent);
        validatePercentageRange(result, "SweptWingWidthPercent", profile.SweptWingWidthPercent);
        validatePercentageRange(result, "BroadWingWidthPercent", profile.BroadWingWidthPercent);
        validatePercentageRange(result, "RearForkStartPercent", profile.RearForkStartPercent);
        validatePercentageRange(result, "CockpitStartPercent", profile.CockpitStartPercent);

        if (profile.HullVerticalPaddingPercent.Max >= 50u)
        {
            addError(result, "HullVerticalPaddingPercent", "vertical padding must leave positive longitudinal hull capacity");
        }
        if (profile.HullHorizontalPaddingPercent.Max >= 50u)
        {
            addError(result, "HullHorizontalPaddingPercent", "horizontal padding must leave positive half-width hull capacity");
        }
        if (profile.NoseEndPercent.Max >= profile.UpperFuselageEndPercent.Min ||
            profile.UpperFuselageEndPercent.Max >= profile.MainBodyEndPercent.Min ||
            profile.MainBodyEndPercent.Max >= profile.RearFuselageStartPercent.Min)
        {
            addError(result, "LongitudinalHullSegments", "nose, upper fuselage, main body, and rear fuselage ranges must remain strictly ordered");
        }

        // Chance fields are probabilities. Fields such as ScalePercent,
        // StrengthPercent, coverage multipliers, and relative weights are not
        // probabilities and intentionally may exceed 100 where meaningful.
        validateProbability(result, "VisualSecondaryAnchorChance", profile.VisualSecondaryAnchorChance);
        validateProbability(result, "HullModifierChance", profile.HullModifierChance);
        validateProbability(result, "StructuralNegativeSpaceChance", profile.StructuralNegativeSpaceChance);
        validateProbability(result, "CoreTreatmentChance", profile.CoreTreatmentChance);
        validateProbability(result, "HullLayerChance", profile.HullLayerChance);
        validateProbability(result, "EngineNacelleChance", profile.EngineNacelleChance);
        validateProbability(result, "MajorFeatureChance", profile.MajorFeatureChance);
        validateProbability(result, "LargeWeaponChance", profile.LargeWeaponChance);
        validateProbability(result, "LargeWeaponSymmetryChance", profile.LargeWeaponSymmetryChance);
        validateProbability(result, "DetailMotifChance", profile.DetailMotifChance);
        validateProbability(result, "SecondaryDetailMotifChance", profile.SecondaryDetailMotifChance);
        validateProbability(result, "HorizontalVentChance", profile.HorizontalVentChance);
        validateProbability(result, "AttachmentChance", profile.AttachmentChance);
        validateProbability(result, "SymmetricAttachmentChance", profile.SymmetricAttachmentChance);
        validateProbability(result, "MaterialCompositionChance", profile.MaterialCompositionChance);
        validateProbability(result, "LiveryChance", profile.LiveryChance);
        validateProbability(result, "SupportingLiveryChance", profile.SupportingLiveryChance);
        validateProbability(result, "LiveryAsymmetricChance", profile.LiveryAsymmetricChance);
        validateProbability(result, "MacroAsymmetryChance", profile.MacroAsymmetryChance);
        validateProbability(result, "MacroAsymmetryOuterRegionChance", profile.MacroAsymmetryOuterRegionChance);
        validateProbability(result, "MacroAsymmetryWingRootRegionChance", profile.MacroAsymmetryWingRootRegionChance);

        validatePercentageLimit(result, "MinimumSilhouetteWidthPercent", profile.MinimumSilhouetteWidthPercent);
        validatePercentageLimit(result, "MinimumSilhouetteHeightPercent", profile.MinimumSilhouetteHeightPercent);
        validatePercentageLimit(result, "SilhouetteMaximumStableRunPercent", profile.SilhouetteMaximumStableRunPercent);
        validatePercentageLimit(result, "SilhouetteConvexFillTriggerPercent", profile.SilhouetteConvexFillTriggerPercent);
        validatePercentageLimit(result, "MaximumCockpitHullPercent", profile.MaximumCockpitHullPercent);
        validatePercentageLimit(result, "MaterialAxialBandWidthPercent", profile.MaterialAxialBandWidthPercent);
        validatePercentageLimit(result, "MaximumLiveryCoveragePercent", profile.MaximumLiveryCoveragePercent);
        validatePercentageLimit(result, "MaximumLiveryConnectedCoveragePercent", profile.MaximumLiveryConnectedCoveragePercent);
        validatePercentageLimit(result, "SecondaryHullToneCoveragePercent", profile.SecondaryHullToneCoveragePercent);

        if (profile.MaximumLiveryConnectedCoveragePercent > profile.MaximumLiveryCoveragePercent)
        {
            addError(result, "MaximumLiveryConnectedCoveragePercent", "connected livery coverage cannot exceed total livery coverage");
        }

        // Mandatory weighted choices.
        validateWeightGroup(result, "WingWeights", { profile.NoWingWeight, profile.SmallWingWeight, profile.SweptWingWeight, profile.BroadWingWeight }, true);
        validateWeightGroup(result, "EngineLayoutWeights", { profile.CentralEngineWeight, profile.TwinEngineWeight, profile.QuadEngineWeight, profile.CentralAuxiliaryEngineWeight, profile.EngineBankWeight }, true);
        validateWeightGroup(result, "EngineSizeWeights", { profile.SmallEngineSizeWeight, profile.MediumEngineSizeWeight, profile.LargeEngineSizeWeight }, true);
        validateWeightGroup(result, "CockpitSizeWeights", { profile.CockpitSizeWeights.Compact, profile.CockpitSizeWeights.Standard, profile.CockpitSizeWeights.Large, profile.CockpitSizeWeights.Massive }, true);
        validateWeightGroup(result, "CockpitShapeWeights", { profile.CockpitShapeWeights.CompactCanopy, profile.CockpitShapeWeights.ElongatedCanopy, profile.CockpitShapeWeights.WideCommandDeck, profile.CockpitShapeWeights.SplitCanopy, profile.CockpitShapeWeights.DorsalBridge, profile.CockpitShapeWeights.LayeredBridge }, true);
        validateWeightGroup(result, "VisualAnchorWeights", { profile.VisualAnchorWeights.Silhouette, profile.VisualAnchorWeights.Cockpit, profile.VisualAnchorWeights.Wings, profile.VisualAnchorWeights.Engines, profile.VisualAnchorWeights.Weapons, profile.VisualAnchorWeights.MajorFeature, profile.VisualAnchorWeights.HullLayers, profile.VisualAnchorWeights.CentralCore, profile.VisualAnchorWeights.MacroAsymmetry, profile.VisualAnchorWeights.NegativeSpace }, profile.VisualHierarchyEnabled);

        // Optional weighted choices may be all-zero only when the owning feature
        // is dormant through chance/count configuration.
        validateWeightGroup(result, "HullModifierWeights", { profile.BroaderShouldersModifierWeight, profile.SideLobesModifierWeight, profile.SteppedWingModifierWeight, profile.NarrowWaistModifierWeight, profile.WingCutoutModifierWeight, profile.SplitNoseModifierWeight }, active(profile.HullModifierChance, profile.MaximumHullModifiers));
        validateWeightGroup(result, "StructuralNegativeSpaceWeights", { profile.StructuralNegativeSpaceWeights.WingChannel, profile.StructuralNegativeSpaceWeights.RearFork, profile.StructuralNegativeSpaceWeights.ShoulderGap, profile.StructuralNegativeSpaceWeights.OpenFrameBay, profile.StructuralNegativeSpaceWeights.NacelleChannel }, active(profile.StructuralNegativeSpaceChance, profile.MaximumStructuralNegativeSpaceStructures));
        validateWeightGroup(result, "CoreTreatmentWeights", { profile.CoreTreatmentWeights.CentralSpine, profile.CoreTreatmentWeights.CockpitSurround, profile.CoreTreatmentWeights.RaisedCorePlate, profile.CoreTreatmentWeights.LateralRecesses, profile.CoreTreatmentWeights.LongitudinalArmorBand, profile.CoreTreatmentWeights.CoreChannel }, active(profile.CoreTreatmentChance, profile.MaximumCoreTreatments));
        validateWeightGroup(result, "HullLayerWeights", { profile.HullLayerWeights.CentralDorsalPlate, profile.HullLayerWeights.ForwardArmor, profile.HullLayerWeights.WingArmor, profile.HullLayerWeights.ShoulderArmor, profile.HullLayerWeights.RearEngineCover }, active(profile.HullLayerChance, profile.MaximumHullLayers));
        validateWeightGroup(result, "MajorFeatureWeights", { profile.MajorFeatureWeights.CentralSpine, profile.MajorFeatureWeights.ArmorPlate, profile.MajorFeatureWeights.RecessedBay, profile.MajorFeatureWeights.VentBank, profile.MajorFeatureWeights.WingPlate, profile.MajorFeatureWeights.TechCore }, active(profile.MajorFeatureChance, profile.MaximumMajorFeatures));
        validateWeightGroup(result, "LargeWeaponWeights", { profile.LargeWeaponWeights.SingleCannon, profile.LargeWeaponWeights.TwinCannon, profile.LargeWeaponWeights.CompactTurret, profile.LargeWeaponWeights.RailWeapon, profile.LargeWeaponWeights.WeaponPod }, active(profile.LargeWeaponChance, profile.MaximumLargeWeaponGroups));
        validateWeightGroup(result, "LargeWeaponHardpointWeights", { profile.LargeWeaponHardpointWeights.CentralNose, profile.LargeWeaponHardpointWeights.ForwardFuselageSide, profile.LargeWeaponHardpointWeights.WingRoot, profile.LargeWeaponHardpointWeights.OuterWing, profile.LargeWeaponHardpointWeights.ForwardShoulder, profile.LargeWeaponHardpointWeights.CentralBody }, active(profile.LargeWeaponChance, profile.MaximumLargeWeaponGroups));
        validateWeightGroup(result, "DetailMotifWeights", { profile.DetailMotifWeights.PairedVents, profile.DetailMotifWeights.TripleVentBank, profile.DetailMotifWeights.PairedLights, profile.DetailMotifWeights.ThreeNodeLights, profile.DetailMotifWeights.ParallelSeams, profile.DetailMotifWeights.RepeatedDashes, profile.DetailMotifWeights.RecessedSlot }, profile.DetailMotifChance > 0u);
        validateWeightGroup(result, "AttachmentWeights", { profile.AttachmentWeights.WeaponMount, profile.AttachmentWeights.SensorArray, profile.AttachmentWeights.AuxiliaryPod, profile.AttachmentWeights.Radiator, profile.AttachmentWeights.ArmorFin, profile.AttachmentWeights.TechnologyNode }, active(profile.AttachmentChance, profile.MaximumAttachmentGroups));
        validateWeightGroup(result, "MaterialZoneWeights", { profile.MaterialZoneWeights.WingSurface, profile.MaterialZoneWeights.ShoulderSurface, profile.MaterialZoneWeights.AxialBand, profile.MaterialZoneWeights.RearMechanical, profile.MaterialZoneWeights.CockpitCollar, profile.MaterialZoneWeights.HardpointSurround }, active(profile.MaterialCompositionChance, profile.MaximumMaterialZones));
        validateWeightGroup(result, "LiveryWeights", { profile.LiveryWeights.CenterStripe, profile.LiveryWeights.DoubleCenterStripe, profile.LiveryWeights.WingBand, profile.LiveryWeights.ShoulderBlock, profile.LiveryWeights.NoseBand, profile.LiveryWeights.Chevron, profile.LiveryWeights.IdPanel, profile.LiveryWeights.GeometricInsignia }, active(profile.LiveryChance, profile.MaximumLiveryMarkings));

        validateWeightGroup(result, "AccentDetailWeights", { profile.AccentPanelWeight, profile.AccentStripeWeight, profile.AccentArmorWeight }, false);
        validateWeightGroup(result, "SupplementalDetailWeights", { profile.SupplementalDetailWeights.PanelSeam, profile.SupplementalDetailWeights.GeometricMarking, profile.SupplementalDetailWeights.MechanicalExposure, profile.SupplementalDetailWeights.RepeatingMotif, profile.SupplementalDetailWeights.IdentificationMarking, profile.SupplementalDetailWeights.LuminousChannel }, false);
        validateWeightGroup(result, "MacroAsymmetryCategoryWeights", { profile.MacroAsymmetryCategoryWeights.HullLayer, profile.MacroAsymmetryCategoryWeights.LargeWeapon, profile.MacroAsymmetryCategoryWeights.Attachment }, profile.MacroAsymmetryChance > 0u);
        validateWeightGroup(result, "SilhouetteGuidanceWeights", { profile.SilhouetteGuidanceWeights.BroaderShoulders, profile.SilhouetteGuidanceWeights.SideLobes, profile.SilhouetteGuidanceWeights.SteppedWingExtension }, profile.SilhouetteGuidanceEnabled && profile.SilhouetteWeakArticulationGuidanceEnabled);

        validateSignedWeights(result, "ComplexityCategoryWeights", profile.ComplexityCategoryWeights);

        if (static_cast<uint32_t>(profile.DetailMotifPlacementBias) > static_cast<uint32_t>(ShipDetailMotifPlacementBias::WING_SURFACE))
        {
            addError(result, "DetailMotifPlacementBias", "enum value is outside the supported range");
        }
        if (static_cast<uint32_t>(profile.DetailMotifOrientationBias) > static_cast<uint32_t>(ShipDetailMotifOrientationBias::LATERAL))
        {
            addError(result, "DetailMotifOrientationBias", "enum value is outside the supported range");
        }
        if (static_cast<uint32_t>(profile.CoreRaisedSurfaceTone) > static_cast<uint32_t>(ShipHullSurfaceTone::HIGHLIGHT))
        {
            addError(result, "CoreRaisedSurfaceTone", "enum value is outside the supported range");
        }
        if (static_cast<uint32_t>(profile.CentralDorsalPlateTone) > static_cast<uint32_t>(ShipHullSurfaceTone::HIGHLIGHT))
        {
            addError(result, "CentralDorsalPlateTone", "enum value is outside the supported range");
        }

        return result;
    }
}
