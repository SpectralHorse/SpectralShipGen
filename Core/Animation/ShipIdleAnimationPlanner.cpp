#include "ShipIdleAnimationInternal.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "ShipGenerationSeeds.h"
#include "GenerationScaleTraits.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
namespace IdleAnimationInternal
{
    constexpr uint64_t AnimationSeedSalt = 0x4D595DF4D0F33173ull;
    constexpr uint64_t EngineVariationSalt = 0xC6BC279692B5CC83ull;
    constexpr uint64_t EngineMechanicalSalt = 0x8CB92BA72F3D8DD7ull;
    constexpr uint64_t LightVariationSalt = 0xD1B54A32D192ED03ull;
    constexpr uint64_t DetailVariationSalt = 0x94D049BB133111EBull;
    constexpr uint64_t MicroMovementSalt = 0xA24BAED4963EE407ull;
    constexpr uint64_t WeaponMovementSalt = 0xDB4F0B9175AE2165ull;
    constexpr uint64_t MajorFeatureSalt = 0x6C8E9CF570932BD5ull;

    uint64_t getAnimationHash(uint64_t seed, uint32_t x, uint32_t y, uint64_t salt)
    {
        uint64_t value = seed;
        value ^= static_cast<uint64_t>(x) * 0x9E3779B185EBCA87ull;
        value ^= static_cast<uint64_t>(y) * 0xC2B2AE3D27D4EB4Full;
        value ^= salt;
        return PixelShipGenerator::mixGenerationSeed64(value);
    }

    IdleAnimationProfile getIdleAnimationProfile(const PixelShipGenerator::GeneratedShip& ship)
    {
        IdleAnimationProfile profile;
        const PixelShipGenerator::ShipIdleAnimationTraits& traits = ship.AnimationTraits.Idle;
        profile.EnginePulseStrength = traits.EnginePulseStrength;
        profile.ExhaustAmplitudePercent = traits.ExhaustAmplitudePercent;
        profile.EngineMechanicalChance = traits.EngineMechanicalChance;
        profile.WeaponMechanicalChance = traits.WeaponMechanicalChance;
        profile.VentActivityChance = traits.VentActivityChance;
        profile.SynchronizeEngines = traits.SynchronizeEngines;
        profile.AsynchronousEngines = traits.AsynchronousEngines;
        profile.AlternateEnginePhases = traits.AlternateEnginePhases;
        profile.SlowMechanicalCycle = traits.SlowMechanicalCycle;

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

    uint32_t getPlannedMaximumExhaustTravel(const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t amplitudePercent)
    {
        const uint32_t extension = component.MaximumExhaustLength > component.ExhaustLength ? component.MaximumExhaustLength - component.ExhaustLength : 0u;
        const uint32_t contraction = component.ExhaustLength > component.MinimumExhaustLength ? component.ExhaustLength - component.MinimumExhaustLength : 0u;
        return std::max(getContinuousLengthDelta(extension, 1.0, amplitudePercent), getContinuousLengthDelta(contraction, 1.0, amplitudePercent));
    }

    uint64_t resolveIdleAnimationSeed(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings)
    {
        return settings.Seed.has_value() ? *settings.Seed : mixGenerationSeed64(ship.Seeds.Master ^ AnimationSeedSalt);
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
}
}
