#include "CoreRegressionSuites.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include <SpectralShipGen/ShipAnimationStateCoordinator.h>
#include <SpectralShipGen/ShipFiringAnimator.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipLateralMovementAnimator.h>
#include <SpectralShipGen/ShipLongitudinalMovementAnimator.h>

namespace
{
    using namespace SpectralShipGen;

    struct AnimationProfileRoutingFixture
    {
        ShipStyle Style;
        uint64_t Seed;
        ShipDimensions Dimensions;
    };

    constexpr std::array<AnimationProfileRoutingFixture, 6u> Fixtures = {{
        { ShipStyle::SLEEK,      0x8000000000000000ull, { 64u,64u } },
        { ShipStyle::FIGHTER,    0x8000000000100000ull, { 80u,64u } },
        { ShipStyle::HEAVY,      0x8000000000200000ull, { 64u,80u } },
        { ShipStyle::INDUSTRIAL, 0x8000000000300000ull, { 96u,64u } },
        { ShipStyle::SPEARHEAD,  0x8000000000400002ull, { 64u,96u } },
        { ShipStyle::DELTA,      0x8000000000500000ull, { 96u,64u } }
    }};

    ShipGenerationSettings makeSettings(const AnimationProfileRoutingFixture& fixture)
    {
        ShipGenerationSettings settings;
        settings.Seed = fixture.Seed;
        settings.Dimensions = fixture.Dimensions;
        settings.Style = fixture.Style;
        settings.Faction = ShipFactionType::FRONTIER;
        return settings;
    }

    bool animationTraitsEqual(const ShipAnimationTraits& first, const ShipAnimationTraits& second)
    {
        return first.Idle.EnginePulseStrength == second.Idle.EnginePulseStrength
            && first.Idle.ExhaustAmplitudePercent == second.Idle.ExhaustAmplitudePercent
            && first.Idle.EngineMechanicalChance == second.Idle.EngineMechanicalChance
            && first.Idle.WeaponMechanicalChance == second.Idle.WeaponMechanicalChance
            && first.Idle.VentActivityChance == second.Idle.VentActivityChance
            && first.Idle.SynchronizeEngines == second.Idle.SynchronizeEngines
            && first.Idle.AsynchronousEngines == second.Idle.AsynchronousEngines
            && first.Idle.AlternateEnginePhases == second.Idle.AlternateEnginePhases
            && first.Idle.SlowMechanicalCycle == second.Idle.SlowMechanicalCycle
            && first.LateralMovement.ResponseStrengthPercent == second.LateralMovement.ResponseStrengthPercent
            && first.LateralMovement.EngineTravelLimit == second.LateralMovement.EngineTravelLimit
            && first.LateralMovement.WeaponTravelLimit == second.LateralMovement.WeaponTravelLimit
            && first.LateralMovement.AttachmentTravelLimit == second.LateralMovement.AttachmentTravelLimit
            && first.LateralMovement.Synchronized == second.LateralMovement.Synchronized
            && first.LateralMovement.Staggered == second.LateralMovement.Staggered
            && first.LateralMovement.HeavyResponse == second.LateralMovement.HeavyResponse
            && first.LateralMovement.Responsive == second.LateralMovement.Responsive
            && first.LongitudinalMovement.ResponseStrengthPercent == second.LongitudinalMovement.ResponseStrengthPercent
            && first.LongitudinalMovement.AccelerationExtensionPercent == second.LongitudinalMovement.AccelerationExtensionPercent
            && first.LongitudinalMovement.BrakingContractionPercent == second.LongitudinalMovement.BrakingContractionPercent
            && first.LongitudinalMovement.ExhaustVariationLimit == second.LongitudinalMovement.ExhaustVariationLimit
            && first.LongitudinalMovement.WeaponTravelLimit == second.LongitudinalMovement.WeaponTravelLimit
            && first.LongitudinalMovement.AttachmentTravelLimit == second.LongitudinalMovement.AttachmentTravelLimit
            && first.LongitudinalMovement.BrakingTravelLimit == second.LongitudinalMovement.BrakingTravelLimit
            && first.LongitudinalMovement.Synchronized == second.LongitudinalMovement.Synchronized
            && first.LongitudinalMovement.Staggered == second.LongitudinalMovement.Staggered
            && first.LongitudinalMovement.HeavyResponse == second.LongitudinalMovement.HeavyResponse
            && first.LongitudinalMovement.Responsive == second.LongitudinalMovement.Responsive
            && first.Firing.ResponseStrengthPercent == second.Firing.ResponseStrengthPercent
            && first.Firing.DurationAdditionMilliseconds == second.Firing.DurationAdditionMilliseconds
            && first.Firing.AdditionalRecoilLimit == second.Firing.AdditionalRecoilLimit
            && first.Firing.RailWeaponAdditionalRecoilLimit == second.Firing.RailWeaponAdditionalRecoilLimit
            && first.Firing.MaximumRecoilLimit == second.Firing.MaximumRecoilLimit
            && first.Firing.MinimumPreFireExtensionLimit == second.Firing.MinimumPreFireExtensionLimit
            && first.Firing.HeavyResponse == second.Firing.HeavyResponse
            && first.Firing.Responsive == second.Firing.Responsive;
    }

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool frameVectorsEqual(const std::vector<Image>& first, const std::vector<Image>& second)
    {
        if (first.size() != second.size()) { return false; }
        for (std::size_t index = 0u; index < first.size(); ++index)
        {
            if (!imagesEqual(first[index], second[index])) { return false; }
        }
        return true;
    }

