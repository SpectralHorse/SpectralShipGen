#include <SpectralShipGen/Serialization/ShipGenerationRecipeProfileSerialization.h>

#include <limits>
#include <stdexcept>
#include <type_traits>

#include <SpectralShipGen/Color.h>

namespace SpectralShipGen::RecipeProfileSerialization
{
    namespace
    {
        using RecipeJson::Type;
        using RecipeJson::Value;

        bool requireObject(const Value& value, std::string& error, const std::string& path)
        {
            if (value.ValueType == Type::Object) { return true; }
            error = path + " must be a JSON object.";
            return false;
        }

        const Value* requiredField(const Value& object, const char* name, std::string& error, const std::string& path)
        {
            if (!requireObject(object, error, path)) { return nullptr; }
            const Value* value = object.find(name);
            if (value == nullptr) { error = "Missing required field: " + path + "." + name + "."; }
            return value;
        }

        Value toValue(uint32_t value) { return Value::number(static_cast<uint64_t>(value)); }
        Value toValue(int32_t value) { return Value::number(static_cast<int64_t>(value)); }
        Value toValue(bool value) { return Value::boolean(value); }
        Value toValue(uint8_t value) { return Value::number(static_cast<uint64_t>(value)); }

        template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
        Value toValue(T value) { return Value::number(static_cast<uint64_t>(value)); }

        bool fromValue(const Value& value, uint32_t& out, std::string& error, const std::string& path)
        {
            if (value.ValueType != Type::Number || (!value.Text.empty() && value.Text.front() == '-')) { error = path + " must be an unsigned integer."; return false; }
            try
            {
                std::size_t parsed = 0u;
                const unsigned long long result = std::stoull(value.Text, &parsed, 10);
                if (parsed != value.Text.size() || result > std::numeric_limits<uint32_t>::max()) { throw std::out_of_range("uint32"); }
                out = static_cast<uint32_t>(result);
                return true;
            }
            catch (...) { error = path + " is outside uint32 range."; return false; }
        }

        bool fromValue(const Value& value, int32_t& out, std::string& error, const std::string& path)
        {
            if (value.ValueType != Type::Number) { error = path + " must be an integer."; return false; }
            try
            {
                std::size_t parsed = 0u;
                const long long result = std::stoll(value.Text, &parsed, 10);
                if (parsed != value.Text.size() || result < std::numeric_limits<int32_t>::min() || result > std::numeric_limits<int32_t>::max()) { throw std::out_of_range("int32"); }
                out = static_cast<int32_t>(result);
                return true;
            }
            catch (...) { error = path + " is outside int32 range."; return false; }
        }

        bool fromValue(const Value& value, bool& out, std::string& error, const std::string& path)
        {
            if (value.ValueType != Type::Boolean) { error = path + " must be a boolean."; return false; }
            out = value.Boolean;
            return true;
        }

        bool fromValue(const Value& value, uint8_t& out, std::string& error, const std::string& path)
        {
            uint32_t parsed = 0u;
            if (!fromValue(value, parsed, error, path)) { return false; }
            if (parsed > 255u) { error = path + " must be in the range 0-255."; return false; }
            out = static_cast<uint8_t>(parsed);
            return true;
        }

        template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
        bool fromValue(const Value& value, T& out, std::string& error, const std::string& path)
        {
            uint32_t parsed = 0u;
            if (!fromValue(value, parsed, error, path)) { return false; }
            out = static_cast<T>(parsed);
            return true;
        }

        Value toValue(const ShipIdleAnimationTraits& value);
        bool fromValue(const Value& json, ShipIdleAnimationTraits& value, std::string& error, const std::string& path);
        Value toValue(const ShipLateralMovementAnimationTraits& value);
        bool fromValue(const Value& json, ShipLateralMovementAnimationTraits& value, std::string& error, const std::string& path);
        Value toValue(const ShipLongitudinalMovementAnimationTraits& value);
        bool fromValue(const Value& json, ShipLongitudinalMovementAnimationTraits& value, std::string& error, const std::string& path);
        Value toValue(const ShipFiringAnimationTraits& value);
        bool fromValue(const Value& json, ShipFiringAnimationTraits& value, std::string& error, const std::string& path);
        Value toValue(const ShipAnimationTraits& value);
        bool fromValue(const Value& json, ShipAnimationTraits& value, std::string& error, const std::string& path);
        Value toValue(const ShipVisualAnchorWeights& value);
        bool fromValue(const Value& json, ShipVisualAnchorWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipComplexityCategoryWeights& value);
        bool fromValue(const Value& json, ShipComplexityCategoryWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipSpatialCapacityBias& value);
        bool fromValue(const Value& json, ShipSpatialCapacityBias& value, std::string& error, const std::string& path);
        Value toValue(const UIntRange& value);
        bool fromValue(const Value& json, UIntRange& value, std::string& error, const std::string& path);
        Value toValue(const ShipSilhouetteGuidanceWeights& value);
        bool fromValue(const Value& json, ShipSilhouetteGuidanceWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipStructuralNegativeSpaceWeights& value);
        bool fromValue(const Value& json, ShipStructuralNegativeSpaceWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipCoreTreatmentWeights& value);
        bool fromValue(const Value& json, ShipCoreTreatmentWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipHullLayerWeights& value);
        bool fromValue(const Value& json, ShipHullLayerWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipCockpitSizeWeights& value);
        bool fromValue(const Value& json, ShipCockpitSizeWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipCockpitShapeWeights& value);
        bool fromValue(const Value& json, ShipCockpitShapeWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipMajorFeatureWeights& value);
        bool fromValue(const Value& json, ShipMajorFeatureWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipWeaponWeights& value);
        bool fromValue(const Value& json, ShipWeaponWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipWeaponHardpointWeights& value);
        bool fromValue(const Value& json, ShipWeaponHardpointWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipDetailMotifWeights& value);
        bool fromValue(const Value& json, ShipDetailMotifWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipAttachmentWeights& value);
        bool fromValue(const Value& json, ShipAttachmentWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipMaterialZoneWeights& value);
        bool fromValue(const Value& json, ShipMaterialZoneWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipLiveryWeights& value);
        bool fromValue(const Value& json, ShipLiveryWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipMacroAsymmetryCategoryWeights& value);
        bool fromValue(const Value& json, ShipMacroAsymmetryCategoryWeights& value, std::string& error, const std::string& path);
        Value toValue(const SupplementalSurfaceDetailWeights& value);
        bool fromValue(const Value& json, SupplementalSurfaceDetailWeights& value, std::string& error, const std::string& path);
        Value toValue(const ShipGenerationProfile& value);
        bool fromValue(const Value& json, ShipGenerationProfile& value, std::string& error, const std::string& path);
        Value toValue(const PaletteUIntRange& value);
        bool fromValue(const Value& json, PaletteUIntRange& value, std::string& error, const std::string& path);
        Value toValue(const PaletteIntRange& value);
        bool fromValue(const Value& json, PaletteIntRange& value, std::string& error, const std::string& path);
        Value toValue(const PaletteRoleProfile& value);
        bool fromValue(const Value& json, PaletteRoleProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionPaletteProfile& value);
        bool fromValue(const Value& json, ShipFactionPaletteProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionPaletteBehaviorProfile& value);
        bool fromValue(const Value& json, ShipFactionPaletteBehaviorProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionSurfaceDetailProfile& value);
        bool fromValue(const Value& json, ShipFactionSurfaceDetailProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionAttachmentProfile& value);
        bool fromValue(const Value& json, ShipFactionAttachmentProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionWeaponWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionWeaponWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionWeaponProfile& value);
        bool fromValue(const Value& json, ShipFactionWeaponProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionEngineLayoutWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionEngineLayoutWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionEngineSizeWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionEngineSizeWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionEngineProfile& value);
        bool fromValue(const Value& json, ShipFactionEngineProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionMajorFeatureWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionMajorFeatureWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionMajorFeatureProfile& value);
        bool fromValue(const Value& json, ShipFactionMajorFeatureProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionCockpitSizeWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionCockpitSizeWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionCockpitShapeWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionCockpitShapeWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionCockpitProfile& value);
        bool fromValue(const Value& json, ShipFactionCockpitProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionNegativeSpaceWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionNegativeSpaceWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionHullProfile& value);
        bool fromValue(const Value& json, ShipFactionHullProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionCoreTreatmentWeightOffsets& value);
        bool fromValue(const Value& json, ShipFactionCoreTreatmentWeightOffsets& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionCoreTreatmentProfile& value);
        bool fromValue(const Value& json, ShipFactionCoreTreatmentProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionValueScale& value);
        bool fromValue(const Value& json, ShipFactionValueScale& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionHullLayerWeightAdjustment& value);
        bool fromValue(const Value& json, ShipFactionHullLayerWeightAdjustment& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionHullLayerWeightAdjustments& value);
        bool fromValue(const Value& json, ShipFactionHullLayerWeightAdjustments& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionHullLayerProfile& value);
        bool fromValue(const Value& json, ShipFactionHullLayerProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionMaterialZoneWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionMaterialZoneWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionMaterialProfile& value);
        bool fromValue(const Value& json, ShipFactionMaterialProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionLiveryWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionLiveryWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionLiveryProfile& value);
        bool fromValue(const Value& json, ShipFactionLiveryProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionVisualAnchorWeightMultipliers& value);
        bool fromValue(const Value& json, ShipFactionVisualAnchorWeightMultipliers& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionVisualHierarchyProfile& value);
        bool fromValue(const Value& json, ShipFactionVisualHierarchyProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionMacroAsymmetryProfile& value);
        bool fromValue(const Value& json, ShipFactionMacroAsymmetryProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionComplexityCategoryOffsets& value);
        bool fromValue(const Value& json, ShipFactionComplexityCategoryOffsets& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionComplexityProfile& value);
        bool fromValue(const Value& json, ShipFactionComplexityProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionFinishProfile& value);
        bool fromValue(const Value& json, ShipFactionFinishProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionIdleAnimationProfile& value);
        bool fromValue(const Value& json, ShipFactionIdleAnimationProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionMovementAnimationProfile& value);
        bool fromValue(const Value& json, ShipFactionMovementAnimationProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionFiringAnimationProfile& value);
        bool fromValue(const Value& json, ShipFactionFiringAnimationProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionAnimationProfile& value);
        bool fromValue(const Value& json, ShipFactionAnimationProfile& value, std::string& error, const std::string& path);
        Value toValue(const ShipFactionProfile& value);
        bool fromValue(const Value& json, ShipFactionProfile& value, std::string& error, const std::string& path);

        Value toValue(const Color& value)
        {
            Value object = Value::object();
            object.Object["R"] = toValue(value.R);
            object.Object["G"] = toValue(value.G);
            object.Object["B"] = toValue(value.B);
            object.Object["A"] = toValue(value.A);
            return object;
        }

        bool fromValue(const Value& json, Color& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "R", error, path); if (field == nullptr || !fromValue(*field, value.R, error, path + ".R")) { return false; }
            field = requiredField(json, "G", error, path); if (field == nullptr || !fromValue(*field, value.G, error, path + ".G")) { return false; }
            field = requiredField(json, "B", error, path); if (field == nullptr || !fromValue(*field, value.B, error, path + ".B")) { return false; }
            field = requiredField(json, "A", error, path); if (field == nullptr || !fromValue(*field, value.A, error, path + ".A")) { return false; }
            return true;
        }

        Value toValue(const ShipIdleAnimationTraits& value)
        {
            Value object = Value::object();
            object.Object["EnginePulseStrength"] = toValue(value.EnginePulseStrength);
            object.Object["ExhaustAmplitudePercent"] = toValue(value.ExhaustAmplitudePercent);
            object.Object["EngineMechanicalChance"] = toValue(value.EngineMechanicalChance);
            object.Object["WeaponMechanicalChance"] = toValue(value.WeaponMechanicalChance);
            object.Object["VentActivityChance"] = toValue(value.VentActivityChance);
            object.Object["SynchronizeEngines"] = toValue(value.SynchronizeEngines);
            object.Object["AsynchronousEngines"] = toValue(value.AsynchronousEngines);
            object.Object["AlternateEnginePhases"] = toValue(value.AlternateEnginePhases);
            object.Object["SlowMechanicalCycle"] = toValue(value.SlowMechanicalCycle);
            return object;
        }

