#include "ShipIdleAnimator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "AnimationSamplingPlanner.h"
#include "ShipPainter.h"
#include "ShipGenerationSeeds.h"

namespace
{
    constexpr uint64_t AnimationSeedSalt = 0x4D595DF4D0F33173ull;
    constexpr uint64_t EngineVariationSalt = 0xC6BC279692B5CC83ull;
    constexpr uint64_t EngineMechanicalSalt = 0x8CB92BA72F3D8DD7ull;
    constexpr uint64_t LightVariationSalt = 0xD1B54A32D192ED03ull;
    constexpr uint64_t DetailVariationSalt = 0x94D049BB133111EBull;
    constexpr uint64_t MicroMovementSalt = 0xA24BAED4963EE407ull;
    constexpr uint64_t WeaponMovementSalt = 0xDB4F0B9175AE2165ull;
    constexpr uint64_t MajorFeatureSalt = 0x6C8E9CF570932BD5ull;

    struct PixelCoordinate
    {
        uint32_t X = 0u;
        uint32_t Y = 0u;
    };

    struct OpaqueBounds
    {
        uint32_t MinX = 0u;
        uint32_t MaxX = 0u;
        uint32_t MinY = 0u;
        uint32_t MaxY = 0u;
        bool Valid = false;
    };

    struct IdleAnimationProfile
    {
        uint32_t EnginePulseStrength = 1u;
        uint32_t ExhaustAmplitudePercent = 85u;
        uint32_t EngineMechanicalChance = 3u;
        uint32_t WeaponMechanicalChance = 50u;
        uint32_t VentActivityChance = 55u;
        uint32_t TechPulseStrength = 1u;
        bool SynchronizeEngines = false;
        bool AsynchronousEngines = false;
        bool AlternateEnginePhases = false;
        bool AlternateWeaponPhases = false;
        bool SlowMechanicalCycle = false;
        bool IrregularEngineCycle = false;
    };

    struct EnginePulseSample
    {
        double Length = 0.0;
        double Intensity = 0.0;
        double TaperBias = 0.0;
        double InnerLength = 0.0;
        double CoreLength = 0.0;
    };

    struct EngineAnimationParameters
    {
        uint32_t CurveVariant = 0u;
        uint32_t ExhaustAmplitudePercent = 100u;
        bool ReverseTime = false;
        bool MechanicalActive = false;
        bool MechanicalAlternatePhase = false;
    };

    struct WeaponAnimationParameters
    {
        bool MechanicalActive = false;
        bool AlternatePhase = false;
    };

    struct MajorFeatureAnimationParameters
    {
        bool Active = false;
        bool AlternatePhase = false;
        uint32_t PatternParity = 0u;
    };

    struct IdleAnimationPlan
    {
        IdleAnimationProfile Profile;
        std::vector<EngineAnimationParameters> EngineParameters;
        std::vector<WeaponAnimationParameters> WeaponParameters;
        std::vector<MajorFeatureAnimationParameters> MajorFeatureParameters;
        std::array<std::vector<PixelCoordinate>, 2u> LightGroupPixels;
        int32_t PreferredMicroMovementDirection = 1;
        std::optional<std::size_t> MicroMovementPlacementIndex;
        std::optional<PixelCoordinate> DetailVariationPixel;
        PixelShipGenerator::AnimationSamplingRequirements SamplingRequirements;
    };

    struct CurvePoint
    {
        double Time = 0.0;
        double Value = 0.0;
    };

    uint64_t getAnimationHash(uint64_t seed, uint32_t x, uint32_t y, uint64_t salt)
    {
        uint64_t value = seed;
        value ^= static_cast<uint64_t>(x) * 0x9E3779B185EBCA87ull;
        value ^= static_cast<uint64_t>(y) * 0xC2B2AE3D27D4EB4Full;
        value ^= salt;
        return PixelShipGenerator::mixGenerationSeed64(value);
    }

    double wrapNormalizedTime(double normalizedTime)
    {
        if (!std::isfinite(normalizedTime))
        {
            return 0.0;
        }

        double wrapped = normalizedTime - std::floor(normalizedTime);
        if (wrapped < 0.0) { wrapped += 1.0; }
        return wrapped;
    }

    template <std::size_t PointCount>
    double sampleCurve(double normalizedTime, const std::array<CurvePoint, PointCount>& points)
    {
        const double time = std::clamp(normalizedTime, 0.0, 1.0);
        if (time <= points.front().Time) { return points.front().Value; }

        for (std::size_t index = 0u; index + 1u < points.size(); ++index)
        {
            const CurvePoint& first = points[index];
            const CurvePoint& second = points[index + 1u];
            if (time > second.Time) { continue; }

            const double range = second.Time - first.Time;
            if (range <= 0.0) { return second.Value; }
            const double local = (time - first.Time) / range;
            return first.Value + (second.Value - first.Value) * local;
        }

        return points.back().Value;
    }

    double samplePulseEnvelope(double normalizedTime, double start, double attackEnd, double holdEnd, double releaseEnd)
    {
        const double time = std::clamp(normalizedTime, 0.0, 1.0);
        if (time <= start || time >= releaseEnd) { return 0.0; }
        if (time < attackEnd) { return (time - start) / std::max(0.000001, attackEnd - start); }
        if (time <= holdEnd) { return 1.0; }
        return (releaseEnd - time) / std::max(0.000001, releaseEnd - holdEnd);
    }

    int32_t quantizeSignedUnit(double value, uint32_t maximumMagnitude = 1u)
    {
        const double clamped = std::clamp(value, -1.0, 1.0);
        const double scaled = clamped * static_cast<double>(maximumMagnitude);
        return scaled >= 0.0 ? static_cast<int32_t>(std::floor(scaled + 0.5)) : static_cast<int32_t>(std::ceil(scaled - 0.5));
    }

