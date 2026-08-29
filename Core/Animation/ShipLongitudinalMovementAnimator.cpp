#include "ShipLongitudinalMovementAnimator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "AnimationSamplingPlanner.h"
#include "GenerationScaleTraits.h"
#include "PixelMask.h"
#include "ShipGenerationSeeds.h"

namespace
{
    constexpr uint64_t LongitudinalMovementSeedSalt = 0x7B4D91E2A6503C8Full;
    constexpr uint64_t EnginePhaseSalt = 0xC15874A9263BE0DFull;
    constexpr uint64_t WeaponPhaseSalt = 0x3F7A09E5D81264B1ull;
    constexpr uint64_t AttachmentPhaseSalt = 0xA65C23F190D87BE4ull;

    struct PixelCoordinate
    {
        uint32_t X = 0u;
        uint32_t Y = 0u;
    };

    struct MovableGroup
    {
        PixelShipGenerator::ShipMovementAnimatedComponentType Type = PixelShipGenerator::ShipMovementAnimatedComponentType::WEAPON_STABILIZATION;
        uint32_t SemanticGroup = 0u;
        std::vector<PixelCoordinate> SourcePixels;
        int32_t MaximumOffsetX = 0;
        int32_t MaximumOffsetY = 0;
        double SustainPhaseOffset = 0.0;
    };

    struct EngineResponseParameters
    {
        uint32_t EngineIndex = 0u;
        uint32_t EnteredExhaustLength = 0u;
        uint32_t SustainVariationPixels = 0u;
        uint32_t MaximumExhaustLengthDelta = 0u;
        double SustainPhaseOffset = 0.0;
    };

    struct LongitudinalMovementProfile
    {
        uint32_t ResponseStrengthPercent = 100u;
        uint32_t AccelerationExtensionPercent = 90u;
        uint32_t BrakingContractionPercent = 80u;
        uint32_t ExhaustVariationLimit = 1u;
        uint32_t WeaponTravelLimit = 1u;
        uint32_t AttachmentTravelLimit = 1u;
        uint32_t BrakingTravelLimit = 1u;
        bool Synchronized = false;
        bool Staggered = false;
        bool HeavyResponse = false;
        bool Responsive = false;
    };

    struct LongitudinalMovementPlan
    {
        PixelShipGenerator::ShipAnimationType Type = PixelShipGenerator::ShipAnimationType::MOVE_UP;
        int32_t DirectionSignY = -1;
        uint64_t Seed = 0u;
        LongitudinalMovementProfile Profile;
        std::vector<EngineResponseParameters> Engines;
        std::vector<MovableGroup> Groups;
        PixelShipGenerator::ShipMovementAnimationDiagnostics Diagnostics;
        PixelShipGenerator::AnimationSamplingRequirements EnterSampling;
        PixelShipGenerator::AnimationSamplingRequirements SustainSampling;
        PixelShipGenerator::AnimationSamplingRequirements ExitSampling;
    };

    bool isLongitudinalAnimationType(PixelShipGenerator::ShipAnimationType type)
    {
        return type == PixelShipGenerator::ShipAnimationType::MOVE_UP || type == PixelShipGenerator::ShipAnimationType::MOVE_DOWN;
    }

    uint64_t getAnimationHash(uint64_t seed, uint32_t x, uint32_t y, uint64_t salt)
    {
        uint64_t value = seed;
        value ^= static_cast<uint64_t>(x) * 0x9E3779B185EBCA87ull;
        value ^= static_cast<uint64_t>(y) * 0xC2B2AE3D27D4EB4Full;
        value ^= salt;
        return PixelShipGenerator::mixGenerationSeed64(value);
    }

    double clampNormalizedTime(double normalizedTime)
    {
        if (!std::isfinite(normalizedTime)) { return 0.0; }
        return std::clamp(normalizedTime, 0.0, 1.0);
    }

    double wrapNormalizedTime(double normalizedTime)
    {
        if (!std::isfinite(normalizedTime)) { return 0.0; }
        double wrapped = normalizedTime - std::floor(normalizedTime);
        if (wrapped < 0.0) { wrapped += 1.0; }
        return wrapped;
    }

