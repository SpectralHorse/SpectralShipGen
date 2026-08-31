#include "ShipIdleAnimationInternal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "ShipPainter.h"
#include <PixelShipGenerator/GenerationScaleTraits.h>
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
namespace IdleAnimationInternal
{
    struct EnginePulseSample
    {
        double Length = 0.0;
        double Intensity = 0.0;
        double TaperBias = 0.0;
        double InnerLength = 0.0;
        double CoreLength = 0.0;
    };

    struct CurvePoint
    {
        double Time = 0.0;
        double Value = 0.0;
    };

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

    bool containsCoordinate(const std::vector<PixelCoordinate>& pixels, uint32_t x, uint32_t y)
    {
        return std::any_of(pixels.begin(), pixels.end(), [x, y](const PixelCoordinate& pixel) { return pixel.X == x && pixel.Y == y; });
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

    std::pair<int32_t, int32_t> getAttachmentTangentOffset(PixelShipGenerator::ShipAttachmentDirection direction)
    {
        return direction == PixelShipGenerator::ShipAttachmentDirection::LEFT || direction == PixelShipGenerator::ShipAttachmentDirection::RIGHT ? std::pair<int32_t, int32_t>{ 0, 1 } : std::pair<int32_t, int32_t>{ 1, 0 };
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
}
