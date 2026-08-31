#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>

#include <cstdint>
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

        void validateRange(Result& result, const char* field, const PaletteUIntRange& range, uint32_t maximum)
        {
            if (range.Min > range.Max) { addError(result, field, "range minimum must not exceed maximum"); }
            if (range.Max > maximum) { addError(result, field, "range exceeds its semantic maximum"); }
        }

        void validateRange(Result& result, const char* field, const PaletteIntRange& range, int32_t minimum, int32_t maximum)
        {
            if (range.Min > range.Max) { addError(result, field, "range minimum must not exceed maximum"); }
            if (range.Min < minimum || range.Max > maximum) { addError(result, field, "range exceeds its safe semantic bounds"); }
        }

        void validateRole(Result& result, const char* prefix, const PaletteRoleProfile& role)
        {
            validateRange(result, (std::string(prefix) + ".HueOffset").c_str(), role.HueOffset, -360, 360);
            validateRange(result, (std::string(prefix) + ".Saturation").c_str(), role.Saturation, 100u);
            validateRange(result, (std::string(prefix) + ".Value").c_str(), role.Value, 100u);
        }

        void validateSafeOffset(Result& result, const char* field, int32_t value)
        {
            constexpr int32_t SafeMagnitude = 1000000;
            if (value < -SafeMagnitude || value > SafeMagnitude) { addError(result, field, "signed offset exceeds the supported safe composition range"); }
        }
    }

    ValidationResult validateShipPaletteGenerationProfile(const ShipPaletteGenerationProfile& profile)
    {
        Result result;
        const ShipPaletteGenerationRanges& ranges = profile.Ranges;
        const ShipPaletteGenerationBehaviorProfile& behavior = profile.Behavior;

        validateRange(result, "Ranges.HullHue", ranges.HullHue, 359u);
        validateRange(result, "Ranges.HullSaturation", ranges.HullSaturation, 100u);
        validateRange(result, "Ranges.HullValue", ranges.HullValue, 100u);
        validateRole(result, "Ranges.Accent", ranges.Accent);
        validateRole(result, "Ranges.Cockpit", ranges.Cockpit);
        validateRole(result, "Ranges.Light", ranges.Light);
        validateRole(result, "Ranges.Exhaust", ranges.Exhaust);
        validateRange(result, "Ranges.MechanicalSaturation", ranges.MechanicalSaturation, 100u);
        validateRange(result, "Ranges.MechanicalValue", ranges.MechanicalValue, 100u);

        if (static_cast<uint32_t>(behavior.HullValueMode) >= static_cast<uint32_t>(ShipFactionHullValueMode::SHIP_FACTION_HULL_VALUE_MODE_END))
        {
            addError(result, "Behavior.HullValueMode", "enum value is outside the supported range");
        }
        if (static_cast<uint32_t>(behavior.SecondaryToneDirection) >= static_cast<uint32_t>(ShipFactionSecondaryToneDirection::SHIP_FACTION_SECONDARY_TONE_DIRECTION_END))
        {
            addError(result, "Behavior.SecondaryToneDirection", "enum value is outside the supported range");
        }
        if (behavior.HullValueMode == ShipFactionHullValueMode::ALTERNATING_BRIGHT_DARK_RANGES)
        {
            validateRange(result, "Behavior.BrightHullValue", behavior.BrightHullValue, 100u);
            validateRange(result, "Behavior.DarkHullValue", behavior.DarkHullValue, 100u);
        }
        if (behavior.MinimumAccentHueDistance > 180u)
        {
            addError(result, "Behavior.MinimumAccentHueDistance", "hue distance cannot exceed 180 degrees");
        }
        validateSafeOffset(result, "Behavior.AccentHueSeparationShiftA", behavior.AccentHueSeparationShiftA);
        validateSafeOffset(result, "Behavior.AccentHueSeparationShiftB", behavior.AccentHueSeparationShiftB);

        return result;
    }
}
