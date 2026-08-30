#include "ShipLateralMovementAnimator.h"

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
    constexpr uint64_t LateralMovementSeedSalt = 0x53A9B4E08C7D21F5ull;
    constexpr uint64_t EnginePhaseSalt = 0x1A47B9C83E5D602Full;
    constexpr uint64_t WeaponPhaseSalt = 0x9C6F21D4A7B835E1ull;
    constexpr uint64_t AttachmentPhaseSalt = 0xE25D8A31C470B69Full;

    struct PixelCoordinate
    {
        uint32_t X = 0u;
        uint32_t Y = 0u;
    };

    struct MovableGroup
    {
        PixelShipGenerator::ShipMovementAnimatedComponentType Type = PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_VECTORING;
        uint32_t SemanticGroup = 0u;
        uint32_t SourceComponentIndex = std::numeric_limits<uint32_t>::max();
        std::vector<PixelCoordinate> SourcePixels;
        int32_t MaximumOffsetX = 0;
        double SustainPhaseOffset = 0.0;
    };

    struct LateralMovementProfile
    {
        uint32_t ResponseStrengthPercent = 100u;
        uint32_t EngineTravelLimit = 2u;
        uint32_t WeaponTravelLimit = 1u;
        uint32_t AttachmentTravelLimit = 1u;
        bool Synchronized = false;
        bool Staggered = false;
        bool HeavyResponse = false;
        bool Responsive = false;
    };

    struct LateralMovementPlan
    {
        PixelShipGenerator::ShipAnimationType Type = PixelShipGenerator::ShipAnimationType::MOVE_LEFT;
        int32_t DirectionSignX = -1;
        int32_t CompensationSignX = 1;
        uint64_t Seed = 0u;
        LateralMovementProfile Profile;
        std::vector<MovableGroup> Groups;
        PixelShipGenerator::ShipMovementAnimationDiagnostics Diagnostics;
        PixelShipGenerator::AnimationSamplingRequirements EnterSampling;
        PixelShipGenerator::AnimationSamplingRequirements SustainSampling;
        PixelShipGenerator::AnimationSamplingRequirements ExitSampling;
    };

    bool isLateralAnimationType(PixelShipGenerator::ShipAnimationType type)
    {
        return type == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || type == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT;
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

    double sampleProfileTransitionResponse(const PixelShipGenerator::GeneratedShip& ship, const LateralMovementProfile& profile, double value)
    {
        double t = std::clamp(value, 0.0, 1.0);
        if (ship.Faction == PixelShipGenerator::ShipFactionType::RELIC) { t *= t; }

        if (profile.Responsive) { return easeOutCubic(t); }
        if (profile.HeavyResponse) { return smoothStep(t * t); }
        return smoothStep(t);
    }

    double sampleTransitionResponse(const PixelShipGenerator::GeneratedShip& ship, const LateralMovementProfile& profile, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, double phaseOffset)
    {
        const double time = clampNormalizedTime(normalizedTime);
        const double maximumDelay = profile.Synchronized ? 0.0 : profile.Staggered ? 0.16 : 0.08;
        const double delay = std::clamp(phaseOffset, 0.0, 1.0) * maximumDelay;

        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER)
        {
            if (time <= delay) { return 0.0; }
            const double local = (time - delay) / std::max(0.000001, 1.0 - delay);
            return sampleProfileTransitionResponse(ship, profile, local);
        }

        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::EXIT)
        {
            if (time >= 1.0) { return 0.0; }
            const double activeDuration = std::max(0.000001, 1.0 - delay);
            const double local = std::clamp(time / activeDuration, 0.0, 1.0);
            return 1.0 - sampleProfileTransitionResponse(ship, profile, local);
        }

        return 1.0;
    }

    double sampleSustainResponse(double normalizedTime, double phaseOffset, bool heavyResponse)
    {
        const double time = wrapNormalizedTime(normalizedTime);
        const double envelope = time < 0.5 ? time * 2.0 : (1.0 - time) * 2.0;
        const double phasedTime = wrapNormalizedTime(time + phaseOffset);
        const double phaseWave = phasedTime < 0.5 ? phasedTime * 2.0 : (1.0 - phasedTime) * 2.0;
        const double activity = std::clamp(envelope * (0.65 + phaseWave * 0.35), 0.0, 1.0);
        const double minimum = heavyResponse ? 0.72 : 0.58;
        return 1.0 - (1.0 - minimum) * smoothStep(activity);
    }

    int32_t quantizeOffset(int32_t maximumOffset, double response)
    {
        if (maximumOffset == 0) { return 0; }
        const double magnitude = std::clamp(response, 0.0, 1.0) * static_cast<double>(std::abs(maximumOffset));
        const int32_t rounded = static_cast<int32_t>(std::floor(magnitude + 0.5));
        return maximumOffset < 0 ? -rounded : rounded;
    }

    LateralMovementProfile getLateralMovementProfile(const PixelShipGenerator::GeneratedShip& ship)
    {
        LateralMovementProfile profile;
        const PixelShipGenerator::ShipLateralMovementAnimationTraits& traits = ship.AnimationTraits.LateralMovement;
        profile.ResponseStrengthPercent = traits.ResponseStrengthPercent;
        profile.EngineTravelLimit = traits.EngineTravelLimit;
        profile.WeaponTravelLimit = traits.WeaponTravelLimit;
        profile.AttachmentTravelLimit = traits.AttachmentTravelLimit;
        profile.Synchronized = traits.Synchronized;
        profile.Staggered = traits.Staggered;
        profile.HeavyResponse = traits.HeavyResponse;
        profile.Responsive = traits.Responsive;

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

    bool containsPixel(const std::vector<PixelCoordinate>& pixels, uint32_t x, uint32_t y)
    {
        return std::find_if(pixels.begin(), pixels.end(), [&](const PixelCoordinate& pixel) { return pixel.X == x && pixel.Y == y; }) != pixels.end();
    }

    bool isMaskPixel(const PixelShipGenerator::PixelMask& mask, int32_t x, int32_t y)
    {
        return x >= 0 && y >= 0 && x < static_cast<int32_t>(mask.getWidth()) && y < static_cast<int32_t>(mask.getHeight()) && mask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
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

    bool isGroupOffsetSafe(const PixelShipGenerator::GeneratedShip& ship, const MovableGroup& group, int32_t offsetX, const PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (offsetX == 0) { return true; }

        for (const PixelCoordinate& source : group.SourcePixels)
        {
            const int32_t destinationX = static_cast<int32_t>(source.X) + offsetX;
            const int32_t destinationY = static_cast<int32_t>(source.Y);
            if (destinationX < 0 || destinationY < 0 || destinationX >= static_cast<int32_t>(ship.FinalImage.getWidth()) || destinationY >= static_cast<int32_t>(ship.FinalImage.getHeight())) { return false; }

            const uint32_t x = static_cast<uint32_t>(destinationX);
            const uint32_t y = static_cast<uint32_t>(destinationY);
            if (isStaticOccupiedOutsideGroup(ship, group.SourcePixels, x, y)) { return false; }
            if (reservedMotionMask.get(x, y) && !containsPixel(group.SourcePixels, x, y)) { return false; }
            if (ship.FinalImage.getPixel(x, y).A == 0u && isLikelyStructuralVoid(ship, x, y)) { return false; }
        }

        return true;
    }

    uint32_t findSafeTravel(const PixelShipGenerator::GeneratedShip& ship, MovableGroup& group, int32_t directionSign, uint32_t desiredTravel, const PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        uint32_t safeTravel = 0u;
        for (uint32_t travel = 1u; travel <= desiredTravel; ++travel)
        {
            if (!isGroupOffsetSafe(ship, group, directionSign * static_cast<int32_t>(travel), reservedMotionMask)) { break; }
            safeTravel = travel;
        }
        return safeTravel;
    }

    void reserveGroupMotion(const MovableGroup& group, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        const int32_t maximumOffset = group.MaximumOffsetX;
        const int32_t direction = maximumOffset < 0 ? -1 : 1;
        const uint32_t travel = static_cast<uint32_t>(std::abs(maximumOffset));

        for (const PixelCoordinate& source : group.SourcePixels)
        {
            reservedMotionMask.set(source.X, source.Y, true);
            for (uint32_t step = 1u; step <= travel; ++step)
            {
                const int32_t x = static_cast<int32_t>(source.X) + direction * static_cast<int32_t>(step);
                if (x >= 0 && x < static_cast<int32_t>(reservedMotionMask.getWidth())) { reservedMotionMask.set(static_cast<uint32_t>(x), source.Y, true); }
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

    double getPhaseOffset(uint64_t hash, const LateralMovementProfile& profile)
    {
        if (profile.Synchronized) { return 0.0; }
        if (profile.Staggered) { return static_cast<double>(hash % 4ull) * 0.125; }
        return (hash & 1ull) == 0ull ? 0.0 : 0.125;
    }

    uint32_t getCanonicalX(uint32_t width, uint32_t x)
    {
        return std::min(x, width - 1u - x);
    }

    std::vector<std::vector<PixelCoordinate>> collectEngineExhaustPixels(const PixelShipGenerator::GeneratedShip& ship)
    {
        const std::size_t engineCount = ship.IdleAnimationMetadata.EngineComponents.size();
        std::vector<std::vector<PixelCoordinate>> pixels(engineCount);
        if (engineCount == 0u) { return pixels; }

        std::vector<uint32_t> centerXTimesTwo(engineCount, 0u);
        for (std::size_t index = 0u; index < engineCount; ++index)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[index];
            centerXTimesTwo[index] = component.NozzleStartX * 2u + component.NozzleWidth - 1u;
        }

        for (uint32_t y = 0u; y < ship.EngineExhaustMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.EngineExhaustMask.getWidth(); ++x)
            {
                if (!ship.EngineExhaustMask.get(x, y)) { continue; }
                if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y) || (!ship.IdleAnimationMetadata.WeaponOccupiedMask.empty() && ship.IdleAnimationMetadata.WeaponOccupiedMask.get(x, y))) { continue; }

                std::size_t bestIndex = 0u;
                uint32_t bestDistance = std::numeric_limits<uint32_t>::max();
                const uint32_t pixelXTimesTwo = x * 2u;
                for (std::size_t engineIndex = 0u; engineIndex < engineCount; ++engineIndex)
                {
                    const uint32_t distance = pixelXTimesTwo > centerXTimesTwo[engineIndex] ? pixelXTimesTwo - centerXTimesTwo[engineIndex] : centerXTimesTwo[engineIndex] - pixelXTimesTwo;
                    if (distance < bestDistance)
                    {
                        bestDistance = distance;
                        bestIndex = engineIndex;
                    }
                }
                pixels[bestIndex].push_back({ x, y });
            }
        }

        return pixels;
    }

    bool isArticulatingAttachmentType(PixelShipGenerator::ShipAttachmentType type)
    {
        return type == PixelShipGenerator::ShipAttachmentType::SENSOR_ARRAY || type == PixelShipGenerator::ShipAttachmentType::AUXILIARY_POD || type == PixelShipGenerator::ShipAttachmentType::RADIATOR || type == PixelShipGenerator::ShipAttachmentType::ARMOR_FIN;
    }

    std::vector<PixelCoordinate> collectAttachmentPixels(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipAttachmentPlacement& placement)
    {
        std::vector<PixelCoordinate> result;
        for (uint32_t y = placement.MinimumY; y <= placement.MaximumY && y < ship.AttachmentMask.getHeight(); ++y)
        {
            for (uint32_t x = placement.MinimumX; x <= placement.MaximumX && x < ship.AttachmentMask.getWidth(); ++x)
            {
                if (ship.AttachmentMask.get(x, y)) { result.push_back({ x, y }); }
            }
        }
        return result;
    }

    std::vector<PixelCoordinate> collectWeaponPixels(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipWeaponAnimationComponent& component)
    {
        std::vector<PixelCoordinate> result;
        if (!component.MovableBarrel) { return result; }

        for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.IdleAnimationMetadata.WeaponMovableMask.getHeight(); ++y)
        {
            for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.IdleAnimationMetadata.WeaponMovableMask.getWidth(); ++x)
            {
                if (ship.IdleAnimationMetadata.WeaponMovableMask.get(x, y)) { result.push_back({ x, y }); }
            }
        }
        return result;
    }


    bool weaponSourcePixelsAreMovable(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& pixels)
    {
        for (const PixelCoordinate& pixel : pixels)
        {
            if (ship.HullMask.get(pixel.X, pixel.Y) || ship.CockpitMask.get(pixel.X, pixel.Y) || ship.EngineMask.get(pixel.X, pixel.Y) || ship.AttachmentMask.get(pixel.X, pixel.Y)) { return false; }
        }
        return true;
    }

    bool attachmentSourcePixelsAreMovable(const PixelShipGenerator::GeneratedShip& ship, const std::vector<PixelCoordinate>& pixels)
    {
        for (const PixelCoordinate& pixel : pixels)
        {
            if (ship.HullMask.get(pixel.X, pixel.Y) || ship.CockpitMask.get(pixel.X, pixel.Y) || ship.EngineMask.get(pixel.X, pixel.Y)) { return false; }
            if (!ship.IdleAnimationMetadata.WeaponOccupiedMask.empty() && ship.IdleAnimationMetadata.WeaponOccupiedMask.get(pixel.X, pixel.Y)) { return false; }
        }
        return true;
    }

    void addDiagnosticComponent(LateralMovementPlan& plan, const MovableGroup& group)
    {
        PixelShipGenerator::ShipMovementComponentDiagnostic diagnostic;
        diagnostic.Type = group.Type;
        diagnostic.SemanticGroup = group.SemanticGroup;
        diagnostic.SourcePixelCount = static_cast<uint32_t>(group.SourcePixels.size());
        diagnostic.MaximumOffsetX = group.MaximumOffsetX;
        diagnostic.MaximumOffsetY = 0;
        diagnostic.MaximumExhaustLengthDelta = group.Type == PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_VECTORING ? static_cast<uint32_t>(std::abs(group.MaximumOffsetX)) : 0u;
        diagnostic.SustainPhaseOffset = group.SustainPhaseOffset;
        plan.Diagnostics.Components.push_back(diagnostic);
        plan.Diagnostics.MaximumMechanicalTravelPixels = std::max(plan.Diagnostics.MaximumMechanicalTravelPixels, static_cast<uint32_t>(std::abs(group.MaximumOffsetX)));
    }

    void addEngineGroups(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LateralMovementPlan& plan, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (!settings.EngineVectoring) { return; }

        const std::vector<std::vector<PixelCoordinate>> exhaustPixels = collectEngineExhaustPixels(ship);
        const uint32_t availableTravel = getScaleTravelCapacity(scaleTraits);
        const uint32_t desiredTravel = scaleTravelForProfile(availableTravel, plan.Profile.EngineTravelLimit, plan.Profile.ResponseStrengthPercent);
        const uint32_t width = ship.FinalImage.getWidth();

        for (std::size_t engineIndex = 0u; engineIndex < ship.IdleAnimationMetadata.EngineComponents.size(); ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            ++plan.Diagnostics.ActiveEngineCount;

            if (engineIndex >= exhaustPixels.size() || exhaustPixels[engineIndex].empty() || desiredTravel == 0u) { continue; }

            MovableGroup group;
            group.Type = PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_VECTORING;
            group.SemanticGroup = static_cast<uint32_t>(engineIndex);
            group.SourceComponentIndex = static_cast<uint32_t>(engineIndex);
            group.SourcePixels = exhaustPixels[engineIndex];
            const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
            const uint64_t hash = getAnimationHash(plan.Seed, getCanonicalX(width, centerX), component.NozzleY, EnginePhaseSalt);
            group.SustainPhaseOffset = getPhaseOffset(hash, plan.Profile);

            const uint32_t safeTravel = findSafeTravel(ship, group, plan.CompensationSignX, desiredTravel, reservedMotionMask);
            if (safeTravel == 0u) { continue; }
            group.MaximumOffsetX = plan.CompensationSignX * static_cast<int32_t>(safeTravel);
            reserveGroupMotion(group, reservedMotionMask);
            addDiagnosticComponent(plan, group);
            plan.Groups.push_back(std::move(group));
        }
    }

    void addWeaponGroups(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LateralMovementPlan& plan, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (!settings.WeaponStabilization) { return; }

        const uint32_t availableTravel = getScaleTravelCapacity(scaleTraits);
        const uint32_t desiredTravel = scaleTravelForProfile(availableTravel, plan.Profile.WeaponTravelLimit, plan.Profile.ResponseStrengthPercent);
        const uint32_t width = ship.FinalImage.getWidth();

        for (std::size_t weaponIndex = 0u; weaponIndex < ship.IdleAnimationMetadata.WeaponComponents.size(); ++weaponIndex)
        {
            const PixelShipGenerator::ShipWeaponAnimationComponent& component = ship.IdleAnimationMetadata.WeaponComponents[weaponIndex];
            MovableGroup group;
            group.Type = PixelShipGenerator::ShipMovementAnimatedComponentType::WEAPON_STABILIZATION;
            group.SemanticGroup = component.SymmetryGroup != 0u ? component.SymmetryGroup : static_cast<uint32_t>(weaponIndex + 1u);
            group.SourceComponentIndex = static_cast<uint32_t>(weaponIndex);
            group.SourcePixels = collectWeaponPixels(ship, component);
            if (group.SourcePixels.empty() || desiredTravel == 0u || !weaponSourcePixelsAreMovable(ship, group.SourcePixels)) { continue; }

            const uint32_t canonicalX = getCanonicalX(width, component.AnchorX);
            const uint64_t hash = getAnimationHash(plan.Seed, canonicalX, component.AnchorY, WeaponPhaseSalt ^ static_cast<uint64_t>(group.SemanticGroup));
            group.SustainPhaseOffset = getPhaseOffset(hash, plan.Profile);
            const uint32_t safeTravel = findSafeTravel(ship, group, plan.CompensationSignX, desiredTravel, reservedMotionMask);
            if (safeTravel == 0u) { continue; }

            group.MaximumOffsetX = plan.CompensationSignX * static_cast<int32_t>(safeTravel);
            reserveGroupMotion(group, reservedMotionMask);
            addDiagnosticComponent(plan, group);
            plan.Groups.push_back(std::move(group));
            ++plan.Diagnostics.ActiveWeaponCount;
        }
    }

    void addAttachmentGroups(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LateralMovementPlan& plan, PixelShipGenerator::PixelMask& reservedMotionMask)
    {
        if (!settings.AttachmentArticulation) { return; }

        const uint32_t availableTravel = getScaleTravelCapacity(scaleTraits);
        const uint32_t desiredTravel = scaleTravelForProfile(availableTravel, plan.Profile.AttachmentTravelLimit, plan.Profile.ResponseStrengthPercent);
        if (desiredTravel == 0u) { return; }

        const uint32_t width = ship.FinalImage.getWidth();
        const int32_t centerXTimesTwo = static_cast<int32_t>(width - 1u);

        for (std::size_t placementIndex = 0u; placementIndex < ship.AttachmentPlacements.size(); ++placementIndex)
        {
            const PixelShipGenerator::ShipAttachmentPlacement& placement = ship.AttachmentPlacements[placementIndex];
            if (!isArticulatingAttachmentType(placement.Type)) { continue; }

            const int32_t anchorSide = static_cast<int32_t>(placement.AnchorX * 2u) - centerXTimesTwo;
            if (anchorSide == 0 || (anchorSide > 0 ? 1 : -1) != plan.CompensationSignX) { continue; }

            MovableGroup group;
            group.Type = PixelShipGenerator::ShipMovementAnimatedComponentType::ATTACHMENT_ARTICULATION;
            group.SemanticGroup = placement.SymmetryGroup != 0u ? placement.SymmetryGroup : static_cast<uint32_t>(placementIndex + 1u);
            group.SourceComponentIndex = static_cast<uint32_t>(placementIndex);
            group.SourcePixels = collectAttachmentPixels(ship, placement);
            if (group.SourcePixels.empty() || !attachmentSourcePixelsAreMovable(ship, group.SourcePixels)) { continue; }

            const uint32_t canonicalX = getCanonicalX(width, placement.AnchorX);
            const uint64_t hash = getAnimationHash(plan.Seed, canonicalX, placement.AnchorY, AttachmentPhaseSalt ^ static_cast<uint64_t>(group.SemanticGroup));
            group.SustainPhaseOffset = getPhaseOffset(hash, plan.Profile);
            const uint32_t safeTravel = findSafeTravel(ship, group, plan.CompensationSignX, desiredTravel, reservedMotionMask);
            if (safeTravel == 0u) { continue; }

            group.MaximumOffsetX = plan.CompensationSignX * static_cast<int32_t>(safeTravel);
            reserveGroupMotion(group, reservedMotionMask);
            addDiagnosticComponent(plan, group);
            plan.Groups.push_back(std::move(group));
            ++plan.Diagnostics.ActiveAttachmentCount;
        }
    }

    void finalizeDiagnosticsAndSampling(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipMovementAnimationSettings& settings, const PixelShipGenerator::GenerationScaleTraits& scaleTraits, LateralMovementPlan& plan)
    {
        plan.Diagnostics.DirectionSignX = plan.DirectionSignX;
        plan.Diagnostics.DirectionSignY = 0;

        std::vector<double> phaseOffsets;
        for (const MovableGroup& group : plan.Groups)
        {
            if (std::find(phaseOffsets.begin(), phaseOffsets.end(), group.SustainPhaseOffset) == phaseOffsets.end()) { phaseOffsets.push_back(group.SustainPhaseOffset); }
        }
        if (settings.EngineVectoring && plan.Diagnostics.ActiveEngineCount > 0u && phaseOffsets.empty()) { phaseOffsets.push_back(0.0); }
        plan.Diagnostics.IndependentPhaseGroupCount = static_cast<uint32_t>(phaseOffsets.size());

        uint32_t maximumExhaustTravel = 0u;
        uint32_t maximumOtherTravel = 0u;
        for (const MovableGroup& group : plan.Groups)
        {
            const uint32_t travel = static_cast<uint32_t>(std::abs(group.MaximumOffsetX));
            if (group.Type == PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_VECTORING) { maximumExhaustTravel = std::max(maximumExhaustTravel, travel); }
            else { maximumOtherTravel = std::max(maximumOtherTravel, travel); }
        }

        plan.Diagnostics.MaximumExhaustTravelPixels = maximumExhaustTravel;
        const uint32_t activeComponentCount = plan.Diagnostics.ActiveEngineCount + plan.Diagnostics.ActiveWeaponCount + plan.Diagnostics.ActiveAttachmentCount;
        const uint32_t phaseGroupCount = std::max(1u, plan.Diagnostics.IndependentPhaseGroupCount);

        auto configure = [&](PixelShipGenerator::AnimationSamplingRequirements& requirements, PixelShipGenerator::ShipMovementAnimationPhase phase)
            {
                requirements.Type = plan.Type;
                requirements.Mode = settings.SamplingMode;
                requirements.ScaleAnimationComplexity = scaleTraits.AnimationComplexity;
                requirements.MaximumMechanicalTravelPixels = maximumOtherTravel;
                requirements.MaximumExhaustTravelPixels = maximumExhaustTravel;
                requirements.ActiveAnimatedComponentCount = activeComponentCount;
                requirements.IndependentPhaseGroupCount = activeComponentCount > 0u ? phaseGroupCount : 0u;
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
                if (activeComponentCount == 0u && settings.SamplingMode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE)
                {
                    requirements.MinimumFrameCount = 1u;
                }
                requirements.MaximumFrameCount = std::max(requirements.MinimumFrameCount, requirements.MaximumFrameCount);
            };

        configure(plan.EnterSampling, PixelShipGenerator::ShipMovementAnimationPhase::ENTER);
        configure(plan.SustainSampling, PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN);
        configure(plan.ExitSampling, PixelShipGenerator::ShipMovementAnimationPhase::EXIT);
    }

    LateralMovementPlan createLateralMovementPlan(const PixelShipGenerator::GeneratedShip& ship, PixelShipGenerator::ShipAnimationType type, const PixelShipGenerator::ShipMovementAnimationSettings& settings, uint64_t seed)
    {
        if (!isLateralAnimationType(type)) { throw std::invalid_argument("ShipLateralMovementAnimator requires MOVE_LEFT or MOVE_RIGHT."); }

        LateralMovementPlan plan;
        plan.Type = type;
        plan.DirectionSignX = type == PixelShipGenerator::ShipAnimationType::MOVE_LEFT ? -1 : 1;
        plan.CompensationSignX = -plan.DirectionSignX;
        plan.Seed = seed;
        plan.Profile = getLateralMovementProfile(ship);

        const PixelShipGenerator::GenerationScaleTraits scaleTraits = PixelShipGenerator::GenerationScaleTraits::fromDimensions({ ship.FinalImage.getWidth(), ship.FinalImage.getHeight() });
        PixelShipGenerator::PixelMask reservedMotionMask(ship.FinalImage.getWidth(), ship.FinalImage.getHeight(), false);

        addEngineGroups(ship, settings, scaleTraits, plan, reservedMotionMask);
        addWeaponGroups(ship, settings, scaleTraits, plan, reservedMotionMask);
        addAttachmentGroups(ship, settings, scaleTraits, plan, reservedMotionMask);
        finalizeDiagnosticsAndSampling(ship, settings, scaleTraits, plan);
        return plan;
    }

    double getGroupResponse(const PixelShipGenerator::GeneratedShip& ship, const LateralMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, double phaseOffset)
    {
        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN) { return sampleSustainResponse(normalizedTime, phaseOffset, plan.Profile.HeavyResponse); }
        return sampleTransitionResponse(ship, plan.Profile, phase, normalizedTime, phaseOffset);
    }

    void applyMovableGroup(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const MovableGroup& group, int32_t offsetX)
    {
        if (offsetX == 0 || group.SourcePixels.empty()) { return; }

        std::vector<PixelShipGenerator::Color> colors;
        colors.reserve(group.SourcePixels.size());
        for (const PixelCoordinate& source : group.SourcePixels) { colors.push_back(ship.FinalImage.getPixel(source.X, source.Y)); }

        for (const PixelCoordinate& source : group.SourcePixels) { frame.setPixel(source.X, source.Y, PixelShipGenerator::Color()); }
        for (std::size_t index = 0u; index < group.SourcePixels.size(); ++index)
        {
            const PixelCoordinate& source = group.SourcePixels[index];
            const uint32_t destinationX = static_cast<uint32_t>(static_cast<int32_t>(source.X) + offsetX);
            frame.setPixel(destinationX, source.Y, colors[index]);
        }
    }

    void applyEngineVectoringPosture(PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const LateralMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime)
    {
        if (plan.Diagnostics.ActiveEngineCount == 0u) { return; }

        for (std::size_t engineIndex = 0u; engineIndex < ship.IdleAnimationMetadata.EngineComponents.size(); ++engineIndex)
        {
            const PixelShipGenerator::ShipEngineAnimationComponent& component = ship.IdleAnimationMetadata.EngineComponents[engineIndex];
            const uint32_t centerX = component.NozzleStartX + component.NozzleWidth / 2u;
            const uint32_t canonicalX = getCanonicalX(ship.FinalImage.getWidth(), centerX);
            const uint64_t hash = getAnimationHash(plan.Seed, canonicalX, component.NozzleY, EnginePhaseSalt);
            const double phaseOffset = getPhaseOffset(hash, plan.Profile);
            const double response = getGroupResponse(ship, plan, phase, normalizedTime, phaseOffset);
            if (response < 0.30) { continue; }

            const int32_t leadingX = plan.CompensationSignX > 0 ? static_cast<int32_t>(component.NozzleStartX + component.NozzleWidth - 1u) : static_cast<int32_t>(component.NozzleStartX);
            const int32_t trailingX = plan.CompensationSignX > 0 ? static_cast<int32_t>(component.NozzleStartX) : static_cast<int32_t>(component.NozzleStartX + component.NozzleWidth - 1u);
            const uint32_t minimumY = component.NozzleY > component.RootStartY ? component.NozzleY - 1u : component.NozzleY;

            for (uint32_t y = minimumY; y <= component.NozzleY && y < ship.EngineMask.getHeight(); ++y)
            {
                if (leadingX >= 0 && leadingX < static_cast<int32_t>(ship.EngineMask.getWidth()) && ship.EngineMask.get(static_cast<uint32_t>(leadingX), y)) { frame.setPixel(static_cast<uint32_t>(leadingX), y, ship.Palette.EngineHighlight); }
                if (trailingX >= 0 && trailingX < static_cast<int32_t>(ship.EngineMask.getWidth()) && ship.EngineMask.get(static_cast<uint32_t>(trailingX), y)) { frame.setPixel(static_cast<uint32_t>(trailingX), y, ship.Palette.EngineDark); }
            }
        }
    }

    PixelShipGenerator::ShipAnimationSemanticComponentType getPoseComponentType(PixelShipGenerator::ShipMovementAnimatedComponentType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_VECTORING:
        case PixelShipGenerator::ShipMovementAnimatedComponentType::ENGINE_PROPULSION:
            return PixelShipGenerator::ShipAnimationSemanticComponentType::ENGINE;
        case PixelShipGenerator::ShipMovementAnimatedComponentType::WEAPON_STABILIZATION:
            return PixelShipGenerator::ShipAnimationSemanticComponentType::WEAPON;
        case PixelShipGenerator::ShipMovementAnimatedComponentType::ATTACHMENT_ARTICULATION:
        case PixelShipGenerator::ShipMovementAnimatedComponentType::BRAKING_ARTICULATION:
        default:
            return PixelShipGenerator::ShipAnimationSemanticComponentType::ATTACHMENT;
        }
    }

    PixelShipGenerator::ShipAnimationPose evaluateMovementPose(const PixelShipGenerator::GeneratedShip& ship, PixelShipGenerator::ShipMovementAnimationPhase phase, double normalizedTime, const LateralMovementPlan& plan)
    {
        const double time = phase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN ? wrapNormalizedTime(normalizedTime) : clampNormalizedTime(normalizedTime);
        PixelShipGenerator::ShipAnimationPose pose;
        pose.Frame = ship.FinalImage;
        pose.Layer = PixelShipGenerator::ShipAnimationPoseLayer::MOVEMENT;
        pose.UnderlyingAnimationType = plan.Type;

        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER && time <= 0.0) { return pose; }
        if (phase == PixelShipGenerator::ShipMovementAnimationPhase::EXIT && time >= 1.0) { return pose; }

        for (const MovableGroup& group : plan.Groups)
        {
            const double response = getGroupResponse(ship, plan, phase, time, group.SustainPhaseOffset);
            const int32_t offsetX = quantizeOffset(group.MaximumOffsetX, response);
            applyMovableGroup(pose.Frame, ship, group, offsetX);
            if (group.SourceComponentIndex != std::numeric_limits<uint32_t>::max())
            {
                pose.ComponentTransforms.push_back({ getPoseComponentType(group.Type), group.SourceComponentIndex, offsetX, 0 });
            }
        }

        applyEngineVectoringPosture(pose.Frame, ship, plan, phase, time);
        return pose;
    }

    PixelShipGenerator::ShipMovementAnimationClip generateClip(const PixelShipGenerator::GeneratedShip& ship, const LateralMovementPlan& plan, PixelShipGenerator::ShipMovementAnimationPhase phase, const PixelShipGenerator::AnimationSamplingRequirements& requirements)
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
            if (clip.Looping)
            {
                normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(clip.Sampling.ActualFrameCount);
            }
            else if (clip.Sampling.ActualFrameCount > 1u)
            {
                normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(clip.Sampling.ActualFrameCount - 1u);
            }

            clip.NormalizedSampleTimes.push_back(normalizedTime);
            clip.Frames.push_back(evaluateMovementPose(ship, phase, normalizedTime, plan).Frame);
        }

        return clip;
    }
}

