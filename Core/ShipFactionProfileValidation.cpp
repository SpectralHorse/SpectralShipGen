#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>

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

        void validateProbability(Result& result, const char* field, uint32_t value)
        {
            if (value > 100u) { addError(result, field, "probability must remain within 0..100"); }
        }

        void validateWeightGroup(Result& result, const char* field, std::initializer_list<uint32_t> weights, bool required)
        {
            uint64_t total = 0u;
            for (const uint32_t weight : weights) { total += weight; }
            if (required && total == 0u) { addError(result, field, "at least one relative weight multiplier must be non-zero"); }
            if (total > std::numeric_limits<uint32_t>::max()) { addError(result, field, "relative-weight multiplier total exceeds the safe 32-bit composition range"); }
        }

        void validateRatio(Result& result, const char* field, const ShipFactionValueScale& scale)
        {
            if (scale.Denominator == 0u) { addError(result, field, "ratio denominator must be non-zero"); }
        }

        void validateSafeOffset(Result& result, const char* field, int32_t value)
        {
            constexpr int32_t SafeMagnitude = 1000000;
            if (value < -SafeMagnitude || value > SafeMagnitude) { addError(result, field, "signed offset exceeds the supported safe composition range"); }
        }

        bool validPaintRole(ShipFactionPaintColorRole role)
        {
            return static_cast<uint32_t>(role) < static_cast<uint32_t>(ShipFactionPaintColorRole::SHIP_FACTION_PAINT_COLOR_ROLE_END);
        }

        bool validAnimationOverride(ShipFactionAnimationBooleanOverride value)
        {
            return static_cast<uint32_t>(value) < static_cast<uint32_t>(ShipFactionAnimationBooleanOverride::SHIP_FACTION_ANIMATION_BOOLEAN_OVERRIDE_END);
        }
    }

    ValidationResult validateShipFactionProfile(const ShipFactionProfile& profile)
    {
        Result result;

        const ValidationResult paletteValidation = validateShipPaletteGenerationProfile(getShipPaletteGenerationProfile(profile));
        for (const ValidationIssue& issue : paletteValidation.Errors)
        {
            std::string field = issue.Field;
            if (field.rfind("Ranges.", 0u) == 0u) { field = "Palette." + field.substr(7u); }
            else if (field.rfind("Behavior.", 0u) == 0u) { field = "PaletteBehavior." + field.substr(9u); }
            result.Errors.push_back({ field, issue.Message });
        }

        validateProbability(result, "SurfaceDetails.LuminousChannelCoreRegionBiasChance", profile.SurfaceDetails.LuminousChannelCoreRegionBiasChance);
        validateSafeOffset(result, "SurfaceDetails.AsymmetricDetailChanceOffset", profile.SurfaceDetails.AsymmetricDetailChanceOffset);
        validateWeightGroup(result, "SurfaceDetails.SupplementalWeightMultipliersPercent", {
            profile.SurfaceDetails.SupplementalWeightMultipliersPercent.PanelSeam,
            profile.SurfaceDetails.SupplementalWeightMultipliersPercent.GeometricMarking,
            profile.SurfaceDetails.SupplementalWeightMultipliersPercent.MechanicalExposure,
            profile.SurfaceDetails.SupplementalWeightMultipliersPercent.RepeatingMotif,
            profile.SurfaceDetails.SupplementalWeightMultipliersPercent.IdentificationMarking,
            profile.SurfaceDetails.SupplementalWeightMultipliersPercent.LuminousChannel }, false);
        validateWeightGroup(result, "SurfaceDetails.MotifWeightMultipliersPercent", {
            profile.SurfaceDetails.MotifWeightMultipliersPercent.PairedVents,
            profile.SurfaceDetails.MotifWeightMultipliersPercent.TripleVentBank,
            profile.SurfaceDetails.MotifWeightMultipliersPercent.PairedLights,
            profile.SurfaceDetails.MotifWeightMultipliersPercent.ThreeNodeLights,
            profile.SurfaceDetails.MotifWeightMultipliersPercent.ParallelSeams,
            profile.SurfaceDetails.MotifWeightMultipliersPercent.RepeatedDashes,
            profile.SurfaceDetails.MotifWeightMultipliersPercent.RecessedSlot }, false);

        validateSafeOffset(result, "Attachments.SymmetryChanceOffset", profile.Attachments.SymmetryChanceOffset);
        validateWeightGroup(result, "Attachments.WeightMultipliersPercent", {
            profile.Attachments.WeightMultipliersPercent.WeaponMount,
            profile.Attachments.WeightMultipliersPercent.SensorArray,
            profile.Attachments.WeightMultipliersPercent.AuxiliaryPod,
            profile.Attachments.WeightMultipliersPercent.Radiator,
            profile.Attachments.WeightMultipliersPercent.ArmorFin,
            profile.Attachments.WeightMultipliersPercent.TechnologyNode }, false);

        validateProbability(result, "Weapons.EmissiveChance", profile.Weapons.EmissiveChance);
        validateSafeOffset(result, "Weapons.SymmetryChanceOffset", profile.Weapons.SymmetryChanceOffset);
        validateWeightGroup(result, "Weapons.WeightMultipliersPercent", {
            profile.Weapons.WeightMultipliersPercent.SingleCannon,
            profile.Weapons.WeightMultipliersPercent.TwinCannon,
            profile.Weapons.WeightMultipliersPercent.CompactTurret,
            profile.Weapons.WeightMultipliersPercent.RailWeapon,
            profile.Weapons.WeightMultipliersPercent.WeaponPod }, false);

        validateWeightGroup(result, "Engines.LayoutWeightMultipliersPercent", {
            profile.Engines.LayoutWeightMultipliersPercent.Central,
            profile.Engines.LayoutWeightMultipliersPercent.Twin,
            profile.Engines.LayoutWeightMultipliersPercent.Quad,
            profile.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary,
            profile.Engines.LayoutWeightMultipliersPercent.WideBank }, true);
        validateWeightGroup(result, "Engines.SizeWeightMultipliersPercent", {
            profile.Engines.SizeWeightMultipliersPercent.Small,
            profile.Engines.SizeWeightMultipliersPercent.Medium,
            profile.Engines.SizeWeightMultipliersPercent.Large }, true);

        validateWeightGroup(result, "MajorFeatures.WeightMultipliersPercent", {
            profile.MajorFeatures.WeightMultipliersPercent.CentralSpine,
            profile.MajorFeatures.WeightMultipliersPercent.ArmorPlate,
            profile.MajorFeatures.WeightMultipliersPercent.RecessedBay,
            profile.MajorFeatures.WeightMultipliersPercent.VentBank,
            profile.MajorFeatures.WeightMultipliersPercent.WingPlate,
            profile.MajorFeatures.WeightMultipliersPercent.TechCore }, false);

        validateWeightGroup(result, "Cockpit.SizeWeightMultipliersPercent", {
            profile.Cockpit.SizeWeightMultipliersPercent.Compact,
            profile.Cockpit.SizeWeightMultipliersPercent.Standard,
            profile.Cockpit.SizeWeightMultipliersPercent.Large,
            profile.Cockpit.SizeWeightMultipliersPercent.Massive }, true);
        validateWeightGroup(result, "Cockpit.ShapeWeightMultipliersPercent", {
            profile.Cockpit.ShapeWeightMultipliersPercent.CompactCanopy,
            profile.Cockpit.ShapeWeightMultipliersPercent.ElongatedCanopy,
            profile.Cockpit.ShapeWeightMultipliersPercent.WideCommandDeck,
            profile.Cockpit.ShapeWeightMultipliersPercent.SplitCanopy,
            profile.Cockpit.ShapeWeightMultipliersPercent.DorsalBridge,
            profile.Cockpit.ShapeWeightMultipliersPercent.LayeredBridge }, true);

        validateWeightGroup(result, "Hull.NegativeSpaceWeightMultipliersPercent", {
            profile.Hull.NegativeSpaceWeightMultipliersPercent.WingChannel,
            profile.Hull.NegativeSpaceWeightMultipliersPercent.RearFork,
            profile.Hull.NegativeSpaceWeightMultipliersPercent.ShoulderGap,
            profile.Hull.NegativeSpaceWeightMultipliersPercent.OpenFrameBay,
            profile.Hull.NegativeSpaceWeightMultipliersPercent.NacelleChannel }, false);

        if (static_cast<uint32_t>(profile.CoreTreatment.CoreChannelLuminousPattern) >= static_cast<uint32_t>(ShipFactionCoreChannelLuminousPattern::SHIP_FACTION_CORE_CHANNEL_LUMINOUS_PATTERN_END))
        {
            addError(result, "CoreTreatment.CoreChannelLuminousPattern", "enum value is outside the supported range");
        }
        for (const int32_t offset : { profile.CoreTreatment.WeightOffsets.CentralSpine, profile.CoreTreatment.WeightOffsets.CockpitSurround, profile.CoreTreatment.WeightOffsets.RaisedCorePlate, profile.CoreTreatment.WeightOffsets.LateralRecesses, profile.CoreTreatment.WeightOffsets.LongitudinalArmorBand, profile.CoreTreatment.WeightOffsets.CoreChannel })
        {
            validateSafeOffset(result, "CoreTreatment.WeightOffsets", offset);
        }

        const ShipFactionHullLayerWeightAdjustment layerAdjustments[] = {
            profile.HullLayers.WeightAdjustments.CentralDorsalPlate,
            profile.HullLayers.WeightAdjustments.ForwardArmor,
            profile.HullLayers.WeightAdjustments.WingArmor,
            profile.HullLayers.WeightAdjustments.ShoulderArmor,
            profile.HullLayers.WeightAdjustments.RearEngineCover
        };
        for (const ShipFactionHullLayerWeightAdjustment& adjustment : layerAdjustments)
        {
            validateSafeOffset(result, "HullLayers.WeightAdjustments.Offset", adjustment.Offset);
            validateRatio(result, "HullLayers.WeightAdjustments.Scale", adjustment.Scale);
        }

        validateWeightGroup(result, "Materials.ZoneWeightMultipliersPercent", {
            profile.Materials.ZoneWeightMultipliersPercent.WingSurface,
            profile.Materials.ZoneWeightMultipliersPercent.ShoulderSurface,
            profile.Materials.ZoneWeightMultipliersPercent.AxialBand,
            profile.Materials.ZoneWeightMultipliersPercent.RearMechanical,
            profile.Materials.ZoneWeightMultipliersPercent.CockpitCollar,
            profile.Materials.ZoneWeightMultipliersPercent.HardpointSurround }, false);

        validateWeightGroup(result, "Livery.WeightMultipliersPercent", {
            profile.Livery.WeightMultipliersPercent.CenterStripe,
            profile.Livery.WeightMultipliersPercent.DoubleCenterStripe,
            profile.Livery.WeightMultipliersPercent.WingBand,
            profile.Livery.WeightMultipliersPercent.ShoulderBlock,
            profile.Livery.WeightMultipliersPercent.NoseBand,
            profile.Livery.WeightMultipliersPercent.Chevron,
            profile.Livery.WeightMultipliersPercent.IdPanel,
            profile.Livery.WeightMultipliersPercent.GeometricInsignia }, false);
        validateSafeOffset(result, "Livery.AsymmetricChanceOffset", profile.Livery.AsymmetricChanceOffset);
        if (profile.Livery.AsymmetricChanceDivisor == 0u) { addError(result, "Livery.AsymmetricChanceDivisor", "chance divisor must be non-zero"); }

        validateWeightGroup(result, "VisualHierarchy.AnchorWeightMultipliersPercent", {
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.Silhouette,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.Cockpit,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.Wings,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.Engines,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.Weapons,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.MajorFeature,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.HullLayers,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.CentralCore,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.MacroAsymmetry,
            profile.VisualHierarchy.AnchorWeightMultipliersPercent.NegativeSpace }, false);

        for (const int32_t offset : {
            profile.Complexity.CategoryOffsets.Silhouette, profile.Complexity.CategoryOffsets.CockpitStructure, profile.Complexity.CategoryOffsets.HullLayer, profile.Complexity.CategoryOffsets.MajorFeature, profile.Complexity.CategoryOffsets.LargeWeapon, profile.Complexity.CategoryOffsets.Attachment, profile.Complexity.CategoryOffsets.Detail })
        {
            validateSafeOffset(result, "Complexity.CategoryOffsets", offset);
        }

        const ShipFactionPaintColorRole paintRoles[] = {
            profile.Finish.WeaponMuzzleRole, profile.Finish.WeaponBodyRole, profile.Finish.WeaponRaisedHighlightRole,
            profile.Finish.CoreSecondaryMaterialRole, profile.Finish.CoreRaisedRole, profile.Finish.CoreLuminousRole,
            profile.Finish.CoreLuminousHighlightRole, profile.Finish.CentralDorsalPlateRole, profile.Finish.CockpitBaseRole,
            profile.Finish.CockpitFrameRole, profile.Finish.EngineHotCoreRole, profile.Finish.EngineInteriorHighlightRole
        };
        for (const ShipFactionPaintColorRole role : paintRoles)
        {
            if (!validPaintRole(role)) { addError(result, "Finish", "paint color role is outside the supported range"); break; }
        }

        validateSafeOffset(result, "Animation.Idle.EngineMechanicalChanceOffset", profile.Animation.Idle.EngineMechanicalChanceOffset);
        validateSafeOffset(result, "Animation.Idle.WeaponMechanicalChanceOffset", profile.Animation.Idle.WeaponMechanicalChanceOffset);
        validateRatio(result, "Animation.Idle.ExhaustAmplitudeScale", profile.Animation.Idle.ExhaustAmplitudeScale);
        validateRatio(result, "Animation.Idle.WeaponMechanicalChanceScale", profile.Animation.Idle.WeaponMechanicalChanceScale);
        validateRatio(result, "Animation.Idle.VentActivityChanceScale", profile.Animation.Idle.VentActivityChanceScale);
        if (profile.Animation.Idle.EngineMechanicalChanceMaximum > 100u || profile.Animation.Idle.EngineMechanicalChanceMinimum > 100u || profile.Animation.Idle.WeaponMechanicalChanceMaximum > 100u || profile.Animation.Idle.WeaponMechanicalChanceMinimum > 100u)
        {
            addError(result, "Animation.Idle.MechanicalChanceLimits", "chance limits must remain within 0..100");
        }
        if (profile.Animation.Idle.EngineMechanicalChanceMaximum != 0u && profile.Animation.Idle.EngineMechanicalChanceMinimum > profile.Animation.Idle.EngineMechanicalChanceMaximum)
        {
            addError(result, "Animation.Idle.EngineMechanicalChanceLimits", "minimum cannot exceed active maximum");
        }
        if (profile.Animation.Idle.WeaponMechanicalChanceMaximum != 0u && profile.Animation.Idle.WeaponMechanicalChanceMinimum > profile.Animation.Idle.WeaponMechanicalChanceMaximum)
        {
            addError(result, "Animation.Idle.WeaponMechanicalChanceLimits", "minimum cannot exceed active maximum");
        }

        const ShipFactionAnimationBooleanOverride animationOverrides[] = {
            profile.Animation.Idle.SynchronizeEngines, profile.Animation.Idle.AsynchronousEngines,
            profile.Animation.Idle.AlternateEnginePhases, profile.Animation.Idle.AlternateWeaponPhases,
            profile.Animation.Idle.SlowMechanicalCycle, profile.Animation.Idle.IrregularEngineCycle,
            profile.Animation.LateralMovement.Synchronized, profile.Animation.LateralMovement.Staggered,
            profile.Animation.LateralMovement.HeavyResponse, profile.Animation.LongitudinalMovement.Synchronized,
            profile.Animation.LongitudinalMovement.Staggered, profile.Animation.LongitudinalMovement.HeavyResponse,
            profile.Animation.Firing.HeavyResponse
        };
        for (const ShipFactionAnimationBooleanOverride value : animationOverrides)
        {
            if (!validAnimationOverride(value)) { addError(result, "Animation", "boolean override is outside the supported range"); break; }
        }
        validateRatio(result, "Animation.LateralMovement.ResponseStrengthScale", profile.Animation.LateralMovement.ResponseStrengthScale);
        validateRatio(result, "Animation.LongitudinalMovement.ResponseStrengthScale", profile.Animation.LongitudinalMovement.ResponseStrengthScale);
        validateRatio(result, "Animation.Firing.DurationScale", profile.Animation.Firing.DurationScale);
        validateRatio(result, "Animation.Firing.ResponseStrengthScale", profile.Animation.Firing.ResponseStrengthScale);
        validateSafeOffset(result, "Animation.Firing.DurationAdditionMilliseconds", profile.Animation.Firing.DurationAdditionMilliseconds);

        return result;
    }
}