    IdleAnimationProfile getIdleAnimationProfile(const PixelShipGenerator::GeneratedShip& ship)
    {
        IdleAnimationProfile profile;

        switch (ship.Style)
        {
        case PixelShipGenerator::ShipStyle::SLEEK:
            profile.EnginePulseStrength = 1u;
            profile.ExhaustAmplitudePercent = 65u;
            profile.EngineMechanicalChance = 1u;
            profile.WeaponMechanicalChance = 30u;
            profile.VentActivityChance = 30u;
            break;
        case PixelShipGenerator::ShipStyle::FIGHTER:
            profile.EnginePulseStrength = 2u;
            profile.ExhaustAmplitudePercent = 100u;
            profile.EngineMechanicalChance = 3u;
            profile.WeaponMechanicalChance = 65u;
            profile.VentActivityChance = 45u;
            profile.AsynchronousEngines = true;
            break;
        case PixelShipGenerator::ShipStyle::HEAVY:
            profile.EnginePulseStrength = 2u;
            profile.ExhaustAmplitudePercent = 100u;
            profile.EngineMechanicalChance = 4u;
            profile.WeaponMechanicalChance = 50u;
            profile.VentActivityChance = 50u;
            profile.SlowMechanicalCycle = true;
            break;
        case PixelShipGenerator::ShipStyle::INDUSTRIAL:
            profile.EnginePulseStrength = 2u;
            profile.ExhaustAmplitudePercent = 90u;
            profile.EngineMechanicalChance = 8u;
            profile.WeaponMechanicalChance = 80u;
            profile.VentActivityChance = 90u;
            profile.AsynchronousEngines = true;
            break;
        case PixelShipGenerator::ShipStyle::SPEARHEAD:
            profile.EnginePulseStrength = 2u;
            profile.ExhaustAmplitudePercent = 110u;
            profile.EngineMechanicalChance = 2u;
            profile.WeaponMechanicalChance = 45u;
            profile.VentActivityChance = 28u;
            profile.SynchronizeEngines = true;
            break;
        case PixelShipGenerator::ShipStyle::DELTA:
            profile.EnginePulseStrength = 2u;
            profile.ExhaustAmplitudePercent = 95u;
            profile.EngineMechanicalChance = 4u;
            profile.WeaponMechanicalChance = 60u;
            profile.VentActivityChance = 42u;
            profile.AlternateEnginePhases = true;
            break;
        default:
            break;
        }

        switch (ship.Faction)
        {
        case PixelShipGenerator::ShipFactionType::FRONTIER:
            profile.EngineMechanicalChance = std::min(100u, profile.EngineMechanicalChance + 2u);
            profile.WeaponMechanicalChance = std::min(100u, profile.WeaponMechanicalChance + 10u);
            profile.IrregularEngineCycle = true;
            profile.AsynchronousEngines = true;
            break;
        case PixelShipGenerator::ShipFactionType::MILITARY:
            profile.ExhaustAmplitudePercent = profile.ExhaustAmplitudePercent * 4u / 5u;
            profile.SynchronizeEngines = true;
            profile.AsynchronousEngines = false;
            break;
        case PixelShipGenerator::ShipFactionType::ASCENDANT:
            profile.TechPulseStrength = 2u;
            profile.ExhaustAmplitudePercent = profile.ExhaustAmplitudePercent * 9u / 10u;
            profile.EngineMechanicalChance = std::min(1u, profile.EngineMechanicalChance);
            profile.WeaponMechanicalChance = std::max(20u, profile.WeaponMechanicalChance * 3u / 4u);
            break;
        case PixelShipGenerator::ShipFactionType::XENO:
            profile.AlternateEnginePhases = true;
            profile.AsynchronousEngines = true;
            profile.AlternateWeaponPhases = true;
            profile.TechPulseStrength = 2u;
            break;
        case PixelShipGenerator::ShipFactionType::CORPORATE:
            profile.ExhaustAmplitudePercent = profile.ExhaustAmplitudePercent * 3u / 4u;
            profile.EngineMechanicalChance = std::min(2u, profile.EngineMechanicalChance);
            profile.WeaponMechanicalChance = std::max(24u, profile.WeaponMechanicalChance * 4u / 5u);
            profile.VentActivityChance = profile.VentActivityChance * 2u / 3u;
            profile.SynchronizeEngines = true;
            profile.AsynchronousEngines = false;
            profile.AlternateEnginePhases = false;
            profile.AlternateWeaponPhases = false;
            break;
        case PixelShipGenerator::ShipFactionType::RELIC:
            profile.EnginePulseStrength = std::max(1u, profile.EnginePulseStrength);
            profile.ExhaustAmplitudePercent = profile.ExhaustAmplitudePercent * 7u / 10u;
            profile.EngineMechanicalChance = std::min(2u, profile.EngineMechanicalChance);
            profile.WeaponMechanicalChance = std::max(18u, profile.WeaponMechanicalChance * 2u / 3u);
            profile.VentActivityChance = profile.VentActivityChance / 2u;
            profile.TechPulseStrength = 2u;
            profile.SlowMechanicalCycle = true;
            profile.IrregularEngineCycle = false;
            break;
        default:
            break;
        }

        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.HullMask.getWidth(), ship.HullMask.getHeight() });
        const uint32_t mechanicalScalePercent = 20u + scaleTraits.AnimationComplexity * 80u / 100u;
        profile.EngineMechanicalChance = static_cast<uint32_t>((static_cast<uint64_t>(profile.EngineMechanicalChance) * mechanicalScalePercent + 50u) / 100u);
        profile.WeaponMechanicalChance = static_cast<uint32_t>((static_cast<uint64_t>(profile.WeaponMechanicalChance) * mechanicalScalePercent + 50u) / 100u);
        profile.VentActivityChance = static_cast<uint32_t>((static_cast<uint64_t>(profile.VentActivityChance) * (35u + scaleTraits.AnimationComplexity * 65u / 100u) + 50u) / 100u);
        if (scaleTraits.AnimationComplexity < 20u) { profile.TechPulseStrength = std::min(1u, profile.TechPulseStrength); }

        return profile;
    }

    bool containsCoordinate(const std::vector<PixelCoordinate>& pixels, uint32_t x, uint32_t y)
    {
        return std::any_of(pixels.begin(), pixels.end(), [x, y](const PixelCoordinate& pixel) { return pixel.X == x && pixel.Y == y; });
    }

    bool isMaskPixel(const PixelShipGenerator::PixelMask& mask, int32_t x, int32_t y)
    {
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(mask.getWidth()) || y >= static_cast<int32_t>(mask.getHeight()))
        {
            return false;
        }

        return mask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    }

    uint32_t getMaskNeighbourCount(const PixelShipGenerator::PixelMask& mask, int32_t x, int32_t y)
    {
        uint32_t count = 0u;

        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                if (isMaskPixel(mask, x + offsetX, y + offsetY))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    bool isBaseStructurePixel(const PixelShipGenerator::GeneratedShip& ship, int32_t x, int32_t y)
    {
        return isMaskPixel(ship.HullMask, x, y) || isMaskPixel(ship.CockpitMask, x, y) || isMaskPixel(ship.EngineMask, x, y) || isMaskPixel(ship.EngineExhaustMask, x, y) || isMaskPixel(ship.IdleAnimationMetadata.WeaponOccupiedMask, x, y);
    }

    PixelShipGenerator::Color brightenEngineColorOnce(const PixelShipGenerator::Color& color, const PixelShipGenerator::ShipPalette& palette)
    {
        if (color == palette.EngineDark) { return palette.EngineBase; }
        if (color == palette.EngineBase) { return palette.EngineHighlight; }
        if (color == palette.EngineHighlight) { return palette.EngineHotCore; }
        if (color == palette.ExhaustBase) { return palette.ExhaustHighlight; }
        if (color == palette.ExhaustHighlight) { return palette.ExhaustHotCore; }
        return color;
    }

    PixelShipGenerator::Color dimEngineColorOnce(const PixelShipGenerator::Color& color, const PixelShipGenerator::ShipPalette& palette)
    {
        if (color == palette.EngineHotCore) { return palette.EngineHighlight; }
        if (color == palette.EngineHighlight) { return palette.EngineBase; }
        if (color == palette.EngineBase) { return palette.EngineDark; }
        if (color == palette.ExhaustHotCore) { return palette.ExhaustHighlight; }
        if (color == palette.ExhaustHighlight) { return palette.ExhaustBase; }
        return color;
    }

    PixelShipGenerator::Color shiftEngineColor(PixelShipGenerator::Color color, const PixelShipGenerator::ShipPalette& palette, int32_t intensity)
    {
        const uint32_t stepCount = static_cast<uint32_t>(intensity < 0 ? -intensity : intensity);

        for (uint32_t step = 0u; step < stepCount; ++step)
        {
            color = intensity > 0 ? brightenEngineColorOnce(color, palette) : dimEngineColorOnce(color, palette);
        }

        return color;
    }

    PixelShipGenerator::Color shiftExhaustColor(PixelShipGenerator::Color color, const PixelShipGenerator::ShipPalette& palette, int32_t intensity)
    {
        const uint32_t stepCount = static_cast<uint32_t>(intensity < 0 ? -intensity : intensity);

        for (uint32_t step = 0u; step < stepCount; ++step)
        {
            if (intensity > 0)
            {
                if (color == palette.ExhaustBase) { color = palette.ExhaustHighlight; }
                else if (color == palette.ExhaustHighlight) { color = palette.ExhaustHotCore; }
            }
            else
            {
                if (color == palette.ExhaustHotCore) { color = palette.ExhaustHighlight; }
                else if (color == palette.ExhaustHighlight) { color = palette.ExhaustBase; }
            }
        }

        return color;
    }

    EnginePulseSample getEnginePulseSample(double normalizedTime, const EngineAnimationParameters& parameters, const IdleAnimationProfile& profile)
    {
        constexpr std::array<CurvePoint, 8u> StandardLength =
        { {
            { 0.00, 0.00 }, { 0.14, 0.72 }, { 0.27, 1.00 }, { 0.40, 0.45 },
            { 0.55, -0.55 }, { 0.68, -1.00 }, { 0.84, 0.35 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 8u> IrregularLengthA =
        { {
            { 0.00, 0.00 }, { 0.11, 0.48 }, { 0.25, 0.82 }, { 0.39, 1.00 },
            { 0.51, -0.20 }, { 0.66, -0.88 }, { 0.83, -0.38 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 8u> IrregularLengthB =
        { {
            { 0.00, 0.00 }, { 0.13, 1.00 }, { 0.29, 0.62 }, { 0.43, 0.36 },
            { 0.56, -0.48 }, { 0.70, -1.00 }, { 0.86, 0.48 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 8u> StandardIntensity =
        { {
            { 0.00, 0.00 }, { 0.10, 0.55 }, { 0.24, 1.00 }, { 0.39, 0.58 },
            { 0.52, -0.32 }, { 0.69, -0.86 }, { 0.84, -0.30 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 8u> IrregularIntensityA =
        { {
            { 0.00, 0.00 }, { 0.09, 0.42 }, { 0.23, 1.00 }, { 0.42, 0.38 },
            { 0.55, -0.12 }, { 0.70, -1.00 }, { 0.85, -0.42 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 8u> IrregularIntensityB =
        { {
            { 0.00, 0.00 }, { 0.12, 0.68 }, { 0.28, 1.00 }, { 0.43, 0.50 },
            { 0.57, -0.52 }, { 0.72, -0.82 }, { 0.87, 0.08 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 7u> InnerLengthCurve =
        { {
            { 0.00, 0.00 }, { 0.18, 0.72 }, { 0.36, 1.00 }, { 0.53, -0.20 },
            { 0.70, -0.78 }, { 0.86, 0.38 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 7u> CoreLengthCurve =
        { {
            { 0.00, 0.00 }, { 0.16, 0.52 }, { 0.31, 1.00 }, { 0.49, -0.28 },
            { 0.68, -0.62 }, { 0.84, 0.50 }, { 1.00, 0.00 }
        } };
        constexpr std::array<CurvePoint, 7u> TaperCurve =
        { {
            { 0.00, 0.00 }, { 0.20, 0.42 }, { 0.34, 0.88 }, { 0.52, -0.58 },
            { 0.70, 0.32 }, { 0.86, 0.18 }, { 1.00, 0.00 }
        } };

        if (normalizedTime <= 0.0)
        {
            return {};
        }

        double effectiveTime = std::clamp(normalizedTime, 0.0, 1.0);
        if (parameters.ReverseTime) { effectiveTime = 1.0 - effectiveTime; }

        EnginePulseSample sample;
        if (parameters.CurveVariant == 1u)
        {
            sample.Length = sampleCurve(effectiveTime, IrregularLengthA);
            sample.Intensity = sampleCurve(effectiveTime, IrregularIntensityA);
        }
        else if (parameters.CurveVariant == 2u)
        {
            sample.Length = sampleCurve(effectiveTime, IrregularLengthB);
            sample.Intensity = sampleCurve(effectiveTime, IrregularIntensityB);
        }
        else
        {
            sample.Length = sampleCurve(effectiveTime, StandardLength);
            sample.Intensity = sampleCurve(effectiveTime, StandardIntensity);
        }

        sample.InnerLength = sampleCurve(effectiveTime, InnerLengthCurve);
        sample.CoreLength = sampleCurve(effectiveTime, CoreLengthCurve);
        sample.TaperBias = sampleCurve(effectiveTime, TaperCurve);

        if (profile.EnginePulseStrength <= 1u)
        {
            sample.Intensity *= 0.72;
            sample.InnerLength *= 0.72;
            sample.CoreLength *= 0.72;
        }

        return sample;
    }

    uint32_t getCenteredStartX(uint32_t centerXTimesTwo, uint32_t width)
    {
        const int32_t start = (static_cast<int32_t>(centerXTimesTwo) - static_cast<int32_t>(width - 1u)) / 2;
        return static_cast<uint32_t>(std::max(0, start));
    }

    uint32_t getTaperedExhaustRowWidth(uint32_t nozzleWidth, uint32_t length, uint32_t row, uint32_t taperMode, int32_t taperBias)
    {
        if (nozzleWidth <= 1u || length <= 1u)
        {
            return std::max(1u, nozzleWidth);
        }

        const uint32_t maximumInset = (nozzleWidth - 1u) / 2u;
        uint32_t effectiveRow = std::min(row, length - 1u);

        if (effectiveRow > 0u && taperMode == 0u && effectiveRow + 1u < length) { ++effectiveRow; }
        if (taperMode == 2u && effectiveRow > length / 3u) { effectiveRow -= length / 3u; }
        if (taperMode == 2u && row <= length / 3u) { effectiveRow = 0u; }

        if (taperBias < 0 && effectiveRow > 0u)
        {
            effectiveRow = std::min(length - 1u, effectiveRow + std::max(1u, length / 4u));
        }
        else if (taperBias > 0)
        {
            const uint32_t hold = std::max(1u, length / 4u);
            effectiveRow = effectiveRow > hold ? effectiveRow - hold : 0u;
        }

        if (row + 1u == length)
        {
            effectiveRow = length - 1u;
        }

        const uint32_t inset = maximumInset == 0u ? 0u : std::min(maximumInset, static_cast<uint32_t>((static_cast<uint64_t>(effectiveRow) * maximumInset + length - 2u) / (length - 1u)));
        return std::max(1u, nozzleWidth - inset * 2u);
    }

    uint32_t getLayerWidth(uint32_t outerWidth, uint32_t inset)
    {
        if (outerWidth <= inset * 2u)
        {
            return outerWidth % 2u == 0u ? std::min(2u, outerWidth) : 1u;
        }

        return outerWidth - inset * 2u;
    }

    uint32_t getContinuousLengthDelta(uint32_t available, double signal, uint32_t amplitudePercent)
    {
        if (available == 0u || signal == 0.0)
        {
            return 0u;
        }

        const double scaledSignal = std::clamp(std::abs(signal) * static_cast<double>(amplitudePercent) / 100.0, 0.0, 1.0);
        return std::min(available, static_cast<uint32_t>(std::floor(static_cast<double>(available) * scaledSignal + 0.5)));
    }

    uint32_t getTargetExhaustLength(const PixelShipGenerator::ShipEngineAnimationComponent& component, const EnginePulseSample& pulse, uint32_t amplitudePercent)
    {
        uint32_t length = component.ExhaustLength;

        if (pulse.Length > 0.0)
        {
            const uint32_t available = component.MaximumExhaustLength > length ? component.MaximumExhaustLength - length : 0u;
            length += getContinuousLengthDelta(available, pulse.Length, amplitudePercent);
        }
        else if (pulse.Length < 0.0)
        {
            const uint32_t available = length > component.MinimumExhaustLength ? length - component.MinimumExhaustLength : 0u;
            length -= getContinuousLengthDelta(available, pulse.Length, amplitudePercent);
        }

        return std::clamp(length, component.MinimumExhaustLength, component.MaximumExhaustLength);
    }

    bool isStaticComponentExhaustPixel(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t x, uint32_t y)
    {
        return x >= component.HousingStartX && x < component.HousingStartX + component.HousingWidth && y >= component.ExhaustStartY && y < component.ExhaustStartY + component.ExhaustLength && ship.EngineExhaustMask.get(x, y);
    }

    bool isAnimatedExhaustShapeSafe(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t length, int32_t taperBias)
    {
        if (length == 0u || component.ExhaustStartY + length > ship.EngineExhaustMask.getHeight())
        {
            return false;
        }

        const uint32_t centerXTimesTwo = component.NozzleStartX * 2u + component.NozzleWidth - 1u;

        for (uint32_t row = 0u; row < length; ++row)
        {
            const uint32_t width = getTaperedExhaustRowWidth(component.NozzleWidth, length, row, component.TaperMode, taperBias);
            const uint32_t startX = getCenteredStartX(centerXTimesTwo, width);
            const uint32_t y = component.ExhaustStartY + row;

            if (startX + width > ship.EngineExhaustMask.getWidth())
            {
                return false;
            }

            for (uint32_t x = startX; x < startX + width; ++x)
            {
                if (frame.getPixel(x, y).A != 0u && !isStaticComponentExhaustPixel(ship, component, x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    uint32_t clampExhaustLengthToSafeEnvelope(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t desiredLength, int32_t taperBias)
    {
        uint32_t length = std::min(desiredLength, component.MaximumExhaustLength);

        while (length > component.MinimumExhaustLength && !isAnimatedExhaustShapeSafe(frame, ship, component, length, taperBias))
        {
            --length;
        }

        if (!isAnimatedExhaustShapeSafe(frame, ship, component, length, taperBias))
        {
            return component.ExhaustLength;
        }

        return length;
    }

    bool isCenteredLayerPixel(uint32_t index, uint32_t outerWidth, uint32_t layerWidth)
    {
        if (layerWidth >= outerWidth)
        {
            return true;
        }

        const uint32_t start = (outerWidth - layerWidth) / 2u;
        return index >= start && index < start + layerWidth;
    }

    PixelShipGenerator::Color getAnimatedExhaustPixelColor(uint32_t index, uint32_t outerWidth, uint32_t row, uint32_t innerLength, uint32_t coreLength, int32_t intensity, const PixelShipGenerator::ShipPalette& palette)
    {
        const uint32_t innerWidth = getLayerWidth(outerWidth, 1u);
        const uint32_t coreWidth = getLayerWidth(outerWidth, outerWidth >= 5u ? 2u : 1u);
        PixelShipGenerator::Color color = palette.ExhaustBase;

        if (row < coreLength && isCenteredLayerPixel(index, outerWidth, coreWidth))
        {
            color = palette.ExhaustHotCore;
        }
        else if (row < innerLength && isCenteredLayerPixel(index, outerWidth, innerWidth))
        {
            color = palette.ExhaustHighlight;
        }

        return shiftExhaustColor(color, palette, intensity);
    }

    uint32_t getLayerLength(uint32_t outerLength, uint32_t numerator, uint32_t denominator, double pulseSignal)
    {
        uint32_t length = std::max(1u, outerLength * numerator / denominator);
        const uint32_t maximumAdjustment = std::max(1u, outerLength / 4u);
        const uint32_t adjustment = static_cast<uint32_t>(std::floor(std::clamp(std::abs(pulseSignal), 0.0, 1.0) * static_cast<double>(maximumAdjustment) + 0.5));

        if (pulseSignal > 0.0 && length < outerLength)
        {
            length = std::min(outerLength, length + adjustment);
        }
        else if (pulseSignal < 0.0 && length > 1u)
        {
            length = adjustment >= length ? 1u : std::max(1u, length - adjustment);
        }

        return length;
    }

    void clearStaticEngineExhaust(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component)
    {
        const uint32_t endX = std::min(ship.EngineExhaustMask.getWidth(), component.HousingStartX + component.HousingWidth);
        const uint32_t maximumY = std::min(ship.EngineExhaustMask.getHeight(), component.ExhaustStartY + component.ExhaustLength);

        for (uint32_t y = component.ExhaustStartY; y < maximumY; ++y)
        {
            for (uint32_t x = component.HousingStartX; x < endX; ++x)
            {
                if (ship.EngineExhaustMask.get(x, y))
                {
                    frame.setPixel(x, y, ship.Palette.Transparent);
                }
            }
        }
    }

    void redrawEngineExhaust(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, const EnginePulseSample& pulse, uint32_t amplitudePercent)
    {
        if (component.ExhaustLength == 0u || component.ExhaustStartY >= ship.EngineExhaustMask.getHeight())
        {
            return;
        }

        const uint32_t requestedLength = getTargetExhaustLength(component, pulse, amplitudePercent);
        const int32_t taperBias = quantizeSignedUnit(pulse.TaperBias);
        const uint32_t desiredLength = clampExhaustLengthToSafeEnvelope(frame, ship, component, requestedLength, taperBias);
        const uint32_t innerLength = std::min(desiredLength, getLayerLength(desiredLength, 3u, 4u, pulse.InnerLength));
        const uint32_t coreLength = std::min(innerLength, getLayerLength(desiredLength, 1u, 2u, pulse.CoreLength));
        const uint32_t centerXTimesTwo = component.NozzleStartX * 2u + component.NozzleWidth - 1u;

        clearStaticEngineExhaust(frame, ship, component);

        for (uint32_t row = 0u; row < desiredLength; ++row)
        {
            const uint32_t width = getTaperedExhaustRowWidth(component.NozzleWidth, desiredLength, row, component.TaperMode, taperBias);
            const uint32_t startX = getCenteredStartX(centerXTimesTwo, width);
            const uint32_t y = component.ExhaustStartY + row;

            for (uint32_t offset = 0u; offset < width; ++offset)
            {
                frame.setPixel(startX + offset, y, getAnimatedExhaustPixelColor(offset, width, row, innerLength, coreLength, quantizeSignedUnit(pulse.Intensity, 2u), ship.Palette));
            }
        }
    }

    uint32_t getEngineAmplitudePercent(const IdleAnimationProfile& profile, const PixelShipGenerator::ShipEngineAnimationComponent& component, std::size_t engineCount)
    {
        uint32_t amplitude = profile.ExhaustAmplitudePercent;

        if (engineCount >= 4u)
        {
            amplitude = amplitude * 2u / 3u;
        }
        else if (engineCount == 3u && !component.DominantEngine)
        {
            amplitude = amplitude * 3u / 5u;
        }

        return std::max(25u, amplitude);
    }

    void applyNozzleGlow(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, int32_t intensity)
    {
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.EngineMask.getWidth(), ship.EngineMask.getHeight() });
        if (scaleTraits.AnimationComplexity < 20u || component.NozzleY >= ship.EngineMask.getHeight())
        {
            return;
        }

        const uint32_t inset = component.NozzleWidth >= 3u ? 1u : 0u;
        const uint32_t startX = component.NozzleStartX + inset;
        const uint32_t endX = component.NozzleStartX + component.NozzleWidth - inset;
        const int32_t glowShift = intensity > 0 ? 1 : intensity < 0 ? -1 : 0;

        for (uint32_t x = startX; x < endX && x < ship.EngineMask.getWidth(); ++x)
        {
            if (ship.EngineMask.get(x, component.NozzleY))
            {
                frame.setPixel(x, component.NozzleY, shiftEngineColor(frame.getPixel(x, component.NozzleY), ship.Palette, glowShift));
            }
        }
    }

    uint32_t getMechanicalTravelPixels(double normalizedTime, bool alternatePhase, bool slowCycle, uint32_t maximumTravelPixels = 1u)
    {
        if (normalizedTime <= 0.0 || maximumTravelPixels == 0u)
        {
            return 0u;
        }

        double envelope = 0.0;
        if (slowCycle)
        {
            envelope = alternatePhase
                ? samplePulseEnvelope(normalizedTime, 0.44, 0.56, 0.70, 0.88)
                : samplePulseEnvelope(normalizedTime, 0.12, 0.26, 0.46, 0.64);
        }
        else
        {
            envelope = alternatePhase
                ? samplePulseEnvelope(normalizedTime, 0.50, 0.60, 0.72, 0.84)
                : samplePulseEnvelope(normalizedTime, 0.14, 0.24, 0.36, 0.48);
        }

        return static_cast<uint32_t>(std::floor(envelope * static_cast<double>(maximumTravelPixels) + 0.5));
    }

    void applyEngineAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan)
    {
        if (normalizedTime <= 0.0)
        {
            return;
        }

        const std::size_t engineCount = std::min(ship.IdleAnimationMetadata.EngineComponents.size(), plan.EngineParameters.size());

        for (std::size_t engineIndex = 0u; engineIndex < engineCount; ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            const EngineAnimationParameters& parameters = plan.EngineParameters[engineIndex];
            const EnginePulseSample pulse = getEnginePulseSample(normalizedTime, parameters, plan.Profile);
            redrawEngineExhaust(frame, ship, component, pulse, parameters.ExhaustAmplitudePercent);
            applyNozzleGlow(frame, ship, component, quantizeSignedUnit(pulse.Intensity, plan.Profile.EnginePulseStrength));
        }
    }

    void applyEngineMechanicalAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan)
    {
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.EngineMask.getWidth(), ship.EngineMask.getHeight() });

        if (normalizedTime <= 0.0 || scaleTraits.AnimationComplexity < 20u)
        {
            return;
        }

        const std::size_t engineCount = std::min(ship.IdleAnimationMetadata.EngineComponents.size(), plan.EngineParameters.size());
        for (std::size_t engineIndex = 0u; engineIndex < engineCount; ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            const EngineAnimationParameters& parameters = plan.EngineParameters[engineIndex];

            if (!parameters.MechanicalActive || getMechanicalTravelPixels(normalizedTime, parameters.MechanicalAlternatePhase, plan.Profile.SlowMechanicalCycle) == 0u)
            {
                continue;
            }

            const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
            if (component.NozzleStartX > component.HousingStartX)
            {
                const uint32_t x = component.NozzleStartX - 1u;
                if (ship.EngineMask.get(x, component.NozzleY)) { frame.setPixel(x, component.NozzleY, ship.Palette.EngineDark); }
            }

            const uint32_t nozzleEndX = component.NozzleStartX + component.NozzleWidth - 1u;
            const uint32_t housingEndX = component.HousingStartX + component.HousingWidth - 1u;
            if (nozzleEndX < housingEndX && nozzleEndX + 1u < ship.EngineMask.getWidth())
            {
                const uint32_t x = nozzleEndX + 1u;
                if (ship.EngineMask.get(x, component.NozzleY)) { frame.setPixel(x, component.NozzleY, ship.Palette.EngineDark); }
            }

            if (scaleTraits.AnimationComplexity >= 40u && component.Nacelle && component.NozzleY > component.RootStartY)
            {
                const uint32_t ventY = component.NozzleY - 1u;
                if (centerX < ship.EngineMask.getWidth() && ship.EngineMask.get(centerX, ventY)) { frame.setPixel(centerX, ventY, ship.Palette.EngineHighlight); }
            }
        }
    }

    void applyLightBlinking(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan)
    {
        if (normalizedTime <= 0.0)
        {
            return;
        }

        const double firstPulse = samplePulseEnvelope(normalizedTime, 0.12, 0.20, 0.32, 0.44);
        const double secondPulse = samplePulseEnvelope(normalizedTime, 0.54, 0.62, 0.74, 0.86);
        int32_t activeGroup = -1;
        if (firstPulse >= 0.5) { activeGroup = 0; }
        else if (secondPulse >= 0.5) { activeGroup = 1; }
        if (activeGroup < 0) { return; }

        for (uint32_t groupIndex = 0u; groupIndex < plan.LightGroupPixels.size(); ++groupIndex)
        {
            const PixelShipGenerator::Color color = groupIndex == static_cast<uint32_t>(activeGroup) ? ship.Palette.LightHighlight : ship.Palette.LightBase;
            for (const PixelCoordinate& pixel : plan.LightGroupPixels[groupIndex])
            {
                frame.setPixel(pixel.X, pixel.Y, color);
            }
        }
    }

    void applyMajorFeatureAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan, bool lightsEnabled, bool detailVariationEnabled)
    {
        if (normalizedTime <= 0.0)
        {
            return;
        }

        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.HullMask.getWidth(), ship.HullMask.getHeight() });
        const std::size_t componentCount = std::min(ship.IdleAnimationMetadata.MajorFeatureComponents.size(), plan.MajorFeatureParameters.size());

        for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex)
        {
            const PixelShipGenerator::ShipMajorFeatureAnimationComponent& component = ship.IdleAnimationMetadata.MajorFeatureComponents[componentIndex];
            const MajorFeatureAnimationParameters& parameters = plan.MajorFeatureParameters[componentIndex];
            if (!parameters.Active) { continue; }

            if (component.Type == PixelShipGenerator::ShipMajorFeatureType::TECH_CORE && lightsEnabled)
            {
                const double primaryPulse = parameters.AlternatePhase
                    ? samplePulseEnvelope(normalizedTime, 0.54, 0.62, 0.75, 0.88)
                    : samplePulseEnvelope(normalizedTime, 0.10, 0.20, 0.34, 0.47);
                const double secondaryPulse = parameters.AlternatePhase
                    ? samplePulseEnvelope(normalizedTime, 0.10, 0.20, 0.31, 0.43)
                    : samplePulseEnvelope(normalizedTime, 0.55, 0.64, 0.74, 0.84);
                const bool bright = primaryPulse >= 0.45;
                const bool dim = secondaryPulse >= 0.45 && plan.Profile.TechPulseStrength >= 2u;
                if (!bright && !dim) { continue; }

                for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.HullMask.getHeight(); ++y)
                {
                    for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.HullMask.getWidth(); ++x)
                    {
                        if (!ship.IdleAnimationMetadata.MajorFeatureEmissiveMask.get(x, y)) { continue; }
                        frame.setPixel(x, y, bright ? ship.Palette.LightHighlight : ship.Palette.LightBase);
                    }
                }
            }
            else if (component.Type == PixelShipGenerator::ShipMajorFeatureType::VENT_BANK && detailVariationEnabled && scaleTraits.AnimationComplexity >= 20u)
            {
                const double firstPulse = samplePulseEnvelope(normalizedTime, 0.16, 0.24, 0.34, 0.44);
                const double secondPulse = samplePulseEnvelope(normalizedTime, 0.56, 0.64, 0.74, 0.84);
                const uint32_t phase = firstPulse >= 0.5 ? 0u : secondPulse >= 0.5 ? 1u : 2u;
                if (phase >= 2u) { continue; }

                for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.HullMask.getHeight(); ++y)
                {
                    for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.HullMask.getWidth(); ++x)
                    {
                        if (!ship.IdleAnimationMetadata.MajorFeatureMechanicalMask.get(x, y)) { continue; }
                        const uint32_t canonicalX = std::min(x, ship.HullMask.getWidth() - 1u - x);
                        const bool highlighted = ((canonicalX + y + phase + parameters.PatternParity) & 1u) == 0u;
                        frame.setPixel(x, y, highlighted ? ship.Palette.MechanicalBase : ship.Palette.MechanicalDark);
                    }
                }
            }
        }
    }

    void applySmallDetailVariation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan)
    {
        if (normalizedTime <= 0.0 || !plan.DetailVariationPixel.has_value())
        {
            return;
        }

        const double highlightPulse = samplePulseEnvelope(normalizedTime, 0.22, 0.29, 0.35, 0.42);
        const double mechanicalPulse = samplePulseEnvelope(normalizedTime, 0.60, 0.67, 0.73, 0.80);
        const PixelCoordinate& selectedPixel = *plan.DetailVariationPixel;
        if (highlightPulse >= 0.5) { frame.setPixel(selectedPixel.X, selectedPixel.Y, ship.Palette.HullAccentHighlight); }
        else if (mechanicalPulse >= 0.5) { frame.setPixel(selectedPixel.X, selectedPixel.Y, ship.Palette.MechanicalBase); }
    }

    std::pair<int32_t, int32_t> getRetractionOffset(PixelShipGenerator::ShipAttachmentDirection direction)
    {
        switch (direction)
        {
        case PixelShipGenerator::ShipAttachmentDirection::UP: return { 0, 1 };
        case PixelShipGenerator::ShipAttachmentDirection::DOWN: return { 0, -1 };
        case PixelShipGenerator::ShipAttachmentDirection::LEFT: return { 1, 0 };
        case PixelShipGenerator::ShipAttachmentDirection::RIGHT: return { -1, 0 };
        default: return { 0, 0 };
        }
    }

    void markAffectedPixel(PixelShipGenerator::PixelMask& affectedMask, int32_t x, int32_t y)
    {
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                const int32_t targetX = x + offsetX;
                const int32_t targetY = y + offsetY;

                if (targetX >= 0 && targetY >= 0 && targetX < static_cast<int32_t>(affectedMask.getWidth()) && targetY < static_cast<int32_t>(affectedMask.getHeight()))
                {
                    affectedMask.set(static_cast<uint32_t>(targetX), static_cast<uint32_t>(targetY), true);
                }
            }
        }
    }

    bool hasFourConnectedMaskNeighbour(const PixelShipGenerator::PixelMask& first, const PixelShipGenerator::PixelMask& second, uint32_t x, uint32_t y)
    {
        constexpr std::array<int32_t, 4u> OffsetX = { -1, 1, 0, 0 };
        constexpr std::array<int32_t, 4u> OffsetY = { 0, 0, -1, 1 };

        for (std::size_t index = 0u; index < OffsetX.size(); ++index)
        {
            const int32_t neighbourX = static_cast<int32_t>(x) + OffsetX[index];
            const int32_t neighbourY = static_cast<int32_t>(y) + OffsetY[index];

            if (isMaskPixel(first, neighbourX, neighbourY) || isMaskPixel(second, neighbourX, neighbourY))
            {
                return true;
            }
        }

        return false;
    }

    bool moveWeaponComponent(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipWeaponAnimationComponent& component, PixelShipGenerator::PixelMask& occupiedMask, PixelShipGenerator::PixelMask& movableMask, PixelShipGenerator::PixelMask& muzzleMask, PixelShipGenerator::PixelMask& emissiveMask, PixelShipGenerator::PixelMask& affectedMask)
    {
        const auto [offsetX, offsetY] = getRetractionOffset(component.Direction);
        std::vector<PixelCoordinate> sourcePixels;
        std::vector<PixelCoordinate> destinationPixels;
        std::vector<bool> muzzlePixels;
        std::vector<bool> emissivePixels;

        for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < movableMask.getHeight(); ++y)
        {
            for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < movableMask.getWidth(); ++x)
            {
                if (!ship.IdleAnimationMetadata.WeaponMovableMask.get(x, y))
                {
                    continue;
                }

                if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y))
                {
                    return false;
                }

                const int32_t destinationX = static_cast<int32_t>(x) + offsetX;
                const int32_t destinationY = static_cast<int32_t>(y) + offsetY;

                if (destinationX < 0 || destinationY < 0 || destinationX >= static_cast<int32_t>(occupiedMask.getWidth()) || destinationY >= static_cast<int32_t>(occupiedMask.getHeight()))
                {
                    return false;
                }

                sourcePixels.push_back({ x, y });
                destinationPixels.push_back({ static_cast<uint32_t>(destinationX), static_cast<uint32_t>(destinationY) });
                muzzlePixels.push_back(ship.IdleAnimationMetadata.WeaponMuzzleMask.get(x, y));
                emissivePixels.push_back(ship.IdleAnimationMetadata.WeaponEmissiveMask.get(x, y));
            }
        }

        if (sourcePixels.size() < 2u)
        {
            return false;
        }

        PixelShipGenerator::PixelMask candidateOccupied = occupiedMask;
        PixelShipGenerator::PixelMask candidateMovable = movableMask;
        PixelShipGenerator::PixelMask candidateMuzzle = muzzleMask;
        PixelShipGenerator::PixelMask candidateEmissive = emissiveMask;

        for (const PixelCoordinate& source : sourcePixels)
        {
            candidateOccupied.set(source.X, source.Y, false);
            candidateMovable.set(source.X, source.Y, false);
            candidateMuzzle.set(source.X, source.Y, false);
            candidateEmissive.set(source.X, source.Y, false);
        }

        const PixelShipGenerator::PixelMask staticOccupiedMask = candidateOccupied;

        for (std::size_t index = 0u; index < destinationPixels.size(); ++index)
        {
            const PixelCoordinate& destination = destinationPixels[index];
            const bool staticWeaponPixel = candidateOccupied.get(destination.X, destination.Y);

            if (!staticWeaponPixel && (ship.HullMask.get(destination.X, destination.Y) || ship.CockpitMask.get(destination.X, destination.Y) || ship.EngineMask.get(destination.X, destination.Y) || ship.EngineExhaustMask.get(destination.X, destination.Y) || ship.AttachmentMask.get(destination.X, destination.Y)))
            {
                return false;
            }

            if (!staticWeaponPixel)
            {
                candidateOccupied.set(destination.X, destination.Y, true);
                candidateMovable.set(destination.X, destination.Y, true);
                if (muzzlePixels[index]) { candidateMuzzle.set(destination.X, destination.Y, true); }
                if (emissivePixels[index]) { candidateEmissive.set(destination.X, destination.Y, true); }
            }
        }

        bool connected = false;

        for (const PixelCoordinate& destination : destinationPixels)
        {
            if (staticOccupiedMask.get(destination.X, destination.Y) || hasFourConnectedMaskNeighbour(staticOccupiedMask, staticOccupiedMask, destination.X, destination.Y))
            {
                connected = true;
                break;
            }
        }

        if (!connected)
        {
            return false;
        }

        for (const PixelCoordinate& source : sourcePixels) { markAffectedPixel(affectedMask, static_cast<int32_t>(source.X), static_cast<int32_t>(source.Y)); }
        for (const PixelCoordinate& destination : destinationPixels) { markAffectedPixel(affectedMask, static_cast<int32_t>(destination.X), static_cast<int32_t>(destination.Y)); }

        occupiedMask = std::move(candidateOccupied);
        movableMask = std::move(candidateMovable);
        muzzleMask = std::move(candidateMuzzle);
        emissiveMask = std::move(candidateEmissive);
        return true;
    }

    void applyWeaponAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan, bool mechanicalEnabled, bool lightsEnabled)
    {
        if (ship.IdleAnimationMetadata.WeaponComponents.empty() || normalizedTime <= 0.0)
        {
            return;
        }

        PixelShipGenerator::PixelMask occupiedMask = ship.IdleAnimationMetadata.WeaponOccupiedMask;
        PixelShipGenerator::PixelMask movableMask = ship.IdleAnimationMetadata.WeaponMovableMask;
        PixelShipGenerator::PixelMask muzzleMask = ship.IdleAnimationMetadata.WeaponMuzzleMask;
        PixelShipGenerator::PixelMask emissiveMask = ship.IdleAnimationMetadata.WeaponEmissiveMask;
        PixelShipGenerator::PixelMask affectedMask(occupiedMask.getWidth(), occupiedMask.getHeight(), false);
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.HullMask.getWidth(), ship.HullMask.getHeight() });
        const std::size_t componentCount = std::min(ship.IdleAnimationMetadata.WeaponComponents.size(), plan.WeaponParameters.size());

        if (mechanicalEnabled && scaleTraits.AnimationComplexity >= 20u)
        {
            for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex)
            {
                const PixelShipGenerator::ShipWeaponAnimationComponent& component = ship.IdleAnimationMetadata.WeaponComponents[componentIndex];
                const WeaponAnimationParameters& parameters = plan.WeaponParameters[componentIndex];
                if (!parameters.MechanicalActive || getMechanicalTravelPixels(normalizedTime, parameters.AlternatePhase, plan.Profile.SlowMechanicalCycle) == 0u)
                {
                    continue;
                }

                moveWeaponComponent(ship, component, occupiedMask, movableMask, muzzleMask, emissiveMask, affectedMask);
            }
        }

        if (PixelShipGenerator::PixelMaskUtils::getMaskPixelCount(affectedMask) > 0u)
        {
            PixelShipGenerator::ShipPainter painter;
            painter.paintIdleWeaponLayer(frame, ship, occupiedMask, movableMask, muzzleMask, emissiveMask, affectedMask);
        }

        if (!lightsEnabled)
        {
            return;
        }

        const double brightPulse = samplePulseEnvelope(normalizedTime, 0.10, 0.18, 0.34, 0.46);
        const double dimPulse = samplePulseEnvelope(normalizedTime, 0.54, 0.62, 0.74, 0.84);
        const bool bright = brightPulse >= 0.5;
        const bool dim = dimPulse >= 0.5;
        if (!bright && !dim) { return; }

        for (uint32_t y = 0u; y < emissiveMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < emissiveMask.getWidth(); ++x)
            {
                if (!emissiveMask.get(x, y)) { continue; }
                frame.setPixel(x, y, bright ? ship.Palette.LightHighlight : ship.Palette.LightBase);
            }
        }
    }

    uint32_t getAttachmentOutwardDistance(const PixelShipGenerator::ShipAttachmentPlacement& placement, uint32_t x, uint32_t y)
    {
        switch (placement.Direction)
        {
        case PixelShipGenerator::ShipAttachmentDirection::LEFT: return placement.AnchorX >= x ? placement.AnchorX - x : 0u;
        case PixelShipGenerator::ShipAttachmentDirection::RIGHT: return x >= placement.AnchorX ? x - placement.AnchorX : 0u;
        case PixelShipGenerator::ShipAttachmentDirection::UP: return placement.AnchorY >= y ? placement.AnchorY - y : 0u;
        case PixelShipGenerator::ShipAttachmentDirection::DOWN: return y >= placement.AnchorY ? y - placement.AnchorY : 0u;
        default: return 0u;
        }
    }

    uint32_t getAttachmentMaximumOutwardDistance(const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        switch (placement.Direction)
        {
        case PixelShipGenerator::ShipAttachmentDirection::LEFT: return placement.AnchorX - placement.MinimumX;
        case PixelShipGenerator::ShipAttachmentDirection::RIGHT: return placement.MaximumX - placement.AnchorX;
        case PixelShipGenerator::ShipAttachmentDirection::UP: return placement.AnchorY - placement.MinimumY;
        case PixelShipGenerator::ShipAttachmentDirection::DOWN: return placement.MaximumY - placement.AnchorY;
        default: return 0u;
        }
    }

    std::pair<int32_t, int32_t> getAttachmentTangentOffset(PixelShipGenerator::ShipAttachmentDirection direction)
    {
        return direction == PixelShipGenerator::ShipAttachmentDirection::LEFT || direction == PixelShipGenerator::ShipAttachmentDirection::RIGHT ? std::pair<int32_t, int32_t>{ 0, 1 } : std::pair<int32_t, int32_t>{ 1, 0 };
    }

    uint32_t countAttachmentPixels(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        uint32_t count = 0u;

        for (uint32_t y = placement.MinimumY; y <= placement.MaximumY; ++y)
        {
            for (uint32_t x = placement.MinimumX; x <= placement.MaximumX; ++x)
            {
                if (ship.AttachmentMask.get(x, y))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    bool isAnimatedAttachmentPixel(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& origins, const std::vector<PixelCoordinate>& destinations, int32_t x, int32_t y)
    {
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(ship.AttachmentMask.getWidth()) || y >= static_cast<int32_t>(ship.AttachmentMask.getHeight()))
        {
            return false;
        }

        const uint32_t pixelX = static_cast<uint32_t>(x);
        const uint32_t pixelY = static_cast<uint32_t>(y);

        if (containsCoordinate(destinations, pixelX, pixelY))
        {
            return true;
        }

        if (containsCoordinate(origins, pixelX, pixelY))
        {
            return false;
        }

        return ship.AttachmentMask.get(pixelX, pixelY);
    }

    bool hasNeighbouringAnimatedStructurePixel(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& origins, const std::vector<PixelCoordinate>& destinations, int32_t x, int32_t y)
    {
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                if (isMaskPixel(ship.HullMask, x + offsetX, y + offsetY) || isMaskPixel(ship.IdleAnimationMetadata.WeaponOccupiedMask, x + offsetX, y + offsetY) || isAnimatedAttachmentPixel(ship, origins, destinations, x + offsetX, y + offsetY))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool hasFourConnectedAnimatedAttachmentNeighbour(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& origins, const std::vector<PixelCoordinate>& destinations, const PixelCoordinate& pixel)
    {
        constexpr std::array<int32_t, 4u> OffsetX = { -1, 1, 0, 0 };
        constexpr std::array<int32_t, 4u> OffsetY = { 0, 0, -1, 1 };

        for (std::size_t index = 0u; index < OffsetX.size(); ++index)
        {
            const int32_t x = static_cast<int32_t>(pixel.X) + OffsetX[index];
            const int32_t y = static_cast<int32_t>(pixel.Y) + OffsetY[index];

            if (isMaskPixel(ship.HullMask, x, y) || isMaskPixel(ship.IdleAnimationMetadata.WeaponOccupiedMask, x, y) || isAnimatedAttachmentPixel(ship, origins, destinations, x, y))
            {
                return true;
            }
        }

        return false;
    }

    void applyMechanicalMicroMovement(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime, const IdleAnimationPlan& plan)
    {
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.AttachmentMask.getWidth(), ship.AttachmentMask.getHeight() });
        if (normalizedTime <= 0.0 || scaleTraits.AnimationComplexity < 20u || !plan.MicroMovementPlacementIndex.has_value() || *plan.MicroMovementPlacementIndex >= ship.AttachmentPlacements.size())
        {
            return;
        }

        const double firstPulse = samplePulseEnvelope(normalizedTime, 0.14, 0.23, 0.35, 0.46);
        const double secondPulse = samplePulseEnvelope(normalizedTime, 0.54, 0.63, 0.75, 0.86);
        const int32_t signedPulse = quantizeSignedUnit(firstPulse - secondPulse);
        const int32_t movement = signedPulse * plan.PreferredMicroMovementDirection;
        if (movement == 0) { return; }

        const PixelShipGenerator::ShipAttachmentPlacement& selectedPlacement = ship.AttachmentPlacements[*plan.MicroMovementPlacementIndex];
        const uint32_t maximumOutwardDistance = getAttachmentMaximumOutwardDistance(selectedPlacement);
        const auto [tangentX, tangentY] = getAttachmentTangentOffset(selectedPlacement.Direction);
        std::vector<PixelCoordinate> origins;
        std::vector<PixelCoordinate> destinations;
        std::vector<PixelShipGenerator::Color> colors;

        for (uint32_t y = selectedPlacement.MinimumY; y <= selectedPlacement.MaximumY; ++y)
        {
            for (uint32_t x = selectedPlacement.MinimumX; x <= selectedPlacement.MaximumX; ++x)
            {
                if (!ship.AttachmentMask.get(x, y) || getAttachmentOutwardDistance(selectedPlacement, x, y) != maximumOutwardDistance)
                {
                    continue;
                }

                const int32_t destinationX = static_cast<int32_t>(x) + tangentX * movement;
                const int32_t destinationY = static_cast<int32_t>(y) + tangentY * movement;
                if (destinationX < 0 || destinationY < 0 || destinationX >= static_cast<int32_t>(ship.AttachmentMask.getWidth()) || destinationY >= static_cast<int32_t>(ship.AttachmentMask.getHeight()))
                {
                    return;
                }

                origins.push_back({ x, y });
                destinations.push_back({ static_cast<uint32_t>(destinationX), static_cast<uint32_t>(destinationY) });
                colors.push_back(frame.getPixel(x, y));
            }
        }

        if (origins.empty()) { return; }

        for (const PixelCoordinate& destination : destinations)
        {
            if (isBaseStructurePixel(ship, static_cast<int32_t>(destination.X), static_cast<int32_t>(destination.Y))) { return; }
            if (ship.AttachmentMask.get(destination.X, destination.Y) && !containsCoordinate(origins, destination.X, destination.Y)) { return; }
        }

        bool connected = false;
        for (const PixelCoordinate& destination : destinations)
        {
            if (hasFourConnectedAnimatedAttachmentNeighbour(ship, origins, destinations, destination))
            {
                connected = true;
                break;
            }
        }
        if (!connected) { return; }

        std::vector<PixelCoordinate> affectedPixels;
        for (const PixelCoordinate& pixel : origins)
        {
            for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
            {
                for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const int32_t x = static_cast<int32_t>(pixel.X) + offsetX;
                    const int32_t y = static_cast<int32_t>(pixel.Y) + offsetY;
                    if (x >= 0 && y >= 0 && x < static_cast<int32_t>(ship.AttachmentMask.getWidth()) && y < static_cast<int32_t>(ship.AttachmentMask.getHeight()) && !containsCoordinate(affectedPixels, static_cast<uint32_t>(x), static_cast<uint32_t>(y))) { affectedPixels.push_back({ static_cast<uint32_t>(x), static_cast<uint32_t>(y) }); }
                }
            }
        }

        for (const PixelCoordinate& pixel : destinations)
        {
            for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
            {
                for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const int32_t x = static_cast<int32_t>(pixel.X) + offsetX;
                    const int32_t y = static_cast<int32_t>(pixel.Y) + offsetY;
                    if (x >= 0 && y >= 0 && x < static_cast<int32_t>(ship.AttachmentMask.getWidth()) && y < static_cast<int32_t>(ship.AttachmentMask.getHeight()) && !containsCoordinate(affectedPixels, static_cast<uint32_t>(x), static_cast<uint32_t>(y))) { affectedPixels.push_back({ static_cast<uint32_t>(x), static_cast<uint32_t>(y) }); }
                }
            }
        }

        for (const PixelCoordinate& pixel : affectedPixels)
        {
            if (isBaseStructurePixel(ship, static_cast<int32_t>(pixel.X), static_cast<int32_t>(pixel.Y))) { continue; }
            if (isAnimatedAttachmentPixel(ship, origins, destinations, static_cast<int32_t>(pixel.X), static_cast<int32_t>(pixel.Y))) { continue; }
            frame.setPixel(pixel.X, pixel.Y, hasNeighbouringAnimatedStructurePixel(ship, origins, destinations, static_cast<int32_t>(pixel.X), static_cast<int32_t>(pixel.Y)) ? ship.Palette.Outline : ship.Palette.Transparent);
        }

        for (std::size_t index = 0u; index < destinations.size(); ++index)
        {
            frame.setPixel(destinations[index].X, destinations[index].Y, colors[index]);
        }
    }

    OpaqueBounds calculateOpaqueBounds(const PixelShipGenerator::Image& image, uint32_t width, uint32_t height)
    {
        OpaqueBounds bounds;
        bounds.MinX = width;
        bounds.MinY = height;

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (image.getPixel(x, y).A == 0u)
                {
                    continue;
                }

                bounds.MinX = std::min(bounds.MinX, x);
                bounds.MaxX = std::max(bounds.MaxX, x);
                bounds.MinY = std::min(bounds.MinY, y);
                bounds.MaxY = std::max(bounds.MaxY, y);
                bounds.Valid = true;
            }
        }

        return bounds;
    }

    int32_t getHoverOffset(double normalizedTime)
    {
        constexpr std::array<CurvePoint, 7u> HoverCurve =
        { {
            { 0.00, 0.00 }, { 0.16, -1.00 }, { 0.32, -1.00 }, { 0.50, 0.00 },
            { 0.66, 1.00 }, { 0.82, 1.00 }, { 1.00, 0.00 }
        } };
        if (normalizedTime <= 0.0) { return 0; }
        return quantizeSignedUnit(sampleCurve(std::clamp(normalizedTime, 0.0, 1.0), HoverCurve));
    }

    PixelShipGenerator::Image translateImageVertically(const PixelShipGenerator::Image& source, uint32_t width, uint32_t height, int32_t offsetY, const PixelShipGenerator::Color& transparent)
    {
        if (offsetY == 0)
        {
            return source;
        }

        PixelShipGenerator::Image translated;
        translated.reset(width, height);
        translated.clear(transparent);

        for (uint32_t y = 0u; y < height; ++y)
        {
            const int32_t destinationY = static_cast<int32_t>(y) + offsetY;

            if (destinationY < 0 || destinationY >= static_cast<int32_t>(height))
            {
                continue;
            }

            for (uint32_t x = 0u; x < width; ++x)
            {
                translated.setPixel(x, static_cast<uint32_t>(destinationY), source.getPixel(x, y));
            }
        }

        return translated;
    }

    void applyHoverOffset(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, double normalizedTime)
    {
        const int32_t desiredOffset = getHoverOffset(normalizedTime);

        if (desiredOffset == 0)
        {
            return;
        }

        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();
        const OpaqueBounds bounds = calculateOpaqueBounds(frame, width, height);

        if (!bounds.Valid)
        {
            return;
        }

        if (desiredOffset < 0 && bounds.MinY == 0u)
        {
            return;
        }

        if (desiredOffset > 0 && bounds.MaxY + 1u >= height)
        {
            return;
        }

        frame = translateImageVertically(frame, width, height, desiredOffset, ship.Palette.Transparent);
    }

    uint32_t getPlannedMaximumExhaustTravel(const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t amplitudePercent)
    {
        const uint32_t extension = component.MaximumExhaustLength > component.ExhaustLength ? component.MaximumExhaustLength - component.ExhaustLength : 0u;
        const uint32_t contraction = component.ExhaustLength > component.MinimumExhaustLength ? component.ExhaustLength - component.MinimumExhaustLength : 0u;
        return std::max(getContinuousLengthDelta(extension, 1.0, amplitudePercent), getContinuousLengthDelta(contraction, 1.0, amplitudePercent));
    }

    IdleAnimationPlan createIdleAnimationPlan(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipIdleAnimationSettings& settings, uint64_t seed)
    {
        IdleAnimationPlan plan;
        plan.Profile = getIdleAnimationProfile(ship);
        plan.PreferredMicroMovementDirection = (getAnimationHash(seed, 0u, 0u, MicroMovementSalt) & 1ull) == 0ull ? -1 : 1;
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.HullMask.getWidth(), ship.HullMask.getHeight() });

        if (settings.LightBlinking)
        {
            const uint32_t width = ship.LightMask.getWidth();
            for (uint32_t y = 0u; y < ship.LightMask.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (!ship.LightMask.get(x, y)) { continue; }
                    const uint32_t canonicalX = std::min(x, width - 1u - x);
                    const uint32_t group = static_cast<uint32_t>(getAnimationHash(seed, canonicalX, y, LightVariationSalt) & 1ull);
                    plan.LightGroupPixels[group].push_back({ x, y });
                }
            }
        }

        plan.EngineParameters.reserve(ship.IdleAnimationMetadata.EngineComponents.size());
        std::vector<uint32_t> enginePhaseKeys;
        bool hasEngineMechanicalNormalPhase = false;
        bool hasEngineMechanicalAlternatePhase = false;

        for (std::size_t engineIndex = 0u; engineIndex < ship.IdleAnimationMetadata.EngineComponents.size(); ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
            const uint64_t variationHash = getAnimationHash(seed, centerX, component.NozzleY, EngineVariationSalt ^ static_cast<uint64_t>(engineIndex));
            const uint64_t mechanicalHash = getAnimationHash(seed, centerX, component.NozzleY, EngineMechanicalSalt);
            EngineAnimationParameters parameters;
            parameters.ExhaustAmplitudePercent = getEngineAmplitudePercent(plan.Profile, component, ship.IdleAnimationMetadata.EngineComponents.size());

            if (!plan.Profile.SynchronizeEngines)
            {
                parameters.ReverseTime = plan.Profile.AlternateEnginePhases ? (engineIndex & 1u) != 0u : plan.Profile.AsynchronousEngines && ((variationHash >> 12u) & 1ull) != 0ull;
                if (plan.Profile.IrregularEngineCycle || plan.Profile.AsynchronousEngines) { parameters.CurveVariant = static_cast<uint32_t>((variationHash >> 20u) % 3ull); }
            }

            parameters.MechanicalAlternatePhase = plan.Profile.AlternateEnginePhases && (mechanicalHash & 1ull) != 0ull;
            parameters.MechanicalActive = settings.MechanicalMicroMovement && scaleTraits.AnimationComplexity >= 20u && component.NozzleWidth >= 3u && component.HousingWidth >= 3u && component.NozzleY < ship.EngineMask.getHeight() && mechanicalHash % 100u < plan.Profile.EngineMechanicalChance;
            plan.EngineParameters.push_back(parameters);

            if (settings.EngineFlicker)
            {
                const uint32_t phaseKey = parameters.CurveVariant * 2u + (parameters.ReverseTime ? 1u : 0u);
                if (std::find(enginePhaseKeys.begin(), enginePhaseKeys.end(), phaseKey) == enginePhaseKeys.end()) { enginePhaseKeys.push_back(phaseKey); }
            }
            if (parameters.MechanicalActive)
            {
                hasEngineMechanicalNormalPhase = hasEngineMechanicalNormalPhase || !parameters.MechanicalAlternatePhase;
                hasEngineMechanicalAlternatePhase = hasEngineMechanicalAlternatePhase || parameters.MechanicalAlternatePhase;
            }
        }

        plan.WeaponParameters.reserve(ship.IdleAnimationMetadata.WeaponComponents.size());
        bool hasWeaponMechanicalNormalPhase = false;
        bool hasWeaponMechanicalAlternatePhase = false;
        for (const PixelShipGenerator::ShipWeaponAnimationComponent& component : ship.IdleAnimationMetadata.WeaponComponents)
        {
            const uint64_t hash = getAnimationHash(seed, component.AnchorX, component.AnchorY, WeaponMovementSalt);
            WeaponAnimationParameters parameters;
            if (component.MovableBarrel && settings.MechanicalMicroMovement && scaleTraits.AnimationComplexity >= 20u && hash % 100u < plan.Profile.WeaponMechanicalChance)
            {
                parameters.MechanicalActive = true;
                if (plan.Profile.AlternateWeaponPhases && component.SymmetryGroup != 0u)
                {
                    parameters.AlternatePhase = component.AnchorX > ship.IdleAnimationMetadata.WeaponOccupiedMask.getWidth() / 2u;
                }
                else if (ship.Faction == PixelShipGenerator::ShipFactionType::FRONTIER && component.SymmetryGroup != 0u && ((hash >> 8u) & 1ull) != 0ull)
                {
                    parameters.AlternatePhase = component.AnchorX > ship.IdleAnimationMetadata.WeaponOccupiedMask.getWidth() / 2u;
                }
                hasWeaponMechanicalNormalPhase = hasWeaponMechanicalNormalPhase || !parameters.AlternatePhase;
                hasWeaponMechanicalAlternatePhase = hasWeaponMechanicalAlternatePhase || parameters.AlternatePhase;
            }
            plan.WeaponParameters.push_back(parameters);
        }

        plan.MajorFeatureParameters.reserve(ship.IdleAnimationMetadata.MajorFeatureComponents.size());
        uint32_t activeMajorFeatureCount = 0u;
        bool hasActiveVentBank = false;
        for (const PixelShipGenerator::ShipMajorFeatureAnimationComponent& component : ship.IdleAnimationMetadata.MajorFeatureComponents)
        {
            const uint64_t hash = getAnimationHash(seed, component.MinimumX, component.MinimumY, MajorFeatureSalt ^ (component.Type == PixelShipGenerator::ShipMajorFeatureType::VENT_BANK ? DetailVariationSalt : 0ull));
            MajorFeatureAnimationParameters parameters;
            if (component.Type == PixelShipGenerator::ShipMajorFeatureType::TECH_CORE && settings.LightBlinking)
            {
                parameters.Active = true;
                parameters.AlternatePhase = ship.Faction == PixelShipGenerator::ShipFactionType::XENO && (hash & 1ull) != 0ull;
            }
            else if (component.Type == PixelShipGenerator::ShipMajorFeatureType::VENT_BANK && settings.SmallDetailVariation && scaleTraits.AnimationComplexity >= 20u && hash % 100u < plan.Profile.VentActivityChance)
            {
                parameters.Active = true;
                parameters.PatternParity = static_cast<uint32_t>(hash & 1ull);
                hasActiveVentBank = true;
            }
            if (parameters.Active) { ++activeMajorFeatureCount; }
            plan.MajorFeatureParameters.push_back(parameters);
        }

        if (settings.SmallDetailVariation)
        {
            uint64_t bestHash = std::numeric_limits<uint64_t>::max();
            for (uint32_t y = 0u; y < ship.MechanicalDetailMask.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < ship.MechanicalDetailMask.getWidth(); ++x)
                {
                    if (!ship.MechanicalDetailMask.get(x, y) || getMaskNeighbourCount(ship.MechanicalDetailMask, static_cast<int32_t>(x), static_cast<int32_t>(y)) > 2u) { continue; }
                    const uint64_t hash = getAnimationHash(seed, x, y, DetailVariationSalt);
                    if (hash < bestHash)
                    {
                        bestHash = hash;
                        plan.DetailVariationPixel = PixelCoordinate{ x, y };
                    }
                }
            }
        }

        if (settings.MechanicalMicroMovement && scaleTraits.AnimationComplexity >= 20u)
        {
            uint64_t selectedHash = std::numeric_limits<uint64_t>::max();
            const uint32_t maximumPixelCount = scaleTraits.AnimationComplexity >= 80u ? 36u : 20u;
            for (std::size_t placementIndex = 0u; placementIndex < ship.AttachmentPlacements.size(); ++placementIndex)
            {
                const PixelShipGenerator::ShipAttachmentPlacement& placement = ship.AttachmentPlacements[placementIndex];
                if (placement.Type != PixelShipGenerator::ShipAttachmentType::SENSOR_ARRAY && placement.Type != PixelShipGenerator::ShipAttachmentType::TECHNOLOGY_NODE) { continue; }
                if (getAttachmentMaximumOutwardDistance(placement) < 2u || countAttachmentPixels(ship, placement) > maximumPixelCount) { continue; }
                const uint64_t hash = getAnimationHash(seed, placement.AnchorX, placement.AnchorY, MicroMovementSalt);
                if (hash < selectedHash)
                {
                    selectedHash = hash;
                    plan.MicroMovementPlacementIndex = placementIndex;
                }
            }
        }

        PixelShipGenerator::AnimationSamplingRequirements& requirements = plan.SamplingRequirements;
        requirements.Type = PixelShipGenerator::ShipAnimationType::IDLE;
        requirements.Mode = settings.SamplingMode;
        requirements.DurationMilliseconds = std::max(1u, settings.AnimationDurationMilliseconds);
        requirements.ExactFrameCount = std::max(1u, settings.FrameCount);
        requirements.MinimumFrameCount = std::max(1u, settings.MinimumFrameCount);
        requirements.MaximumFrameCount = std::max(requirements.MinimumFrameCount, settings.MaximumFrameCount);
        requirements.ScaleAnimationComplexity = scaleTraits.AnimationComplexity;

        if (settings.EngineFlicker)
        {
            requirements.ActiveAnimatedComponentCount += static_cast<uint32_t>(ship.IdleAnimationMetadata.EngineComponents.size());
            requirements.IndependentPhaseGroupCount += static_cast<uint32_t>(enginePhaseKeys.size());
            for (std::size_t engineIndex = 0u; engineIndex < ship.IdleAnimationMetadata.EngineComponents.size(); ++engineIndex)
            {
                requirements.MaximumExhaustTravelPixels = std::max(requirements.MaximumExhaustTravelPixels, getPlannedMaximumExhaustTravel(ship.IdleAnimationMetadata.EngineComponents[engineIndex], plan.EngineParameters[engineIndex].ExhaustAmplitudePercent));
            }
        }

        for (const EngineAnimationParameters& parameters : plan.EngineParameters)
        {
            if (parameters.MechanicalActive) { ++requirements.ActiveAnimatedComponentCount; requirements.MaximumMechanicalTravelPixels = 1u; }
        }
        if (hasEngineMechanicalNormalPhase) { ++requirements.IndependentPhaseGroupCount; }
        if (hasEngineMechanicalAlternatePhase) { ++requirements.IndependentPhaseGroupCount; }

        for (std::size_t weaponIndex = 0u; weaponIndex < plan.WeaponParameters.size(); ++weaponIndex)
        {
            const WeaponAnimationParameters& parameters = plan.WeaponParameters[weaponIndex];
            if (parameters.MechanicalActive) { ++requirements.ActiveAnimatedComponentCount; requirements.MaximumMechanicalTravelPixels = 1u; }
            if (settings.LightBlinking && ship.IdleAnimationMetadata.WeaponComponents[weaponIndex].Emissive) { ++requirements.ActiveAnimatedComponentCount; }
        }
        if (hasWeaponMechanicalNormalPhase) { ++requirements.IndependentPhaseGroupCount; }
        if (hasWeaponMechanicalAlternatePhase) { ++requirements.IndependentPhaseGroupCount; }
        if (settings.LightBlinking && PixelShipGenerator::PixelMaskUtils::getMaskPixelCount(ship.IdleAnimationMetadata.WeaponEmissiveMask) > 0u) { ++requirements.IndependentPhaseGroupCount; }

        uint32_t activeLightGroupCount = 0u;
        for (const std::vector<PixelCoordinate>& group : plan.LightGroupPixels)
        {
            if (!group.empty()) { ++activeLightGroupCount; }
        }
        if (activeLightGroupCount > 0u)
        {
            ++requirements.ActiveAnimatedComponentCount;
            requirements.IndependentPhaseGroupCount += activeLightGroupCount;
            requirements.MaximumTemporalCyclesPerClip = std::max(requirements.MaximumTemporalCyclesPerClip, 2u);
        }

        requirements.ActiveAnimatedComponentCount += activeMajorFeatureCount;
        requirements.IndependentPhaseGroupCount += activeMajorFeatureCount;
        if (hasActiveVentBank) { requirements.MaximumTemporalCyclesPerClip = std::max(requirements.MaximumTemporalCyclesPerClip, 2u); }

        if (plan.DetailVariationPixel.has_value())
        {
            ++requirements.ActiveAnimatedComponentCount;
            ++requirements.IndependentPhaseGroupCount;
            requirements.MaximumTemporalCyclesPerClip = std::max(requirements.MaximumTemporalCyclesPerClip, 2u);
        }
        if (plan.MicroMovementPlacementIndex.has_value())
        {
            ++requirements.ActiveAnimatedComponentCount;
            ++requirements.IndependentPhaseGroupCount;
            requirements.MaximumMechanicalTravelPixels = 1u;
            requirements.MaximumTemporalCyclesPerClip = std::max(requirements.MaximumTemporalCyclesPerClip, 2u);
        }

        if (settings.HoverOffset)
        {
            const OpaqueBounds bounds = calculateOpaqueBounds(ship.FinalImage, ship.HullMask.getWidth(), ship.HullMask.getHeight());
            if (bounds.Valid && (bounds.MinY > 0u || bounds.MaxY + 1u < ship.HullMask.getHeight()))
            {
                ++requirements.ActiveAnimatedComponentCount;
                ++requirements.IndependentPhaseGroupCount;
                requirements.MaximumMechanicalTravelPixels = 1u;
            }
        }

        if (requirements.ActiveAnimatedComponentCount > 0u && requirements.MaximumTemporalCyclesPerClip == 0u) { requirements.MaximumTemporalCyclesPerClip = 1u; }
        return plan;
    }

    PixelShipGenerator::Image evaluateIdleFrame(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipIdleAnimationSettings& settings, double normalizedTime, const IdleAnimationPlan& plan)
    {
        const double time = wrapNormalizedTime(normalizedTime);
        PixelShipGenerator::Image frame = ship.FinalImage;
        if (time <= 0.0) { return frame; }

        if (settings.MechanicalMicroMovement) { applyEngineMechanicalAnimation(frame, ship, time, plan); }
        if (settings.LightBlinking) { applyLightBlinking(frame, ship, time, plan); }
        applyMajorFeatureAnimation(frame, ship, time, plan, settings.LightBlinking, settings.SmallDetailVariation);
        if (settings.SmallDetailVariation) { applySmallDetailVariation(frame, ship, time, plan); }
        applyWeaponAnimation(frame, ship, time, plan, settings.MechanicalMicroMovement, settings.LightBlinking);
        if (settings.MechanicalMicroMovement) { applyMechanicalMicroMovement(frame, ship, time, plan); }
        if (settings.EngineFlicker) { applyEngineAnimation(frame, ship, time, plan); }
        if (settings.HoverOffset) { applyHoverOffset(frame, ship, time); }
        return frame;
    }
}