namespace PixelShipGenerator
{
    ShipMovementAnimation ShipLateralMovementAnimator::generate(const GeneratedShip& ship, ShipAnimationType type, const ShipMovementAnimationSettings& settings) const
    {
        if (!isLateralAnimationType(type)) { throw std::invalid_argument("ShipLateralMovementAnimator requires MOVE_LEFT or MOVE_RIGHT."); }

        ShipMovementAnimation animation;
        animation.Type = type;
        animation.Seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ LateralMovementSeedSalt);

        const LateralMovementPlan movementPlan = createLateralMovementPlan(ship, type, settings, animation.Seed);
        animation.Diagnostics = movementPlan.Diagnostics;
        animation.Enter = generateClip(ship, movementPlan, ShipMovementAnimationPhase::ENTER, movementPlan.EnterSampling);
        animation.Sustain = generateClip(ship, movementPlan, ShipMovementAnimationPhase::SUSTAIN, movementPlan.SustainSampling);
        animation.Exit = generateClip(ship, movementPlan, ShipMovementAnimationPhase::EXIT, movementPlan.ExitSampling);
        return animation;
    }

    Image ShipLateralMovementAnimator::evaluateFrameAtNormalizedTime(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings) const
    {
        if (!isLateralAnimationType(type)) { throw std::invalid_argument("ShipLateralMovementAnimator requires MOVE_LEFT or MOVE_RIGHT."); }
        const uint64_t seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ LateralMovementSeedSalt);
        const LateralMovementPlan movementPlan = createLateralMovementPlan(ship, type, settings, seed);
        return evaluateMovementPose(ship, phase, normalizedTime, movementPlan).Frame;
    }

    ShipAnimationPose ShipLateralMovementAnimator::evaluatePoseAtNormalizedTime(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings) const
    {
        if (!isLateralAnimationType(type)) { throw std::invalid_argument("ShipLateralMovementAnimator requires MOVE_LEFT or MOVE_RIGHT."); }
        const uint64_t seed = settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ LateralMovementSeedSalt);
        const LateralMovementPlan movementPlan = createLateralMovementPlan(ship, type, settings, seed);
        return evaluateMovementPose(ship, phase, normalizedTime, movementPlan);
    }
}
