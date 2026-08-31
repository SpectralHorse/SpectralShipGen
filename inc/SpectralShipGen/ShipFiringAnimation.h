#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <SpectralShipGen/AnimationSamplingPlanner.h>
#include <SpectralShipGen/Image.h>
#include <SpectralShipGen/ShipAnimationType.h>
#include <SpectralShipGen/ShipWeaponType.h>

namespace SpectralShipGen
{
    enum class ShipFiringAnimationPhase : uint32_t
    {
        REST = 0u,
        PRE_FIRE,
        RECOIL,
        RECOVERY,
        SHIP_FIRING_ANIMATION_PHASE_END
    };

    struct ShipFiringAnimationTarget
    {
        uint32_t WeaponComponentIndex = 0u;
        bool IncludeSymmetryGroup = true;
    };

    struct ShipFiringAnimationSettings
    {
        uint32_t MinimumFrameCount = 10u;
        uint32_t MaximumFrameCount = 36u;
        uint32_t ExactFrameCount = 14u;
        AnimationSamplingMode SamplingMode = AnimationSamplingMode::ADAPTIVE;
        bool PreFireMotion = true;
        std::optional<uint64_t> Seed;
    };

    struct ShipFiringWeaponDiagnostic
    {
        uint32_t WeaponComponentIndex = 0u;
        ShipWeaponType Type = ShipWeaponType::SINGLE_CANNON;
        ShipWeaponHardpointRegion Region = ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE;
        uint32_t SymmetryGroup = 0u;
        uint32_t MovablePixelCount = 0u;
        uint32_t MaximumRecoilTravelPixels = 0u;
        uint32_t MaximumPreFireExtensionPixels = 0u;
    };

    struct ShipFiringAnimationDiagnostics
    {
        bool ValidTarget = false;
        bool PairedOrGrouped = false;
        bool PreFireMotion = false;
        uint32_t TargetWeaponComponentIndex = 0u;
        uint32_t TargetSymmetryGroup = 0u;
        uint32_t ActiveWeaponCount = 0u;
        uint32_t MaximumRecoilTravelPixels = 0u;
        uint32_t MaximumPreFireExtensionPixels = 0u;
        uint32_t IndependentPhaseGroupCount = 0u;
        std::vector<ShipFiringWeaponDiagnostic> Weapons;
    };

    ShipFiringAnimationPhase getFiringAnimationPhase(double normalizedTime);

    struct ShipFiringAnimation
    {
        ShipAnimationType Type = ShipAnimationType::FIRE;
        uint64_t Seed = 0u;
        ShipFiringAnimationTarget Target;
        std::vector<Image> Frames;
        std::vector<double> NormalizedSampleTimes;
        uint32_t FrameWidth = 0u;
        uint32_t FrameHeight = 0u;
        uint32_t DurationMilliseconds = 0u;
        double FrameDurationMilliseconds = 0.0;
        AnimationSamplingPlan Sampling;
        ShipFiringAnimationDiagnostics Diagnostics;
    };
}