    bool idleAnimationsEqual(const ShipIdleAnimation& first, const ShipIdleAnimation& second)
    {
        return first.Type == second.Type
            && first.NormalizedSampleTimes == second.NormalizedSampleTimes
            && first.FrameWidth == second.FrameWidth
            && first.FrameHeight == second.FrameHeight
            && first.Seed == second.Seed
            && first.DurationMilliseconds == second.DurationMilliseconds
            && std::abs(first.FrameDurationMilliseconds - second.FrameDurationMilliseconds) <= 0.000001
            && first.Sampling.ActualFrameCount == second.Sampling.ActualFrameCount
            && first.Sampling.MaximumMechanicalTravelPixels == second.Sampling.MaximumMechanicalTravelPixels
            && first.Sampling.MaximumExhaustTravelPixels == second.Sampling.MaximumExhaustTravelPixels
            && first.Sampling.ActiveAnimatedComponentCount == second.Sampling.ActiveAnimatedComponentCount
            && first.Sampling.IndependentPhaseGroupCount == second.Sampling.IndependentPhaseGroupCount
            && frameVectorsEqual(first.Frames, second.Frames);
    }

    bool movementClipsEqual(const ShipMovementAnimationClip& first, const ShipMovementAnimationClip& second)
    {
        return first.Type == second.Type
            && first.Phase == second.Phase
            && first.Looping == second.Looping
            && first.NormalizedSampleTimes == second.NormalizedSampleTimes
            && first.FrameWidth == second.FrameWidth
            && first.FrameHeight == second.FrameHeight
            && first.DurationMilliseconds == second.DurationMilliseconds
            && std::abs(first.FrameDurationMilliseconds - second.FrameDurationMilliseconds) <= 0.000001
            && first.Sampling.ActualFrameCount == second.Sampling.ActualFrameCount
            && first.Sampling.MaximumMechanicalTravelPixels == second.Sampling.MaximumMechanicalTravelPixels
            && first.Sampling.MaximumExhaustTravelPixels == second.Sampling.MaximumExhaustTravelPixels
            && frameVectorsEqual(first.Frames, second.Frames);
    }

    bool movementAnimationsEqual(const ShipMovementAnimation& first, const ShipMovementAnimation& second)
    {
        return first.Type == second.Type
            && first.Seed == second.Seed
            && movementClipsEqual(first.Enter, second.Enter)
            && movementClipsEqual(first.Sustain, second.Sustain)
            && movementClipsEqual(first.Exit, second.Exit);
    }

    bool firingAnimationsEqual(const ShipFiringAnimation& first, const ShipFiringAnimation& second)
    {
        return first.Type == second.Type
            && first.Seed == second.Seed
            && first.Target.WeaponComponentIndex == second.Target.WeaponComponentIndex
            && first.Target.IncludeSymmetryGroup == second.Target.IncludeSymmetryGroup
            && first.NormalizedSampleTimes == second.NormalizedSampleTimes
            && first.FrameWidth == second.FrameWidth
            && first.FrameHeight == second.FrameHeight
            && first.DurationMilliseconds == second.DurationMilliseconds
            && std::abs(first.FrameDurationMilliseconds - second.FrameDurationMilliseconds) <= 0.000001
            && first.Sampling.ActualFrameCount == second.Sampling.ActualFrameCount
            && first.Sampling.MaximumMechanicalTravelPixels == second.Sampling.MaximumMechanicalTravelPixels
            && first.Diagnostics.MaximumRecoilTravelPixels == second.Diagnostics.MaximumRecoilTravelPixels
            && first.Diagnostics.MaximumPreFireExtensionPixels == second.Diagnostics.MaximumPreFireExtensionPixels
            && frameVectorsEqual(first.Frames, second.Frames);
    }