namespace PixelShipGenerator
{
    ShipIdleAnimation ShipIdleAnimator::generate(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings) const
    {
        ShipIdleAnimation animation;
        animation.Type = ShipAnimationType::IDLE;
        animation.FrameWidth = ship.HullMask.getWidth();
        animation.FrameHeight = ship.HullMask.getHeight();
        animation.Seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ AnimationSeedSalt);

        const IdleAnimationPlan idlePlan = createIdleAnimationPlan(ship, settings, animation.Seed);
        AnimationSamplingPlanner samplingPlanner;
        animation.Sampling = samplingPlanner.plan(idlePlan.SamplingRequirements);
        animation.DurationMilliseconds = animation.Sampling.DurationMilliseconds;
        animation.FrameDurationMilliseconds = animation.Sampling.ActualFrameDurationMilliseconds;
        animation.Frames.reserve(animation.Sampling.ActualFrameCount);
        animation.NormalizedSampleTimes.reserve(animation.Sampling.ActualFrameCount);

        for (uint32_t frameIndex = 0u; frameIndex < animation.Sampling.ActualFrameCount; ++frameIndex)
        {
            const double normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(animation.Sampling.ActualFrameCount);
            animation.NormalizedSampleTimes.push_back(normalizedTime);
            animation.Frames.push_back(evaluateIdleFrame(ship, settings, normalizedTime, idlePlan));
        }

        return animation;
    }

    Image ShipIdleAnimator::evaluateFrameAtNormalizedTime(const GeneratedShip& ship, double normalizedTime, const ShipIdleAnimationSettings& settings) const
    {
        const uint64_t seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ AnimationSeedSalt);
        const IdleAnimationPlan idlePlan = createIdleAnimationPlan(ship, settings, seed);
        return evaluateIdleFrame(ship, settings, normalizedTime, idlePlan);
    }
}