    double smoothStep(double value)
    {
        const double t = std::clamp(value, 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    double easeOutCubic(double value)
    {
        const double t = std::clamp(value, 0.0, 1.0);
        const double inverse = 1.0 - t;
        return 1.0 - inverse * inverse * inverse;
    }

    LongitudinalMovementProfile getLongitudinalMovementProfile(const PixelShipGenerator::GeneratedShip& ship)
    {
        LongitudinalMovementProfile profile;

        switch (ship.Style)
        {
        case PixelShipGenerator::ShipStyle::SLEEK:
            profile.ResponseStrengthPercent = 70u;
            profile.AccelerationExtensionPercent = 75u;
            profile.BrakingContractionPercent = 65u;
            profile.Responsive = true;
            break;
        case PixelShipGenerator::ShipStyle::FIGHTER:
            profile.ResponseStrengthPercent = 100u;
            profile.AccelerationExtensionPercent = 100u;
            profile.BrakingContractionPercent = 80u;
            profile.Responsive = true;
            break;
        case PixelShipGenerator::ShipStyle::HEAVY:
            profile.ResponseStrengthPercent = 65u;
            profile.AccelerationExtensionPercent = 70u;
            profile.BrakingContractionPercent = 60u;
            profile.HeavyResponse = true;
            break;
        case PixelShipGenerator::ShipStyle::INDUSTRIAL:
            profile.ResponseStrengthPercent = 115u;
            profile.AccelerationExtensionPercent = 100u;
            profile.BrakingContractionPercent = 100u;
            profile.ExhaustVariationLimit = 2u;
            profile.AttachmentTravelLimit = 2u;
            profile.BrakingTravelLimit = 2u;
            profile.Staggered = true;
            break;
        case PixelShipGenerator::ShipStyle::SPEARHEAD:
            profile.ResponseStrengthPercent = 105u;
            profile.AccelerationExtensionPercent = 100u;
            profile.BrakingContractionPercent = 75u;
            profile.Synchronized = true;
            break;
        case PixelShipGenerator::ShipStyle::DELTA:
            profile.ResponseStrengthPercent = 95u;
            profile.AccelerationExtensionPercent = 90u;
            profile.BrakingContractionPercent = 90u;
            profile.AttachmentTravelLimit = 2u;
            profile.BrakingTravelLimit = 2u;
            profile.HeavyResponse = true;
            break;
        default:
            break;
        }

        switch (ship.Faction)
        {
        case PixelShipGenerator::ShipFactionType::FRONTIER:
            profile.Staggered = true;
            profile.Synchronized = false;
            break;
        case PixelShipGenerator::ShipFactionType::MILITARY:
            profile.Synchronized = true;
            profile.Staggered = false;
            profile.ResponseStrengthPercent = profile.ResponseStrengthPercent * 9u / 10u;
            break;
        case PixelShipGenerator::ShipFactionType::ASCENDANT:
            profile.ResponseStrengthPercent = profile.ResponseStrengthPercent * 4u / 5u;
            break;
        case PixelShipGenerator::ShipFactionType::XENO:
            profile.Staggered = true;
            profile.Synchronized = false;
            profile.ExhaustVariationLimit = std::max(1u, profile.ExhaustVariationLimit);
            break;
        case PixelShipGenerator::ShipFactionType::CORPORATE:
            profile.Synchronized = true;
            profile.Staggered = false;
            profile.ResponseStrengthPercent = profile.ResponseStrengthPercent * 4u / 5u;
            break;
        case PixelShipGenerator::ShipFactionType::RELIC:
            profile.HeavyResponse = true;
            profile.ResponseStrengthPercent = profile.ResponseStrengthPercent * 3u / 4u;
            break;
        default:
            break;
        }

        profile.ResponseStrengthPercent = std::clamp(profile.ResponseStrengthPercent, 40u, 120u);
        return profile;
    }

    double getPhaseOffset(uint64_t hash, const LongitudinalMovementProfile& profile)
    {
        if (profile.Synchronized) { return 0.0; }
        if (profile.Staggered) { return static_cast<double>(hash % 4ull) * 0.125; }
        return (hash & 1ull) == 0ull ? 0.0 : 0.125;
    }

    double sampleStyleTransitionResponse(const PixelShipGenerator::GeneratedShip& ship, const LongitudinalMovementProfile& profile, double value)
    {
        double t = std::clamp(value, 0.0, 1.0);
        if (ship.Faction == PixelShipGenerator::ShipFactionType::RELIC) { t *= t; }
        if (profile.Responsive) { return easeOutCubic(t); }
        if (profile.HeavyResponse) { return smoothStep(t * t); }
        return smoothStep(t);
    }

    double sampleTransitionResponse(const PixelShipGenerator::GeneratedShip& ship, const LongitudinalMovementProfile& profile, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, double phaseOffset)
    {
        const double time = clampNormalizedTime(normalizedTime);
        const double maximumDelay = profile.Synchronized ? 0.0 : profile.Staggered ? 0.14 : 0.07;
        const double delay = std::clamp(phaseOffset, 0.0, 1.0) * maximumDelay;

        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER)
        {
            if (time <= delay) { return 0.0; }
            const double local = (time - delay) / std::max(0.000001, 1.0 - delay);
            return sampleStyleTransitionResponse(ship, profile, local);
        }

        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::EXIT)
        {
            if (time >= 1.0) { return 0.0; }
            const double activeDuration = std::max(0.000001, 1.0 - delay);
            const double local = std::clamp(time / activeDuration, 0.0, 1.0);
            return 1.0 - sampleStyleTransitionResponse(ship, profile, local);
        }

        return 1.0;
    }

    double sampleAnchoredSustainActivity(double normalizedTime, double phaseOffset)
    {
        const double time = wrapNormalizedTime(normalizedTime);
        const double envelope = time < 0.5 ? time * 2.0 : (1.0 - time) * 2.0;
        const double phasedTime = wrapNormalizedTime(time + phaseOffset);
        const double phaseWave = phasedTime < 0.5 ? phasedTime * 2.0 : (1.0 - phasedTime) * 2.0;
        return smoothStep(std::clamp(envelope * (0.55 + phaseWave * 0.45), 0.0, 1.0));
    }

    int32_t quantizeOffset(int32_t maximumOffset, double response)
    {
        if (maximumOffset == 0) { return 0; }
        const double magnitude = std::clamp(response, 0.0, 1.0) * static_cast<double>(std::abs(maximumOffset));
        const int32_t rounded = static_cast<int32_t>(std::floor(magnitude + 0.5));
        return maximumOffset < 0 ? -rounded : rounded;
    }

    uint32_t interpolateLength(uint32_t start, uint32_t end, double response)
    {
        const double value = static_cast<double>(start) + (static_cast<double>(end) - static_cast<double>(start)) * std::clamp(response, 0.0, 1.0);
        return static_cast<uint32_t>(std::max(0.0, std::floor(value + 0.5)));
    }

    bool containsPixel(const std::vector<PixelCoordinate>& pixels, uint32_t x, uint32_t y)
    {
        return std::find_if(pixels.begin(), pixels.end(), [&](const PixelCoordinate& pixel) { return pixel.X == x && pixel.Y == y; }) != pixels.end();
    }