    bool posesEqual(const ShipAnimationPose& first, const ShipAnimationPose& second)
    {
        if (!imagesEqual(first.Frame, second.Frame) || first.Layer != second.Layer || first.UnderlyingAnimationType != second.UnderlyingAnimationType || first.ComponentTransforms.size() != second.ComponentTransforms.size()) { return false; }
        for (std::size_t index = 0u; index < first.ComponentTransforms.size(); ++index)
        {
            const ShipAnimationComponentTransform& a = first.ComponentTransforms[index];
            const ShipAnimationComponentTransform& b = second.ComponentTransforms[index];
            if (a.Type != b.Type || a.ComponentIndex != b.ComponentIndex || a.OffsetX != b.OffsetX || a.OffsetY != b.OffsetY) { return false; }
        }
        return true;
    }
}

int SpectralShipGenTests::runAnimationProfileRoutingRegression()
{
    using namespace SpectralShipGen;

    ShipGenerator generator;
    ShipIdleAnimator idleAnimator;
    ShipLateralMovementAnimator lateralAnimator;
    ShipLongitudinalMovementAnimator longitudinalAnimator;
    ShipFiringAnimator firingAnimator;
    ShipAnimationStateCoordinator coordinator;

    for (const AnimationProfileRoutingFixture& fixture : Fixtures)
    {
        const GeneratedShip generated = generator.generate(makeSettings(fixture));
        if (!animationTraitsEqual(generated.AnimationTraits, getShipGenerationProfile(fixture.Style).AnimationTraits))
        {
            std::cerr << "Task 80 regression failed: generated ship did not retain resolved animation traits.\n";
            return 1;
        }

        GeneratedShip withoutStyleProvenance = generated;
        withoutStyleProvenance.Provenance.StructuralPreset.reset();

        const ShipIdleAnimation idle = idleAnimator.generate(generated);
        const ShipIdleAnimation idleWithoutStyle = idleAnimator.generate(withoutStyleProvenance);
        if (!idleAnimationsEqual(idle, idleWithoutStyle) || idle.Frames.empty() || !imagesEqual(idle.Frames.front(), generated.FinalImage))
        {
            std::cerr << "Task 80 regression failed: IDLE animation depends on built-in ShipStyle provenance.\n";
            return 1;
        }

        for (ShipAnimationType type : { ShipAnimationType::MOVE_LEFT, ShipAnimationType::MOVE_RIGHT })
        {
            if (!movementAnimationsEqual(lateralAnimator.generate(generated, type), lateralAnimator.generate(withoutStyleProvenance, type)))
            {
                std::cerr << "Task 80 regression failed: lateral movement depends on built-in ShipStyle provenance.\n";
                return 1;
            }
        }

        for (ShipAnimationType type : { ShipAnimationType::MOVE_UP, ShipAnimationType::MOVE_DOWN })
        {
            if (!movementAnimationsEqual(longitudinalAnimator.generate(generated, type), longitudinalAnimator.generate(withoutStyleProvenance, type)))
            {
                std::cerr << "Task 80 regression failed: longitudinal movement depends on built-in ShipStyle provenance.\n";
                return 1;
            }
        }

        const std::vector<ShipFiringAnimationTarget> targets = firingAnimator.getAvailableTargets(generated);
        if (targets.empty())
        {
            std::cerr << "Task 80 regression failed: profile-routing fixture no longer contains a firing target.\n";
            return 1;
        }

        const ShipFiringAnimationTarget target = targets.front();
        if (!firingAnimationsEqual(firingAnimator.generate(generated, target), firingAnimator.generate(withoutStyleProvenance, target)))
        {
            std::cerr << "Task 80 regression failed: firing animation depends on built-in ShipStyle provenance.\n";
            return 1;
        }

        ShipAnimationStateRequest request;
        request.UnderlyingMovementType = ShipAnimationType::MOVE_LEFT;
        request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        request.MovementNormalizedTime = 0.31;
        request.FireActive = true;
        request.FiringTarget = target;
        request.FiringNormalizedTime = 0.28;

        const ShipAnimationStateEvaluation composed = coordinator.evaluate(generated, request);
        const ShipAnimationStateEvaluation composedWithoutStyle = coordinator.evaluate(withoutStyleProvenance, request);
        if (!posesEqual(composed.Pose, composedWithoutStyle.Pose)
            || composed.Diagnostics.ResultLayer != composedWithoutStyle.Diagnostics.ResultLayer
            || composed.Diagnostics.EventPhase != composedWithoutStyle.Diagnostics.EventPhase
            || composed.Diagnostics.EventOverriddenWeaponComponents != composedWithoutStyle.Diagnostics.EventOverriddenWeaponComponents)
        {
            std::cerr << "Task 80 regression failed: composed movement + FIRE state depends on built-in ShipStyle provenance.\n";
            return 1;
        }
    }

    std::cout << "Task 80 animation profile routing regression passed.\n";
    return 0;
}
