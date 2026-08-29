#include "ShipPaletteGenerator.h"

#include "Color.h"
#include "ShipFactionPaletteProfile.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <random>

namespace PixelShipGenerator
{
    namespace
    {
        struct HSVColor
        {
            int32_t Hue = 0;
            uint32_t Saturation = 0u;
            uint32_t Value = 0u;
        };

        struct ShadedColors
        {
            Color Dark;
            Color Base;
            Color Highlight;
        };

        uint64_t mixSeed(uint64_t value)
        {
            value += 0x9E3779B97F4A7C15ull;
            value = (value ^ (value >> 30u)) * 0xBF58476D1CE4E5B9ull;
            value = (value ^ (value >> 27u)) * 0x94D049BB133111EBull;
            return value ^ (value >> 31u);
        }

        uint32_t getRandomUInt(std::mt19937_64& randomGenerator, const PaletteUIntRange& range)
        {
            if (range.Min == range.Max) { return range.Min; }
            std::uniform_int_distribution<uint32_t> distribution(range.Min, range.Max);
            return distribution(randomGenerator);
        }

        int32_t getRandomInt(std::mt19937_64& randomGenerator, const PaletteIntRange& range)
        {
            if (range.Min == range.Max) { return range.Min; }
            std::uniform_int_distribution<int32_t> distribution(range.Min, range.Max);
            return distribution(randomGenerator);
        }

        int32_t wrapHue(int32_t hue)
        {
            hue %= 360;
            if (hue < 0) { hue += 360; }
            return hue;
        }

        uint32_t hueDistance(int32_t first, int32_t second)
        {
            const uint32_t direct = static_cast<uint32_t>(std::abs(wrapHue(first) - wrapHue(second)));
            return std::min(direct, 360u - direct);
        }

        uint32_t scalePercentage(uint32_t value, uint32_t percentage)
        {
            return std::min(100u, static_cast<uint32_t>((static_cast<uint64_t>(value) * percentage + 50u) / 100u));
        }

        uint32_t applySignedOffset(uint32_t value, int32_t offset)
        {
            const int32_t adjustedValue = static_cast<int32_t>(value) + offset;
            return static_cast<uint32_t>(std::clamp(adjustedValue, 0, 100));
        }

        Color hsvToColor(const HSVColor& hsv)
        {
            const uint32_t saturation = std::min(hsv.Saturation, 100u);
            const uint32_t value = std::min(hsv.Value, 100u);
            const uint32_t s = (saturation * 255u) / 100u;
            const uint32_t v = (value * 255u) / 100u;

            if (s == 0u)
            {
                return Color(static_cast<uint8_t>(v), static_cast<uint8_t>(v), static_cast<uint8_t>(v), 255u);
            }

            const uint32_t hue = static_cast<uint32_t>(wrapHue(hsv.Hue));
            const uint32_t region = hue / 60u;
            const uint32_t remainder = ((hue % 60u) * 255u) / 60u;
            const uint32_t p = (v * (255u - s)) / 255u;
            const uint32_t q = (v * (255u - ((s * remainder) / 255u))) / 255u;
            const uint32_t t = (v * (255u - ((s * (255u - remainder)) / 255u))) / 255u;

            switch (region)
            {
            case 0u: return Color(static_cast<uint8_t>(v), static_cast<uint8_t>(t), static_cast<uint8_t>(p), 255u);
            case 1u: return Color(static_cast<uint8_t>(q), static_cast<uint8_t>(v), static_cast<uint8_t>(p), 255u);
            case 2u: return Color(static_cast<uint8_t>(p), static_cast<uint8_t>(v), static_cast<uint8_t>(t), 255u);
            case 3u: return Color(static_cast<uint8_t>(p), static_cast<uint8_t>(q), static_cast<uint8_t>(v), 255u);
            case 4u: return Color(static_cast<uint8_t>(t), static_cast<uint8_t>(p), static_cast<uint8_t>(v), 255u);
            default: return Color(static_cast<uint8_t>(v), static_cast<uint8_t>(p), static_cast<uint8_t>(q), 255u);
            }
        }

        HSVColor generateRoleColor(std::mt19937_64& randomGenerator, int32_t hullHue, const PaletteRoleProfile& roleProfile)
        {
            HSVColor color;
            color.Hue = wrapHue(hullHue + getRandomInt(randomGenerator, roleProfile.HueOffset));
            color.Saturation = getRandomUInt(randomGenerator, roleProfile.Saturation);
            color.Value = getRandomUInt(randomGenerator, roleProfile.Value);
            return color;
        }

        ShadedColors createShadedColors(const HSVColor& baseColor, uint32_t contrast)
        {
            HSVColor dark = baseColor;
            HSVColor highlight = baseColor;

            dark.Value = baseColor.Value > contrast ? baseColor.Value - contrast : 0u;
            highlight.Value = std::min(100u, baseColor.Value + contrast);
            highlight.Saturation = scalePercentage(baseColor.Saturation, 90u);

            return { hsvToColor(dark), hsvToColor(baseColor), hsvToColor(highlight) };
        }