    bool hasHullPixelInDirection(const PixelShipGenerator::GeneratedShip& ship, int32_t startX, int32_t startY, int32_t stepX, int32_t stepY)
    {
        int32_t x = startX + stepX;
        int32_t y = startY + stepY;
        while (x >= 0 && y >= 0 && x < static_cast<int32_t>(ship.HullMask.getWidth()) && y < static_cast<int32_t>(ship.HullMask.getHeight()))
        {
            if (ship.HullMask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y))) { return true; }
            x += stepX;
            y += stepY;
        }
        return false;
    }

    bool isLikelyStructuralVoid(const PixelShipGenerator::GeneratedShip& ship, uint32_t x, uint32_t y)
    {
        if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y)) { return false; }
        const int32_t px = static_cast<int32_t>(x);
        const int32_t py = static_cast<int32_t>(y);
        const bool horizontallyEnclosed = hasHullPixelInDirection(ship, px, py, -1, 0) && hasHullPixelInDirection(ship, px, py, 1, 0);
        const bool verticallyEnclosed = hasHullPixelInDirection(ship, px, py, 0, -1) && hasHullPixelInDirection(ship, px, py, 0, 1);
        return horizontallyEnclosed || verticallyEnclosed;
    }

    bool isStaticOccupiedOutsideGroup(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& sourcePixels, uint32_t x, uint32_t y)
    {
        if (containsPixel(sourcePixels, x, y)) { return false; }
        if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y)) { return true; }
        if (!ship.IdleAnimationMetadata.WeaponOccupiedMask.empty() && ship.IdleAnimationMetadata.WeaponOccupiedMask.get(x, y)) { return true; }
        return false;
    }

    bool isGroupOffsetSafe(const PixelShipGenerator::GeneratedShip& ship, const MovableGroup& group, int32_t offsetX, int32_t offsetY, const PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (offsetX == 0 && offsetY == 0) { return true; }
        for (const PixelCoordinate& source : group.SourcePixels)
        {
            const int32_t destinationX = static_cast<int32_t>(source.X) + offsetX;
            const int32_t destinationY = static_cast<int32_t>(source.Y) + offsetY;
            if (destinationX < 0 || destinationY < 0 || destinationX >= static_cast<int32_t>(ship.FinalImage.getWidth()) || destinationY >= static_cast<int32_t>(ship.FinalImage.getHeight())) { return false; }
            const uint32_t x = static_cast<uint32_t>(destinationX);
            const uint32_t y = static_cast<uint32_t>(destinationY);
            if (isStaticOccupiedOutsideGroup(ship, group.SourcePixels, x, y)) { return false; }
            if (reservedMotionMask.get(x, y) && !containsPixel(group.SourcePixels, x, y)) { return false; }
            if (ship.FinalImage.getPixel(x, y).A == 0u && isLikelyStructuralVoid(ship, x, y)) { return false; }
        }
        return true;
    }

    uint32_t findSafeTravel(const PixelShipGenerator::GeneratedShip& ship, MovableGroup& group, int32_t directionX, int32_t directionY, uint32_t desiredTravel, const PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        uint32_t safeTravel = 0u;
        for (uint32_t travel = 1u; travel <= desiredTravel; ++travel)
        {
            if (!isGroupOffsetSafe(ship, group, directionX * static_cast<int32_t>(travel), directionY * static_cast<int32_t>(travel), reservedMotionMask)) { break; }
            safeTravel = travel;
        }
        return safeTravel;
    }

    void reserveGroupMotion(const MovableGroup& group, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        const uint32_t steps = std::max(static_cast<uint32_t>(std::abs(group.MaximumOffsetX)), static_cast<uint32_t>(std::abs(group.MaximumOffsetY)));
        const int32_t directionX = group.MaximumOffsetX < 0 ? -1 : group.MaximumOffsetX > 0 ? 1 : 0;
        const int32_t directionY = group.MaximumOffsetY < 0 ? -1 : group.MaximumOffsetY > 0 ? 1 : 0;
        for (const PixelCoordinate& source : group.SourcePixels)
        {
            reservedMotionMask.set(source.X, source.Y, true);
            for (uint32_t step = 1u; step <= steps; ++step)
            {
                const int32_t x = static_cast<int32_t>(source.X) + directionX * static_cast<int32_t>(step);
                const int32_t y = static_cast<int32_t>(source.Y) + directionY * static_cast<int32_t>(step);
                if (x >= 0 && y >= 0 && x < static_cast<int32_t>(reservedMotionMask.getWidth()) && y < static_cast<int32_t>(reservedMotionMask.getHeight())) { reservedMotionMask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), true); }
            }
        }
    }

    uint32_t getScaleTravelCapacity(const PixelShipGenerator::GenerationScaleTraits& scaleTraits)
    {
        return 1u + scaleTraits.AnimationComplexity / 70u;
    }

    uint32_t scaleTravelForProfile(uint32_t availableTravel, uint32_t profileLimit, uint32_t responseStrengthPercent)
    {
        const uint32_t limited = std::min(availableTravel, profileLimit);
        if (limited == 0u) { return 0u; }
        const uint32_t scaled = static_cast<uint32_t>((static_cast<uint64_t>(limited) * responseStrengthPercent + 99u) / 100u);
        return std::clamp(scaled, 1u, limited);
    }

    std::vector<PixelCoordinate> collectWeaponPixels(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipWeaponAnimationComponent& component)
    {
        std::vector<PixelCoordinate> result;
        if (!component.MovableBarrel) { return result; }
        for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.IdleAnimationMetadata.WeaponMovableMask.getHeight(); ++y)
            for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.IdleAnimationMetadata.WeaponMovableMask.getWidth(); ++x)
                if (ship.IdleAnimationMetadata.WeaponMovableMask.get(x, y)) { result.push_back({ x, y }); }
        return result;
    }

    std::vector<PixelCoordinate> collectAttachmentPixels(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        std::vector<PixelCoordinate> result;
        for (uint32_t y = placement.MinimumY; y <= placement.MaximumY && y < ship.AttachmentMask.getHeight(); ++y)
            for (uint32_t x = placement.MinimumX; x <= placement.MaximumX && x < ship.AttachmentMask.getWidth(); ++x)
                if (ship.AttachmentMask.get(x, y)) { result.push_back({ x, y }); }
        return result;
    }

    bool sourcePixelsAreMovable(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& pixels, bool attachmentPixels)
    {
        for (const PixelCoordinate& pixel : pixels)
        {
            if (ship.HullMask.get(pixel.X, pixel.Y) || ship.CockpitMask.get(pixel.X, pixel.Y) || ship.EngineMask.get(pixel.X, pixel.Y)) { return false; }
            if (!attachmentPixels && ship.AttachmentMask.get(pixel.X, pixel.Y)) { return false; }
            if (attachmentPixels && !ship.IdleAnimationMetadata.WeaponOccupiedMask.empty() && ship.IdleAnimationMetadata.WeaponOccupiedMask.get(pixel.X, pixel.Y)) { return false; }
        }
        return true;
    }

    uint32_t getTaperedExhaustRowWidth(uint32_t nozzleWidth, uint32_t length, uint32_t row, uint32_t taperMode)
    {
        if (nozzleWidth <= 1u || length <= 1u) { return std::max(1u, nozzleWidth); }
        const uint32_t maximumInset = (nozzleWidth - 1u) / 2u;
        uint32_t effectiveRow = std::min(row, length - 1u);
        if (effectiveRow > 0u && taperMode == 0u && effectiveRow + 1u < length) { ++effectiveRow; }
        if (taperMode == 2u && effectiveRow > length / 3u) { effectiveRow -= length / 3u; }
        if (taperMode == 2u && row <= length / 3u) { effectiveRow = 0u; }
        if (row + 1u == length) { effectiveRow = length - 1u; }
        const uint32_t inset = maximumInset == 0u ? 0u : std::min(maximumInset, static_cast<uint32_t>((static_cast<uint64_t>(effectiveRow) * maximumInset + length - 2u) / (length - 1u)));
        return std::max(1u, nozzleWidth - inset * 2u);
    }

    uint32_t getCenteredStartX(uint32_t centerXTimesTwo, uint32_t width)
    {
        const int32_t start = (static_cast<int32_t>(centerXTimesTwo) - static_cast<int32_t>(width - 1u)) / 2;
        return static_cast<uint32_t>(std::max(0, start));
    }

    bool isStaticComponentExhaustPixel(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t x, uint32_t y)
    {
        return x >= component.HousingStartX && x < component.HousingStartX + component.HousingWidth && y >= component.ExhaustStartY && y < component.ExhaustStartY + component.ExhaustLength && ship.EngineExhaustMask.get(x, y);
    }

    bool isAnimatedExhaustShapeSafe(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t length)
    {
        if (length == 0u || component.ExhaustStartY + length > ship.FinalImage.getHeight()) { return false; }
        const uint32_t centerXTimesTwo = component.NozzleStartX * 2u + component.NozzleWidth - 1u;
        for (uint32_t row = 0u; row < length; ++row)
        {
            const uint32_t width = getTaperedExhaustRowWidth(component.NozzleWidth, length, row, component.TaperMode);
            const uint32_t startX = getCenteredStartX(centerXTimesTwo, width);
            const uint32_t y = component.ExhaustStartY + row;
            if (startX + width > ship.FinalImage.getWidth()) { return false; }
            for (uint32_t offset = 0u; offset < width; ++offset)
            {
                const uint32_t x = startX + offset;
                if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y) || (!ship.IdleAnimationMetadata.WeaponOccupiedMask.empty() && ship.IdleAnimationMetadata.WeaponOccupiedMask.get(x, y))) { return false; }
                if (frame.getPixel(x, y).A != 0u && !isStaticComponentExhaustPixel(ship, component, x, y)) { return false; }
                if (ship.FinalImage.getPixel(x, y).A == 0u && isLikelyStructuralVoid(ship, x, y)) { return false; }
            }
        }
        return true;
    }

    uint32_t clampExhaustLengthToSafeEnvelope(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t desiredLength)
    {
        uint32_t length = std::clamp(desiredLength, component.MinimumExhaustLength, component.MaximumExhaustLength);
        while (length > component.MinimumExhaustLength && !isAnimatedExhaustShapeSafe(frame, ship, component, length)) { --length; }
        if (!isAnimatedExhaustShapeSafe(frame, ship, component, length)) { return component.ExhaustLength; }
        return length;
    }

    PixelShipGenerator::Color brightenExhaust(PixelShipGenerator::Color color, const PixelShipGenerator::ShipPalette& palette)
    {
        if (color == palette.ExhaustBase) { return palette.ExhaustHighlight; }
        if (color == palette.ExhaustHighlight) { return palette.ExhaustHotCore; }
        return color;
    }

    PixelShipGenerator::Color dimExhaust(PixelShipGenerator::Color color, const PixelShipGenerator::ShipPalette& palette)
    {
        if (color == palette.ExhaustHotCore) { return palette.ExhaustHighlight; }
        if (color == palette.ExhaustHighlight) { return palette.ExhaustBase; }
        return color;
    }

    void clearStaticEngineExhaust(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component)
    {
        const uint32_t endX = std::min(ship.EngineExhaustMask.getWidth(), component.HousingStartX + component.HousingWidth);
        const uint32_t maximumY = std::min(ship.EngineExhaustMask.getHeight(), component.ExhaustStartY + component.ExhaustLength);
        for (uint32_t y = component.ExhaustStartY; y < maximumY; ++y)
            for (uint32_t x = component.HousingStartX; x < endX; ++x)
                if (ship.EngineExhaustMask.get(x, y)) { frame.setPixel(x, y, PixelShipGenerator::Color()); }
    }

    void redrawEngineExhaust(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t desiredLength, bool acceleration, double response)
    {
        if (component.ExhaustLength == 0u || component.ExhaustStartY >= ship.FinalImage.getHeight()) { return; }
        desiredLength = clampExhaustLengthToSafeEnvelope(frame, ship, component, desiredLength);
        clearStaticEngineExhaust(frame, ship, component);

        const uint32_t centerXTimesTwo = component.NozzleStartX * 2u + component.NozzleWidth - 1u;
        const uint32_t innerLength = acceleration ? std::max(1u, desiredLength * 2u / 3u) : std::max(1u, desiredLength / 2u);
        const uint32_t coreLength = acceleration ? std::max(1u, desiredLength / 2u) : std::max(1u, desiredLength / 4u);
        const bool strongResponse = response >= 0.50;

        for (uint32_t row = 0u; row < desiredLength; ++row)
        {
            const uint32_t width = getTaperedExhaustRowWidth(component.NozzleWidth, desiredLength, row, component.TaperMode);
            const uint32_t startX = getCenteredStartX(centerXTimesTwo, width);
            const uint32_t y = component.ExhaustStartY + row;
            for (uint32_t offset = 0u; offset < width; ++offset)
            {
                PixelShipGenerator::Color color = ship.Palette.ExhaustBase;
                const bool inner = width <= 2u || (offset > 0u && offset + 1u < width);
                if (inner && row < innerLength) { color = ship.Palette.ExhaustHighlight; }
                if (inner && row < coreLength) { color = ship.Palette.ExhaustHotCore; }
                if (strongResponse) { color = acceleration ? brightenExhaust(color, ship.Palette) : dimExhaust(color, ship.Palette); }
                frame.setPixel(startX + offset, y, color);
            }
        }
    }

    void addDiagnosticComponent(LongitudinalMovementPlan& plan, const MovableGroup& group)
    {
        PixelShipGenerator::ShipMovementComponentDiagnostic diagnostic;
        diagnostic.Type = group.Type;
        diagnostic.SemanticGroup = group.SemanticGroup;
        diagnostic.SourcePixelCount = static_cast<uint32_t>(group.SourcePixels.size());
        diagnostic.MaximumOffsetX = group.MaximumOffsetX;
        diagnostic.MaximumOffsetY = group.MaximumOffsetY;
        diagnostic.SustainPhaseOffset = group.SustainPhaseOffset;
        plan.Diagnostics.Components.push_back(diagnostic);
        plan.Diagnostics.MaximumMechanicalTravelPixels = std::max(plan.Diagnostics.MaximumMechanicalTravelPixels, std::max(static_cast<uint32_t>(std::abs(group.MaximumOffsetX)), static_cast<uint32_t>(std::abs(group.MaximumOffsetY))));
    }

    void addEngineResponses(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LongitudinalMovementPlan& plan)
    {
        if (!settings.EnginePropulsionResponse && !settings.ExhaustResponse) { return; }
        const uint32_t scaleVariation = 1u + scaleTraits.AnimationComplexity / 90u;
        for (std::size_t engineIndex = 0u; engineIndex < ship.IdleAnimationMetadata.EngineComponents.size(); ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            EngineResponseParameters parameters;
            parameters.EngineIndex = static_cast<uint32_t>(engineIndex);
            const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
            parameters.SustainPhaseOffset = getPhaseOffset(getAnimationHash(plan.Seed, centerX, component.NozzleY, EnginePhaseSalt), plan.Profile);

            parameters.EnteredExhaustLength = component.ExhaustLength;
            if (settings.ExhaustResponse)
            {
                if (plan.Type == PixelShipGenerator::ShipAnimationType::MOVE_UP)
                {
                    const uint32_t available = component.MaximumExhaustLength > component.ExhaustLength ? component.MaximumExhaustLength - component.ExhaustLength : 0u;
                    uint32_t extension = available == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(available) * plan.Profile.AccelerationExtensionPercent + 99u) / 100u);
                    extension = std::min(available, std::max(available > 0u ? 1u : 0u, extension));
                    parameters.EnteredExhaustLength = component.ExhaustLength + extension;
                    parameters.SustainVariationPixels = std::min({ plan.Profile.ExhaustVariationLimit, scaleVariation, component.MaximumExhaustLength - parameters.EnteredExhaustLength });
                }
                else
                {
                    const uint32_t available = component.ExhaustLength > component.MinimumExhaustLength ? component.ExhaustLength - component.MinimumExhaustLength : 0u;
                    uint32_t contraction = available == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(available) * plan.Profile.BrakingContractionPercent + 99u) / 100u);
                    contraction = std::min(available, std::max(available > 0u ? 1u : 0u, contraction));
                    parameters.EnteredExhaustLength = component.ExhaustLength - contraction;
                    parameters.SustainVariationPixels = std::min({ plan.Profile.ExhaustVariationLimit, scaleVariation, parameters.EnteredExhaustLength - component.MinimumExhaustLength });
                }
            }

            parameters.MaximumExhaustLengthDelta = static_cast<uint32_t>(std::abs(static_cast<int32_t>(parameters.EnteredExhaustLength) - static_cast<int32_t>(component.ExhaustLength))) + parameters.SustainVariationPixels;
            plan.Diagnostics.MaximumExhaustTravelPixels = std::max(plan.Diagnostics.MaximumExhaustTravelPixels, parameters.MaximumExhaustLengthDelta);

            PixelShipGenerator::ShipMovementComponentDiagnostic diagnostic;
            diagnostic.Type = PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_PROPULSION;
            diagnostic.SemanticGroup = static_cast<uint32_t>(engineIndex);
            diagnostic.SourcePixelCount = component.ExhaustLength * std::max(1u, component.NozzleWidth);
            diagnostic.MaximumExhaustLengthDelta = parameters.MaximumExhaustLengthDelta;
            diagnostic.SustainPhaseOffset = parameters.SustainPhaseOffset;
            plan.Diagnostics.Components.push_back(diagnostic);
            plan.Engines.push_back(parameters);
            ++plan.Diagnostics.ActiveEngineCount;
        }
    }

    void addWeaponGroups(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LongitudinalMovementPlan& plan, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (!settings.WeaponStabilization) { return; }
        const uint32_t desiredTravel = scaleTravelForProfile(getScaleTravelCapacity(scaleTraits), plan.Profile.WeaponTravelLimit, plan.Profile.ResponseStrengthPercent);
        if (desiredTravel == 0u) { return; }
        const int32_t directionY = plan.Type == PixelShipGenerator::ShipAnimationType::MOVE_UP ? -1 : 1;

        for (std::size_t weaponIndex = 0u; weaponIndex < ship.IdleAnimationMetadata.WeaponComponents.size(); ++weaponIndex)
        {
            const PixelShipGenerator::ShipWeaponAnimationComponent& component = ship.IdleAnimationMetadata.WeaponComponents[weaponIndex];
            MovableGroup group;
            group.Type = PixelShipGenerator::ShipMovementAnimatedComponentType::WEAPON_STABILIZATION;
            group.SemanticGroup = component.SymmetryGroup != 0u ? component.SymmetryGroup : static_cast<uint32_t>(weaponIndex + 1u);
            group.SourcePixels = collectWeaponPixels(ship, component);
            if (group.SourcePixels.empty() || !sourcePixelsAreMovable(ship, group.SourcePixels, false)) { continue; }
            group.SustainPhaseOffset = getPhaseOffset(getAnimationHash(plan.Seed, component.AnchorX, component.AnchorY, WeaponPhaseSalt ^ static_cast<uint64_t>(group.SemanticGroup)), plan.Profile);
            const uint32_t safeTravel = findSafeTravel(ship, group, 0, directionY, desiredTravel, reservedMotionMask);
            if (safeTravel == 0u) { continue; }
            group.MaximumOffsetY = directionY * static_cast<int32_t>(safeTravel);
            reserveGroupMotion(group, reservedMotionMask);
            addDiagnosticComponent(plan, group);
            plan.Groups.push_back(std::move(group));
            ++plan.Diagnostics.ActiveWeaponCount;
        }
    }

    bool isAccelerationAttachment(const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        const bool rearRegion = placement.Region == PixelShipGenerator::ShipAttachmentRegion::REAR || placement.Region == PixelShipGenerator::ShipAttachmentRegion::REAR_SIDE;
        const bool propulsionHardware = placement.Type == PixelShipGenerator::ShipAttachmentType::AUXILIARY_POD || placement.Type == PixelShipGenerator::ShipAttachmentType::RADIATOR || placement.Type == PixelShipGenerator::ShipAttachmentType::TECHNOLOGY_NODE;
        return rearRegion && propulsionHardware;
    }

    bool isBrakingAttachment(const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        const bool sideRegion = placement.Region == PixelShipGenerator::ShipAttachmentRegion::FRONT_SIDE || placement.Region == PixelShipGenerator::ShipAttachmentRegion::MIDDLE_SIDE || placement.Region == PixelShipGenerator::ShipAttachmentRegion::WING_OUTER_SIDE || placement.Region == PixelShipGenerator::ShipAttachmentRegion::REAR_SIDE;
        const bool brakingHardware = placement.Type == PixelShipGenerator::ShipAttachmentType::ARMOR_FIN || placement.Type == PixelShipGenerator::ShipAttachmentType::RADIATOR || placement.Type == PixelShipGenerator::ShipAttachmentType::AUXILIARY_POD;
        return sideRegion && brakingHardware;
    }

    void addAttachmentGroups(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LongitudinalMovementPlan& plan, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (!settings.AttachmentArticulation) { return; }
        const uint32_t availableTravel = getScaleTravelCapacity(scaleTraits);
        const uint32_t width = ship.FinalImage.getWidth();
        const int32_t centerXTimesTwo = static_cast<int32_t>(width - 1u);

        for (std::size_t placementIndex = 0u; placementIndex < ship.AttachmentPlacements.size(); ++placementIndex)
        {
            const PixelShipGenerator::ShipAttachmentPlacement& placement = ship.AttachmentPlacements[placementIndex];
            const bool acceleration = plan.Type == PixelShipGenerator::ShipAnimationType::MOVE_UP;
            if (acceleration && !isAccelerationAttachment(placement)) { continue; }
            if (!acceleration && (!settings.BrakingArticulation || !isBrakingAttachment(placement))) { continue; }

            MovableGroup group;
            group.Type = acceleration ? PixelShipGenerator::ShipMovementAnimatedComponentType::ATTACHMENT_ARTICULATION : PixelShipGenerator::ShipMovementAnimatedComponentType::BRAKING_ARTICULATION;
            group.SemanticGroup = placement.SymmetryGroup != 0u ? placement.SymmetryGroup : static_cast<uint32_t>(placementIndex + 1u);
            group.SourcePixels = collectAttachmentPixels(ship, placement);
            if (group.SourcePixels.empty() || !sourcePixelsAreMovable(ship, group.SourcePixels, true)) { continue; }
            group.SustainPhaseOffset = getPhaseOffset(getAnimationHash(plan.Seed, placement.AnchorX, placement.AnchorY, AttachmentPhaseSalt ^ static_cast<uint64_t>(group.SemanticGroup)), plan.Profile);

            uint32_t safeTravel = 0u;
            if (acceleration)
            {
                const uint32_t desiredTravel = scaleTravelForProfile(availableTravel, plan.Profile.AttachmentTravelLimit, plan.Profile.ResponseStrengthPercent);
                safeTravel = findSafeTravel(ship, group, 0, 1, desiredTravel, reservedMotionMask);
                if (safeTravel > 0u) { group.MaximumOffsetY = static_cast<int32_t>(safeTravel); }
            }
            else
            {
                const int32_t anchorSide = static_cast<int32_t>(placement.AnchorX * 2u) - centerXTimesTwo;
                if (anchorSide == 0) { continue; }
                const int32_t directionX = anchorSide > 0 ? 1 : -1;
                const uint32_t desiredTravel = scaleTravelForProfile(availableTravel, plan.Profile.BrakingTravelLimit, plan.Profile.ResponseStrengthPercent);
                safeTravel = findSafeTravel(ship, group, directionX, 0, desiredTravel, reservedMotionMask);
                if (safeTravel > 0u) { group.MaximumOffsetX = directionX * static_cast<int32_t>(safeTravel); }
            }

            if (safeTravel == 0u) { continue; }
            reserveGroupMotion(group, reservedMotionMask);
            addDiagnosticComponent(plan, group);
            plan.Groups.push_back(std::move(group));
            ++plan.Diagnostics.ActiveAttachmentCount;
            if (!acceleration) { ++plan.Diagnostics.ActiveBrakingComponentCount; }
        }
    }

    void finalizeDiagnosticsAndSampling(const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LongitudinalMovementPlan& plan)
    {
        plan.Diagnostics.DirectionSignX = 0;
        plan.Diagnostics.DirectionSignY = plan.DirectionSignY;
        std::vector<double> phaseOffsets;
        for (const EngineResponseParameters& engine : plan.Engines)
            if (std::find(phaseOffsets.begin(), phaseOffsets.end(), engine.SustainPhaseOffset) == phaseOffsets.end()) { phaseOffsets.push_back(engine.SustainPhaseOffset); }
        for (const MovableGroup& group : plan.Groups)
            if (std::find(phaseOffsets.begin(), phaseOffsets.end(), group.SustainPhaseOffset) == phaseOffsets.end()) { phaseOffsets.push_back(group.SustainPhaseOffset); }
        plan.Diagnostics.IndependentPhaseGroupCount = static_cast<uint32_t>(phaseOffsets.size());

        const uint32_t activeComponentCount = plan.Diagnostics.ActiveEngineCount + plan.Diagnostics.ActiveWeaponCount + plan.Diagnostics.ActiveAttachmentCount;
        auto configure = [&](PixelShipGenerator::AnimationSamplingRequirements& requirements, PixelShipGenerator::ShipMovementAnimationPhase phase)
        {
            requirements.Type = plan.Type;
            requirements.Mode = settings.SamplingMode;
            requirements.ScaleAnimationComplexity = scaleTraits.AnimationComplexity;
            requirements.MaximumMechanicalTravelPixels = plan.Diagnostics.MaximumMechanicalTravelPixels;
            requirements.MaximumExhaustTravelPixels = plan.Diagnostics.MaximumExhaustTravelPixels;
            requirements.ActiveAnimatedComponentCount = activeComponentCount;
            requirements.IndependentPhaseGroupCount = plan.Diagnostics.IndependentPhaseGroupCount;
            requirements.MaximumTemporalCyclesPerClip = phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN && activeComponentCount > 0u ? 2u : activeComponentCount > 0u ? 1u : 0u;
            requirements.MaximumFrameCount = std::max(1u, settings.MaximumFrameCount);

            if (phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN)
            {
                requirements.DurationMilliseconds = std::max(1u, settings.SustainLoopDurationMilliseconds);
                requirements.ExactFrameCount = std::max(1u, settings.SustainFrameCount);
                requirements.MinimumFrameCount = std::max(1u, settings.MinimumSustainFrameCount);
            }
            else
            {
                requirements.DurationMilliseconds = std::max(1u, phase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER ? settings.EnterDurationMilliseconds : settings.ExitDurationMilliseconds);
                requirements.ExactFrameCount = std::max(2u, settings.TransitionFrameCount);
                requirements.MinimumFrameCount = std::max(2u, settings.MinimumTransitionFrameCount);
            }
            if (activeComponentCount == 0u && settings.SamplingMode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE) { requirements.MinimumFrameCount = 1u; }
            requirements.MaximumFrameCount = std::max(requirements.MinimumFrameCount, requirements.MaximumFrameCount);
        };

        configure(plan.EnterSampling, PixelShipGenerator::ShipMovementAnimationPhase::ENTER);
        configure(plan.SustainSampling, PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN);
        configure(plan.ExitSampling, PixelShipGenerator::ShipMovementAnimationPhase::EXIT);
    }

    LongitudinalMovementPlan createLongitudinalMovementPlan(const PixelShipGenerator::GeneratedShip& ship, PixelShipGenerator::ShipAnimationType type, const PixelShipGenerator::ShipMovementAnimationSettings& settings, uint64_t seed)
    {
        if (!isLongitudinalAnimationType(type)) { throw std::invalid_argument("ShipLongitudinalMovementAnimator requires MOVE_UP or MOVE_DOWN."); }
        LongitudinalMovementPlan plan;
        plan.Type = type;
        plan.DirectionSignY = type == PixelShipGenerator::ShipAnimationType::MOVE_UP ? -1 : 1;
        plan.Seed = seed;
        plan.Profile = getLongitudinalMovementProfile(ship);

        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.FinalImage.getWidth(), ship.FinalImage.getHeight() });
        PixelShipGenerator::PixelMask reservedMotionMask(ship.FinalImage.getWidth(), ship.FinalImage.getHeight(), false);
        addEngineResponses(ship, settings, scaleTraits, plan);
        addWeaponGroups(ship, settings, scaleTraits, plan, reservedMotionMask);
        addAttachmentGroups(ship, settings, scaleTraits, plan, reservedMotionMask);
        finalizeDiagnosticsAndSampling(settings, scaleTraits, plan);
        return plan;
    }

    double getGroupResponse(const PixelShipGenerator::GeneratedShip& ship, const LongitudinalMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, double phaseOffset)
    {
        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN)
        {
            const double activity = sampleAnchoredSustainActivity(normalizedTime, phaseOffset);
            const double minimum = plan.Profile.HeavyResponse ? 0.76 : 0.64;
            return 1.0 - (1.0 - minimum) * activity;
        }
        return sampleTransitionResponse(ship, plan.Profile, phase, normalizedTime, phaseOffset);
    }

    void applyMovableGroup(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const MovableGroup& group, int32_t offsetX, int32_t offsetY)
    {
        if ((offsetX == 0 && offsetY == 0) || group.SourcePixels.empty()) { return; }
        std::vector<PixelShipGenerator::Color> colors;
        colors.reserve(group.SourcePixels.size());
        for (const PixelCoordinate& source : group.SourcePixels) { colors.push_back(ship.FinalImage.getPixel(source.X, source.Y)); }
        for (const PixelCoordinate& source : group.SourcePixels) { frame.setPixel(source.X, source.Y, PixelShipGenerator::Color()); }
        for (std::size_t index = 0u; index < group.SourcePixels.size(); ++index)
        {
            const PixelCoordinate& source = group.SourcePixels[index];
            const uint32_t destinationX = static_cast<uint32_t>(static_cast<int32_t>(source.X) + offsetX);
            const uint32_t destinationY = static_cast<uint32_t>(static_cast<int32_t>(source.Y) + offsetY);
            frame.setPixel(destinationX, destinationY, colors[index]);
        }
    }

    void applyEngineHousingPosture(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const LongitudinalMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, const PixelShipGenerator::ShipMovementAnimationSettings& settings)
    {
        if (!settings.EnginePropulsionResponse || !plan.Diagnostics.ActiveEngineCount) { return; }
        for (const EngineResponseParameters& parameters : plan.Engines)
        {
            if (parameters.EngineIndex >= ship.IdleAnimationMetadata.EngineComponents.size()) { continue; }
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[parameters.EngineIndex];
            const double response = phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN ? 1.0 : sampleTransitionResponse(ship, plan.Profile, phase, normalizedTime, parameters.SustainPhaseOffset);
            if (response < 0.35) { continue; }
            const uint32_t startY = component.NozzleY > 0u ? component.NozzleY - 1u : component.NozzleY;
            for (uint32_t y = startY; y <= component.NozzleY && y < ship.EngineMask.getHeight(); ++y)
            {
                for (uint32_t x = component.HousingStartX; x < component.HousingStartX + component.HousingWidth && x < ship.EngineMask.getWidth(); ++x)
                {
                    if (!ship.EngineMask.get(x, y) || ship.HullMask.get(x, y)) { continue; }
                    if (plan.Type == PixelShipGenerator::ShipAnimationType::MOVE_UP)
                    {
                        frame.setPixel(x, y, x >= component.NozzleStartX && x < component.NozzleStartX + component.NozzleWidth ? ship.Palette.EngineHighlight : ship.Palette.EngineBase);
                    }
                    else
                    {
                        frame.setPixel(x, y, ship.Palette.EngineDark);
                    }
                }
            }
        }
    }

    uint32_t getEngineTargetLength(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component, const EngineResponseParameters& parameters, const LongitudinalMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime)
    {
        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN)
        {
            uint32_t length = parameters.EnteredExhaustLength;
            const uint32_t variation = static_cast<uint32_t>(std::floor(sampleAnchoredSustainActivity(normalizedTime, parameters.SustainPhaseOffset) * static_cast<double>(parameters.SustainVariationPixels) + 0.5));
            if (plan.Type == PixelShipGenerator::ShipAnimationType::MOVE_UP) { length = std::min(component.MaximumExhaustLength, length + variation); }
            else { length = length > component.MinimumExhaustLength + variation ? length - variation : component.MinimumExhaustLength; }
            return length;
        }
        const double response = sampleTransitionResponse(ship, plan.Profile, phase, normalizedTime, parameters.SustainPhaseOffset);
        return interpolateLength(component.ExhaustLength, parameters.EnteredExhaustLength, response);
    }

    void applyEnginePropulsion(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const LongitudinalMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, const PixelShipGenerator::ShipMovementAnimationSettings& settings)
    {
        if (!settings.ExhaustResponse) { return; }
        for (const EngineResponseParameters& parameters : plan.Engines)
        {
            if (parameters.EngineIndex >= ship.IdleAnimationMetadata.EngineComponents.size()) { continue; }
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[parameters.EngineIndex];
            const uint32_t targetLength = getEngineTargetLength(ship, component, parameters, plan, phase, normalizedTime);
            const double response = phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN ? 1.0 : sampleTransitionResponse(ship, plan.Profile, phase, normalizedTime, parameters.SustainPhaseOffset);
            redrawEngineExhaust(frame, ship, component, targetLength, plan.Type == PixelShipGenerator::ShipAnimationType::MOVE_UP, response);
        }
    }

    PixelShipGenerator::Image evaluateMovementFrame(const PixelShipGenerator::GeneratedShip& ship, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, const LongitudinalMovementPlan& plan, const PixelShipGenerator::ShipMovementAnimationSettings& settings)
    {
        const double time = phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN ? wrapNormalizedTime(normalizedTime) : clampNormalizedTime(normalizedTime);
        PixelShipGenerator::Image frame = ship.FinalImage;
        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER && time <= 0.0) { return frame; }
        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::EXIT && time >= 1.0) { return frame; }

        applyEnginePropulsion(frame, ship, plan, phase, time, settings);
        for (const MovableGroup& group : plan.Groups)
        {
            const double response = getGroupResponse(ship, plan, phase, time, group.SustainPhaseOffset);
            applyMovableGroup(frame, ship, group, quantizeOffset(group.MaximumOffsetX, response), quantizeOffset(group.MaximumOffsetY, response));
        }
        applyEngineHousingPosture(frame, ship, plan, phase, time, settings);
        return frame;
    }

    PixelShipGenerator::ShipMovementAnimationClip generateClip(const PixelShipGenerator::GeneratedShip& ship, const LongitudinalMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, const PixelShipGenerator::AnimationSamplingRequirements& requirements, const PixelShipGenerator::ShipMovementAnimationSettings& settings)
    {
        PixelShipGenerator::ShipMovementAnimationClip clip;
        clip.Type = plan.Type;
        clip.Phase = phase;
        clip.Looping = phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
        clip.FrameWidth = ship.FinalImage.getWidth();
        clip.FrameHeight = ship.FinalImage.getHeight();
        PixelShipGenerator::AnimationSamplingPlanner samplingPlanner;
        clip.Sampling = samplingPlanner.plan(requirements);
        clip.DurationMilliseconds = clip.Sampling.DurationMilliseconds;
        clip.FrameDurationMilliseconds = clip.Sampling.ActualFrameDurationMilliseconds;
        clip.Frames.reserve(clip.Sampling.ActualFrameCount);
        clip.NormalizedSampleTimes.reserve(clip.Sampling.ActualFrameCount);

        for (uint32_t frameIndex = 0u; frameIndex < clip.Sampling.ActualFrameCount; ++frameIndex)
        {
            double normalizedTime = 0.0;
            if (clip.Looping) { normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(clip.Sampling.ActualFrameCount); }
            else if (clip.Sampling.ActualFrameCount > 1u) { normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(clip.Sampling.ActualFrameCount - 1u); }
            clip.NormalizedSampleTimes.push_back(normalizedTime);
            clip.Frames.push_back(evaluateMovementFrame(ship, phase, normalizedTime, plan, settings));
        }
        return clip;
    }
}

