#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "GeneratedShip.h"
#include "ShipIdleAnimation.h"

namespace PixelShipGenerator
{
namespace IdleAnimationInternal
{
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
        AnimationSamplingRequirements SamplingRequirements;
    };

    uint64_t resolveIdleAnimationSeed(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings);

    bool isMaskPixel(const PixelMask& mask, int32_t x, int32_t y);
    uint32_t getContinuousLengthDelta(uint32_t available, double signal, uint32_t amplitudePercent);
    uint32_t getAttachmentMaximumOutwardDistance(const ShipAttachmentPlacement& placement);
    OpaqueBounds calculateOpaqueBounds(const Image& image, uint32_t width, uint32_t height);

    IdleAnimationPlan createIdleAnimationPlan(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings, uint64_t seed);
    Image evaluateIdleFrame(const GeneratedShip& ship, const ShipIdleAnimationSettings& settings, double normalizedTime, const IdleAnimationPlan& plan);
}
}