        bool fromValue(const Value& json, ShipIdleAnimationTraits& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "EnginePulseStrength", error, path); if (field == nullptr || !fromValue(*field, value.EnginePulseStrength, error, path + ".EnginePulseStrength")) { return false; }
            field = requiredField(json, "ExhaustAmplitudePercent", error, path); if (field == nullptr || !fromValue(*field, value.ExhaustAmplitudePercent, error, path + ".ExhaustAmplitudePercent")) { return false; }
            field = requiredField(json, "EngineMechanicalChance", error, path); if (field == nullptr || !fromValue(*field, value.EngineMechanicalChance, error, path + ".EngineMechanicalChance")) { return false; }
            field = requiredField(json, "WeaponMechanicalChance", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMechanicalChance, error, path + ".WeaponMechanicalChance")) { return false; }
            field = requiredField(json, "VentActivityChance", error, path); if (field == nullptr || !fromValue(*field, value.VentActivityChance, error, path + ".VentActivityChance")) { return false; }
            field = requiredField(json, "SynchronizeEngines", error, path); if (field == nullptr || !fromValue(*field, value.SynchronizeEngines, error, path + ".SynchronizeEngines")) { return false; }
            field = requiredField(json, "AsynchronousEngines", error, path); if (field == nullptr || !fromValue(*field, value.AsynchronousEngines, error, path + ".AsynchronousEngines")) { return false; }
            field = requiredField(json, "AlternateEnginePhases", error, path); if (field == nullptr || !fromValue(*field, value.AlternateEnginePhases, error, path + ".AlternateEnginePhases")) { return false; }
            field = requiredField(json, "SlowMechanicalCycle", error, path); if (field == nullptr || !fromValue(*field, value.SlowMechanicalCycle, error, path + ".SlowMechanicalCycle")) { return false; }
            return true;
        }

        Value toValue(const ShipLateralMovementAnimationTraits& value)
        {
            Value object = Value::object();
            object.Object["ResponseStrengthPercent"] = toValue(value.ResponseStrengthPercent);
            object.Object["EngineTravelLimit"] = toValue(value.EngineTravelLimit);
            object.Object["WeaponTravelLimit"] = toValue(value.WeaponTravelLimit);
            object.Object["AttachmentTravelLimit"] = toValue(value.AttachmentTravelLimit);
            object.Object["Synchronized"] = toValue(value.Synchronized);
            object.Object["Staggered"] = toValue(value.Staggered);
            object.Object["HeavyResponse"] = toValue(value.HeavyResponse);
            object.Object["Responsive"] = toValue(value.Responsive);
            return object;
        }

        bool fromValue(const Value& json, ShipLateralMovementAnimationTraits& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ResponseStrengthPercent", error, path); if (field == nullptr || !fromValue(*field, value.ResponseStrengthPercent, error, path + ".ResponseStrengthPercent")) { return false; }
            field = requiredField(json, "EngineTravelLimit", error, path); if (field == nullptr || !fromValue(*field, value.EngineTravelLimit, error, path + ".EngineTravelLimit")) { return false; }
            field = requiredField(json, "WeaponTravelLimit", error, path); if (field == nullptr || !fromValue(*field, value.WeaponTravelLimit, error, path + ".WeaponTravelLimit")) { return false; }
            field = requiredField(json, "AttachmentTravelLimit", error, path); if (field == nullptr || !fromValue(*field, value.AttachmentTravelLimit, error, path + ".AttachmentTravelLimit")) { return false; }
            field = requiredField(json, "Synchronized", error, path); if (field == nullptr || !fromValue(*field, value.Synchronized, error, path + ".Synchronized")) { return false; }
            field = requiredField(json, "Staggered", error, path); if (field == nullptr || !fromValue(*field, value.Staggered, error, path + ".Staggered")) { return false; }
            field = requiredField(json, "HeavyResponse", error, path); if (field == nullptr || !fromValue(*field, value.HeavyResponse, error, path + ".HeavyResponse")) { return false; }
            field = requiredField(json, "Responsive", error, path); if (field == nullptr || !fromValue(*field, value.Responsive, error, path + ".Responsive")) { return false; }
            return true;
        }

        Value toValue(const ShipLongitudinalMovementAnimationTraits& value)
        {
            Value object = Value::object();
            object.Object["ResponseStrengthPercent"] = toValue(value.ResponseStrengthPercent);
            object.Object["AccelerationExtensionPercent"] = toValue(value.AccelerationExtensionPercent);
            object.Object["BrakingContractionPercent"] = toValue(value.BrakingContractionPercent);
            object.Object["ExhaustVariationLimit"] = toValue(value.ExhaustVariationLimit);
            object.Object["WeaponTravelLimit"] = toValue(value.WeaponTravelLimit);
            object.Object["AttachmentTravelLimit"] = toValue(value.AttachmentTravelLimit);
            object.Object["BrakingTravelLimit"] = toValue(value.BrakingTravelLimit);
            object.Object["Synchronized"] = toValue(value.Synchronized);
            object.Object["Staggered"] = toValue(value.Staggered);
            object.Object["HeavyResponse"] = toValue(value.HeavyResponse);
            object.Object["Responsive"] = toValue(value.Responsive);
            return object;
        }

        bool fromValue(const Value& json, ShipLongitudinalMovementAnimationTraits& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ResponseStrengthPercent", error, path); if (field == nullptr || !fromValue(*field, value.ResponseStrengthPercent, error, path + ".ResponseStrengthPercent")) { return false; }
            field = requiredField(json, "AccelerationExtensionPercent", error, path); if (field == nullptr || !fromValue(*field, value.AccelerationExtensionPercent, error, path + ".AccelerationExtensionPercent")) { return false; }
            field = requiredField(json, "BrakingContractionPercent", error, path); if (field == nullptr || !fromValue(*field, value.BrakingContractionPercent, error, path + ".BrakingContractionPercent")) { return false; }
            field = requiredField(json, "ExhaustVariationLimit", error, path); if (field == nullptr || !fromValue(*field, value.ExhaustVariationLimit, error, path + ".ExhaustVariationLimit")) { return false; }
            field = requiredField(json, "WeaponTravelLimit", error, path); if (field == nullptr || !fromValue(*field, value.WeaponTravelLimit, error, path + ".WeaponTravelLimit")) { return false; }
            field = requiredField(json, "AttachmentTravelLimit", error, path); if (field == nullptr || !fromValue(*field, value.AttachmentTravelLimit, error, path + ".AttachmentTravelLimit")) { return false; }
            field = requiredField(json, "BrakingTravelLimit", error, path); if (field == nullptr || !fromValue(*field, value.BrakingTravelLimit, error, path + ".BrakingTravelLimit")) { return false; }
            field = requiredField(json, "Synchronized", error, path); if (field == nullptr || !fromValue(*field, value.Synchronized, error, path + ".Synchronized")) { return false; }
            field = requiredField(json, "Staggered", error, path); if (field == nullptr || !fromValue(*field, value.Staggered, error, path + ".Staggered")) { return false; }
            field = requiredField(json, "HeavyResponse", error, path); if (field == nullptr || !fromValue(*field, value.HeavyResponse, error, path + ".HeavyResponse")) { return false; }
            field = requiredField(json, "Responsive", error, path); if (field == nullptr || !fromValue(*field, value.Responsive, error, path + ".Responsive")) { return false; }
            return true;
        }

        Value toValue(const ShipFiringAnimationTraits& value)
        {
            Value object = Value::object();
            object.Object["ResponseStrengthPercent"] = toValue(value.ResponseStrengthPercent);
            object.Object["DurationAdditionMilliseconds"] = toValue(value.DurationAdditionMilliseconds);
            object.Object["AdditionalRecoilLimit"] = toValue(value.AdditionalRecoilLimit);
            object.Object["RailWeaponAdditionalRecoilLimit"] = toValue(value.RailWeaponAdditionalRecoilLimit);
            object.Object["MaximumRecoilLimit"] = toValue(value.MaximumRecoilLimit);
            object.Object["MinimumPreFireExtensionLimit"] = toValue(value.MinimumPreFireExtensionLimit);
            object.Object["HeavyResponse"] = toValue(value.HeavyResponse);
            object.Object["Responsive"] = toValue(value.Responsive);
            return object;
        }

        bool fromValue(const Value& json, ShipFiringAnimationTraits& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ResponseStrengthPercent", error, path); if (field == nullptr || !fromValue(*field, value.ResponseStrengthPercent, error, path + ".ResponseStrengthPercent")) { return false; }
            field = requiredField(json, "DurationAdditionMilliseconds", error, path); if (field == nullptr || !fromValue(*field, value.DurationAdditionMilliseconds, error, path + ".DurationAdditionMilliseconds")) { return false; }
            field = requiredField(json, "AdditionalRecoilLimit", error, path); if (field == nullptr || !fromValue(*field, value.AdditionalRecoilLimit, error, path + ".AdditionalRecoilLimit")) { return false; }
            field = requiredField(json, "RailWeaponAdditionalRecoilLimit", error, path); if (field == nullptr || !fromValue(*field, value.RailWeaponAdditionalRecoilLimit, error, path + ".RailWeaponAdditionalRecoilLimit")) { return false; }
            field = requiredField(json, "MaximumRecoilLimit", error, path); if (field == nullptr || !fromValue(*field, value.MaximumRecoilLimit, error, path + ".MaximumRecoilLimit")) { return false; }
            field = requiredField(json, "MinimumPreFireExtensionLimit", error, path); if (field == nullptr || !fromValue(*field, value.MinimumPreFireExtensionLimit, error, path + ".MinimumPreFireExtensionLimit")) { return false; }
            field = requiredField(json, "HeavyResponse", error, path); if (field == nullptr || !fromValue(*field, value.HeavyResponse, error, path + ".HeavyResponse")) { return false; }
            field = requiredField(json, "Responsive", error, path); if (field == nullptr || !fromValue(*field, value.Responsive, error, path + ".Responsive")) { return false; }
            return true;
        }

        Value toValue(const ShipAnimationTraits& value)
        {
            Value object = Value::object();
            object.Object["Idle"] = toValue(value.Idle);
            object.Object["LateralMovement"] = toValue(value.LateralMovement);
            object.Object["LongitudinalMovement"] = toValue(value.LongitudinalMovement);
            object.Object["Firing"] = toValue(value.Firing);
            return object;
        }

        bool fromValue(const Value& json, ShipAnimationTraits& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Idle", error, path); if (field == nullptr || !fromValue(*field, value.Idle, error, path + ".Idle")) { return false; }
            field = requiredField(json, "LateralMovement", error, path); if (field == nullptr || !fromValue(*field, value.LateralMovement, error, path + ".LateralMovement")) { return false; }
            field = requiredField(json, "LongitudinalMovement", error, path); if (field == nullptr || !fromValue(*field, value.LongitudinalMovement, error, path + ".LongitudinalMovement")) { return false; }
            field = requiredField(json, "Firing", error, path); if (field == nullptr || !fromValue(*field, value.Firing, error, path + ".Firing")) { return false; }
            return true;
        }

        Value toValue(const ShipVisualAnchorWeights& value)
        {
            Value object = Value::object();
            object.Object["Silhouette"] = toValue(value.Silhouette);
            object.Object["Cockpit"] = toValue(value.Cockpit);
            object.Object["Wings"] = toValue(value.Wings);
            object.Object["Engines"] = toValue(value.Engines);
            object.Object["Weapons"] = toValue(value.Weapons);
            object.Object["MajorFeature"] = toValue(value.MajorFeature);
            object.Object["HullLayers"] = toValue(value.HullLayers);
            object.Object["CentralCore"] = toValue(value.CentralCore);
            object.Object["MacroAsymmetry"] = toValue(value.MacroAsymmetry);
            object.Object["NegativeSpace"] = toValue(value.NegativeSpace);
            return object;
        }

        bool fromValue(const Value& json, ShipVisualAnchorWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Silhouette", error, path); if (field == nullptr || !fromValue(*field, value.Silhouette, error, path + ".Silhouette")) { return false; }
            field = requiredField(json, "Cockpit", error, path); if (field == nullptr || !fromValue(*field, value.Cockpit, error, path + ".Cockpit")) { return false; }
            field = requiredField(json, "Wings", error, path); if (field == nullptr || !fromValue(*field, value.Wings, error, path + ".Wings")) { return false; }
            field = requiredField(json, "Engines", error, path); if (field == nullptr || !fromValue(*field, value.Engines, error, path + ".Engines")) { return false; }
            field = requiredField(json, "Weapons", error, path); if (field == nullptr || !fromValue(*field, value.Weapons, error, path + ".Weapons")) { return false; }
            field = requiredField(json, "MajorFeature", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeature, error, path + ".MajorFeature")) { return false; }
            field = requiredField(json, "HullLayers", error, path); if (field == nullptr || !fromValue(*field, value.HullLayers, error, path + ".HullLayers")) { return false; }
            field = requiredField(json, "CentralCore", error, path); if (field == nullptr || !fromValue(*field, value.CentralCore, error, path + ".CentralCore")) { return false; }
            field = requiredField(json, "MacroAsymmetry", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetry, error, path + ".MacroAsymmetry")) { return false; }
            field = requiredField(json, "NegativeSpace", error, path); if (field == nullptr || !fromValue(*field, value.NegativeSpace, error, path + ".NegativeSpace")) { return false; }
            return true;
        }

        Value toValue(const ShipComplexityCategoryWeights& value)
        {
            Value object = Value::object();
            object.Object["Silhouette"] = toValue(value.Silhouette);
            object.Object["CockpitStructure"] = toValue(value.CockpitStructure);
            object.Object["HullLayer"] = toValue(value.HullLayer);
            object.Object["MajorFeature"] = toValue(value.MajorFeature);
            object.Object["LargeWeapon"] = toValue(value.LargeWeapon);
            object.Object["Attachment"] = toValue(value.Attachment);
            object.Object["Detail"] = toValue(value.Detail);
            return object;
        }

        bool fromValue(const Value& json, ShipComplexityCategoryWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Silhouette", error, path); if (field == nullptr || !fromValue(*field, value.Silhouette, error, path + ".Silhouette")) { return false; }
            field = requiredField(json, "CockpitStructure", error, path); if (field == nullptr || !fromValue(*field, value.CockpitStructure, error, path + ".CockpitStructure")) { return false; }
            field = requiredField(json, "HullLayer", error, path); if (field == nullptr || !fromValue(*field, value.HullLayer, error, path + ".HullLayer")) { return false; }
            field = requiredField(json, "MajorFeature", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeature, error, path + ".MajorFeature")) { return false; }
            field = requiredField(json, "LargeWeapon", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeapon, error, path + ".LargeWeapon")) { return false; }
            field = requiredField(json, "Attachment", error, path); if (field == nullptr || !fromValue(*field, value.Attachment, error, path + ".Attachment")) { return false; }
            field = requiredField(json, "Detail", error, path); if (field == nullptr || !fromValue(*field, value.Detail, error, path + ".Detail")) { return false; }
            return true;
        }

        Value toValue(const ShipSpatialCapacityBias& value)
        {
            Value object = Value::object();
            object.Object["Nose"] = toValue(value.Nose);
            object.Object["FrontFuselage"] = toValue(value.FrontFuselage);
            object.Object["MidFuselage"] = toValue(value.MidFuselage);
            object.Object["RearFuselage"] = toValue(value.RearFuselage);
            object.Object["WingRoot"] = toValue(value.WingRoot);
            object.Object["OuterWing"] = toValue(value.OuterWing);
            return object;
        }

        bool fromValue(const Value& json, ShipSpatialCapacityBias& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Nose", error, path); if (field == nullptr || !fromValue(*field, value.Nose, error, path + ".Nose")) { return false; }
            field = requiredField(json, "FrontFuselage", error, path); if (field == nullptr || !fromValue(*field, value.FrontFuselage, error, path + ".FrontFuselage")) { return false; }
            field = requiredField(json, "MidFuselage", error, path); if (field == nullptr || !fromValue(*field, value.MidFuselage, error, path + ".MidFuselage")) { return false; }
            field = requiredField(json, "RearFuselage", error, path); if (field == nullptr || !fromValue(*field, value.RearFuselage, error, path + ".RearFuselage")) { return false; }
            field = requiredField(json, "WingRoot", error, path); if (field == nullptr || !fromValue(*field, value.WingRoot, error, path + ".WingRoot")) { return false; }
            field = requiredField(json, "OuterWing", error, path); if (field == nullptr || !fromValue(*field, value.OuterWing, error, path + ".OuterWing")) { return false; }
            return true;
        }

        Value toValue(const UIntRange& value)
        {
            Value object = Value::object();
            object.Object["Min"] = toValue(value.Min);
            object.Object["Max"] = toValue(value.Max);
            return object;
        }

        bool fromValue(const Value& json, UIntRange& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Min", error, path); if (field == nullptr || !fromValue(*field, value.Min, error, path + ".Min")) { return false; }
            field = requiredField(json, "Max", error, path); if (field == nullptr || !fromValue(*field, value.Max, error, path + ".Max")) { return false; }
            return true;
        }

        Value toValue(const ShipSilhouetteGuidanceWeights& value)
        {
            Value object = Value::object();
            object.Object["BroaderShoulders"] = toValue(value.BroaderShoulders);
            object.Object["SideLobes"] = toValue(value.SideLobes);
            object.Object["SteppedWingExtension"] = toValue(value.SteppedWingExtension);
            return object;
        }

        bool fromValue(const Value& json, ShipSilhouetteGuidanceWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "BroaderShoulders", error, path); if (field == nullptr || !fromValue(*field, value.BroaderShoulders, error, path + ".BroaderShoulders")) { return false; }
            field = requiredField(json, "SideLobes", error, path); if (field == nullptr || !fromValue(*field, value.SideLobes, error, path + ".SideLobes")) { return false; }
            field = requiredField(json, "SteppedWingExtension", error, path); if (field == nullptr || !fromValue(*field, value.SteppedWingExtension, error, path + ".SteppedWingExtension")) { return false; }
            return true;
        }

        Value toValue(const ShipStructuralNegativeSpaceWeights& value)
        {
            Value object = Value::object();
            object.Object["WingChannel"] = toValue(value.WingChannel);
            object.Object["RearFork"] = toValue(value.RearFork);
            object.Object["ShoulderGap"] = toValue(value.ShoulderGap);
            object.Object["OpenFrameBay"] = toValue(value.OpenFrameBay);
            object.Object["NacelleChannel"] = toValue(value.NacelleChannel);
            return object;
        }

        bool fromValue(const Value& json, ShipStructuralNegativeSpaceWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WingChannel", error, path); if (field == nullptr || !fromValue(*field, value.WingChannel, error, path + ".WingChannel")) { return false; }
            field = requiredField(json, "RearFork", error, path); if (field == nullptr || !fromValue(*field, value.RearFork, error, path + ".RearFork")) { return false; }
            field = requiredField(json, "ShoulderGap", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderGap, error, path + ".ShoulderGap")) { return false; }
            field = requiredField(json, "OpenFrameBay", error, path); if (field == nullptr || !fromValue(*field, value.OpenFrameBay, error, path + ".OpenFrameBay")) { return false; }
            field = requiredField(json, "NacelleChannel", error, path); if (field == nullptr || !fromValue(*field, value.NacelleChannel, error, path + ".NacelleChannel")) { return false; }
            return true;
        }

        Value toValue(const ShipCoreTreatmentWeights& value)
        {
            Value object = Value::object();
            object.Object["CentralSpine"] = toValue(value.CentralSpine);
            object.Object["CockpitSurround"] = toValue(value.CockpitSurround);
            object.Object["RaisedCorePlate"] = toValue(value.RaisedCorePlate);
            object.Object["LateralRecesses"] = toValue(value.LateralRecesses);
            object.Object["LongitudinalArmorBand"] = toValue(value.LongitudinalArmorBand);
            object.Object["CoreChannel"] = toValue(value.CoreChannel);
            return object;
        }

        bool fromValue(const Value& json, ShipCoreTreatmentWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralSpine", error, path); if (field == nullptr || !fromValue(*field, value.CentralSpine, error, path + ".CentralSpine")) { return false; }
            field = requiredField(json, "CockpitSurround", error, path); if (field == nullptr || !fromValue(*field, value.CockpitSurround, error, path + ".CockpitSurround")) { return false; }
            field = requiredField(json, "RaisedCorePlate", error, path); if (field == nullptr || !fromValue(*field, value.RaisedCorePlate, error, path + ".RaisedCorePlate")) { return false; }
            field = requiredField(json, "LateralRecesses", error, path); if (field == nullptr || !fromValue(*field, value.LateralRecesses, error, path + ".LateralRecesses")) { return false; }
            field = requiredField(json, "LongitudinalArmorBand", error, path); if (field == nullptr || !fromValue(*field, value.LongitudinalArmorBand, error, path + ".LongitudinalArmorBand")) { return false; }
            field = requiredField(json, "CoreChannel", error, path); if (field == nullptr || !fromValue(*field, value.CoreChannel, error, path + ".CoreChannel")) { return false; }
            return true;
        }

        Value toValue(const ShipHullLayerWeights& value)
        {
            Value object = Value::object();
            object.Object["CentralDorsalPlate"] = toValue(value.CentralDorsalPlate);
            object.Object["ForwardArmor"] = toValue(value.ForwardArmor);
            object.Object["WingArmor"] = toValue(value.WingArmor);
            object.Object["ShoulderArmor"] = toValue(value.ShoulderArmor);
            object.Object["RearEngineCover"] = toValue(value.RearEngineCover);
            return object;
        }

        bool fromValue(const Value& json, ShipHullLayerWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralDorsalPlate", error, path); if (field == nullptr || !fromValue(*field, value.CentralDorsalPlate, error, path + ".CentralDorsalPlate")) { return false; }
            field = requiredField(json, "ForwardArmor", error, path); if (field == nullptr || !fromValue(*field, value.ForwardArmor, error, path + ".ForwardArmor")) { return false; }
            field = requiredField(json, "WingArmor", error, path); if (field == nullptr || !fromValue(*field, value.WingArmor, error, path + ".WingArmor")) { return false; }
            field = requiredField(json, "ShoulderArmor", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderArmor, error, path + ".ShoulderArmor")) { return false; }
            field = requiredField(json, "RearEngineCover", error, path); if (field == nullptr || !fromValue(*field, value.RearEngineCover, error, path + ".RearEngineCover")) { return false; }
            return true;
        }

        Value toValue(const ShipCockpitSizeWeights& value)
        {
            Value object = Value::object();
            object.Object["Compact"] = toValue(value.Compact);
            object.Object["Standard"] = toValue(value.Standard);
            object.Object["Large"] = toValue(value.Large);
            object.Object["Massive"] = toValue(value.Massive);
            return object;
        }

        bool fromValue(const Value& json, ShipCockpitSizeWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Compact", error, path); if (field == nullptr || !fromValue(*field, value.Compact, error, path + ".Compact")) { return false; }
            field = requiredField(json, "Standard", error, path); if (field == nullptr || !fromValue(*field, value.Standard, error, path + ".Standard")) { return false; }
            field = requiredField(json, "Large", error, path); if (field == nullptr || !fromValue(*field, value.Large, error, path + ".Large")) { return false; }
            field = requiredField(json, "Massive", error, path); if (field == nullptr || !fromValue(*field, value.Massive, error, path + ".Massive")) { return false; }
            return true;
        }

        Value toValue(const ShipCockpitShapeWeights& value)
        {
            Value object = Value::object();
            object.Object["CompactCanopy"] = toValue(value.CompactCanopy);
            object.Object["ElongatedCanopy"] = toValue(value.ElongatedCanopy);
            object.Object["WideCommandDeck"] = toValue(value.WideCommandDeck);
            object.Object["SplitCanopy"] = toValue(value.SplitCanopy);
            object.Object["DorsalBridge"] = toValue(value.DorsalBridge);
            object.Object["LayeredBridge"] = toValue(value.LayeredBridge);
            return object;
        }

        bool fromValue(const Value& json, ShipCockpitShapeWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CompactCanopy", error, path); if (field == nullptr || !fromValue(*field, value.CompactCanopy, error, path + ".CompactCanopy")) { return false; }
            field = requiredField(json, "ElongatedCanopy", error, path); if (field == nullptr || !fromValue(*field, value.ElongatedCanopy, error, path + ".ElongatedCanopy")) { return false; }
            field = requiredField(json, "WideCommandDeck", error, path); if (field == nullptr || !fromValue(*field, value.WideCommandDeck, error, path + ".WideCommandDeck")) { return false; }
            field = requiredField(json, "SplitCanopy", error, path); if (field == nullptr || !fromValue(*field, value.SplitCanopy, error, path + ".SplitCanopy")) { return false; }
            field = requiredField(json, "DorsalBridge", error, path); if (field == nullptr || !fromValue(*field, value.DorsalBridge, error, path + ".DorsalBridge")) { return false; }
            field = requiredField(json, "LayeredBridge", error, path); if (field == nullptr || !fromValue(*field, value.LayeredBridge, error, path + ".LayeredBridge")) { return false; }
            return true;
        }

        Value toValue(const ShipMajorFeatureWeights& value)
        {
            Value object = Value::object();
            object.Object["CentralSpine"] = toValue(value.CentralSpine);
            object.Object["ArmorPlate"] = toValue(value.ArmorPlate);
            object.Object["RecessedBay"] = toValue(value.RecessedBay);
            object.Object["VentBank"] = toValue(value.VentBank);
            object.Object["WingPlate"] = toValue(value.WingPlate);
            object.Object["TechCore"] = toValue(value.TechCore);
            return object;
        }

        bool fromValue(const Value& json, ShipMajorFeatureWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralSpine", error, path); if (field == nullptr || !fromValue(*field, value.CentralSpine, error, path + ".CentralSpine")) { return false; }
            field = requiredField(json, "ArmorPlate", error, path); if (field == nullptr || !fromValue(*field, value.ArmorPlate, error, path + ".ArmorPlate")) { return false; }
            field = requiredField(json, "RecessedBay", error, path); if (field == nullptr || !fromValue(*field, value.RecessedBay, error, path + ".RecessedBay")) { return false; }
            field = requiredField(json, "VentBank", error, path); if (field == nullptr || !fromValue(*field, value.VentBank, error, path + ".VentBank")) { return false; }
            field = requiredField(json, "WingPlate", error, path); if (field == nullptr || !fromValue(*field, value.WingPlate, error, path + ".WingPlate")) { return false; }
            field = requiredField(json, "TechCore", error, path); if (field == nullptr || !fromValue(*field, value.TechCore, error, path + ".TechCore")) { return false; }
            return true;
        }

        Value toValue(const ShipWeaponWeights& value)
        {
            Value object = Value::object();
            object.Object["SingleCannon"] = toValue(value.SingleCannon);
            object.Object["TwinCannon"] = toValue(value.TwinCannon);
            object.Object["CompactTurret"] = toValue(value.CompactTurret);
            object.Object["RailWeapon"] = toValue(value.RailWeapon);
            object.Object["WeaponPod"] = toValue(value.WeaponPod);
            return object;
        }

        bool fromValue(const Value& json, ShipWeaponWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "SingleCannon", error, path); if (field == nullptr || !fromValue(*field, value.SingleCannon, error, path + ".SingleCannon")) { return false; }
            field = requiredField(json, "TwinCannon", error, path); if (field == nullptr || !fromValue(*field, value.TwinCannon, error, path + ".TwinCannon")) { return false; }
            field = requiredField(json, "CompactTurret", error, path); if (field == nullptr || !fromValue(*field, value.CompactTurret, error, path + ".CompactTurret")) { return false; }
            field = requiredField(json, "RailWeapon", error, path); if (field == nullptr || !fromValue(*field, value.RailWeapon, error, path + ".RailWeapon")) { return false; }
            field = requiredField(json, "WeaponPod", error, path); if (field == nullptr || !fromValue(*field, value.WeaponPod, error, path + ".WeaponPod")) { return false; }
            return true;
        }

        Value toValue(const ShipWeaponHardpointWeights& value)
        {
            Value object = Value::object();
            object.Object["CentralNose"] = toValue(value.CentralNose);
            object.Object["ForwardFuselageSide"] = toValue(value.ForwardFuselageSide);
            object.Object["WingRoot"] = toValue(value.WingRoot);
            object.Object["OuterWing"] = toValue(value.OuterWing);
            object.Object["ForwardShoulder"] = toValue(value.ForwardShoulder);
            object.Object["CentralBody"] = toValue(value.CentralBody);
            return object;
        }

        bool fromValue(const Value& json, ShipWeaponHardpointWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralNose", error, path); if (field == nullptr || !fromValue(*field, value.CentralNose, error, path + ".CentralNose")) { return false; }
            field = requiredField(json, "ForwardFuselageSide", error, path); if (field == nullptr || !fromValue(*field, value.ForwardFuselageSide, error, path + ".ForwardFuselageSide")) { return false; }
            field = requiredField(json, "WingRoot", error, path); if (field == nullptr || !fromValue(*field, value.WingRoot, error, path + ".WingRoot")) { return false; }
            field = requiredField(json, "OuterWing", error, path); if (field == nullptr || !fromValue(*field, value.OuterWing, error, path + ".OuterWing")) { return false; }
            field = requiredField(json, "ForwardShoulder", error, path); if (field == nullptr || !fromValue(*field, value.ForwardShoulder, error, path + ".ForwardShoulder")) { return false; }
            field = requiredField(json, "CentralBody", error, path); if (field == nullptr || !fromValue(*field, value.CentralBody, error, path + ".CentralBody")) { return false; }
            return true;
        }

        Value toValue(const ShipDetailMotifWeights& value)
        {
            Value object = Value::object();
            object.Object["PairedVents"] = toValue(value.PairedVents);
            object.Object["TripleVentBank"] = toValue(value.TripleVentBank);
            object.Object["PairedLights"] = toValue(value.PairedLights);
            object.Object["ThreeNodeLights"] = toValue(value.ThreeNodeLights);
            object.Object["ParallelSeams"] = toValue(value.ParallelSeams);
            object.Object["RepeatedDashes"] = toValue(value.RepeatedDashes);
            object.Object["RecessedSlot"] = toValue(value.RecessedSlot);
            return object;
        }

        bool fromValue(const Value& json, ShipDetailMotifWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "PairedVents", error, path); if (field == nullptr || !fromValue(*field, value.PairedVents, error, path + ".PairedVents")) { return false; }
            field = requiredField(json, "TripleVentBank", error, path); if (field == nullptr || !fromValue(*field, value.TripleVentBank, error, path + ".TripleVentBank")) { return false; }
            field = requiredField(json, "PairedLights", error, path); if (field == nullptr || !fromValue(*field, value.PairedLights, error, path + ".PairedLights")) { return false; }
            field = requiredField(json, "ThreeNodeLights", error, path); if (field == nullptr || !fromValue(*field, value.ThreeNodeLights, error, path + ".ThreeNodeLights")) { return false; }
            field = requiredField(json, "ParallelSeams", error, path); if (field == nullptr || !fromValue(*field, value.ParallelSeams, error, path + ".ParallelSeams")) { return false; }
            field = requiredField(json, "RepeatedDashes", error, path); if (field == nullptr || !fromValue(*field, value.RepeatedDashes, error, path + ".RepeatedDashes")) { return false; }
            field = requiredField(json, "RecessedSlot", error, path); if (field == nullptr || !fromValue(*field, value.RecessedSlot, error, path + ".RecessedSlot")) { return false; }
            return true;
        }

        Value toValue(const ShipAttachmentWeights& value)
        {
            Value object = Value::object();
            object.Object["WeaponMount"] = toValue(value.WeaponMount);
            object.Object["SensorArray"] = toValue(value.SensorArray);
            object.Object["AuxiliaryPod"] = toValue(value.AuxiliaryPod);
            object.Object["Radiator"] = toValue(value.Radiator);
            object.Object["ArmorFin"] = toValue(value.ArmorFin);
            object.Object["TechnologyNode"] = toValue(value.TechnologyNode);
            return object;
        }

        bool fromValue(const Value& json, ShipAttachmentWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WeaponMount", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMount, error, path + ".WeaponMount")) { return false; }
            field = requiredField(json, "SensorArray", error, path); if (field == nullptr || !fromValue(*field, value.SensorArray, error, path + ".SensorArray")) { return false; }
            field = requiredField(json, "AuxiliaryPod", error, path); if (field == nullptr || !fromValue(*field, value.AuxiliaryPod, error, path + ".AuxiliaryPod")) { return false; }
            field = requiredField(json, "Radiator", error, path); if (field == nullptr || !fromValue(*field, value.Radiator, error, path + ".Radiator")) { return false; }
            field = requiredField(json, "ArmorFin", error, path); if (field == nullptr || !fromValue(*field, value.ArmorFin, error, path + ".ArmorFin")) { return false; }
            field = requiredField(json, "TechnologyNode", error, path); if (field == nullptr || !fromValue(*field, value.TechnologyNode, error, path + ".TechnologyNode")) { return false; }
            return true;
        }

        Value toValue(const ShipMaterialZoneWeights& value)
        {
            Value object = Value::object();
            object.Object["WingSurface"] = toValue(value.WingSurface);
            object.Object["ShoulderSurface"] = toValue(value.ShoulderSurface);
            object.Object["AxialBand"] = toValue(value.AxialBand);
            object.Object["RearMechanical"] = toValue(value.RearMechanical);
            object.Object["CockpitCollar"] = toValue(value.CockpitCollar);
            object.Object["HardpointSurround"] = toValue(value.HardpointSurround);
            return object;
        }

        bool fromValue(const Value& json, ShipMaterialZoneWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WingSurface", error, path); if (field == nullptr || !fromValue(*field, value.WingSurface, error, path + ".WingSurface")) { return false; }
            field = requiredField(json, "ShoulderSurface", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderSurface, error, path + ".ShoulderSurface")) { return false; }
            field = requiredField(json, "AxialBand", error, path); if (field == nullptr || !fromValue(*field, value.AxialBand, error, path + ".AxialBand")) { return false; }
            field = requiredField(json, "RearMechanical", error, path); if (field == nullptr || !fromValue(*field, value.RearMechanical, error, path + ".RearMechanical")) { return false; }
            field = requiredField(json, "CockpitCollar", error, path); if (field == nullptr || !fromValue(*field, value.CockpitCollar, error, path + ".CockpitCollar")) { return false; }
            field = requiredField(json, "HardpointSurround", error, path); if (field == nullptr || !fromValue(*field, value.HardpointSurround, error, path + ".HardpointSurround")) { return false; }
            return true;
        }

        Value toValue(const ShipLiveryWeights& value)
        {
            Value object = Value::object();
            object.Object["CenterStripe"] = toValue(value.CenterStripe);
            object.Object["DoubleCenterStripe"] = toValue(value.DoubleCenterStripe);
            object.Object["WingBand"] = toValue(value.WingBand);
            object.Object["ShoulderBlock"] = toValue(value.ShoulderBlock);
            object.Object["NoseBand"] = toValue(value.NoseBand);
            object.Object["Chevron"] = toValue(value.Chevron);
            object.Object["IdPanel"] = toValue(value.IdPanel);
            object.Object["GeometricInsignia"] = toValue(value.GeometricInsignia);
            return object;
        }

        bool fromValue(const Value& json, ShipLiveryWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CenterStripe", error, path); if (field == nullptr || !fromValue(*field, value.CenterStripe, error, path + ".CenterStripe")) { return false; }
            field = requiredField(json, "DoubleCenterStripe", error, path); if (field == nullptr || !fromValue(*field, value.DoubleCenterStripe, error, path + ".DoubleCenterStripe")) { return false; }
            field = requiredField(json, "WingBand", error, path); if (field == nullptr || !fromValue(*field, value.WingBand, error, path + ".WingBand")) { return false; }
            field = requiredField(json, "ShoulderBlock", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderBlock, error, path + ".ShoulderBlock")) { return false; }
            field = requiredField(json, "NoseBand", error, path); if (field == nullptr || !fromValue(*field, value.NoseBand, error, path + ".NoseBand")) { return false; }
            field = requiredField(json, "Chevron", error, path); if (field == nullptr || !fromValue(*field, value.Chevron, error, path + ".Chevron")) { return false; }
            field = requiredField(json, "IdPanel", error, path); if (field == nullptr || !fromValue(*field, value.IdPanel, error, path + ".IdPanel")) { return false; }
            field = requiredField(json, "GeometricInsignia", error, path); if (field == nullptr || !fromValue(*field, value.GeometricInsignia, error, path + ".GeometricInsignia")) { return false; }
            return true;
        }

        Value toValue(const ShipMacroAsymmetryCategoryWeights& value)
        {
            Value object = Value::object();
            object.Object["HullLayer"] = toValue(value.HullLayer);
            object.Object["LargeWeapon"] = toValue(value.LargeWeapon);
            object.Object["Attachment"] = toValue(value.Attachment);
            return object;
        }

        bool fromValue(const Value& json, ShipMacroAsymmetryCategoryWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "HullLayer", error, path); if (field == nullptr || !fromValue(*field, value.HullLayer, error, path + ".HullLayer")) { return false; }
            field = requiredField(json, "LargeWeapon", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeapon, error, path + ".LargeWeapon")) { return false; }
            field = requiredField(json, "Attachment", error, path); if (field == nullptr || !fromValue(*field, value.Attachment, error, path + ".Attachment")) { return false; }
            return true;
        }

        Value toValue(const SupplementalSurfaceDetailWeights& value)
        {
            Value object = Value::object();
            object.Object["PanelSeam"] = toValue(value.PanelSeam);
            object.Object["GeometricMarking"] = toValue(value.GeometricMarking);
            object.Object["MechanicalExposure"] = toValue(value.MechanicalExposure);
            object.Object["RepeatingMotif"] = toValue(value.RepeatingMotif);
            object.Object["IdentificationMarking"] = toValue(value.IdentificationMarking);
            object.Object["LuminousChannel"] = toValue(value.LuminousChannel);
            return object;
        }

        bool fromValue(const Value& json, SupplementalSurfaceDetailWeights& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "PanelSeam", error, path); if (field == nullptr || !fromValue(*field, value.PanelSeam, error, path + ".PanelSeam")) { return false; }
            field = requiredField(json, "GeometricMarking", error, path); if (field == nullptr || !fromValue(*field, value.GeometricMarking, error, path + ".GeometricMarking")) { return false; }
            field = requiredField(json, "MechanicalExposure", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalExposure, error, path + ".MechanicalExposure")) { return false; }
            field = requiredField(json, "RepeatingMotif", error, path); if (field == nullptr || !fromValue(*field, value.RepeatingMotif, error, path + ".RepeatingMotif")) { return false; }
            field = requiredField(json, "IdentificationMarking", error, path); if (field == nullptr || !fromValue(*field, value.IdentificationMarking, error, path + ".IdentificationMarking")) { return false; }
            field = requiredField(json, "LuminousChannel", error, path); if (field == nullptr || !fromValue(*field, value.LuminousChannel, error, path + ".LuminousChannel")) { return false; }
            return true;
        }

        Value toValue(const ShipGenerationProfile& value)
        {
            Value object = Value::object();
            object.Object["AnimationTraits"] = toValue(value.AnimationTraits);
            object.Object["VisualAnchorWeights"] = toValue(value.VisualAnchorWeights);
            object.Object["VisualSecondaryAnchorChance"] = toValue(value.VisualSecondaryAnchorChance);
            object.Object["VisualHierarchyEnabled"] = toValue(value.VisualHierarchyEnabled);
            object.Object["HullLayerHierarchyUsesWingRoot"] = toValue(value.HullLayerHierarchyUsesWingRoot);
            object.Object["WeaponHierarchyUsesWingRoot"] = toValue(value.WeaponHierarchyUsesWingRoot);
            object.Object["ComplexityBudgetPercent"] = toValue(value.ComplexityBudgetPercent);
            object.Object["ComplexityCategoryWeights"] = toValue(value.ComplexityCategoryWeights);
            object.Object["LegacyComplexityCategoryWeights"] = toValue(value.LegacyComplexityCategoryWeights);
            object.Object["SpatialCapacityBias"] = toValue(value.SpatialCapacityBias);
            object.Object["HullVerticalPaddingPercent"] = toValue(value.HullVerticalPaddingPercent);
            object.Object["HullHorizontalPaddingPercent"] = toValue(value.HullHorizontalPaddingPercent);
            object.Object["NoseEndPercent"] = toValue(value.NoseEndPercent);
            object.Object["UpperFuselageEndPercent"] = toValue(value.UpperFuselageEndPercent);
            object.Object["MainBodyEndPercent"] = toValue(value.MainBodyEndPercent);
            object.Object["RearFuselageStartPercent"] = toValue(value.RearFuselageStartPercent);
            object.Object["NoseWidthPercent"] = toValue(value.NoseWidthPercent);
            object.Object["UpperFuselageWidthPercent"] = toValue(value.UpperFuselageWidthPercent);
            object.Object["MainBodyWidthPercent"] = toValue(value.MainBodyWidthPercent);
            object.Object["RearFuselageWidthPercent"] = toValue(value.RearFuselageWidthPercent);
            object.Object["RearWidthPercent"] = toValue(value.RearWidthPercent);
            object.Object["NoWingWeight"] = toValue(value.NoWingWeight);
            object.Object["SmallWingWeight"] = toValue(value.SmallWingWeight);
            object.Object["SweptWingWeight"] = toValue(value.SweptWingWeight);
            object.Object["BroadWingWeight"] = toValue(value.BroadWingWeight);
            object.Object["SmallWingIncreasePercent"] = toValue(value.SmallWingIncreasePercent);
            object.Object["SweptWingWidthPercent"] = toValue(value.SweptWingWidthPercent);
            object.Object["BroadWingWidthPercent"] = toValue(value.BroadWingWidthPercent);
            object.Object["WingLongitudinalOffsetPercent"] = toValue(value.WingLongitudinalOffsetPercent);
            object.Object["WingRootLengthPercent"] = toValue(value.WingRootLengthPercent);
            object.Object["WingRootWidthPercent"] = toValue(value.WingRootWidthPercent);
            object.Object["SmallWingTaperPercent"] = toValue(value.SmallWingTaperPercent);
            object.Object["SweptWingTaperPercent"] = toValue(value.SweptWingTaperPercent);
            object.Object["BroadWingTaperPercent"] = toValue(value.BroadWingTaperPercent);
            object.Object["HullModifierChance"] = toValue(value.HullModifierChance);
            object.Object["MaximumHullModifiers"] = toValue(value.MaximumHullModifiers);
            object.Object["BroaderShouldersModifierWeight"] = toValue(value.BroaderShouldersModifierWeight);
            object.Object["SideLobesModifierWeight"] = toValue(value.SideLobesModifierWeight);
            object.Object["SteppedWingModifierWeight"] = toValue(value.SteppedWingModifierWeight);
            object.Object["NarrowWaistModifierWeight"] = toValue(value.NarrowWaistModifierWeight);
            object.Object["WingCutoutModifierWeight"] = toValue(value.WingCutoutModifierWeight);
            object.Object["SplitNoseModifierWeight"] = toValue(value.SplitNoseModifierWeight);
            object.Object["MinimumSilhouetteWidthPercent"] = toValue(value.MinimumSilhouetteWidthPercent);
            object.Object["MinimumSilhouetteHeightPercent"] = toValue(value.MinimumSilhouetteHeightPercent);
            object.Object["SilhouetteArticulationTarget"] = toValue(value.SilhouetteArticulationTarget);
            object.Object["SilhouetteMaximumStableRunPercent"] = toValue(value.SilhouetteMaximumStableRunPercent);
            object.Object["SilhouetteConvexFillTriggerPercent"] = toValue(value.SilhouetteConvexFillTriggerPercent);
            object.Object["SilhouetteGuidanceEnabled"] = toValue(value.SilhouetteGuidanceEnabled);
            object.Object["SilhouetteWeakArticulationGuidanceEnabled"] = toValue(value.SilhouetteWeakArticulationGuidanceEnabled);
            object.Object["SilhouetteGuidanceWeights"] = toValue(value.SilhouetteGuidanceWeights);
            object.Object["SilhouetteProfileValidationEnabled"] = toValue(value.SilhouetteProfileValidationEnabled);
            object.Object["AllowTinyBroadSilhouetteLegacyValidationException"] = toValue(value.AllowTinyBroadSilhouetteLegacyValidationException);
            object.Object["TinySilhouetteExtraWidthRelaxationPercent"] = toValue(value.TinySilhouetteExtraWidthRelaxationPercent);
            object.Object["CleanAxialTaperArticulationExemption"] = toValue(value.CleanAxialTaperArticulationExemption);
            object.Object["WingWedgeArticulationExemption"] = toValue(value.WingWedgeArticulationExemption);
            object.Object["StructuralNegativeSpaceChance"] = toValue(value.StructuralNegativeSpaceChance);
            object.Object["MaximumStructuralNegativeSpaceStructures"] = toValue(value.MaximumStructuralNegativeSpaceStructures);
            object.Object["StructuralNegativeSpaceScalePercent"] = toValue(value.StructuralNegativeSpaceScalePercent);
            object.Object["StructuralNegativeSpaceWeights"] = toValue(value.StructuralNegativeSpaceWeights);
            object.Object["RearForkStartPercent"] = toValue(value.RearForkStartPercent);
            object.Object["CoreTreatmentChance"] = toValue(value.CoreTreatmentChance);
            object.Object["CoreRegionWidthBasePercent"] = toValue(value.CoreRegionWidthBasePercent);
            object.Object["CoreRegionWidthHorizontalCapacityDivisor"] = toValue(value.CoreRegionWidthHorizontalCapacityDivisor);
            object.Object["CoreRegionWidthMaximumPercent"] = toValue(value.CoreRegionWidthMaximumPercent);
            object.Object["RaisedCorePlateWidthPercent"] = toValue(value.RaisedCorePlateWidthPercent);
            object.Object["MaximumCoreTreatments"] = toValue(value.MaximumCoreTreatments);
            object.Object["CoreTreatmentWeights"] = toValue(value.CoreTreatmentWeights);
            object.Object["HullLayerChance"] = toValue(value.HullLayerChance);
            object.Object["MaximumHullLayers"] = toValue(value.MaximumHullLayers);
            object.Object["HullLayerWeights"] = toValue(value.HullLayerWeights);
            object.Object["CockpitStartPercent"] = toValue(value.CockpitStartPercent);
            object.Object["CockpitHeightPercent"] = toValue(value.CockpitHeightPercent);
            object.Object["CockpitWidthPercent"] = toValue(value.CockpitWidthPercent);
            object.Object["MaximumCockpitHullPercent"] = toValue(value.MaximumCockpitHullPercent);
            object.Object["CockpitSizeWeights"] = toValue(value.CockpitSizeWeights);
            object.Object["CockpitShapeWeights"] = toValue(value.CockpitShapeWeights);
            object.Object["CentralEngineWeight"] = toValue(value.CentralEngineWeight);
            object.Object["TwinEngineWeight"] = toValue(value.TwinEngineWeight);
            object.Object["QuadEngineWeight"] = toValue(value.QuadEngineWeight);
            object.Object["CentralAuxiliaryEngineWeight"] = toValue(value.CentralAuxiliaryEngineWeight);
            object.Object["EngineBankWeight"] = toValue(value.EngineBankWeight);
            object.Object["SmallEngineSizeWeight"] = toValue(value.SmallEngineSizeWeight);
            object.Object["MediumEngineSizeWeight"] = toValue(value.MediumEngineSizeWeight);
            object.Object["LargeEngineSizeWeight"] = toValue(value.LargeEngineSizeWeight);
            object.Object["EngineNacelleChance"] = toValue(value.EngineNacelleChance);
            object.Object["MajorFeatureChance"] = toValue(value.MajorFeatureChance);
            object.Object["MaximumMajorFeatures"] = toValue(value.MaximumMajorFeatures);
            object.Object["MajorFeatureScalePercent"] = toValue(value.MajorFeatureScalePercent);
            object.Object["MajorFeatureWeights"] = toValue(value.MajorFeatureWeights);
            object.Object["LargeWeaponChance"] = toValue(value.LargeWeaponChance);
            object.Object["MaximumLargeWeaponGroups"] = toValue(value.MaximumLargeWeaponGroups);
            object.Object["LargeWeaponSymmetryChance"] = toValue(value.LargeWeaponSymmetryChance);
            object.Object["LargeWeaponScalePercent"] = toValue(value.LargeWeaponScalePercent);
            object.Object["LargeWeaponWeights"] = toValue(value.LargeWeaponWeights);
            object.Object["LargeWeaponHardpointWeights"] = toValue(value.LargeWeaponHardpointWeights);
            object.Object["DetailDensityPercent"] = toValue(value.DetailDensityPercent);
            object.Object["MechanicalPatternCountPercent"] = toValue(value.MechanicalPatternCountPercent);
            object.Object["DetailMotifChance"] = toValue(value.DetailMotifChance);
            object.Object["SecondaryDetailMotifChance"] = toValue(value.SecondaryDetailMotifChance);
            object.Object["DetailMotifRepeatPercent"] = toValue(value.DetailMotifRepeatPercent);
            object.Object["DetailMotifWeights"] = toValue(value.DetailMotifWeights);
            object.Object["DetailMotifMirroringBonusPercent"] = toValue(value.DetailMotifMirroringBonusPercent);
            object.Object["DetailMotifPlacementBias"] = toValue(value.DetailMotifPlacementBias);
            object.Object["DetailMotifOrientationBias"] = toValue(value.DetailMotifOrientationBias);
            object.Object["AccentPanelWeight"] = toValue(value.AccentPanelWeight);
            object.Object["AccentStripeWeight"] = toValue(value.AccentStripeWeight);
            object.Object["AccentArmorWeight"] = toValue(value.AccentArmorWeight);
            object.Object["HorizontalVentChance"] = toValue(value.HorizontalVentChance);
            object.Object["AttachmentWeights"] = toValue(value.AttachmentWeights);
            object.Object["AttachmentChance"] = toValue(value.AttachmentChance);
            object.Object["MaximumAttachmentGroups"] = toValue(value.MaximumAttachmentGroups);
            object.Object["SymmetricAttachmentChance"] = toValue(value.SymmetricAttachmentChance);
            object.Object["AttachmentSizePercent"] = toValue(value.AttachmentSizePercent);
            object.Object["MaterialCompositionChance"] = toValue(value.MaterialCompositionChance);
            object.Object["MaximumMaterialZones"] = toValue(value.MaximumMaterialZones);
            object.Object["MaterialSecondaryContrastPercent"] = toValue(value.MaterialSecondaryContrastPercent);
            object.Object["MaterialZoneWeights"] = toValue(value.MaterialZoneWeights);
            object.Object["MaterialWingSurfaceUsesFullWing"] = toValue(value.MaterialWingSurfaceUsesFullWing);
            object.Object["MaterialAxialBandWidthPercent"] = toValue(value.MaterialAxialBandWidthPercent);
            object.Object["LiveryChance"] = toValue(value.LiveryChance);
            object.Object["MaximumLiveryMarkings"] = toValue(value.MaximumLiveryMarkings);
            object.Object["SupportingLiveryChance"] = toValue(value.SupportingLiveryChance);
            object.Object["LiveryAsymmetricChance"] = toValue(value.LiveryAsymmetricChance);
            object.Object["MaximumLiveryCoveragePercent"] = toValue(value.MaximumLiveryCoveragePercent);
            object.Object["MaximumLiveryConnectedCoveragePercent"] = toValue(value.MaximumLiveryConnectedCoveragePercent);
            object.Object["LiveryWeights"] = toValue(value.LiveryWeights);
            object.Object["PaletteHullValueOffset"] = toValue(value.PaletteHullValueOffset);
            object.Object["PaletteContrastPercent"] = toValue(value.PaletteContrastPercent);
            object.Object["PaletteHullSaturationPercent"] = toValue(value.PaletteHullSaturationPercent);
            object.Object["PaletteAccentSaturationPercent"] = toValue(value.PaletteAccentSaturationPercent);
            object.Object["PaletteEmissiveValuePercent"] = toValue(value.PaletteEmissiveValuePercent);
            object.Object["SecondaryHullToneCoveragePercent"] = toValue(value.SecondaryHullToneCoveragePercent);
            object.Object["CoreRaisedSurfaceTone"] = toValue(value.CoreRaisedSurfaceTone);
            object.Object["CentralDorsalPlateTone"] = toValue(value.CentralDorsalPlateTone);
            object.Object["AxialRidgeUsesEdgeHighlight"] = toValue(value.AxialRidgeUsesEdgeHighlight);
            object.Object["MacroAsymmetryChance"] = toValue(value.MacroAsymmetryChance);
            object.Object["MacroAsymmetryCategoryWeights"] = toValue(value.MacroAsymmetryCategoryWeights);
            object.Object["MacroAsymmetryOuterRegionChance"] = toValue(value.MacroAsymmetryOuterRegionChance);
            object.Object["MacroAsymmetryWingRootRegionChance"] = toValue(value.MacroAsymmetryWingRootRegionChance);
            object.Object["MacroAsymmetryVisualWeightPercent"] = toValue(value.MacroAsymmetryVisualWeightPercent);
            object.Object["SupplementalDetailWeights"] = toValue(value.SupplementalDetailWeights);
            return object;
        }

        bool fromValue(const Value& json, ShipGenerationProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "AnimationTraits", error, path); if (field == nullptr || !fromValue(*field, value.AnimationTraits, error, path + ".AnimationTraits")) { return false; }
            field = requiredField(json, "VisualAnchorWeights", error, path); if (field == nullptr || !fromValue(*field, value.VisualAnchorWeights, error, path + ".VisualAnchorWeights")) { return false; }
            field = requiredField(json, "VisualSecondaryAnchorChance", error, path); if (field == nullptr || !fromValue(*field, value.VisualSecondaryAnchorChance, error, path + ".VisualSecondaryAnchorChance")) { return false; }
            field = requiredField(json, "VisualHierarchyEnabled", error, path); if (field == nullptr || !fromValue(*field, value.VisualHierarchyEnabled, error, path + ".VisualHierarchyEnabled")) { return false; }
            field = requiredField(json, "HullLayerHierarchyUsesWingRoot", error, path); if (field == nullptr || !fromValue(*field, value.HullLayerHierarchyUsesWingRoot, error, path + ".HullLayerHierarchyUsesWingRoot")) { return false; }
            field = requiredField(json, "WeaponHierarchyUsesWingRoot", error, path); if (field == nullptr || !fromValue(*field, value.WeaponHierarchyUsesWingRoot, error, path + ".WeaponHierarchyUsesWingRoot")) { return false; }
            field = requiredField(json, "ComplexityBudgetPercent", error, path); if (field == nullptr || !fromValue(*field, value.ComplexityBudgetPercent, error, path + ".ComplexityBudgetPercent")) { return false; }
            field = requiredField(json, "ComplexityCategoryWeights", error, path); if (field == nullptr || !fromValue(*field, value.ComplexityCategoryWeights, error, path + ".ComplexityCategoryWeights")) { return false; }
            field = requiredField(json, "LegacyComplexityCategoryWeights", error, path); if (field == nullptr || !fromValue(*field, value.LegacyComplexityCategoryWeights, error, path + ".LegacyComplexityCategoryWeights")) { return false; }
            field = requiredField(json, "SpatialCapacityBias", error, path); if (field == nullptr || !fromValue(*field, value.SpatialCapacityBias, error, path + ".SpatialCapacityBias")) { return false; }
            field = requiredField(json, "HullVerticalPaddingPercent", error, path); if (field == nullptr || !fromValue(*field, value.HullVerticalPaddingPercent, error, path + ".HullVerticalPaddingPercent")) { return false; }
            field = requiredField(json, "HullHorizontalPaddingPercent", error, path); if (field == nullptr || !fromValue(*field, value.HullHorizontalPaddingPercent, error, path + ".HullHorizontalPaddingPercent")) { return false; }
            field = requiredField(json, "NoseEndPercent", error, path); if (field == nullptr || !fromValue(*field, value.NoseEndPercent, error, path + ".NoseEndPercent")) { return false; }
            field = requiredField(json, "UpperFuselageEndPercent", error, path); if (field == nullptr || !fromValue(*field, value.UpperFuselageEndPercent, error, path + ".UpperFuselageEndPercent")) { return false; }
            field = requiredField(json, "MainBodyEndPercent", error, path); if (field == nullptr || !fromValue(*field, value.MainBodyEndPercent, error, path + ".MainBodyEndPercent")) { return false; }
            field = requiredField(json, "RearFuselageStartPercent", error, path); if (field == nullptr || !fromValue(*field, value.RearFuselageStartPercent, error, path + ".RearFuselageStartPercent")) { return false; }
            field = requiredField(json, "NoseWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.NoseWidthPercent, error, path + ".NoseWidthPercent")) { return false; }
            field = requiredField(json, "UpperFuselageWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.UpperFuselageWidthPercent, error, path + ".UpperFuselageWidthPercent")) { return false; }
            field = requiredField(json, "MainBodyWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.MainBodyWidthPercent, error, path + ".MainBodyWidthPercent")) { return false; }
            field = requiredField(json, "RearFuselageWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.RearFuselageWidthPercent, error, path + ".RearFuselageWidthPercent")) { return false; }
            field = requiredField(json, "RearWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.RearWidthPercent, error, path + ".RearWidthPercent")) { return false; }
            field = requiredField(json, "NoWingWeight", error, path); if (field == nullptr || !fromValue(*field, value.NoWingWeight, error, path + ".NoWingWeight")) { return false; }
            field = requiredField(json, "SmallWingWeight", error, path); if (field == nullptr || !fromValue(*field, value.SmallWingWeight, error, path + ".SmallWingWeight")) { return false; }
            field = requiredField(json, "SweptWingWeight", error, path); if (field == nullptr || !fromValue(*field, value.SweptWingWeight, error, path + ".SweptWingWeight")) { return false; }
            field = requiredField(json, "BroadWingWeight", error, path); if (field == nullptr || !fromValue(*field, value.BroadWingWeight, error, path + ".BroadWingWeight")) { return false; }
            field = requiredField(json, "SmallWingIncreasePercent", error, path); if (field == nullptr || !fromValue(*field, value.SmallWingIncreasePercent, error, path + ".SmallWingIncreasePercent")) { return false; }
            field = requiredField(json, "SweptWingWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.SweptWingWidthPercent, error, path + ".SweptWingWidthPercent")) { return false; }
            field = requiredField(json, "BroadWingWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.BroadWingWidthPercent, error, path + ".BroadWingWidthPercent")) { return false; }
            field = requiredField(json, "WingLongitudinalOffsetPercent", error, path); if (field == nullptr || !fromValue(*field, value.WingLongitudinalOffsetPercent, error, path + ".WingLongitudinalOffsetPercent")) { return false; }
            field = requiredField(json, "WingRootLengthPercent", error, path); if (field == nullptr || !fromValue(*field, value.WingRootLengthPercent, error, path + ".WingRootLengthPercent")) { return false; }
            field = requiredField(json, "WingRootWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.WingRootWidthPercent, error, path + ".WingRootWidthPercent")) { return false; }
            field = requiredField(json, "SmallWingTaperPercent", error, path); if (field == nullptr || !fromValue(*field, value.SmallWingTaperPercent, error, path + ".SmallWingTaperPercent")) { return false; }
            field = requiredField(json, "SweptWingTaperPercent", error, path); if (field == nullptr || !fromValue(*field, value.SweptWingTaperPercent, error, path + ".SweptWingTaperPercent")) { return false; }
            field = requiredField(json, "BroadWingTaperPercent", error, path); if (field == nullptr || !fromValue(*field, value.BroadWingTaperPercent, error, path + ".BroadWingTaperPercent")) { return false; }
            field = requiredField(json, "HullModifierChance", error, path); if (field == nullptr || !fromValue(*field, value.HullModifierChance, error, path + ".HullModifierChance")) { return false; }
            field = requiredField(json, "MaximumHullModifiers", error, path); if (field == nullptr || !fromValue(*field, value.MaximumHullModifiers, error, path + ".MaximumHullModifiers")) { return false; }
            field = requiredField(json, "BroaderShouldersModifierWeight", error, path); if (field == nullptr || !fromValue(*field, value.BroaderShouldersModifierWeight, error, path + ".BroaderShouldersModifierWeight")) { return false; }
            field = requiredField(json, "SideLobesModifierWeight", error, path); if (field == nullptr || !fromValue(*field, value.SideLobesModifierWeight, error, path + ".SideLobesModifierWeight")) { return false; }
            field = requiredField(json, "SteppedWingModifierWeight", error, path); if (field == nullptr || !fromValue(*field, value.SteppedWingModifierWeight, error, path + ".SteppedWingModifierWeight")) { return false; }
            field = requiredField(json, "NarrowWaistModifierWeight", error, path); if (field == nullptr || !fromValue(*field, value.NarrowWaistModifierWeight, error, path + ".NarrowWaistModifierWeight")) { return false; }
            field = requiredField(json, "WingCutoutModifierWeight", error, path); if (field == nullptr || !fromValue(*field, value.WingCutoutModifierWeight, error, path + ".WingCutoutModifierWeight")) { return false; }
            field = requiredField(json, "SplitNoseModifierWeight", error, path); if (field == nullptr || !fromValue(*field, value.SplitNoseModifierWeight, error, path + ".SplitNoseModifierWeight")) { return false; }
            field = requiredField(json, "MinimumSilhouetteWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.MinimumSilhouetteWidthPercent, error, path + ".MinimumSilhouetteWidthPercent")) { return false; }
            field = requiredField(json, "MinimumSilhouetteHeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.MinimumSilhouetteHeightPercent, error, path + ".MinimumSilhouetteHeightPercent")) { return false; }
            field = requiredField(json, "SilhouetteArticulationTarget", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteArticulationTarget, error, path + ".SilhouetteArticulationTarget")) { return false; }
            field = requiredField(json, "SilhouetteMaximumStableRunPercent", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteMaximumStableRunPercent, error, path + ".SilhouetteMaximumStableRunPercent")) { return false; }
            field = requiredField(json, "SilhouetteConvexFillTriggerPercent", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteConvexFillTriggerPercent, error, path + ".SilhouetteConvexFillTriggerPercent")) { return false; }
            field = requiredField(json, "SilhouetteGuidanceEnabled", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteGuidanceEnabled, error, path + ".SilhouetteGuidanceEnabled")) { return false; }
            field = requiredField(json, "SilhouetteWeakArticulationGuidanceEnabled", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteWeakArticulationGuidanceEnabled, error, path + ".SilhouetteWeakArticulationGuidanceEnabled")) { return false; }
            field = requiredField(json, "SilhouetteGuidanceWeights", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteGuidanceWeights, error, path + ".SilhouetteGuidanceWeights")) { return false; }
            field = requiredField(json, "SilhouetteProfileValidationEnabled", error, path); if (field == nullptr || !fromValue(*field, value.SilhouetteProfileValidationEnabled, error, path + ".SilhouetteProfileValidationEnabled")) { return false; }
            field = requiredField(json, "AllowTinyBroadSilhouetteLegacyValidationException", error, path); if (field == nullptr || !fromValue(*field, value.AllowTinyBroadSilhouetteLegacyValidationException, error, path + ".AllowTinyBroadSilhouetteLegacyValidationException")) { return false; }
            field = requiredField(json, "TinySilhouetteExtraWidthRelaxationPercent", error, path); if (field == nullptr || !fromValue(*field, value.TinySilhouetteExtraWidthRelaxationPercent, error, path + ".TinySilhouetteExtraWidthRelaxationPercent")) { return false; }
            field = requiredField(json, "CleanAxialTaperArticulationExemption", error, path); if (field == nullptr || !fromValue(*field, value.CleanAxialTaperArticulationExemption, error, path + ".CleanAxialTaperArticulationExemption")) { return false; }
            field = requiredField(json, "WingWedgeArticulationExemption", error, path); if (field == nullptr || !fromValue(*field, value.WingWedgeArticulationExemption, error, path + ".WingWedgeArticulationExemption")) { return false; }
            field = requiredField(json, "StructuralNegativeSpaceChance", error, path); if (field == nullptr || !fromValue(*field, value.StructuralNegativeSpaceChance, error, path + ".StructuralNegativeSpaceChance")) { return false; }
            field = requiredField(json, "MaximumStructuralNegativeSpaceStructures", error, path); if (field == nullptr || !fromValue(*field, value.MaximumStructuralNegativeSpaceStructures, error, path + ".MaximumStructuralNegativeSpaceStructures")) { return false; }
            field = requiredField(json, "StructuralNegativeSpaceScalePercent", error, path); if (field == nullptr || !fromValue(*field, value.StructuralNegativeSpaceScalePercent, error, path + ".StructuralNegativeSpaceScalePercent")) { return false; }
            field = requiredField(json, "StructuralNegativeSpaceWeights", error, path); if (field == nullptr || !fromValue(*field, value.StructuralNegativeSpaceWeights, error, path + ".StructuralNegativeSpaceWeights")) { return false; }
            field = requiredField(json, "RearForkStartPercent", error, path); if (field == nullptr || !fromValue(*field, value.RearForkStartPercent, error, path + ".RearForkStartPercent")) { return false; }
            field = requiredField(json, "CoreTreatmentChance", error, path); if (field == nullptr || !fromValue(*field, value.CoreTreatmentChance, error, path + ".CoreTreatmentChance")) { return false; }
            field = requiredField(json, "CoreRegionWidthBasePercent", error, path); if (field == nullptr || !fromValue(*field, value.CoreRegionWidthBasePercent, error, path + ".CoreRegionWidthBasePercent")) { return false; }
            field = requiredField(json, "CoreRegionWidthHorizontalCapacityDivisor", error, path); if (field == nullptr || !fromValue(*field, value.CoreRegionWidthHorizontalCapacityDivisor, error, path + ".CoreRegionWidthHorizontalCapacityDivisor")) { return false; }
            field = requiredField(json, "CoreRegionWidthMaximumPercent", error, path); if (field == nullptr || !fromValue(*field, value.CoreRegionWidthMaximumPercent, error, path + ".CoreRegionWidthMaximumPercent")) { return false; }
            field = requiredField(json, "RaisedCorePlateWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.RaisedCorePlateWidthPercent, error, path + ".RaisedCorePlateWidthPercent")) { return false; }
            field = requiredField(json, "MaximumCoreTreatments", error, path); if (field == nullptr || !fromValue(*field, value.MaximumCoreTreatments, error, path + ".MaximumCoreTreatments")) { return false; }
            field = requiredField(json, "CoreTreatmentWeights", error, path); if (field == nullptr || !fromValue(*field, value.CoreTreatmentWeights, error, path + ".CoreTreatmentWeights")) { return false; }
            field = requiredField(json, "HullLayerChance", error, path); if (field == nullptr || !fromValue(*field, value.HullLayerChance, error, path + ".HullLayerChance")) { return false; }
            field = requiredField(json, "MaximumHullLayers", error, path); if (field == nullptr || !fromValue(*field, value.MaximumHullLayers, error, path + ".MaximumHullLayers")) { return false; }
            field = requiredField(json, "HullLayerWeights", error, path); if (field == nullptr || !fromValue(*field, value.HullLayerWeights, error, path + ".HullLayerWeights")) { return false; }
            field = requiredField(json, "CockpitStartPercent", error, path); if (field == nullptr || !fromValue(*field, value.CockpitStartPercent, error, path + ".CockpitStartPercent")) { return false; }
            field = requiredField(json, "CockpitHeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.CockpitHeightPercent, error, path + ".CockpitHeightPercent")) { return false; }
            field = requiredField(json, "CockpitWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.CockpitWidthPercent, error, path + ".CockpitWidthPercent")) { return false; }
            field = requiredField(json, "MaximumCockpitHullPercent", error, path); if (field == nullptr || !fromValue(*field, value.MaximumCockpitHullPercent, error, path + ".MaximumCockpitHullPercent")) { return false; }
            field = requiredField(json, "CockpitSizeWeights", error, path); if (field == nullptr || !fromValue(*field, value.CockpitSizeWeights, error, path + ".CockpitSizeWeights")) { return false; }
            field = requiredField(json, "CockpitShapeWeights", error, path); if (field == nullptr || !fromValue(*field, value.CockpitShapeWeights, error, path + ".CockpitShapeWeights")) { return false; }
            field = requiredField(json, "CentralEngineWeight", error, path); if (field == nullptr || !fromValue(*field, value.CentralEngineWeight, error, path + ".CentralEngineWeight")) { return false; }
            field = requiredField(json, "TwinEngineWeight", error, path); if (field == nullptr || !fromValue(*field, value.TwinEngineWeight, error, path + ".TwinEngineWeight")) { return false; }
            field = requiredField(json, "QuadEngineWeight", error, path); if (field == nullptr || !fromValue(*field, value.QuadEngineWeight, error, path + ".QuadEngineWeight")) { return false; }
            field = requiredField(json, "CentralAuxiliaryEngineWeight", error, path); if (field == nullptr || !fromValue(*field, value.CentralAuxiliaryEngineWeight, error, path + ".CentralAuxiliaryEngineWeight")) { return false; }
            field = requiredField(json, "EngineBankWeight", error, path); if (field == nullptr || !fromValue(*field, value.EngineBankWeight, error, path + ".EngineBankWeight")) { return false; }
            field = requiredField(json, "SmallEngineSizeWeight", error, path); if (field == nullptr || !fromValue(*field, value.SmallEngineSizeWeight, error, path + ".SmallEngineSizeWeight")) { return false; }
            field = requiredField(json, "MediumEngineSizeWeight", error, path); if (field == nullptr || !fromValue(*field, value.MediumEngineSizeWeight, error, path + ".MediumEngineSizeWeight")) { return false; }
            field = requiredField(json, "LargeEngineSizeWeight", error, path); if (field == nullptr || !fromValue(*field, value.LargeEngineSizeWeight, error, path + ".LargeEngineSizeWeight")) { return false; }
            field = requiredField(json, "EngineNacelleChance", error, path); if (field == nullptr || !fromValue(*field, value.EngineNacelleChance, error, path + ".EngineNacelleChance")) { return false; }
            field = requiredField(json, "MajorFeatureChance", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeatureChance, error, path + ".MajorFeatureChance")) { return false; }
            field = requiredField(json, "MaximumMajorFeatures", error, path); if (field == nullptr || !fromValue(*field, value.MaximumMajorFeatures, error, path + ".MaximumMajorFeatures")) { return false; }
            field = requiredField(json, "MajorFeatureScalePercent", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeatureScalePercent, error, path + ".MajorFeatureScalePercent")) { return false; }
            field = requiredField(json, "MajorFeatureWeights", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeatureWeights, error, path + ".MajorFeatureWeights")) { return false; }
            field = requiredField(json, "LargeWeaponChance", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeaponChance, error, path + ".LargeWeaponChance")) { return false; }
            field = requiredField(json, "MaximumLargeWeaponGroups", error, path); if (field == nullptr || !fromValue(*field, value.MaximumLargeWeaponGroups, error, path + ".MaximumLargeWeaponGroups")) { return false; }
            field = requiredField(json, "LargeWeaponSymmetryChance", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeaponSymmetryChance, error, path + ".LargeWeaponSymmetryChance")) { return false; }
            field = requiredField(json, "LargeWeaponScalePercent", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeaponScalePercent, error, path + ".LargeWeaponScalePercent")) { return false; }
            field = requiredField(json, "LargeWeaponWeights", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeaponWeights, error, path + ".LargeWeaponWeights")) { return false; }
            field = requiredField(json, "LargeWeaponHardpointWeights", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeaponHardpointWeights, error, path + ".LargeWeaponHardpointWeights")) { return false; }
            field = requiredField(json, "DetailDensityPercent", error, path); if (field == nullptr || !fromValue(*field, value.DetailDensityPercent, error, path + ".DetailDensityPercent")) { return false; }
            field = requiredField(json, "MechanicalPatternCountPercent", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalPatternCountPercent, error, path + ".MechanicalPatternCountPercent")) { return false; }
            field = requiredField(json, "DetailMotifChance", error, path); if (field == nullptr || !fromValue(*field, value.DetailMotifChance, error, path + ".DetailMotifChance")) { return false; }
            field = requiredField(json, "SecondaryDetailMotifChance", error, path); if (field == nullptr || !fromValue(*field, value.SecondaryDetailMotifChance, error, path + ".SecondaryDetailMotifChance")) { return false; }
            field = requiredField(json, "DetailMotifRepeatPercent", error, path); if (field == nullptr || !fromValue(*field, value.DetailMotifRepeatPercent, error, path + ".DetailMotifRepeatPercent")) { return false; }
            field = requiredField(json, "DetailMotifWeights", error, path); if (field == nullptr || !fromValue(*field, value.DetailMotifWeights, error, path + ".DetailMotifWeights")) { return false; }
            field = requiredField(json, "DetailMotifMirroringBonusPercent", error, path); if (field == nullptr || !fromValue(*field, value.DetailMotifMirroringBonusPercent, error, path + ".DetailMotifMirroringBonusPercent")) { return false; }
            field = requiredField(json, "DetailMotifPlacementBias", error, path); if (field == nullptr || !fromValue(*field, value.DetailMotifPlacementBias, error, path + ".DetailMotifPlacementBias")) { return false; }
            field = requiredField(json, "DetailMotifOrientationBias", error, path); if (field == nullptr || !fromValue(*field, value.DetailMotifOrientationBias, error, path + ".DetailMotifOrientationBias")) { return false; }
            field = requiredField(json, "AccentPanelWeight", error, path); if (field == nullptr || !fromValue(*field, value.AccentPanelWeight, error, path + ".AccentPanelWeight")) { return false; }
            field = requiredField(json, "AccentStripeWeight", error, path); if (field == nullptr || !fromValue(*field, value.AccentStripeWeight, error, path + ".AccentStripeWeight")) { return false; }
            field = requiredField(json, "AccentArmorWeight", error, path); if (field == nullptr || !fromValue(*field, value.AccentArmorWeight, error, path + ".AccentArmorWeight")) { return false; }
            field = requiredField(json, "HorizontalVentChance", error, path); if (field == nullptr || !fromValue(*field, value.HorizontalVentChance, error, path + ".HorizontalVentChance")) { return false; }
            field = requiredField(json, "AttachmentWeights", error, path); if (field == nullptr || !fromValue(*field, value.AttachmentWeights, error, path + ".AttachmentWeights")) { return false; }
            field = requiredField(json, "AttachmentChance", error, path); if (field == nullptr || !fromValue(*field, value.AttachmentChance, error, path + ".AttachmentChance")) { return false; }
            field = requiredField(json, "MaximumAttachmentGroups", error, path); if (field == nullptr || !fromValue(*field, value.MaximumAttachmentGroups, error, path + ".MaximumAttachmentGroups")) { return false; }
            field = requiredField(json, "SymmetricAttachmentChance", error, path); if (field == nullptr || !fromValue(*field, value.SymmetricAttachmentChance, error, path + ".SymmetricAttachmentChance")) { return false; }
            field = requiredField(json, "AttachmentSizePercent", error, path); if (field == nullptr || !fromValue(*field, value.AttachmentSizePercent, error, path + ".AttachmentSizePercent")) { return false; }
            field = requiredField(json, "MaterialCompositionChance", error, path); if (field == nullptr || !fromValue(*field, value.MaterialCompositionChance, error, path + ".MaterialCompositionChance")) { return false; }
            field = requiredField(json, "MaximumMaterialZones", error, path); if (field == nullptr || !fromValue(*field, value.MaximumMaterialZones, error, path + ".MaximumMaterialZones")) { return false; }
            field = requiredField(json, "MaterialSecondaryContrastPercent", error, path); if (field == nullptr || !fromValue(*field, value.MaterialSecondaryContrastPercent, error, path + ".MaterialSecondaryContrastPercent")) { return false; }
            field = requiredField(json, "MaterialZoneWeights", error, path); if (field == nullptr || !fromValue(*field, value.MaterialZoneWeights, error, path + ".MaterialZoneWeights")) { return false; }
            field = requiredField(json, "MaterialWingSurfaceUsesFullWing", error, path); if (field == nullptr || !fromValue(*field, value.MaterialWingSurfaceUsesFullWing, error, path + ".MaterialWingSurfaceUsesFullWing")) { return false; }
            field = requiredField(json, "MaterialAxialBandWidthPercent", error, path); if (field == nullptr || !fromValue(*field, value.MaterialAxialBandWidthPercent, error, path + ".MaterialAxialBandWidthPercent")) { return false; }
            field = requiredField(json, "LiveryChance", error, path); if (field == nullptr || !fromValue(*field, value.LiveryChance, error, path + ".LiveryChance")) { return false; }
            field = requiredField(json, "MaximumLiveryMarkings", error, path); if (field == nullptr || !fromValue(*field, value.MaximumLiveryMarkings, error, path + ".MaximumLiveryMarkings")) { return false; }
            field = requiredField(json, "SupportingLiveryChance", error, path); if (field == nullptr || !fromValue(*field, value.SupportingLiveryChance, error, path + ".SupportingLiveryChance")) { return false; }
            field = requiredField(json, "LiveryAsymmetricChance", error, path); if (field == nullptr || !fromValue(*field, value.LiveryAsymmetricChance, error, path + ".LiveryAsymmetricChance")) { return false; }
            field = requiredField(json, "MaximumLiveryCoveragePercent", error, path); if (field == nullptr || !fromValue(*field, value.MaximumLiveryCoveragePercent, error, path + ".MaximumLiveryCoveragePercent")) { return false; }
            field = requiredField(json, "MaximumLiveryConnectedCoveragePercent", error, path); if (field == nullptr || !fromValue(*field, value.MaximumLiveryConnectedCoveragePercent, error, path + ".MaximumLiveryConnectedCoveragePercent")) { return false; }
            field = requiredField(json, "LiveryWeights", error, path); if (field == nullptr || !fromValue(*field, value.LiveryWeights, error, path + ".LiveryWeights")) { return false; }
            field = requiredField(json, "PaletteHullValueOffset", error, path); if (field == nullptr || !fromValue(*field, value.PaletteHullValueOffset, error, path + ".PaletteHullValueOffset")) { return false; }
            field = requiredField(json, "PaletteContrastPercent", error, path); if (field == nullptr || !fromValue(*field, value.PaletteContrastPercent, error, path + ".PaletteContrastPercent")) { return false; }
            field = requiredField(json, "PaletteHullSaturationPercent", error, path); if (field == nullptr || !fromValue(*field, value.PaletteHullSaturationPercent, error, path + ".PaletteHullSaturationPercent")) { return false; }
            field = requiredField(json, "PaletteAccentSaturationPercent", error, path); if (field == nullptr || !fromValue(*field, value.PaletteAccentSaturationPercent, error, path + ".PaletteAccentSaturationPercent")) { return false; }
            field = requiredField(json, "PaletteEmissiveValuePercent", error, path); if (field == nullptr || !fromValue(*field, value.PaletteEmissiveValuePercent, error, path + ".PaletteEmissiveValuePercent")) { return false; }
            field = requiredField(json, "SecondaryHullToneCoveragePercent", error, path); if (field == nullptr || !fromValue(*field, value.SecondaryHullToneCoveragePercent, error, path + ".SecondaryHullToneCoveragePercent")) { return false; }
            field = requiredField(json, "CoreRaisedSurfaceTone", error, path); if (field == nullptr || !fromValue(*field, value.CoreRaisedSurfaceTone, error, path + ".CoreRaisedSurfaceTone")) { return false; }
            field = requiredField(json, "CentralDorsalPlateTone", error, path); if (field == nullptr || !fromValue(*field, value.CentralDorsalPlateTone, error, path + ".CentralDorsalPlateTone")) { return false; }
            field = requiredField(json, "AxialRidgeUsesEdgeHighlight", error, path); if (field == nullptr || !fromValue(*field, value.AxialRidgeUsesEdgeHighlight, error, path + ".AxialRidgeUsesEdgeHighlight")) { return false; }
            field = requiredField(json, "MacroAsymmetryChance", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetryChance, error, path + ".MacroAsymmetryChance")) { return false; }
            field = requiredField(json, "MacroAsymmetryCategoryWeights", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetryCategoryWeights, error, path + ".MacroAsymmetryCategoryWeights")) { return false; }
            field = requiredField(json, "MacroAsymmetryOuterRegionChance", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetryOuterRegionChance, error, path + ".MacroAsymmetryOuterRegionChance")) { return false; }
            field = requiredField(json, "MacroAsymmetryWingRootRegionChance", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetryWingRootRegionChance, error, path + ".MacroAsymmetryWingRootRegionChance")) { return false; }
            field = requiredField(json, "MacroAsymmetryVisualWeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetryVisualWeightPercent, error, path + ".MacroAsymmetryVisualWeightPercent")) { return false; }
            field = requiredField(json, "SupplementalDetailWeights", error, path); if (field == nullptr || !fromValue(*field, value.SupplementalDetailWeights, error, path + ".SupplementalDetailWeights")) { return false; }
            return true;
        }

        Value toValue(const PaletteUIntRange& value)
        {
            Value object = Value::object();
            object.Object["Min"] = toValue(value.Min);
            object.Object["Max"] = toValue(value.Max);
            return object;
        }

        bool fromValue(const Value& json, PaletteUIntRange& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Min", error, path); if (field == nullptr || !fromValue(*field, value.Min, error, path + ".Min")) { return false; }
            field = requiredField(json, "Max", error, path); if (field == nullptr || !fromValue(*field, value.Max, error, path + ".Max")) { return false; }
            return true;
        }

        Value toValue(const PaletteIntRange& value)
        {
            Value object = Value::object();
            object.Object["Min"] = toValue(value.Min);
            object.Object["Max"] = toValue(value.Max);
            return object;
        }

        bool fromValue(const Value& json, PaletteIntRange& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Min", error, path); if (field == nullptr || !fromValue(*field, value.Min, error, path + ".Min")) { return false; }
            field = requiredField(json, "Max", error, path); if (field == nullptr || !fromValue(*field, value.Max, error, path + ".Max")) { return false; }
            return true;
        }

        Value toValue(const PaletteRoleProfile& value)
        {
            Value object = Value::object();
            object.Object["HueOffset"] = toValue(value.HueOffset);
            object.Object["Saturation"] = toValue(value.Saturation);
            object.Object["Value"] = toValue(value.Value);
            return object;
        }

        bool fromValue(const Value& json, PaletteRoleProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "HueOffset", error, path); if (field == nullptr || !fromValue(*field, value.HueOffset, error, path + ".HueOffset")) { return false; }
            field = requiredField(json, "Saturation", error, path); if (field == nullptr || !fromValue(*field, value.Saturation, error, path + ".Saturation")) { return false; }
            field = requiredField(json, "Value", error, path); if (field == nullptr || !fromValue(*field, value.Value, error, path + ".Value")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionPaletteProfile& value)
        {
            Value object = Value::object();
            object.Object["HullHue"] = toValue(value.HullHue);
            object.Object["HullSaturation"] = toValue(value.HullSaturation);
            object.Object["HullValue"] = toValue(value.HullValue);
            object.Object["Accent"] = toValue(value.Accent);
            object.Object["Cockpit"] = toValue(value.Cockpit);
            object.Object["Light"] = toValue(value.Light);
            object.Object["Exhaust"] = toValue(value.Exhaust);
            object.Object["MechanicalSaturation"] = toValue(value.MechanicalSaturation);
            object.Object["MechanicalValue"] = toValue(value.MechanicalValue);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionPaletteProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "HullHue", error, path); if (field == nullptr || !fromValue(*field, value.HullHue, error, path + ".HullHue")) { return false; }
            field = requiredField(json, "HullSaturation", error, path); if (field == nullptr || !fromValue(*field, value.HullSaturation, error, path + ".HullSaturation")) { return false; }
            field = requiredField(json, "HullValue", error, path); if (field == nullptr || !fromValue(*field, value.HullValue, error, path + ".HullValue")) { return false; }
            field = requiredField(json, "Accent", error, path); if (field == nullptr || !fromValue(*field, value.Accent, error, path + ".Accent")) { return false; }
            field = requiredField(json, "Cockpit", error, path); if (field == nullptr || !fromValue(*field, value.Cockpit, error, path + ".Cockpit")) { return false; }
            field = requiredField(json, "Light", error, path); if (field == nullptr || !fromValue(*field, value.Light, error, path + ".Light")) { return false; }
            field = requiredField(json, "Exhaust", error, path); if (field == nullptr || !fromValue(*field, value.Exhaust, error, path + ".Exhaust")) { return false; }
            field = requiredField(json, "MechanicalSaturation", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalSaturation, error, path + ".MechanicalSaturation")) { return false; }
            field = requiredField(json, "MechanicalValue", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalValue, error, path + ".MechanicalValue")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionPaletteBehaviorProfile& value)
        {
            Value object = Value::object();
            object.Object["HullValueMode"] = toValue(value.HullValueMode);
            object.Object["BrightHullValue"] = toValue(value.BrightHullValue);
            object.Object["DarkHullValue"] = toValue(value.DarkHullValue);
            object.Object["SecondaryToneDirection"] = toValue(value.SecondaryToneDirection);
            object.Object["MinimumAccentHueDistance"] = toValue(value.MinimumAccentHueDistance);
            object.Object["AccentHueSeparationShiftA"] = toValue(value.AccentHueSeparationShiftA);
            object.Object["AccentHueSeparationShiftB"] = toValue(value.AccentHueSeparationShiftB);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionPaletteBehaviorProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "HullValueMode", error, path); if (field == nullptr || !fromValue(*field, value.HullValueMode, error, path + ".HullValueMode")) { return false; }
            field = requiredField(json, "BrightHullValue", error, path); if (field == nullptr || !fromValue(*field, value.BrightHullValue, error, path + ".BrightHullValue")) { return false; }
            field = requiredField(json, "DarkHullValue", error, path); if (field == nullptr || !fromValue(*field, value.DarkHullValue, error, path + ".DarkHullValue")) { return false; }
            field = requiredField(json, "SecondaryToneDirection", error, path); if (field == nullptr || !fromValue(*field, value.SecondaryToneDirection, error, path + ".SecondaryToneDirection")) { return false; }
            field = requiredField(json, "MinimumAccentHueDistance", error, path); if (field == nullptr || !fromValue(*field, value.MinimumAccentHueDistance, error, path + ".MinimumAccentHueDistance")) { return false; }
            field = requiredField(json, "AccentHueSeparationShiftA", error, path); if (field == nullptr || !fromValue(*field, value.AccentHueSeparationShiftA, error, path + ".AccentHueSeparationShiftA")) { return false; }
            field = requiredField(json, "AccentHueSeparationShiftB", error, path); if (field == nullptr || !fromValue(*field, value.AccentHueSeparationShiftB, error, path + ".AccentHueSeparationShiftB")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionSurfaceDetailProfile& value)
        {
            Value object = Value::object();
            object.Object["DetailDensityPercent"] = toValue(value.DetailDensityPercent);
            object.Object["MechanicalPatternCountPercent"] = toValue(value.MechanicalPatternCountPercent);
            object.Object["LightPatternCountPercent"] = toValue(value.LightPatternCountPercent);
            object.Object["AccentPanelWeightPercent"] = toValue(value.AccentPanelWeightPercent);
            object.Object["AccentStripeWeightPercent"] = toValue(value.AccentStripeWeightPercent);
            object.Object["AccentArmorWeightPercent"] = toValue(value.AccentArmorWeightPercent);
            object.Object["HorizontalVentChancePercent"] = toValue(value.HorizontalVentChancePercent);
            object.Object["SupplementalWeightMultipliersPercent"] = toValue(value.SupplementalWeightMultipliersPercent);
            object.Object["MotifWeightMultipliersPercent"] = toValue(value.MotifWeightMultipliersPercent);
            object.Object["MotifRepeatPercent"] = toValue(value.MotifRepeatPercent);
            object.Object["AsymmetricDetailChanceOffset"] = toValue(value.AsymmetricDetailChanceOffset);
            object.Object["LuminousChannelCoreRegionBiasChance"] = toValue(value.LuminousChannelCoreRegionBiasChance);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionSurfaceDetailProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "DetailDensityPercent", error, path); if (field == nullptr || !fromValue(*field, value.DetailDensityPercent, error, path + ".DetailDensityPercent")) { return false; }
            field = requiredField(json, "MechanicalPatternCountPercent", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalPatternCountPercent, error, path + ".MechanicalPatternCountPercent")) { return false; }
            field = requiredField(json, "LightPatternCountPercent", error, path); if (field == nullptr || !fromValue(*field, value.LightPatternCountPercent, error, path + ".LightPatternCountPercent")) { return false; }
            field = requiredField(json, "AccentPanelWeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.AccentPanelWeightPercent, error, path + ".AccentPanelWeightPercent")) { return false; }
            field = requiredField(json, "AccentStripeWeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.AccentStripeWeightPercent, error, path + ".AccentStripeWeightPercent")) { return false; }
            field = requiredField(json, "AccentArmorWeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.AccentArmorWeightPercent, error, path + ".AccentArmorWeightPercent")) { return false; }
            field = requiredField(json, "HorizontalVentChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.HorizontalVentChancePercent, error, path + ".HorizontalVentChancePercent")) { return false; }
            field = requiredField(json, "SupplementalWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.SupplementalWeightMultipliersPercent, error, path + ".SupplementalWeightMultipliersPercent")) { return false; }
            field = requiredField(json, "MotifWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.MotifWeightMultipliersPercent, error, path + ".MotifWeightMultipliersPercent")) { return false; }
            field = requiredField(json, "MotifRepeatPercent", error, path); if (field == nullptr || !fromValue(*field, value.MotifRepeatPercent, error, path + ".MotifRepeatPercent")) { return false; }
            field = requiredField(json, "AsymmetricDetailChanceOffset", error, path); if (field == nullptr || !fromValue(*field, value.AsymmetricDetailChanceOffset, error, path + ".AsymmetricDetailChanceOffset")) { return false; }
            field = requiredField(json, "LuminousChannelCoreRegionBiasChance", error, path); if (field == nullptr || !fromValue(*field, value.LuminousChannelCoreRegionBiasChance, error, path + ".LuminousChannelCoreRegionBiasChance")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionAttachmentProfile& value)
        {
            Value object = Value::object();
            object.Object["WeightMultipliersPercent"] = toValue(value.WeightMultipliersPercent);
            object.Object["AttachmentChancePercent"] = toValue(value.AttachmentChancePercent);
            object.Object["SymmetryChanceOffset"] = toValue(value.SymmetryChanceOffset);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionAttachmentProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.WeightMultipliersPercent, error, path + ".WeightMultipliersPercent")) { return false; }
            field = requiredField(json, "AttachmentChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.AttachmentChancePercent, error, path + ".AttachmentChancePercent")) { return false; }
            field = requiredField(json, "SymmetryChanceOffset", error, path); if (field == nullptr || !fromValue(*field, value.SymmetryChanceOffset, error, path + ".SymmetryChanceOffset")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionWeaponWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["SingleCannon"] = toValue(value.SingleCannon);
            object.Object["TwinCannon"] = toValue(value.TwinCannon);
            object.Object["CompactTurret"] = toValue(value.CompactTurret);
            object.Object["RailWeapon"] = toValue(value.RailWeapon);
            object.Object["WeaponPod"] = toValue(value.WeaponPod);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionWeaponWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "SingleCannon", error, path); if (field == nullptr || !fromValue(*field, value.SingleCannon, error, path + ".SingleCannon")) { return false; }
            field = requiredField(json, "TwinCannon", error, path); if (field == nullptr || !fromValue(*field, value.TwinCannon, error, path + ".TwinCannon")) { return false; }
            field = requiredField(json, "CompactTurret", error, path); if (field == nullptr || !fromValue(*field, value.CompactTurret, error, path + ".CompactTurret")) { return false; }
            field = requiredField(json, "RailWeapon", error, path); if (field == nullptr || !fromValue(*field, value.RailWeapon, error, path + ".RailWeapon")) { return false; }
            field = requiredField(json, "WeaponPod", error, path); if (field == nullptr || !fromValue(*field, value.WeaponPod, error, path + ".WeaponPod")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionWeaponProfile& value)
        {
            Value object = Value::object();
            object.Object["ChancePercent"] = toValue(value.ChancePercent);
            object.Object["SymmetryChanceOffset"] = toValue(value.SymmetryChanceOffset);
            object.Object["WeightMultipliersPercent"] = toValue(value.WeightMultipliersPercent);
            object.Object["EmissiveChance"] = toValue(value.EmissiveChance);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionWeaponProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.ChancePercent, error, path + ".ChancePercent")) { return false; }
            field = requiredField(json, "SymmetryChanceOffset", error, path); if (field == nullptr || !fromValue(*field, value.SymmetryChanceOffset, error, path + ".SymmetryChanceOffset")) { return false; }
            field = requiredField(json, "WeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.WeightMultipliersPercent, error, path + ".WeightMultipliersPercent")) { return false; }
            field = requiredField(json, "EmissiveChance", error, path); if (field == nullptr || !fromValue(*field, value.EmissiveChance, error, path + ".EmissiveChance")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionEngineLayoutWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["Central"] = toValue(value.Central);
            object.Object["Twin"] = toValue(value.Twin);
            object.Object["Quad"] = toValue(value.Quad);
            object.Object["CentralAuxiliary"] = toValue(value.CentralAuxiliary);
            object.Object["WideBank"] = toValue(value.WideBank);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionEngineLayoutWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Central", error, path); if (field == nullptr || !fromValue(*field, value.Central, error, path + ".Central")) { return false; }
            field = requiredField(json, "Twin", error, path); if (field == nullptr || !fromValue(*field, value.Twin, error, path + ".Twin")) { return false; }
            field = requiredField(json, "Quad", error, path); if (field == nullptr || !fromValue(*field, value.Quad, error, path + ".Quad")) { return false; }
            field = requiredField(json, "CentralAuxiliary", error, path); if (field == nullptr || !fromValue(*field, value.CentralAuxiliary, error, path + ".CentralAuxiliary")) { return false; }
            field = requiredField(json, "WideBank", error, path); if (field == nullptr || !fromValue(*field, value.WideBank, error, path + ".WideBank")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionEngineSizeWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["Small"] = toValue(value.Small);
            object.Object["Medium"] = toValue(value.Medium);
            object.Object["Large"] = toValue(value.Large);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionEngineSizeWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Small", error, path); if (field == nullptr || !fromValue(*field, value.Small, error, path + ".Small")) { return false; }
            field = requiredField(json, "Medium", error, path); if (field == nullptr || !fromValue(*field, value.Medium, error, path + ".Medium")) { return false; }
            field = requiredField(json, "Large", error, path); if (field == nullptr || !fromValue(*field, value.Large, error, path + ".Large")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionEngineProfile& value)
        {
            Value object = Value::object();
            object.Object["LayoutWeightMultipliersPercent"] = toValue(value.LayoutWeightMultipliersPercent);
            object.Object["SizeWeightMultipliersPercent"] = toValue(value.SizeWeightMultipliersPercent);
            object.Object["NacelleChancePercent"] = toValue(value.NacelleChancePercent);
            object.Object["ExternalHeightPercent"] = toValue(value.ExternalHeightPercent);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionEngineProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "LayoutWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.LayoutWeightMultipliersPercent, error, path + ".LayoutWeightMultipliersPercent")) { return false; }
            field = requiredField(json, "SizeWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.SizeWeightMultipliersPercent, error, path + ".SizeWeightMultipliersPercent")) { return false; }
            field = requiredField(json, "NacelleChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.NacelleChancePercent, error, path + ".NacelleChancePercent")) { return false; }
            field = requiredField(json, "ExternalHeightPercent", error, path); if (field == nullptr || !fromValue(*field, value.ExternalHeightPercent, error, path + ".ExternalHeightPercent")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionMajorFeatureWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["CentralSpine"] = toValue(value.CentralSpine);
            object.Object["ArmorPlate"] = toValue(value.ArmorPlate);
            object.Object["RecessedBay"] = toValue(value.RecessedBay);
            object.Object["VentBank"] = toValue(value.VentBank);
            object.Object["WingPlate"] = toValue(value.WingPlate);
            object.Object["TechCore"] = toValue(value.TechCore);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionMajorFeatureWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralSpine", error, path); if (field == nullptr || !fromValue(*field, value.CentralSpine, error, path + ".CentralSpine")) { return false; }
            field = requiredField(json, "ArmorPlate", error, path); if (field == nullptr || !fromValue(*field, value.ArmorPlate, error, path + ".ArmorPlate")) { return false; }
            field = requiredField(json, "RecessedBay", error, path); if (field == nullptr || !fromValue(*field, value.RecessedBay, error, path + ".RecessedBay")) { return false; }
            field = requiredField(json, "VentBank", error, path); if (field == nullptr || !fromValue(*field, value.VentBank, error, path + ".VentBank")) { return false; }
            field = requiredField(json, "WingPlate", error, path); if (field == nullptr || !fromValue(*field, value.WingPlate, error, path + ".WingPlate")) { return false; }
            field = requiredField(json, "TechCore", error, path); if (field == nullptr || !fromValue(*field, value.TechCore, error, path + ".TechCore")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionMajorFeatureProfile& value)
        {
            Value object = Value::object();
            object.Object["ChancePercent"] = toValue(value.ChancePercent);
            object.Object["WeightMultipliersPercent"] = toValue(value.WeightMultipliersPercent);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionMajorFeatureProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.ChancePercent, error, path + ".ChancePercent")) { return false; }
            field = requiredField(json, "WeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.WeightMultipliersPercent, error, path + ".WeightMultipliersPercent")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionCockpitSizeWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["Compact"] = toValue(value.Compact);
            object.Object["Standard"] = toValue(value.Standard);
            object.Object["Large"] = toValue(value.Large);
            object.Object["Massive"] = toValue(value.Massive);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionCockpitSizeWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Compact", error, path); if (field == nullptr || !fromValue(*field, value.Compact, error, path + ".Compact")) { return false; }
            field = requiredField(json, "Standard", error, path); if (field == nullptr || !fromValue(*field, value.Standard, error, path + ".Standard")) { return false; }
            field = requiredField(json, "Large", error, path); if (field == nullptr || !fromValue(*field, value.Large, error, path + ".Large")) { return false; }
            field = requiredField(json, "Massive", error, path); if (field == nullptr || !fromValue(*field, value.Massive, error, path + ".Massive")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionCockpitShapeWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["CompactCanopy"] = toValue(value.CompactCanopy);
            object.Object["ElongatedCanopy"] = toValue(value.ElongatedCanopy);
            object.Object["WideCommandDeck"] = toValue(value.WideCommandDeck);
            object.Object["SplitCanopy"] = toValue(value.SplitCanopy);
            object.Object["DorsalBridge"] = toValue(value.DorsalBridge);
            object.Object["LayeredBridge"] = toValue(value.LayeredBridge);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionCockpitShapeWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CompactCanopy", error, path); if (field == nullptr || !fromValue(*field, value.CompactCanopy, error, path + ".CompactCanopy")) { return false; }
            field = requiredField(json, "ElongatedCanopy", error, path); if (field == nullptr || !fromValue(*field, value.ElongatedCanopy, error, path + ".ElongatedCanopy")) { return false; }
            field = requiredField(json, "WideCommandDeck", error, path); if (field == nullptr || !fromValue(*field, value.WideCommandDeck, error, path + ".WideCommandDeck")) { return false; }
            field = requiredField(json, "SplitCanopy", error, path); if (field == nullptr || !fromValue(*field, value.SplitCanopy, error, path + ".SplitCanopy")) { return false; }
            field = requiredField(json, "DorsalBridge", error, path); if (field == nullptr || !fromValue(*field, value.DorsalBridge, error, path + ".DorsalBridge")) { return false; }
            field = requiredField(json, "LayeredBridge", error, path); if (field == nullptr || !fromValue(*field, value.LayeredBridge, error, path + ".LayeredBridge")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionCockpitProfile& value)
        {
            Value object = Value::object();
            object.Object["SizeWeightMultipliersPercent"] = toValue(value.SizeWeightMultipliersPercent);
            object.Object["ShapeWeightMultipliersPercent"] = toValue(value.ShapeWeightMultipliersPercent);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionCockpitProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "SizeWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.SizeWeightMultipliersPercent, error, path + ".SizeWeightMultipliersPercent")) { return false; }
            field = requiredField(json, "ShapeWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.ShapeWeightMultipliersPercent, error, path + ".ShapeWeightMultipliersPercent")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionNegativeSpaceWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["WingChannel"] = toValue(value.WingChannel);
            object.Object["RearFork"] = toValue(value.RearFork);
            object.Object["ShoulderGap"] = toValue(value.ShoulderGap);
            object.Object["OpenFrameBay"] = toValue(value.OpenFrameBay);
            object.Object["NacelleChannel"] = toValue(value.NacelleChannel);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionNegativeSpaceWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WingChannel", error, path); if (field == nullptr || !fromValue(*field, value.WingChannel, error, path + ".WingChannel")) { return false; }
            field = requiredField(json, "RearFork", error, path); if (field == nullptr || !fromValue(*field, value.RearFork, error, path + ".RearFork")) { return false; }
            field = requiredField(json, "ShoulderGap", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderGap, error, path + ".ShoulderGap")) { return false; }
            field = requiredField(json, "OpenFrameBay", error, path); if (field == nullptr || !fromValue(*field, value.OpenFrameBay, error, path + ".OpenFrameBay")) { return false; }
            field = requiredField(json, "NacelleChannel", error, path); if (field == nullptr || !fromValue(*field, value.NacelleChannel, error, path + ".NacelleChannel")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionHullProfile& value)
        {
            Value object = Value::object();
            object.Object["NegativeSpaceChancePercent"] = toValue(value.NegativeSpaceChancePercent);
            object.Object["NegativeSpaceWeightMultipliersPercent"] = toValue(value.NegativeSpaceWeightMultipliersPercent);
            object.Object["PreferAlternateArticulationOrder"] = toValue(value.PreferAlternateArticulationOrder);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionHullProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "NegativeSpaceChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.NegativeSpaceChancePercent, error, path + ".NegativeSpaceChancePercent")) { return false; }
            field = requiredField(json, "NegativeSpaceWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.NegativeSpaceWeightMultipliersPercent, error, path + ".NegativeSpaceWeightMultipliersPercent")) { return false; }
            field = requiredField(json, "PreferAlternateArticulationOrder", error, path); if (field == nullptr || !fromValue(*field, value.PreferAlternateArticulationOrder, error, path + ".PreferAlternateArticulationOrder")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionCoreTreatmentWeightOffsets& value)
        {
            Value object = Value::object();
            object.Object["CentralSpine"] = toValue(value.CentralSpine);
            object.Object["CockpitSurround"] = toValue(value.CockpitSurround);
            object.Object["RaisedCorePlate"] = toValue(value.RaisedCorePlate);
            object.Object["LateralRecesses"] = toValue(value.LateralRecesses);
            object.Object["LongitudinalArmorBand"] = toValue(value.LongitudinalArmorBand);
            object.Object["CoreChannel"] = toValue(value.CoreChannel);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionCoreTreatmentWeightOffsets& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralSpine", error, path); if (field == nullptr || !fromValue(*field, value.CentralSpine, error, path + ".CentralSpine")) { return false; }
            field = requiredField(json, "CockpitSurround", error, path); if (field == nullptr || !fromValue(*field, value.CockpitSurround, error, path + ".CockpitSurround")) { return false; }
            field = requiredField(json, "RaisedCorePlate", error, path); if (field == nullptr || !fromValue(*field, value.RaisedCorePlate, error, path + ".RaisedCorePlate")) { return false; }
            field = requiredField(json, "LateralRecesses", error, path); if (field == nullptr || !fromValue(*field, value.LateralRecesses, error, path + ".LateralRecesses")) { return false; }
            field = requiredField(json, "LongitudinalArmorBand", error, path); if (field == nullptr || !fromValue(*field, value.LongitudinalArmorBand, error, path + ".LongitudinalArmorBand")) { return false; }
            field = requiredField(json, "CoreChannel", error, path); if (field == nullptr || !fromValue(*field, value.CoreChannel, error, path + ".CoreChannel")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionCoreTreatmentProfile& value)
        {
            Value object = Value::object();
            object.Object["ChancePercent"] = toValue(value.ChancePercent);
            object.Object["WeightOffsets"] = toValue(value.WeightOffsets);
            object.Object["CoreChannelLuminousPattern"] = toValue(value.CoreChannelLuminousPattern);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionCoreTreatmentProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.ChancePercent, error, path + ".ChancePercent")) { return false; }
            field = requiredField(json, "WeightOffsets", error, path); if (field == nullptr || !fromValue(*field, value.WeightOffsets, error, path + ".WeightOffsets")) { return false; }
            field = requiredField(json, "CoreChannelLuminousPattern", error, path); if (field == nullptr || !fromValue(*field, value.CoreChannelLuminousPattern, error, path + ".CoreChannelLuminousPattern")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionValueScale& value)
        {
            Value object = Value::object();
            object.Object["Numerator"] = toValue(value.Numerator);
            object.Object["Denominator"] = toValue(value.Denominator);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionValueScale& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Numerator", error, path); if (field == nullptr || !fromValue(*field, value.Numerator, error, path + ".Numerator")) { return false; }
            field = requiredField(json, "Denominator", error, path); if (field == nullptr || !fromValue(*field, value.Denominator, error, path + ".Denominator")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionHullLayerWeightAdjustment& value)
        {
            Value object = Value::object();
            object.Object["Offset"] = toValue(value.Offset);
            object.Object["Scale"] = toValue(value.Scale);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionHullLayerWeightAdjustment& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Offset", error, path); if (field == nullptr || !fromValue(*field, value.Offset, error, path + ".Offset")) { return false; }
            field = requiredField(json, "Scale", error, path); if (field == nullptr || !fromValue(*field, value.Scale, error, path + ".Scale")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionHullLayerWeightAdjustments& value)
        {
            Value object = Value::object();
            object.Object["CentralDorsalPlate"] = toValue(value.CentralDorsalPlate);
            object.Object["ForwardArmor"] = toValue(value.ForwardArmor);
            object.Object["WingArmor"] = toValue(value.WingArmor);
            object.Object["ShoulderArmor"] = toValue(value.ShoulderArmor);
            object.Object["RearEngineCover"] = toValue(value.RearEngineCover);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionHullLayerWeightAdjustments& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CentralDorsalPlate", error, path); if (field == nullptr || !fromValue(*field, value.CentralDorsalPlate, error, path + ".CentralDorsalPlate")) { return false; }
            field = requiredField(json, "ForwardArmor", error, path); if (field == nullptr || !fromValue(*field, value.ForwardArmor, error, path + ".ForwardArmor")) { return false; }
            field = requiredField(json, "WingArmor", error, path); if (field == nullptr || !fromValue(*field, value.WingArmor, error, path + ".WingArmor")) { return false; }
            field = requiredField(json, "ShoulderArmor", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderArmor, error, path + ".ShoulderArmor")) { return false; }
            field = requiredField(json, "RearEngineCover", error, path); if (field == nullptr || !fromValue(*field, value.RearEngineCover, error, path + ".RearEngineCover")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionHullLayerProfile& value)
        {
            Value object = Value::object();
            object.Object["ChancePercent"] = toValue(value.ChancePercent);
            object.Object["MaximumLayerCount"] = toValue(value.MaximumLayerCount);
            object.Object["WeightAdjustments"] = toValue(value.WeightAdjustments);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionHullLayerProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.ChancePercent, error, path + ".ChancePercent")) { return false; }
            field = requiredField(json, "MaximumLayerCount", error, path); if (field == nullptr || !fromValue(*field, value.MaximumLayerCount, error, path + ".MaximumLayerCount")) { return false; }
            field = requiredField(json, "WeightAdjustments", error, path); if (field == nullptr || !fromValue(*field, value.WeightAdjustments, error, path + ".WeightAdjustments")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionMaterialZoneWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["WingSurface"] = toValue(value.WingSurface);
            object.Object["ShoulderSurface"] = toValue(value.ShoulderSurface);
            object.Object["AxialBand"] = toValue(value.AxialBand);
            object.Object["RearMechanical"] = toValue(value.RearMechanical);
            object.Object["CockpitCollar"] = toValue(value.CockpitCollar);
            object.Object["HardpointSurround"] = toValue(value.HardpointSurround);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionMaterialZoneWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WingSurface", error, path); if (field == nullptr || !fromValue(*field, value.WingSurface, error, path + ".WingSurface")) { return false; }
            field = requiredField(json, "ShoulderSurface", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderSurface, error, path + ".ShoulderSurface")) { return false; }
            field = requiredField(json, "AxialBand", error, path); if (field == nullptr || !fromValue(*field, value.AxialBand, error, path + ".AxialBand")) { return false; }
            field = requiredField(json, "RearMechanical", error, path); if (field == nullptr || !fromValue(*field, value.RearMechanical, error, path + ".RearMechanical")) { return false; }
            field = requiredField(json, "CockpitCollar", error, path); if (field == nullptr || !fromValue(*field, value.CockpitCollar, error, path + ".CockpitCollar")) { return false; }
            field = requiredField(json, "HardpointSurround", error, path); if (field == nullptr || !fromValue(*field, value.HardpointSurround, error, path + ".HardpointSurround")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionMaterialProfile& value)
        {
            Value object = Value::object();
            object.Object["ZoneWeightMultipliersPercent"] = toValue(value.ZoneWeightMultipliersPercent);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionMaterialProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ZoneWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.ZoneWeightMultipliersPercent, error, path + ".ZoneWeightMultipliersPercent")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionLiveryWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["CenterStripe"] = toValue(value.CenterStripe);
            object.Object["DoubleCenterStripe"] = toValue(value.DoubleCenterStripe);
            object.Object["WingBand"] = toValue(value.WingBand);
            object.Object["ShoulderBlock"] = toValue(value.ShoulderBlock);
            object.Object["NoseBand"] = toValue(value.NoseBand);
            object.Object["Chevron"] = toValue(value.Chevron);
            object.Object["IdPanel"] = toValue(value.IdPanel);
            object.Object["GeometricInsignia"] = toValue(value.GeometricInsignia);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionLiveryWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "CenterStripe", error, path); if (field == nullptr || !fromValue(*field, value.CenterStripe, error, path + ".CenterStripe")) { return false; }
            field = requiredField(json, "DoubleCenterStripe", error, path); if (field == nullptr || !fromValue(*field, value.DoubleCenterStripe, error, path + ".DoubleCenterStripe")) { return false; }
            field = requiredField(json, "WingBand", error, path); if (field == nullptr || !fromValue(*field, value.WingBand, error, path + ".WingBand")) { return false; }
            field = requiredField(json, "ShoulderBlock", error, path); if (field == nullptr || !fromValue(*field, value.ShoulderBlock, error, path + ".ShoulderBlock")) { return false; }
            field = requiredField(json, "NoseBand", error, path); if (field == nullptr || !fromValue(*field, value.NoseBand, error, path + ".NoseBand")) { return false; }
            field = requiredField(json, "Chevron", error, path); if (field == nullptr || !fromValue(*field, value.Chevron, error, path + ".Chevron")) { return false; }
            field = requiredField(json, "IdPanel", error, path); if (field == nullptr || !fromValue(*field, value.IdPanel, error, path + ".IdPanel")) { return false; }
            field = requiredField(json, "GeometricInsignia", error, path); if (field == nullptr || !fromValue(*field, value.GeometricInsignia, error, path + ".GeometricInsignia")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionLiveryProfile& value)
        {
            Value object = Value::object();
            object.Object["ChancePercent"] = toValue(value.ChancePercent);
            object.Object["WeightMultipliersPercent"] = toValue(value.WeightMultipliersPercent);
            object.Object["AsymmetricChanceOffset"] = toValue(value.AsymmetricChanceOffset);
            object.Object["AsymmetricChanceDivisor"] = toValue(value.AsymmetricChanceDivisor);
            object.Object["AllowAsymmetricGeometricInsignia"] = toValue(value.AllowAsymmetricGeometricInsignia);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionLiveryProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.ChancePercent, error, path + ".ChancePercent")) { return false; }
            field = requiredField(json, "WeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.WeightMultipliersPercent, error, path + ".WeightMultipliersPercent")) { return false; }
            field = requiredField(json, "AsymmetricChanceOffset", error, path); if (field == nullptr || !fromValue(*field, value.AsymmetricChanceOffset, error, path + ".AsymmetricChanceOffset")) { return false; }
            field = requiredField(json, "AsymmetricChanceDivisor", error, path); if (field == nullptr || !fromValue(*field, value.AsymmetricChanceDivisor, error, path + ".AsymmetricChanceDivisor")) { return false; }
            field = requiredField(json, "AllowAsymmetricGeometricInsignia", error, path); if (field == nullptr || !fromValue(*field, value.AllowAsymmetricGeometricInsignia, error, path + ".AllowAsymmetricGeometricInsignia")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionVisualAnchorWeightMultipliers& value)
        {
            Value object = Value::object();
            object.Object["Silhouette"] = toValue(value.Silhouette);
            object.Object["Cockpit"] = toValue(value.Cockpit);
            object.Object["Wings"] = toValue(value.Wings);
            object.Object["Engines"] = toValue(value.Engines);
            object.Object["Weapons"] = toValue(value.Weapons);
            object.Object["MajorFeature"] = toValue(value.MajorFeature);
            object.Object["HullLayers"] = toValue(value.HullLayers);
            object.Object["CentralCore"] = toValue(value.CentralCore);
            object.Object["MacroAsymmetry"] = toValue(value.MacroAsymmetry);
            object.Object["NegativeSpace"] = toValue(value.NegativeSpace);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionVisualAnchorWeightMultipliers& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Silhouette", error, path); if (field == nullptr || !fromValue(*field, value.Silhouette, error, path + ".Silhouette")) { return false; }
            field = requiredField(json, "Cockpit", error, path); if (field == nullptr || !fromValue(*field, value.Cockpit, error, path + ".Cockpit")) { return false; }
            field = requiredField(json, "Wings", error, path); if (field == nullptr || !fromValue(*field, value.Wings, error, path + ".Wings")) { return false; }
            field = requiredField(json, "Engines", error, path); if (field == nullptr || !fromValue(*field, value.Engines, error, path + ".Engines")) { return false; }
            field = requiredField(json, "Weapons", error, path); if (field == nullptr || !fromValue(*field, value.Weapons, error, path + ".Weapons")) { return false; }
            field = requiredField(json, "MajorFeature", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeature, error, path + ".MajorFeature")) { return false; }
            field = requiredField(json, "HullLayers", error, path); if (field == nullptr || !fromValue(*field, value.HullLayers, error, path + ".HullLayers")) { return false; }
            field = requiredField(json, "CentralCore", error, path); if (field == nullptr || !fromValue(*field, value.CentralCore, error, path + ".CentralCore")) { return false; }
            field = requiredField(json, "MacroAsymmetry", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetry, error, path + ".MacroAsymmetry")) { return false; }
            field = requiredField(json, "NegativeSpace", error, path); if (field == nullptr || !fromValue(*field, value.NegativeSpace, error, path + ".NegativeSpace")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionVisualHierarchyProfile& value)
        {
            Value object = Value::object();
            object.Object["AnchorWeightMultipliersPercent"] = toValue(value.AnchorWeightMultipliersPercent);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionVisualHierarchyProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "AnchorWeightMultipliersPercent", error, path); if (field == nullptr || !fromValue(*field, value.AnchorWeightMultipliersPercent, error, path + ".AnchorWeightMultipliersPercent")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionMacroAsymmetryProfile& value)
        {
            Value object = Value::object();
            object.Object["ChancePercent"] = toValue(value.ChancePercent);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionMacroAsymmetryProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ChancePercent", error, path); if (field == nullptr || !fromValue(*field, value.ChancePercent, error, path + ".ChancePercent")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionComplexityCategoryOffsets& value)
        {
            Value object = Value::object();
            object.Object["Silhouette"] = toValue(value.Silhouette);
            object.Object["CockpitStructure"] = toValue(value.CockpitStructure);
            object.Object["HullLayer"] = toValue(value.HullLayer);
            object.Object["MajorFeature"] = toValue(value.MajorFeature);
            object.Object["LargeWeapon"] = toValue(value.LargeWeapon);
            object.Object["Attachment"] = toValue(value.Attachment);
            object.Object["Detail"] = toValue(value.Detail);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionComplexityCategoryOffsets& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Silhouette", error, path); if (field == nullptr || !fromValue(*field, value.Silhouette, error, path + ".Silhouette")) { return false; }
            field = requiredField(json, "CockpitStructure", error, path); if (field == nullptr || !fromValue(*field, value.CockpitStructure, error, path + ".CockpitStructure")) { return false; }
            field = requiredField(json, "HullLayer", error, path); if (field == nullptr || !fromValue(*field, value.HullLayer, error, path + ".HullLayer")) { return false; }
            field = requiredField(json, "MajorFeature", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeature, error, path + ".MajorFeature")) { return false; }
            field = requiredField(json, "LargeWeapon", error, path); if (field == nullptr || !fromValue(*field, value.LargeWeapon, error, path + ".LargeWeapon")) { return false; }
            field = requiredField(json, "Attachment", error, path); if (field == nullptr || !fromValue(*field, value.Attachment, error, path + ".Attachment")) { return false; }
            field = requiredField(json, "Detail", error, path); if (field == nullptr || !fromValue(*field, value.Detail, error, path + ".Detail")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionComplexityProfile& value)
        {
            Value object = Value::object();
            object.Object["TotalBudgetPercent"] = toValue(value.TotalBudgetPercent);
            object.Object["LegacyCategoryOffsets"] = toValue(value.LegacyCategoryOffsets);
            object.Object["CategoryOffsets"] = toValue(value.CategoryOffsets);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionComplexityProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "TotalBudgetPercent", error, path); if (field == nullptr || !fromValue(*field, value.TotalBudgetPercent, error, path + ".TotalBudgetPercent")) { return false; }
            field = requiredField(json, "LegacyCategoryOffsets", error, path); if (field == nullptr || !fromValue(*field, value.LegacyCategoryOffsets, error, path + ".LegacyCategoryOffsets")) { return false; }
            field = requiredField(json, "CategoryOffsets", error, path); if (field == nullptr || !fromValue(*field, value.CategoryOffsets, error, path + ".CategoryOffsets")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionFinishProfile& value)
        {
            Value object = Value::object();
            object.Object["WeaponMuzzleRole"] = toValue(value.WeaponMuzzleRole);
            object.Object["WeaponBodyRole"] = toValue(value.WeaponBodyRole);
            object.Object["WeaponRaisedHighlightRole"] = toValue(value.WeaponRaisedHighlightRole);
            object.Object["CoreSecondaryMaterialRole"] = toValue(value.CoreSecondaryMaterialRole);
            object.Object["CoreRaisedRole"] = toValue(value.CoreRaisedRole);
            object.Object["CoreLuminousRole"] = toValue(value.CoreLuminousRole);
            object.Object["CoreLuminousHighlightRole"] = toValue(value.CoreLuminousHighlightRole);
            object.Object["CentralDorsalPlateRole"] = toValue(value.CentralDorsalPlateRole);
            object.Object["CockpitBaseRole"] = toValue(value.CockpitBaseRole);
            object.Object["CockpitFrameRole"] = toValue(value.CockpitFrameRole);
            object.Object["EngineHotCoreRole"] = toValue(value.EngineHotCoreRole);
            object.Object["EngineInteriorHighlightRole"] = toValue(value.EngineInteriorHighlightRole);
            object.Object["ForceAxialRidgeEdgeHighlight"] = toValue(value.ForceAxialRidgeEdgeHighlight);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionFinishProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "WeaponMuzzleRole", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMuzzleRole, error, path + ".WeaponMuzzleRole")) { return false; }
            field = requiredField(json, "WeaponBodyRole", error, path); if (field == nullptr || !fromValue(*field, value.WeaponBodyRole, error, path + ".WeaponBodyRole")) { return false; }
            field = requiredField(json, "WeaponRaisedHighlightRole", error, path); if (field == nullptr || !fromValue(*field, value.WeaponRaisedHighlightRole, error, path + ".WeaponRaisedHighlightRole")) { return false; }
            field = requiredField(json, "CoreSecondaryMaterialRole", error, path); if (field == nullptr || !fromValue(*field, value.CoreSecondaryMaterialRole, error, path + ".CoreSecondaryMaterialRole")) { return false; }
            field = requiredField(json, "CoreRaisedRole", error, path); if (field == nullptr || !fromValue(*field, value.CoreRaisedRole, error, path + ".CoreRaisedRole")) { return false; }
            field = requiredField(json, "CoreLuminousRole", error, path); if (field == nullptr || !fromValue(*field, value.CoreLuminousRole, error, path + ".CoreLuminousRole")) { return false; }
            field = requiredField(json, "CoreLuminousHighlightRole", error, path); if (field == nullptr || !fromValue(*field, value.CoreLuminousHighlightRole, error, path + ".CoreLuminousHighlightRole")) { return false; }
            field = requiredField(json, "CentralDorsalPlateRole", error, path); if (field == nullptr || !fromValue(*field, value.CentralDorsalPlateRole, error, path + ".CentralDorsalPlateRole")) { return false; }
            field = requiredField(json, "CockpitBaseRole", error, path); if (field == nullptr || !fromValue(*field, value.CockpitBaseRole, error, path + ".CockpitBaseRole")) { return false; }
            field = requiredField(json, "CockpitFrameRole", error, path); if (field == nullptr || !fromValue(*field, value.CockpitFrameRole, error, path + ".CockpitFrameRole")) { return false; }
            field = requiredField(json, "EngineHotCoreRole", error, path); if (field == nullptr || !fromValue(*field, value.EngineHotCoreRole, error, path + ".EngineHotCoreRole")) { return false; }
            field = requiredField(json, "EngineInteriorHighlightRole", error, path); if (field == nullptr || !fromValue(*field, value.EngineInteriorHighlightRole, error, path + ".EngineInteriorHighlightRole")) { return false; }
            field = requiredField(json, "ForceAxialRidgeEdgeHighlight", error, path); if (field == nullptr || !fromValue(*field, value.ForceAxialRidgeEdgeHighlight, error, path + ".ForceAxialRidgeEdgeHighlight")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionIdleAnimationProfile& value)
        {
            Value object = Value::object();
            object.Object["EngineMechanicalChanceOffset"] = toValue(value.EngineMechanicalChanceOffset);
            object.Object["EngineMechanicalChanceMaximum"] = toValue(value.EngineMechanicalChanceMaximum);
            object.Object["EngineMechanicalChanceMinimum"] = toValue(value.EngineMechanicalChanceMinimum);
            object.Object["EnginePulseStrengthMinimum"] = toValue(value.EnginePulseStrengthMinimum);
            object.Object["ExhaustAmplitudeScale"] = toValue(value.ExhaustAmplitudeScale);
            object.Object["WeaponMechanicalChanceOffset"] = toValue(value.WeaponMechanicalChanceOffset);
            object.Object["WeaponMechanicalChanceScale"] = toValue(value.WeaponMechanicalChanceScale);
            object.Object["WeaponMechanicalChanceMinimum"] = toValue(value.WeaponMechanicalChanceMinimum);
            object.Object["WeaponMechanicalChanceMaximum"] = toValue(value.WeaponMechanicalChanceMaximum);
            object.Object["VentActivityChanceScale"] = toValue(value.VentActivityChanceScale);
            object.Object["TechPulseStrength"] = toValue(value.TechPulseStrength);
            object.Object["SynchronizeEngines"] = toValue(value.SynchronizeEngines);
            object.Object["AsynchronousEngines"] = toValue(value.AsynchronousEngines);
            object.Object["AlternateEnginePhases"] = toValue(value.AlternateEnginePhases);
            object.Object["AlternateWeaponPhases"] = toValue(value.AlternateWeaponPhases);
            object.Object["SlowMechanicalCycle"] = toValue(value.SlowMechanicalCycle);
            object.Object["IrregularEngineCycle"] = toValue(value.IrregularEngineCycle);
            object.Object["RandomizeSymmetricWeaponAlternatePhase"] = toValue(value.RandomizeSymmetricWeaponAlternatePhase);
            object.Object["AlternateTechCorePhases"] = toValue(value.AlternateTechCorePhases);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionIdleAnimationProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "EngineMechanicalChanceOffset", error, path); if (field == nullptr || !fromValue(*field, value.EngineMechanicalChanceOffset, error, path + ".EngineMechanicalChanceOffset")) { return false; }
            field = requiredField(json, "EngineMechanicalChanceMaximum", error, path); if (field == nullptr || !fromValue(*field, value.EngineMechanicalChanceMaximum, error, path + ".EngineMechanicalChanceMaximum")) { return false; }
            field = requiredField(json, "EngineMechanicalChanceMinimum", error, path); if (field == nullptr || !fromValue(*field, value.EngineMechanicalChanceMinimum, error, path + ".EngineMechanicalChanceMinimum")) { return false; }
            field = requiredField(json, "EnginePulseStrengthMinimum", error, path); if (field == nullptr || !fromValue(*field, value.EnginePulseStrengthMinimum, error, path + ".EnginePulseStrengthMinimum")) { return false; }
            field = requiredField(json, "ExhaustAmplitudeScale", error, path); if (field == nullptr || !fromValue(*field, value.ExhaustAmplitudeScale, error, path + ".ExhaustAmplitudeScale")) { return false; }
            field = requiredField(json, "WeaponMechanicalChanceOffset", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMechanicalChanceOffset, error, path + ".WeaponMechanicalChanceOffset")) { return false; }
            field = requiredField(json, "WeaponMechanicalChanceScale", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMechanicalChanceScale, error, path + ".WeaponMechanicalChanceScale")) { return false; }
            field = requiredField(json, "WeaponMechanicalChanceMinimum", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMechanicalChanceMinimum, error, path + ".WeaponMechanicalChanceMinimum")) { return false; }
            field = requiredField(json, "WeaponMechanicalChanceMaximum", error, path); if (field == nullptr || !fromValue(*field, value.WeaponMechanicalChanceMaximum, error, path + ".WeaponMechanicalChanceMaximum")) { return false; }
            field = requiredField(json, "VentActivityChanceScale", error, path); if (field == nullptr || !fromValue(*field, value.VentActivityChanceScale, error, path + ".VentActivityChanceScale")) { return false; }
            field = requiredField(json, "TechPulseStrength", error, path); if (field == nullptr || !fromValue(*field, value.TechPulseStrength, error, path + ".TechPulseStrength")) { return false; }
            field = requiredField(json, "SynchronizeEngines", error, path); if (field == nullptr || !fromValue(*field, value.SynchronizeEngines, error, path + ".SynchronizeEngines")) { return false; }
            field = requiredField(json, "AsynchronousEngines", error, path); if (field == nullptr || !fromValue(*field, value.AsynchronousEngines, error, path + ".AsynchronousEngines")) { return false; }
            field = requiredField(json, "AlternateEnginePhases", error, path); if (field == nullptr || !fromValue(*field, value.AlternateEnginePhases, error, path + ".AlternateEnginePhases")) { return false; }
            field = requiredField(json, "AlternateWeaponPhases", error, path); if (field == nullptr || !fromValue(*field, value.AlternateWeaponPhases, error, path + ".AlternateWeaponPhases")) { return false; }
            field = requiredField(json, "SlowMechanicalCycle", error, path); if (field == nullptr || !fromValue(*field, value.SlowMechanicalCycle, error, path + ".SlowMechanicalCycle")) { return false; }
            field = requiredField(json, "IrregularEngineCycle", error, path); if (field == nullptr || !fromValue(*field, value.IrregularEngineCycle, error, path + ".IrregularEngineCycle")) { return false; }
            field = requiredField(json, "RandomizeSymmetricWeaponAlternatePhase", error, path); if (field == nullptr || !fromValue(*field, value.RandomizeSymmetricWeaponAlternatePhase, error, path + ".RandomizeSymmetricWeaponAlternatePhase")) { return false; }
            field = requiredField(json, "AlternateTechCorePhases", error, path); if (field == nullptr || !fromValue(*field, value.AlternateTechCorePhases, error, path + ".AlternateTechCorePhases")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionMovementAnimationProfile& value)
        {
            Value object = Value::object();
            object.Object["ResponseStrengthScale"] = toValue(value.ResponseStrengthScale);
            object.Object["Synchronized"] = toValue(value.Synchronized);
            object.Object["Staggered"] = toValue(value.Staggered);
            object.Object["HeavyResponse"] = toValue(value.HeavyResponse);
            object.Object["SquareTransitionInput"] = toValue(value.SquareTransitionInput);
            object.Object["MinimumExhaustVariationLimit"] = toValue(value.MinimumExhaustVariationLimit);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionMovementAnimationProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "ResponseStrengthScale", error, path); if (field == nullptr || !fromValue(*field, value.ResponseStrengthScale, error, path + ".ResponseStrengthScale")) { return false; }
            field = requiredField(json, "Synchronized", error, path); if (field == nullptr || !fromValue(*field, value.Synchronized, error, path + ".Synchronized")) { return false; }
            field = requiredField(json, "Staggered", error, path); if (field == nullptr || !fromValue(*field, value.Staggered, error, path + ".Staggered")) { return false; }
            field = requiredField(json, "HeavyResponse", error, path); if (field == nullptr || !fromValue(*field, value.HeavyResponse, error, path + ".HeavyResponse")) { return false; }
            field = requiredField(json, "SquareTransitionInput", error, path); if (field == nullptr || !fromValue(*field, value.SquareTransitionInput, error, path + ".SquareTransitionInput")) { return false; }
            field = requiredField(json, "MinimumExhaustVariationLimit", error, path); if (field == nullptr || !fromValue(*field, value.MinimumExhaustVariationLimit, error, path + ".MinimumExhaustVariationLimit")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionFiringAnimationProfile& value)
        {
            Value object = Value::object();
            object.Object["DurationScale"] = toValue(value.DurationScale);
            object.Object["DurationAdditionMilliseconds"] = toValue(value.DurationAdditionMilliseconds);
            object.Object["ResponseStrengthScale"] = toValue(value.ResponseStrengthScale);
            object.Object["MaximumPreFireExtensionLimit"] = toValue(value.MaximumPreFireExtensionLimit);
            object.Object["HeavyResponse"] = toValue(value.HeavyResponse);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionFiringAnimationProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "DurationScale", error, path); if (field == nullptr || !fromValue(*field, value.DurationScale, error, path + ".DurationScale")) { return false; }
            field = requiredField(json, "DurationAdditionMilliseconds", error, path); if (field == nullptr || !fromValue(*field, value.DurationAdditionMilliseconds, error, path + ".DurationAdditionMilliseconds")) { return false; }
            field = requiredField(json, "ResponseStrengthScale", error, path); if (field == nullptr || !fromValue(*field, value.ResponseStrengthScale, error, path + ".ResponseStrengthScale")) { return false; }
            field = requiredField(json, "MaximumPreFireExtensionLimit", error, path); if (field == nullptr || !fromValue(*field, value.MaximumPreFireExtensionLimit, error, path + ".MaximumPreFireExtensionLimit")) { return false; }
            field = requiredField(json, "HeavyResponse", error, path); if (field == nullptr || !fromValue(*field, value.HeavyResponse, error, path + ".HeavyResponse")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionAnimationProfile& value)
        {
            Value object = Value::object();
            object.Object["Idle"] = toValue(value.Idle);
            object.Object["LateralMovement"] = toValue(value.LateralMovement);
            object.Object["LongitudinalMovement"] = toValue(value.LongitudinalMovement);
            object.Object["Firing"] = toValue(value.Firing);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionAnimationProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Idle", error, path); if (field == nullptr || !fromValue(*field, value.Idle, error, path + ".Idle")) { return false; }
            field = requiredField(json, "LateralMovement", error, path); if (field == nullptr || !fromValue(*field, value.LateralMovement, error, path + ".LateralMovement")) { return false; }
            field = requiredField(json, "LongitudinalMovement", error, path); if (field == nullptr || !fromValue(*field, value.LongitudinalMovement, error, path + ".LongitudinalMovement")) { return false; }
            field = requiredField(json, "Firing", error, path); if (field == nullptr || !fromValue(*field, value.Firing, error, path + ".Firing")) { return false; }
            return true;
        }

        Value toValue(const ShipFactionProfile& value)
        {
            Value object = Value::object();
            object.Object["Palette"] = toValue(value.Palette);
            object.Object["PaletteBehavior"] = toValue(value.PaletteBehavior);
            object.Object["SurfaceDetails"] = toValue(value.SurfaceDetails);
            object.Object["Attachments"] = toValue(value.Attachments);
            object.Object["Weapons"] = toValue(value.Weapons);
            object.Object["Engines"] = toValue(value.Engines);
            object.Object["MajorFeatures"] = toValue(value.MajorFeatures);
            object.Object["Cockpit"] = toValue(value.Cockpit);
            object.Object["Hull"] = toValue(value.Hull);
            object.Object["CoreTreatment"] = toValue(value.CoreTreatment);
            object.Object["HullLayers"] = toValue(value.HullLayers);
            object.Object["Materials"] = toValue(value.Materials);
            object.Object["Livery"] = toValue(value.Livery);
            object.Object["VisualHierarchy"] = toValue(value.VisualHierarchy);
            object.Object["MacroAsymmetry"] = toValue(value.MacroAsymmetry);
            object.Object["Complexity"] = toValue(value.Complexity);
            object.Object["Finish"] = toValue(value.Finish);
            object.Object["Animation"] = toValue(value.Animation);
            return object;
        }

        bool fromValue(const Value& json, ShipFactionProfile& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Palette", error, path); if (field == nullptr || !fromValue(*field, value.Palette, error, path + ".Palette")) { return false; }
            field = requiredField(json, "PaletteBehavior", error, path); if (field == nullptr || !fromValue(*field, value.PaletteBehavior, error, path + ".PaletteBehavior")) { return false; }
            field = requiredField(json, "SurfaceDetails", error, path); if (field == nullptr || !fromValue(*field, value.SurfaceDetails, error, path + ".SurfaceDetails")) { return false; }
            field = requiredField(json, "Attachments", error, path); if (field == nullptr || !fromValue(*field, value.Attachments, error, path + ".Attachments")) { return false; }
            field = requiredField(json, "Weapons", error, path); if (field == nullptr || !fromValue(*field, value.Weapons, error, path + ".Weapons")) { return false; }
            field = requiredField(json, "Engines", error, path); if (field == nullptr || !fromValue(*field, value.Engines, error, path + ".Engines")) { return false; }
            field = requiredField(json, "MajorFeatures", error, path); if (field == nullptr || !fromValue(*field, value.MajorFeatures, error, path + ".MajorFeatures")) { return false; }
            field = requiredField(json, "Cockpit", error, path); if (field == nullptr || !fromValue(*field, value.Cockpit, error, path + ".Cockpit")) { return false; }
            field = requiredField(json, "Hull", error, path); if (field == nullptr || !fromValue(*field, value.Hull, error, path + ".Hull")) { return false; }
            field = requiredField(json, "CoreTreatment", error, path); if (field == nullptr || !fromValue(*field, value.CoreTreatment, error, path + ".CoreTreatment")) { return false; }
            field = requiredField(json, "HullLayers", error, path); if (field == nullptr || !fromValue(*field, value.HullLayers, error, path + ".HullLayers")) { return false; }
            field = requiredField(json, "Materials", error, path); if (field == nullptr || !fromValue(*field, value.Materials, error, path + ".Materials")) { return false; }
            field = requiredField(json, "Livery", error, path); if (field == nullptr || !fromValue(*field, value.Livery, error, path + ".Livery")) { return false; }
            field = requiredField(json, "VisualHierarchy", error, path); if (field == nullptr || !fromValue(*field, value.VisualHierarchy, error, path + ".VisualHierarchy")) { return false; }
            field = requiredField(json, "MacroAsymmetry", error, path); if (field == nullptr || !fromValue(*field, value.MacroAsymmetry, error, path + ".MacroAsymmetry")) { return false; }
            field = requiredField(json, "Complexity", error, path); if (field == nullptr || !fromValue(*field, value.Complexity, error, path + ".Complexity")) { return false; }
            field = requiredField(json, "Finish", error, path); if (field == nullptr || !fromValue(*field, value.Finish, error, path + ".Finish")) { return false; }
            field = requiredField(json, "Animation", error, path); if (field == nullptr || !fromValue(*field, value.Animation, error, path + ".Animation")) { return false; }
            return true;
        }

        Value toValue(const ShipPaletteGenerationProfile& value)
        {
            Value object = Value::object();
            object.Object["Ranges"] = toValue(value.Ranges);
            object.Object["Behavior"] = toValue(value.Behavior);
            return object;
        }

        bool fromValue(const Value& json, ShipPaletteGenerationProfile& value, std::string& error, const std::string& path)
        {
            const Value* ranges = requiredField(json, "Ranges", error, path);
            if (ranges == nullptr || !fromValue(*ranges, value.Ranges, error, path + ".Ranges")) { return false; }
            const Value* behavior = requiredField(json, "Behavior", error, path);
            return behavior != nullptr && fromValue(*behavior, value.Behavior, error, path + ".Behavior");
        }

        Value toValue(const ShipPalette& value)
        {
            Value object = Value::object();
            object.Object["Transparent"] = toValue(value.Transparent);
            object.Object["Outline"] = toValue(value.Outline);
            object.Object["HullDeepShadow"] = toValue(value.HullDeepShadow);
            object.Object["HullShadow"] = toValue(value.HullShadow);
            object.Object["HullBase"] = toValue(value.HullBase);
            object.Object["HullHighlight"] = toValue(value.HullHighlight);
            object.Object["HullSecondary"] = toValue(value.HullSecondary);
            object.Object["HullEdgeHighlight"] = toValue(value.HullEdgeHighlight);
            object.Object["CockpitDark"] = toValue(value.CockpitDark);
            object.Object["CockpitBase"] = toValue(value.CockpitBase);
            object.Object["CockpitHighlight"] = toValue(value.CockpitHighlight);
            object.Object["CockpitGlint"] = toValue(value.CockpitGlint);
            object.Object["EngineDark"] = toValue(value.EngineDark);
            object.Object["EngineBase"] = toValue(value.EngineBase);
            object.Object["EngineHighlight"] = toValue(value.EngineHighlight);
            object.Object["EngineHotCore"] = toValue(value.EngineHotCore);
            object.Object["ExhaustBase"] = toValue(value.ExhaustBase);
            object.Object["ExhaustHighlight"] = toValue(value.ExhaustHighlight);
            object.Object["ExhaustHotCore"] = toValue(value.ExhaustHotCore);
            object.Object["HullAccentDark"] = toValue(value.HullAccentDark);
            object.Object["HullAccent"] = toValue(value.HullAccent);
            object.Object["HullAccentHighlight"] = toValue(value.HullAccentHighlight);
            object.Object["MechanicalDark"] = toValue(value.MechanicalDark);
            object.Object["MechanicalBase"] = toValue(value.MechanicalBase);
            object.Object["LightBase"] = toValue(value.LightBase);
            object.Object["LightHighlight"] = toValue(value.LightHighlight);
            return object;
        }

        bool fromValue(const Value& json, ShipPalette& value, std::string& error, const std::string& path)
        {
            const Value* field = nullptr;
            field = requiredField(json, "Transparent", error, path); if (field == nullptr || !fromValue(*field, value.Transparent, error, path + ".Transparent")) { return false; }
            field = requiredField(json, "Outline", error, path); if (field == nullptr || !fromValue(*field, value.Outline, error, path + ".Outline")) { return false; }
            field = requiredField(json, "HullDeepShadow", error, path); if (field == nullptr || !fromValue(*field, value.HullDeepShadow, error, path + ".HullDeepShadow")) { return false; }
            field = requiredField(json, "HullShadow", error, path); if (field == nullptr || !fromValue(*field, value.HullShadow, error, path + ".HullShadow")) { return false; }
            field = requiredField(json, "HullBase", error, path); if (field == nullptr || !fromValue(*field, value.HullBase, error, path + ".HullBase")) { return false; }
            field = requiredField(json, "HullHighlight", error, path); if (field == nullptr || !fromValue(*field, value.HullHighlight, error, path + ".HullHighlight")) { return false; }
            field = requiredField(json, "HullSecondary", error, path); if (field == nullptr || !fromValue(*field, value.HullSecondary, error, path + ".HullSecondary")) { return false; }
            field = requiredField(json, "HullEdgeHighlight", error, path); if (field == nullptr || !fromValue(*field, value.HullEdgeHighlight, error, path + ".HullEdgeHighlight")) { return false; }
            field = requiredField(json, "CockpitDark", error, path); if (field == nullptr || !fromValue(*field, value.CockpitDark, error, path + ".CockpitDark")) { return false; }
            field = requiredField(json, "CockpitBase", error, path); if (field == nullptr || !fromValue(*field, value.CockpitBase, error, path + ".CockpitBase")) { return false; }
            field = requiredField(json, "CockpitHighlight", error, path); if (field == nullptr || !fromValue(*field, value.CockpitHighlight, error, path + ".CockpitHighlight")) { return false; }
            field = requiredField(json, "CockpitGlint", error, path); if (field == nullptr || !fromValue(*field, value.CockpitGlint, error, path + ".CockpitGlint")) { return false; }
            field = requiredField(json, "EngineDark", error, path); if (field == nullptr || !fromValue(*field, value.EngineDark, error, path + ".EngineDark")) { return false; }
            field = requiredField(json, "EngineBase", error, path); if (field == nullptr || !fromValue(*field, value.EngineBase, error, path + ".EngineBase")) { return false; }
            field = requiredField(json, "EngineHighlight", error, path); if (field == nullptr || !fromValue(*field, value.EngineHighlight, error, path + ".EngineHighlight")) { return false; }
            field = requiredField(json, "EngineHotCore", error, path); if (field == nullptr || !fromValue(*field, value.EngineHotCore, error, path + ".EngineHotCore")) { return false; }
            field = requiredField(json, "ExhaustBase", error, path); if (field == nullptr || !fromValue(*field, value.ExhaustBase, error, path + ".ExhaustBase")) { return false; }
            field = requiredField(json, "ExhaustHighlight", error, path); if (field == nullptr || !fromValue(*field, value.ExhaustHighlight, error, path + ".ExhaustHighlight")) { return false; }
            field = requiredField(json, "ExhaustHotCore", error, path); if (field == nullptr || !fromValue(*field, value.ExhaustHotCore, error, path + ".ExhaustHotCore")) { return false; }
            field = requiredField(json, "HullAccentDark", error, path); if (field == nullptr || !fromValue(*field, value.HullAccentDark, error, path + ".HullAccentDark")) { return false; }
            field = requiredField(json, "HullAccent", error, path); if (field == nullptr || !fromValue(*field, value.HullAccent, error, path + ".HullAccent")) { return false; }
            field = requiredField(json, "HullAccentHighlight", error, path); if (field == nullptr || !fromValue(*field, value.HullAccentHighlight, error, path + ".HullAccentHighlight")) { return false; }
            field = requiredField(json, "MechanicalDark", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalDark, error, path + ".MechanicalDark")) { return false; }
            field = requiredField(json, "MechanicalBase", error, path); if (field == nullptr || !fromValue(*field, value.MechanicalBase, error, path + ".MechanicalBase")) { return false; }
            field = requiredField(json, "LightBase", error, path); if (field == nullptr || !fromValue(*field, value.LightBase, error, path + ".LightBase")) { return false; }
            field = requiredField(json, "LightHighlight", error, path); if (field == nullptr || !fromValue(*field, value.LightHighlight, error, path + ".LightHighlight")) { return false; }
            return true;
        }
    }

    RecipeJson::Value serialize(const ShipGenerationProfile& profile) { return toValue(profile); }
    RecipeJson::Value serialize(const ShipFactionProfile& profile) { return toValue(profile); }
    RecipeJson::Value serialize(const ShipPaletteGenerationProfile& profile) { return toValue(profile); }
    RecipeJson::Value serialize(const ShipPalette& palette) { return toValue(palette); }

    bool deserialize(const RecipeJson::Value& value, ShipGenerationProfile& profile, std::string& error, const std::string& path) { return fromValue(value, profile, error, path); }
    bool deserialize(const RecipeJson::Value& value, ShipFactionProfile& profile, std::string& error, const std::string& path) { return fromValue(value, profile, error, path); }
    bool deserialize(const RecipeJson::Value& value, ShipPaletteGenerationProfile& profile, std::string& error, const std::string& path) { return fromValue(value, profile, error, path); }
    bool deserialize(const RecipeJson::Value& value, ShipPalette& palette, std::string& error, const std::string& path) { return fromValue(value, palette, error, path); }
}