namespace PixelShipGenerator
{
    ShipMovementAnimation ShipLongitudinalMovementAnimator::generate(const GeneratedShip& ship, ShipAnimationType type, const ShipMovementAnimationSettings& settings) const
    {
        if (!isLongitudinalAnimationType(type)) { throw std::invalid_argument("ShipLongitudinalMovementAnimator requires MOVE_UP or MOVE_DOWN."); }
        ShipMovementAnimation animation;
        animation.Type = type;
        animation.Seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ LongitudinalMovementSeedSalt);
        const LongitudinalMovementPlan movementPlan = createLongitudinalMovementPlan(ship, type, settings, animation.Seed);
        animation.Diagnostics = movementPlan.Diagnostics;
        animation.Enter = generateClip(ship, movementPlan, ShipMovementAnimationPhase::ENTER, movementPlan.EnterSampling, settings);
        animation.Sustain = generateClip(ship, movementPlan, ShipMovementAnimationPhase::SUSTAIN, movementPlan.SustainSampling, settings);
        animation.Exit = generateClip(ship, movementPlan, ShipMovementAnimationPhase::EXIT, movementPlan.ExitSampling, settings);
        return animation;
    }

    Image ShipLongitudinalMovementAnimator::evaluateFrameAtNormalizedTime(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings) const
    {
        if (!isLongitudinalAnimationType(type)) { throw std::invalid_argument("ShipLongitudinalMovementAnimator requires MOVE_UP or MOVE_DOWN."); }
        const uint64_t seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ LongitudinalMovementSeedSalt);
        const LongitudinalMovementPlan movementPlan = createLongitudinalMovementPlan(ship, type, settings, seed);
        return evaluateMovementFrame(ship, phase, normalizedTime, movementPlan, settings);
    }
}
