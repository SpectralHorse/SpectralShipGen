#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    struct ShipIdleAnimationTraits
    {
        uint32_t EnginePulseStrength = 1u;
        uint32_t ExhaustAmplitudePercent = 85u;
        uint32_t EngineMechanicalChance = 3u;
        uint32_t WeaponMechanicalChance = 50u;
        uint32_t VentActivityChance = 55u;
        bool SynchronizeEngines = false;
        bool AsynchronousEngines = false;
        bool AlternateEnginePhases = false;
        bool SlowMechanicalCycle = false;
    };

    struct ShipLateralMovementAnimationTraits
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

    struct ShipLongitudinalMovementAnimationTraits
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

    struct ShipFiringAnimationTraits
    {
        uint32_t ResponseStrengthPercent = 100u;
        uint32_t DurationAdditionMilliseconds = 0u;
        uint32_t AdditionalRecoilLimit = 0u;
        uint32_t RailWeaponAdditionalRecoilLimit = 0u;
        uint32_t MaximumRecoilLimit = 0u; // 0 = no profile-level cap
        uint32_t MinimumPreFireExtensionLimit = 0u;
        bool HeavyResponse = false;
        bool Responsive = false;
    };

    struct ShipAnimationTraits
    {
        ShipIdleAnimationTraits Idle;
        ShipLateralMovementAnimationTraits LateralMovement;
        ShipLongitudinalMovementAnimationTraits LongitudinalMovement;
        ShipFiringAnimationTraits Firing;
    };
} // namespace SpectralShipGen
