#include "ShipIdleAnimator.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

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

    struct EnginePulseState
    {
        int32_t Length = 0;
        int32_t Intensity = 0;
        int32_t TaperBias = 0;
        int32_t InnerLength = 0;
        int32_t CoreLength = 0;
    };

    uint64_t getAnimationHash(uint64_t seed, uint32_t x, uint32_t y, uint64_t salt)
    {
        uint64_t value = seed;
        value ^= static_cast<uint64_t>(x) * 0x9E3779B185EBCA87ull;
        value ^= static_cast<uint64_t>(y) * 0xC2B2AE3D27D4EB4Full;
        value ^= salt;
        return PixelShipGenerator::mixGenerationSeed64(value);
    }

    uint32_t getLoopStep(uint32_t frameIndex, uint32_t frameCount)
    {
        if (frameCount <= 1u)
        {
            return 0u;
        }

        return std::min(8u, static_cast<uint32_t>((static_cast<uint64_t>(frameIndex) * 8u) / (frameCount - 1u)));
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

    EnginePulseState getEnginePulseState(uint32_t loopStep, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t engineIndex, uint64_t seed, const IdleAnimationProfile& profile)
    {
        constexpr std::array<EnginePulseState, 9u> Standard =
        { {
            { 0, 0, 0, 0, 0 },
            { 1, 1, 0, 1, 1 },
            { 2, 2, 1, 1, 2 },
            { 1, 1, 0, 2, 1 },
            { -1, -1, -1, 0, -1 },
            { -2, -1, 0, -1, -1 },
            { -1, -2, 1, -1, 0 },
            { 1, -1, 0, 1, 1 },
            { 0, 0, 0, 0, 0 }
        } };
        constexpr std::array<EnginePulseState, 9u> IrregularA =
        { {
            { 0, 0, 0, 0, 0 },
            { 1, 1, 0, 1, 0 },
            { 1, 2, 1, 2, 1 },
            { 2, 1, 1, 1, 2 },
            { -1, 0, -1, 0, -1 },
            { -1, -1, 0, -1, 0 },
            { -2, -2, 1, -1, -1 },
            { -1, -1, 0, 0, 1 },
            { 0, 0, 0, 0, 0 }
        } };
        constexpr std::array<EnginePulseState, 9u> IrregularB =
        { {
            { 0, 0, 0, 0, 0 },
            { 2, 1, 1, 1, 1 },
            { 1, 2, 0, 2, 2 },
            { 1, 1, 0, 1, 1 },
            { -1, -1, -1, 0, -1 },
            { -2, -2, 0, -1, -1 },
            { -1, -1, 1, 0, 0 },
            { 1, 0, 0, 1, 1 },
            { 0, 0, 0, 0, 0 }
        } };

        if (loopStep == 0u || loopStep >= 8u)
        {
            return {};
        }

        const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
        const uint64_t hash = getAnimationHash(seed, centerX, component.NozzleY, EngineVariationSalt ^ static_cast<uint64_t>(engineIndex));
        uint32_t effectiveStep = loopStep;

        if (!profile.SynchronizeEngines)
        {
            const bool reversePhase = profile.AlternateEnginePhases ? (engineIndex & 1u) != 0u : profile.AsynchronousEngines && ((hash >> 12u) & 1ull) != 0ull;
            if (reversePhase) { effectiveStep = 8u - loopStep; }
        }

        const std::array<EnginePulseState, 9u>* sequence = &Standard;

        if (!profile.SynchronizeEngines && (profile.IrregularEngineCycle || profile.AsynchronousEngines))
        {
            const uint32_t variation = static_cast<uint32_t>((hash >> 20u) % 3ull);
            if (variation == 1u) { sequence = &IrregularA; }
            if (variation == 2u) { sequence = &IrregularB; }
        }

        EnginePulseState state = (*sequence)[effectiveStep];

        if (profile.EnginePulseStrength <= 1u)
        {
            state.Length = std::clamp(state.Length, -1, 1);
            state.Intensity = std::clamp(state.Intensity, -1, 1);
            state.InnerLength = std::clamp(state.InnerLength, -1, 1);
            state.CoreLength = std::clamp(state.CoreLength, -1, 1);
        }

        return state;
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

    uint32_t getLengthDelta(uint32_t available, int32_t signal, uint32_t amplitudePercent)
    {
        if (available == 0u || signal == 0)
        {
            return 0u;
        }

        const uint32_t scaledAvailable = std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(available) * amplitudePercent + 99u) / 100u));
        const uint32_t magnitude = static_cast<uint32_t>(signal < 0 ? -signal : signal);
        return magnitude >= 2u ? std::min(available, scaledAvailable) : std::min(available, std::max(1u, (scaledAvailable + 1u) / 2u));
    }

    uint32_t getTargetExhaustLength(const PixelShipGenerator::ShipEngineAnimationComponent& component, const EnginePulseState& pulse, uint32_t amplitudePercent)
    {
        uint32_t length = component.ExhaustLength;

        if (pulse.Length > 0)
        {
            length += getLengthDelta(component.MaximumExhaustLength > length ? component.MaximumExhaustLength - length : 0u, pulse.Length, amplitudePercent);
        }
        else if (pulse.Length < 0)
        {
            const uint32_t available = length > component.MinimumExhaustLength ? length - component.MinimumExhaustLength : 0u;
            length -= getLengthDelta(available, pulse.Length, amplitudePercent);
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

    uint32_t getLayerLength(uint32_t outerLength, uint32_t numerator, uint32_t denominator, int32_t pulseSignal)
    {
        uint32_t length = std::max(1u, outerLength * numerator / denominator);

        if (pulseSignal > 0 && length < outerLength)
        {
            length = std::min(outerLength, length + static_cast<uint32_t>(pulseSignal));
        }
        else if (pulseSignal < 0 && length > 1u)
        {
            length = std::max(1u, length - static_cast<uint32_t>(-pulseSignal));
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

    void redrawEngineExhaust(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, const EnginePulseState& pulse, uint32_t amplitudePercent)
    {
        if (component.ExhaustLength == 0u || component.ExhaustStartY >= ship.EngineExhaustMask.getHeight())
        {
            return;
        }

        const uint32_t requestedLength = getTargetExhaustLength(component, pulse, amplitudePercent);
        const uint32_t desiredLength = clampExhaustLengthToSafeEnvelope(frame, ship, component, requestedLength, pulse.TaperBias);
        const uint32_t innerLength = std::min(desiredLength, getLayerLength(desiredLength, 3u, 4u, pulse.InnerLength));
        const uint32_t coreLength = std::min(innerLength, getLayerLength(desiredLength, 1u, 2u, pulse.CoreLength));
        const uint32_t centerXTimesTwo = component.NozzleStartX * 2u + component.NozzleWidth - 1u;

        clearStaticEngineExhaust(frame, ship, component);

        for (uint32_t row = 0u; row < desiredLength; ++row)
        {
            const uint32_t width = getTaperedExhaustRowWidth(component.NozzleWidth, desiredLength, row, component.TaperMode, pulse.TaperBias);
            const uint32_t startX = getCenteredStartX(centerXTimesTwo, width);
            const uint32_t y = component.ExhaustStartY + row;

            for (uint32_t offset = 0u; offset < width; ++offset)
            {
                frame.setPixel(startX + offset, y, getAnimatedExhaustPixelColor(offset, width, row, innerLength, coreLength, pulse.Intensity, ship.Palette));
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

    void applyEngineAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed, const IdleAnimationProfile& profile)
    {
        if (loopStep == 0u || loopStep >= 8u)
        {
            return;
        }

        const std::size_t engineCount = ship.IdleAnimationMetadata.EngineComponents.size();

        for (std::size_t engineIndex = 0u; engineIndex < engineCount; ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            const EnginePulseState pulse = getEnginePulseState(loopStep, component, static_cast<uint32_t>(engineIndex), seed, profile);
            redrawEngineExhaust(frame, ship, component, pulse, getEngineAmplitudePercent(profile, component, engineCount));
            applyNozzleGlow(frame, ship, component, pulse.Intensity);
        }
    }

    bool isMechanicalPhaseActive(uint32_t loopStep, bool alternatePhase, bool slowCycle)
    {
        if (loopStep == 0u || loopStep >= 8u)
        {
            return false;
        }

        if (slowCycle)
        {
            return alternatePhase ? loopStep >= 4u && loopStep <= 6u : loopStep >= 2u && loopStep <= 4u;
        }

        return alternatePhase ? loopStep == 5u || loopStep == 6u : loopStep == 2u || loopStep == 3u;
    }

    void applyEngineMechanicalAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed, const IdleAnimationProfile& profile)
    {
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.EngineMask.getWidth(), ship.EngineMask.getHeight() });

        if (scaleTraits.AnimationComplexity < 20u)
        {
            return;
        }

        for (const PixelShipGenerator::ShipEngineAnimationComponent& component : ship.IdleAnimationMetadata.EngineComponents)
        {
            if (component.NozzleWidth < 3u || component.HousingWidth < 3u || component.NozzleY >= ship.EngineMask.getHeight())
            {
                continue;
            }

            const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
            const uint64_t hash = getAnimationHash(seed, centerX, component.NozzleY, EngineMechanicalSalt);

            if (hash % 100u >= profile.EngineMechanicalChance)
            {
                continue;
            }

            const bool alternatePhase = profile.AlternateEnginePhases && (hash & 1ull) != 0ull;

            if (!isMechanicalPhaseActive(loopStep, alternatePhase, profile.SlowMechanicalCycle))
            {
                continue;
            }

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

    void applyLightBlinking(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed)
    {
        int32_t activeGroup = -1;

        if (loopStep == 2u || loopStep == 3u) { activeGroup = 0; }
        if (loopStep == 5u || loopStep == 6u) { activeGroup = 1; }

        if (activeGroup < 0)
        {
            return;
        }

        const uint32_t width = ship.LightMask.getWidth();
        const uint32_t height = ship.LightMask.getHeight();

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (!ship.LightMask.get(x, y))
                {
                    continue;
                }

                const uint32_t canonicalX = std::min(x, width - 1u - x);
                const uint32_t group = static_cast<uint32_t>(getAnimationHash(seed, canonicalX, y, LightVariationSalt) & 1ull);
                frame.setPixel(x, y, group == static_cast<uint32_t>(activeGroup) ? ship.Palette.LightHighlight : ship.Palette.LightBase);
            }
        }
    }

    void applyMajorFeatureAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed, const IdleAnimationProfile& profile, bool lightsEnabled, bool detailVariationEnabled)
    {
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.HullMask.getWidth(), ship.HullMask.getHeight() });

        for (const PixelShipGenerator::ShipMajorFeatureAnimationComponent& component : ship.IdleAnimationMetadata.MajorFeatureComponents)
        {
            if (component.Type == PixelShipGenerator::ShipMajorFeatureType::TECH_CORE && lightsEnabled && loopStep > 0u && loopStep < 8u)
            {
                const uint64_t hash = getAnimationHash(seed, component.MinimumX, component.MinimumY, MajorFeatureSalt);
                const bool alternatePhase = ship.Faction == PixelShipGenerator::ShipFactionType::XENO && (hash & 1ull) != 0ull;
                const bool bright = alternatePhase ? loopStep >= 5u && loopStep <= 7u : loopStep >= 1u && loopStep <= 3u;
                const bool dim = alternatePhase ? loopStep >= 1u && loopStep <= 2u : loopStep >= 5u && loopStep <= 6u;

                for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.HullMask.getHeight(); ++y)
                {
                    for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.HullMask.getWidth(); ++x)
                    {
                        if (!ship.IdleAnimationMetadata.MajorFeatureEmissiveMask.get(x, y))
                        {
                            continue;
                        }

                        if (bright)
                        {
                            frame.setPixel(x, y, ship.Palette.LightHighlight);
                        }
                        else if (dim && profile.TechPulseStrength >= 2u)
                        {
                            frame.setPixel(x, y, ship.Palette.LightBase);
                        }
                    }
                }
            }
            else if (component.Type == PixelShipGenerator::ShipMajorFeatureType::VENT_BANK && detailVariationEnabled && scaleTraits.AnimationComplexity >= 20u && loopStep > 0u && loopStep < 8u)
            {
                const uint64_t hash = getAnimationHash(seed, component.MinimumX, component.MinimumY, MajorFeatureSalt ^ DetailVariationSalt);

                if (hash % 100u >= profile.VentActivityChance)
                {
                    continue;
                }

                const uint32_t phase = (loopStep == 2u || loopStep == 3u) ? 0u : (loopStep == 5u || loopStep == 6u) ? 1u : 2u;

                if (phase >= 2u)
                {
                    continue;
                }

                for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.HullMask.getHeight(); ++y)
                {
                    for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.HullMask.getWidth(); ++x)
                    {
                        if (!ship.IdleAnimationMetadata.MajorFeatureMechanicalMask.get(x, y))
                        {
                            continue;
                        }

                        const uint32_t canonicalX = std::min(x, ship.HullMask.getWidth() - 1u - x);
                        const bool highlighted = ((canonicalX + y + phase + static_cast<uint32_t>(hash & 1ull)) & 1u) == 0u;
                        frame.setPixel(x, y, highlighted ? ship.Palette.MechanicalBase : ship.Palette.MechanicalDark);
                    }
                }
            }
        }
    }

    void applySmallDetailVariation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed)
    {
        if (loopStep != 3u && loopStep != 6u)
        {
            return;
        }

        uint64_t bestHash = std::numeric_limits<uint64_t>::max();
        PixelCoordinate selectedPixel;
        bool foundPixel = false;

        for (uint32_t y = 0u; y < ship.MechanicalDetailMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.MechanicalDetailMask.getWidth(); ++x)
            {
                if (!ship.MechanicalDetailMask.get(x, y) || getMaskNeighbourCount(ship.MechanicalDetailMask, static_cast<int32_t>(x), static_cast<int32_t>(y)) > 2u)
                {
                    continue;
                }

                const uint64_t hash = getAnimationHash(seed, x, y, DetailVariationSalt);

                if (hash < bestHash)
                {
                    bestHash = hash;
                    selectedPixel = { x, y };
                    foundPixel = true;
                }
            }
        }

        if (!foundPixel)
        {
            return;
        }

        frame.setPixel(selectedPixel.X, selectedPixel.Y, loopStep == 3u ? ship.Palette.HullAccentHighlight : ship.Palette.MechanicalBase);
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

    void applyWeaponAnimation(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed, const IdleAnimationProfile& profile, bool mechanicalEnabled, bool lightsEnabled)
    {
        if (ship.IdleAnimationMetadata.WeaponComponents.empty())
        {
            return;
        }

        PixelShipGenerator::PixelMask occupiedMask = ship.IdleAnimationMetadata.WeaponOccupiedMask;
        PixelShipGenerator::PixelMask movableMask = ship.IdleAnimationMetadata.WeaponMovableMask;
        PixelShipGenerator::PixelMask muzzleMask = ship.IdleAnimationMetadata.WeaponMuzzleMask;
        PixelShipGenerator::PixelMask emissiveMask = ship.IdleAnimationMetadata.WeaponEmissiveMask;
        PixelShipGenerator::PixelMask affectedMask(occupiedMask.getWidth(), occupiedMask.getHeight(), false);
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.HullMask.getWidth(), ship.HullMask.getHeight() });

        if (mechanicalEnabled && scaleTraits.AnimationComplexity >= 20u && loopStep > 0u && loopStep < 8u)
        {
            for (const PixelShipGenerator::ShipWeaponAnimationComponent& component : ship.IdleAnimationMetadata.WeaponComponents)
            {
                if (!component.MovableBarrel)
                {
                    continue;
                }

                const uint64_t hash = getAnimationHash(seed, component.AnchorX, component.AnchorY, WeaponMovementSalt);

                if (hash % 100u >= profile.WeaponMechanicalChance)
                {
                    continue;
                }

                bool alternatePhase = false;

                if (profile.AlternateWeaponPhases && component.SymmetryGroup != 0u)
                {
                    alternatePhase = component.AnchorX > occupiedMask.getWidth() / 2u;
                }
                else if (ship.Faction == PixelShipGenerator::ShipFactionType::FRONTIER && component.SymmetryGroup != 0u && ((hash >> 8u) & 1ull) != 0ull)
                {
                    alternatePhase = component.AnchorX > occupiedMask.getWidth() / 2u;
                }

                if (!isMechanicalPhaseActive(loopStep, alternatePhase, profile.SlowMechanicalCycle))
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

        if (!lightsEnabled || loopStep == 0u || loopStep >= 8u)
        {
            return;
        }

        const bool bright = loopStep >= 1u && loopStep <= 3u;
        const bool dim = loopStep >= 5u && loopStep <= 6u;

        if (!bright && !dim)
        {
            return;
        }

        for (uint32_t y = 0u; y < emissiveMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < emissiveMask.getWidth(); ++x)
            {
                if (!emissiveMask.get(x, y))
                {
                    continue;
                }

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

    void applyMechanicalMicroMovement(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep, uint64_t seed, int32_t preferredDirection)
    {
        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.AttachmentMask.getWidth(), ship.AttachmentMask.getHeight() });
        if (scaleTraits.AnimationComplexity < 20u)
        {
            return;
        }

        int32_t movement = 0;

        if (loopStep == 2u || loopStep == 3u) { movement = preferredDirection; }
        if (loopStep == 5u || loopStep == 6u) { movement = -preferredDirection; }

        if (movement == 0)
        {
            return;
        }

        const PixelShipGenerator::ShipAttachmentPlacement* selectedPlacement = nullptr;
        uint64_t selectedHash = std::numeric_limits<uint64_t>::max();
        const uint32_t maximumPixelCount = scaleTraits.AnimationComplexity >= 80u ? 36u : 20u;

        for (const PixelShipGenerator::ShipAttachmentPlacement& placement : ship.AttachmentPlacements)
        {
            if (placement.Type != PixelShipGenerator::ShipAttachmentType::SENSOR_ARRAY && placement.Type != PixelShipGenerator::ShipAttachmentType::TECHNOLOGY_NODE)
            {
                continue;
            }

            if (getAttachmentMaximumOutwardDistance(placement) < 2u || countAttachmentPixels(ship, placement) > maximumPixelCount)
            {
                continue;
            }

            const uint64_t hash = getAnimationHash(seed, placement.AnchorX, placement.AnchorY, MicroMovementSalt);

            if (hash < selectedHash)
            {
                selectedHash = hash;
                selectedPlacement = &placement;
            }
        }

        if (selectedPlacement == nullptr)
        {
            return;
        }

        const uint32_t maximumOutwardDistance = getAttachmentMaximumOutwardDistance(*selectedPlacement);
        const auto [tangentX, tangentY] = getAttachmentTangentOffset(selectedPlacement->Direction);
        std::vector<PixelCoordinate> origins;
        std::vector<PixelCoordinate> destinations;
        std::vector<PixelShipGenerator::Color> colors;

        for (uint32_t y = selectedPlacement->MinimumY; y <= selectedPlacement->MaximumY; ++y)
        {
            for (uint32_t x = selectedPlacement->MinimumX; x <= selectedPlacement->MaximumX; ++x)
            {
                if (!ship.AttachmentMask.get(x, y) || getAttachmentOutwardDistance(*selectedPlacement, x, y) != maximumOutwardDistance)
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

        if (origins.empty())
        {
            return;
        }

        for (const PixelCoordinate& destination : destinations)
        {
            if (isBaseStructurePixel(ship, static_cast<int32_t>(destination.X), static_cast<int32_t>(destination.Y)))
            {
                return;
            }

            if (ship.AttachmentMask.get(destination.X, destination.Y) && !containsCoordinate(origins, destination.X, destination.Y))
            {
                return;
            }
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

        if (!connected)
        {
            return;
        }

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
            if (isBaseStructurePixel(ship, static_cast<int32_t>(pixel.X), static_cast<int32_t>(pixel.Y)))
            {
                continue;
            }

            if (isAnimatedAttachmentPixel(ship, origins, destinations, static_cast<int32_t>(pixel.X), static_cast<int32_t>(pixel.Y)))
            {
                continue;
            }

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

    int32_t getHoverOffset(uint32_t loopStep)
    {
        constexpr std::array<int32_t, 9u> Offsets = { 0, -1, -1, 0, 1, 1, 0, 0, 0 };
        return Offsets[std::min<std::size_t>(loopStep, Offsets.size() - 1u)];
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

    void applyHoverOffset(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, uint32_t loopStep)
    {
        const int32_t desiredOffset = getHoverOffset(loopStep);

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
}

namespace PixelShipGenerator
{
    ShipIdleAnimation ShipIdleAnimator::generate(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings) const
    {
        ShipIdleAnimation animation;
        animation.FrameWidth = ship.HullMask.getWidth();
        animation.FrameHeight = ship.HullMask.getHeight();
        animation.Seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ AnimationSeedSalt);

        const uint32_t frameCount = std::max(1u, settings.FrameCount);
        animation.Frames.reserve(frameCount);
        const int32_t preferredMicroMovementDirection = (getAnimationHash(animation.Seed, 0u, 0u, MicroMovementSalt) & 1ull) == 0ull ? -1 : 1;
        const IdleAnimationProfile profile = getIdleAnimationProfile(ship);

        for (uint32_t frameIndex = 0u; frameIndex < frameCount; ++frameIndex)
        {
            Image frame = ship.FinalImage;

            if (frameIndex == 0u)
            {
                animation.Frames.push_back(std::move(frame));
                continue;
            }

            const uint32_t loopStep = getLoopStep(frameIndex, frameCount);

            if (settings.MechanicalMicroMovement) { applyEngineMechanicalAnimation(frame, ship, loopStep, animation.Seed, profile); }
            if (settings.LightBlinking) { applyLightBlinking(frame, ship, loopStep, animation.Seed); }
            applyMajorFeatureAnimation(frame, ship, loopStep, animation.Seed, profile, settings.LightBlinking, settings.SmallDetailVariation);
            if (settings.SmallDetailVariation) { applySmallDetailVariation(frame, ship, loopStep, animation.Seed); }
            applyWeaponAnimation(frame, ship, loopStep, animation.Seed, profile, settings.MechanicalMicroMovement, settings.LightBlinking);
            if (settings.MechanicalMicroMovement) { applyMechanicalMicroMovement(frame, ship, loopStep, animation.Seed, preferredMicroMovementDirection); }
            if (settings.EngineFlicker) { applyEngineAnimation(frame, ship, loopStep, animation.Seed, profile); }
            if (settings.HoverOffset) { applyHoverOffset(frame, ship, loopStep); }

            animation.Frames.push_back(std::move(frame));
        }

        return animation;
    }
}
