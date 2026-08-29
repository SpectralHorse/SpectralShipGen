#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "AnimationSamplingPlanner.h"
#include "Image.h"
#include "ShipAnimationType.h"

namespace PixelShipGenerator
{
    enum class ShipMovementAnimationPhase : uint32_t
    {
        ENTER = 0u,
        SUSTAIN,
        EXIT,
        SHIP_MOVEMENT_ANIMATION_PHASE_END
    };

    enum class ShipMovementAnimatedComponentType : uint32_t
    {
        ENGINE_VECTORING = 0u,
        WEAPON_STABILIZATION,
        ATTACHMENT_ARTICULATION,
        SHIP_MOVEMENT_ANIMATED_COMPONENT_TYPE_END
    };

    struct ShipMovementAnimationSettings
    {
        uint32_t EnterDurationMilliseconds = 300u;
        uint32_t SustainLoopDurationMilliseconds = 900u;
        uint32_t ExitDurationMilliseconds = 300u;

        uint32_t TransitionFrameCount = 8u;
        uint32_t SustainFrameCount = 16u;
        uint32_t MinimumTransitionFrameCount = 5u;
        uint32_t MinimumSustainFrameCount = 10u;
        uint32_t MaximumFrameCount = 60u;
        AnimationSamplingMode SamplingMode = AnimationSamplingMode::ADAPTIVE;

        bool EngineVectoring = true;
        bool WeaponStabilization = true;
        bool AttachmentArticulation = true;
        std::optional<uint64_t> Seed;
    };

    struct ShipMovementComponentDiagnostic
    {
        ShipMovementAnimatedComponentType Type = ShipMovementAnimatedComponentType::ENGINE_VECTORING;
        uint32_t SemanticGroup = 0u;
        uint32_t SourcePixelCount = 0u;
        int32_t MaximumOffsetX = 0;
        double SustainPhaseOffset = 0.0;
    };

    struct ShipMovementAnimationDiagnostics
    {
        int32_t DirectionSignX = 0;
        uint32_t ActiveEngineCount = 0u;
        uint32_t ActiveWeaponCount = 0u;
        uint32_t ActiveAttachmentCount = 0u;
        uint32_t MaximumMechanicalTravelPixels = 0u;
        uint32_t IndependentPhaseGroupCount = 0u;
        std::vector<ShipMovementComponentDiagnostic> Components;
    };

    struct ShipMovementAnimationClip
    {
        ShipAnimationType Type = ShipAnimationType::MOVE_LEFT;
        ShipMovementAnimationPhase Phase = ShipMovementAnimationPhase::ENTER;
        bool Looping = false;
        std::vector<Image> Frames;
        std::vector<double> NormalizedSampleTimes;
        uint32_t FrameWidth = 0u;
        uint32_t FrameHeight = 0u;
        uint32_t DurationMilliseconds = 0u;
        double FrameDurationMilliseconds = 0.0;
        AnimationSamplingPlan Sampling;
    };

    struct ShipMovementAnimation
    {
        ShipAnimationType Type = ShipAnimationType::MOVE_LEFT;
        uint64_t Seed = 0u;
        ShipMovementAnimationClip Enter;
        ShipMovementAnimationClip Sustain;
        ShipMovementAnimationClip Exit;
        ShipMovementAnimationDiagnostics Diagnostics;
    };

    const ShipMovementAnimationClip& getMovementAnimationClip(const ShipMovementAnimation& animation, ShipMovementAnimationPhase phase);
    ShipMovementAnimationClip& getMovementAnimationClip(ShipMovementAnimation& animation, ShipMovementAnimationPhase phase);
}