        HSVColor offsetValue(const HSVColor& color, int32_t amount)
        {
            HSVColor result = color;
            result.Value = applySignedOffset(result.Value, amount);
            return result;
        }

        HSVColor offsetSaturation(const HSVColor& color, int32_t amount)
        {
            HSVColor result = color;
            result.Saturation = applySignedOffset(result.Saturation, amount);
            return result;
        }
    }

    ShipPalette ShipPaletteGenerator::generate(uint64_t paletteSeed, ShipStyle style, ShipFactionType faction, const ShipGenerationProfile& styleProfile, bool enhancedMaterialContrast)
    {
        const ShipFactionPaletteProfile& factionProfile = getShipFactionPaletteProfile(faction);
        std::mt19937_64 randomGenerator(paletteSeed);

        HSVColor hullColor;
        hullColor.Hue = static_cast<int32_t>(getRandomUInt(randomGenerator, factionProfile.HullHue));
        hullColor.Saturation = getRandomUInt(randomGenerator, factionProfile.HullSaturation);
        hullColor.Value = getRandomUInt(randomGenerator, factionProfile.HullValue);

        if (faction == ShipFactionType::CORPORATE)
        {
            // Two deterministic finish families keep Corporate from collapsing
            // into one white/blue palette: bright commercial alloy or dark
            // premium bodywork, both with a low-saturation primary hull.
            const uint64_t finishVariant = mixSeed(paletteSeed ^ 0xC08B3A7E5D1F249Bull);
            hullColor.Value = (finishVariant & 1ull) != 0ull
                ? 68u + static_cast<uint32_t>((finishVariant >> 8u) % 17ull)
                : 32u + static_cast<uint32_t>((finishVariant >> 8u) % 17ull);
        }

        hullColor.Saturation = scalePercentage(hullColor.Saturation, styleProfile.PaletteHullSaturationPercent);
        hullColor.Value = applySignedOffset(hullColor.Value, styleProfile.PaletteHullValueOffset);

        const uint32_t contrast = std::max(4u, (12u * styleProfile.PaletteContrastPercent + 50u) / 100u);

        HSVColor hullDeepShadowColor = hullColor;
        hullDeepShadowColor.Value = applySignedOffset(hullColor.Value, -static_cast<int32_t>(contrast * 2u));

        HSVColor hullSecondaryColor = hullColor;
        hullSecondaryColor.Hue = wrapHue(hullColor.Hue + getRandomInt(randomGenerator, { -8, 8 }));
        hullSecondaryColor.Saturation = scalePercentage(hullColor.Saturation, getRandomUInt(randomGenerator, { 85u, 110u }));
        if (enhancedMaterialContrast)
        {
            const int32_t secondaryMagnitude = std::max(4, static_cast<int32_t>((8u * styleProfile.MaterialSecondaryContrastPercent + 50u) / 100u));
            int32_t secondaryDirection = (mixSeed(paletteSeed ^ 0xA3C59AC3E17B5D6Full) & 1ull) != 0ull ? 1 : -1;
            if (faction == ShipFactionType::ASCENDANT) { secondaryDirection = -1; }
            else if (faction == ShipFactionType::CORPORATE) { secondaryDirection = hullColor.Value >= 55u ? -1 : 1; }
            else if (faction == ShipFactionType::RELIC) { secondaryDirection = 1; }
            hullSecondaryColor.Value = applySignedOffset(hullColor.Value, secondaryDirection * (secondaryMagnitude + getRandomInt(randomGenerator, { -2, 2 })));
        }
        else
        {
            hullSecondaryColor.Value = applySignedOffset(hullColor.Value, getRandomInt(randomGenerator, { -5, 5 }));
        }

        HSVColor hullEdgeHighlightColor = hullColor;
        hullEdgeHighlightColor.Saturation = scalePercentage(hullColor.Saturation, 80u);
        hullEdgeHighlightColor.Value = applySignedOffset(hullColor.Value, static_cast<int32_t>(contrast * 2u));

        const ShadedColors hullColors = createShadedColors(hullColor, contrast);

        HSVColor accentColor = generateRoleColor(randomGenerator, hullColor.Hue, factionProfile.Accent);
        if (faction == ShipFactionType::CORPORATE && hueDistance(accentColor.Hue, hullColor.Hue) < 65u)
        {
            // Brand accents must stay visibly distinct from the neutral primary
            // finish even when the broad procedural hue ranges overlap.
            accentColor.Hue = wrapHue(accentColor.Hue + ((mixSeed(paletteSeed) & 1ull) != 0ull ? 120 : 210));
        }
        accentColor.Saturation = scalePercentage(accentColor.Saturation, styleProfile.PaletteAccentSaturationPercent);
        const ShadedColors accentColors = createShadedColors(accentColor, std::max(3u, contrast - 2u));

        HSVColor cockpitColor = generateRoleColor(randomGenerator, hullColor.Hue, factionProfile.Cockpit);
        cockpitColor.Value = scalePercentage(cockpitColor.Value, styleProfile.PaletteEmissiveValuePercent);
        const ShadedColors cockpitColors = createShadedColors(cockpitColor, std::max(4u, contrast));

        HSVColor cockpitGlintColor = cockpitColor;
        cockpitGlintColor.Saturation = scalePercentage(cockpitColor.Saturation, 70u);
        cockpitGlintColor.Value = std::min(100u, cockpitColor.Value + 18u);

        HSVColor lightColor = generateRoleColor(randomGenerator, hullColor.Hue, factionProfile.Light);
        lightColor.Value = scalePercentage(lightColor.Value, styleProfile.PaletteEmissiveValuePercent);

        HSVColor exhaustColor = generateRoleColor(randomGenerator, hullColor.Hue, factionProfile.Exhaust);
        exhaustColor.Value = scalePercentage(exhaustColor.Value, styleProfile.PaletteEmissiveValuePercent);

        HSVColor engineHotCoreColor = exhaustColor;
        engineHotCoreColor.Value = std::min(100u, exhaustColor.Value + 8u);

        HSVColor exhaustHotCoreColor = exhaustColor;
        exhaustHotCoreColor.Saturation = scalePercentage(exhaustColor.Saturation, 70u);
        exhaustHotCoreColor.Value = 100u;

        HSVColor engineColor;
        engineColor.Hue = wrapHue(hullColor.Hue + getRandomInt(randomGenerator, { -8, 8 }));
        engineColor.Saturation = getRandomUInt(randomGenerator, factionProfile.MechanicalSaturation);
        engineColor.Value = getRandomUInt(randomGenerator, factionProfile.MechanicalValue);
        HSVColor mechanicalBaseColor = engineColor;
        const ShadedColors engineColors = createShadedColors(engineColor, std::max(4u, contrast - 2u));

        HSVColor outlineColor = hullColor;
        outlineColor.Saturation = scalePercentage(hullColor.Saturation, 50u);
        outlineColor.Value = hullColor.Value > contrast * 2u + 8u ? hullColor.Value - contrast * 2u - 8u : 4u;

        HSVColor mechanicalDarkColor = engineColor;
        mechanicalDarkColor.Value = engineColor.Value > contrast + 6u ? engineColor.Value - contrast - 6u : 5u;

        HSVColor lightHighlightColor = lightColor;
        lightHighlightColor.Value = std::min(100u, lightColor.Value + 10u);

        HSVColor exhaustHighlightColor = exhaustColor;
        exhaustHighlightColor.Value = std::min(100u, exhaustColor.Value + 10u);
        exhaustHighlightColor.Saturation = scalePercentage(exhaustColor.Saturation, 85u);

        ShipPalette palette;

        palette.Transparent = Color(0u, 0u, 0u, 0u);
        palette.Outline = hsvToColor(outlineColor);

        palette.HullDeepShadow = hsvToColor(hullDeepShadowColor);
        palette.HullShadow = hullColors.Dark;
        palette.HullBase = hullColors.Base;
        palette.HullHighlight = hullColors.Highlight;
        palette.HullSecondary = hsvToColor(hullSecondaryColor);
        palette.HullEdgeHighlight = hsvToColor(hullEdgeHighlightColor);

        palette.CockpitDark = cockpitColors.Dark;
        palette.CockpitBase = cockpitColors.Base;
        palette.CockpitHighlight = cockpitColors.Highlight;
        palette.CockpitGlint = hsvToColor(cockpitGlintColor);

        palette.EngineDark = engineColors.Dark;
        palette.EngineBase = engineColors.Base;
        palette.EngineHighlight = engineColors.Highlight;
        palette.EngineHotCore = hsvToColor(engineHotCoreColor);

        palette.ExhaustBase = hsvToColor(exhaustColor);
        palette.ExhaustHighlight = hsvToColor(exhaustHighlightColor);
        palette.ExhaustHotCore = hsvToColor(exhaustHotCoreColor);

        palette.HullAccentDark = accentColors.Dark;
        palette.HullAccent = accentColors.Base;
        palette.HullAccentHighlight = accentColors.Highlight;

        palette.MechanicalDark = hsvToColor(mechanicalDarkColor);
        palette.MechanicalBase = hsvToColor(mechanicalBaseColor);


        palette.LightBase = hsvToColor(lightColor);
        palette.LightHighlight = hsvToColor(lightHighlightColor);

        return palette;
    }
}