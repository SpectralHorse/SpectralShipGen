#include "RegressionSuites.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "ShipAnimationStateCoordinator.h"
#include "ShipFiringAnimator.h"
#include "ShipGenerator.h"
#include "ShipIdleAnimator.h"
#include "ShipLateralMovementAnimator.h"
#include "ShipLongitudinalMovementAnimator.h"

namespace
{
    using namespace PixelShipGenerator;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    ShipGenerationSettings makeSettings(uint64_t seed, ShipDimensions dimensions, ShipStyle style, ShipFactionType faction)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        return settings;
    }

    bool isLateral(ShipAnimationType type)
    {
        return type == ShipAnimationType::MOVE_LEFT || type == ShipAnimationType::MOVE_RIGHT;
    }

    ShipMovementAnimation generateMovement(const GeneratedShip& ship, ShipAnimationType type, const ShipMovementAnimationSettings& settings = {})
    {
        if (isLateral(type))
        {
            ShipLateralMovementAnimator animator;
            return animator.generate(ship, type, settings);
        }
        ShipLongitudinalMovementAnimator animator;
        return animator.generate(ship, type, settings);
    }

    ShipAnimationPose evaluateMovementPose(const GeneratedShip& ship, ShipAnimationType type, ShipMovementAnimationPhase phase, double normalizedTime, const ShipMovementAnimationSettings& settings = {})
    {
        if (isLateral(type))
        {
            ShipLateralMovementAnimator animator;
            return animator.evaluatePoseAtNormalizedTime(ship, type, phase, normalizedTime, settings);
        }
        ShipLongitudinalMovementAnimator animator;
        return animator.evaluatePoseAtNormalizedTime(ship, type, phase, normalizedTime, settings);
    }

    bool protectedPropulsionMatches(const GeneratedShip& ship, const Image& first, const Image& second)
    {
        for (uint32_t y = 0u; y < ship.FinalImage.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.FinalImage.getWidth(); ++x)
            {
                if ((ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y)) && first.getPixel(x, y) != second.getPixel(x, y)) { return false; }
            }
        }
        return true;
    }

    bool transitionPlanEqual(const ShipMovementTransitionPlan& first, const ShipMovementTransitionPlan& second)
    {
        return first.From == second.From && first.To == second.To && first.Policy == second.Policy && first.ExitCurrentMovement == second.ExitCurrentMovement && first.EnterTargetMovement == second.EnterTargetMovement && first.UsesNeutralIntermediate == second.UsesNeutralIntermediate && first.DurationMilliseconds == second.DurationMilliseconds;
    }
}

int PixelShipGeneratorTests::runAnimationStateCompatibilityRegression()
{
    using namespace PixelShipGenerator;

    constexpr std::array<ShipAnimationType, 4u> MovementTypes =
    {
        ShipAnimationType::MOVE_LEFT,
        ShipAnimationType::MOVE_RIGHT,
        ShipAnimationType::MOVE_UP,
        ShipAnimationType::MOVE_DOWN
    };

    ShipGenerator generator;
    ShipAnimationStateCoordinator coordinator;
    ShipFiringAnimator firingAnimator;
    ShipIdleAnimator idleAnimator;
    ShipMovementAnimationSettings movementSettings;

    const GeneratedShip transitionShip = generator.generate(makeSettings(0x7100000000000001ull, { 96u,96u }, ShipStyle::INDUSTRIAL, ShipFactionType::MILITARY));
    const Image staticSnapshot = transitionShip.FinalImage;
    const ShipIdleAnimation idleAnimation = idleAnimator.generate(transitionShip);
    if (idleAnimation.Frames.empty() || !imagesEqual(idleAnimation.Frames.front(), transitionShip.FinalImage) || idleAnimation.Sampling.Mode != AnimationSamplingMode::ADAPTIVE)
    {
        std::cerr << "Task 71 regression failed: IDLE neutral anchor/adaptive sampling invariant is invalid.\n";
        return 1;
    }

    for (ShipAnimationType movementType : MovementTypes)
    {
        const ShipMovementTransitionPlan idleToMovement = coordinator.planMovementTransition(ShipAnimationType::IDLE, movementType, movementSettings);
        const ShipMovementTransitionPlan idleToMovementRepeat = coordinator.planMovementTransition(ShipAnimationType::IDLE, movementType, movementSettings);
        if (!transitionPlanEqual(idleToMovement, idleToMovementRepeat) || idleToMovement.Policy != ShipMovementTransitionPolicy::ENTER_FROM_NEUTRAL || idleToMovement.ExitCurrentMovement || !idleToMovement.EnterTargetMovement || !idleToMovement.UsesNeutralIntermediate || idleToMovement.DurationMilliseconds != movementSettings.EnterDurationMilliseconds)
        {
            std::cerr << "Task 71 regression failed: IDLE -> movement transition policy is invalid or non-deterministic.\n";
            return 1;
        }

        const ShipMovementTransitionPlan movementToIdle = coordinator.planMovementTransition(movementType, ShipAnimationType::IDLE, movementSettings);
        if (movementToIdle.Policy != ShipMovementTransitionPolicy::EXIT_TO_NEUTRAL || !movementToIdle.ExitCurrentMovement || movementToIdle.EnterTargetMovement || !movementToIdle.UsesNeutralIntermediate || movementToIdle.DurationMilliseconds != movementSettings.ExitDurationMilliseconds)
        {
            std::cerr << "Task 71 regression failed: movement -> IDLE transition policy is invalid.\n";
            return 1;
        }

        const ShipMovementAnimation movement = generateMovement(transitionShip, movementType, movementSettings);
        if (movement.Enter.Frames.empty() || movement.Exit.Frames.empty() || !imagesEqual(movement.Enter.Frames.front(), transitionShip.FinalImage) || !imagesEqual(movement.Exit.Frames.back(), transitionShip.FinalImage))
        {
            std::cerr << "Task 71 regression failed: movement neutral anchors are incompatible with the static base pose.\n";
            return 1;
        }
        if (movement.Enter.Sampling.Mode != AnimationSamplingMode::ADAPTIVE || movement.Sustain.Sampling.Mode != AnimationSamplingMode::ADAPTIVE || movement.Exit.Sampling.Mode != AnimationSamplingMode::ADAPTIVE)
        {
            std::cerr << "Task 71 regression failed: movement compatibility reverted an Enter/Sustain/Exit clip to fixed sampling.\n";
            return 1;
        }
    }

    constexpr std::array<std::pair<ShipAnimationType, ShipAnimationType>, 8u> MovementChanges =
    {{
        { ShipAnimationType::MOVE_LEFT, ShipAnimationType::MOVE_RIGHT },
        { ShipAnimationType::MOVE_RIGHT, ShipAnimationType::MOVE_LEFT },
        { ShipAnimationType::MOVE_UP, ShipAnimationType::MOVE_DOWN },
        { ShipAnimationType::MOVE_DOWN, ShipAnimationType::MOVE_UP },
        { ShipAnimationType::MOVE_LEFT, ShipAnimationType::MOVE_UP },
        { ShipAnimationType::MOVE_UP, ShipAnimationType::MOVE_RIGHT },
        { ShipAnimationType::MOVE_RIGHT, ShipAnimationType::MOVE_DOWN },
        { ShipAnimationType::MOVE_DOWN, ShipAnimationType::MOVE_LEFT }
    }};

    for (const auto& [from, to] : MovementChanges)
    {
        const ShipMovementTransitionPlan plan = coordinator.planMovementTransition(from, to, movementSettings);
        if (plan.Policy != ShipMovementTransitionPolicy::EXIT_THEN_ENTER_VIA_NEUTRAL || !plan.ExitCurrentMovement || !plan.EnterTargetMovement || !plan.UsesNeutralIntermediate || plan.DurationMilliseconds != movementSettings.ExitDurationMilliseconds + movementSettings.EnterDurationMilliseconds)
        {
            std::cerr << "Task 71 regression failed: movement-to-movement transition does not use the shared neutral intermediate policy.\n";
            return 1;
        }

        const ShipMovementAnimation fromAnimation = generateMovement(transitionShip, from, movementSettings);
        const ShipMovementAnimation toAnimation = generateMovement(transitionShip, to, movementSettings);
        if (!imagesEqual(fromAnimation.Exit.Frames.back(), transitionShip.FinalImage) || !imagesEqual(toAnimation.Enter.Frames.front(), transitionShip.FinalImage))
        {
            std::cerr << "Task 71 regression failed: neutral-mediated movement transition endpoints do not meet exactly.\n";
            return 1;
        }
    }

    const std::vector<ShipFiringAnimationTarget> targets = firingAnimator.getAvailableTargets(transitionShip);
    if (targets.empty())
    {
        std::cerr << "Task 71 regression failed: firing compatibility fixture no longer contains a movable weapon target.\n";
        return 1;
    }
    const ShipFiringAnimationTarget firingTarget = targets.front();
    const ShipFiringAnimation neutralFiringAnimation = firingAnimator.generate(transitionShip, firingTarget);
    if (neutralFiringAnimation.Frames.empty() || neutralFiringAnimation.Sampling.Mode != AnimationSamplingMode::ADAPTIVE)
    {
        std::cerr << "Task 71 regression failed: transient FIRE compatibility reverted adaptive event sampling.\n";
        return 1;
    }

    ShipAnimationStateRequest neutralFire;
    neutralFire.FireActive = true;
    neutralFire.FiringTarget = firingTarget;
    neutralFire.FiringNormalizedTime = 0.0;
    const ShipAnimationStateEvaluation neutralFireStart = coordinator.evaluate(transitionShip, neutralFire);
    neutralFire.FiringNormalizedTime = 1.0;
    const ShipAnimationStateEvaluation neutralFireEnd = coordinator.evaluate(transitionShip, neutralFire);
    if (!imagesEqual(neutralFireStart.Pose.Frame, transitionShip.FinalImage) || !imagesEqual(neutralFireEnd.Pose.Frame, transitionShip.FinalImage))
    {
        std::cerr << "Task 71 regression failed: FIRE-from-neutral start/recovery lost static identity.\n";
        return 1;
    }

    for (ShipAnimationType movementType : MovementTypes)
    {
        ShipAnimationStateRequest request;
        request.UnderlyingMovementType = movementType;
        request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        request.MovementNormalizedTime = 0.31;
        request.FireActive = false;

        const ShipAnimationStateEvaluation movementOnly = coordinator.evaluate(transitionShip, request, movementSettings);
        const ShipAnimationPose directMovementPose = evaluateMovementPose(transitionShip, movementType, ShipMovementAnimationPhase::SUSTAIN, 0.31, movementSettings);
        if (!imagesEqual(movementOnly.Pose.Frame, directMovementPose.Frame) || movementOnly.Pose.UnderlyingAnimationType != movementType)
        {
            std::cerr << "Task 71 regression failed: common pose evaluation does not preserve the selected movement posture.\n";
            return 1;
        }

        request.FireActive = true;
        request.FiringTarget = firingTarget;
        request.FiringNormalizedTime = 0.0;
        const ShipAnimationStateEvaluation fireStart = coordinator.evaluate(transitionShip, request, movementSettings);
        request.FiringNormalizedTime = 0.28;
        const ShipAnimationStateEvaluation fireRecoil = coordinator.evaluate(transitionShip, request, movementSettings);
        const ShipAnimationStateEvaluation fireRecoilRepeat = coordinator.evaluate(transitionShip, request, movementSettings);
        request.FiringNormalizedTime = 1.0;
        const ShipAnimationStateEvaluation fireRecovery = coordinator.evaluate(transitionShip, request, movementSettings);

        if (!imagesEqual(fireStart.Pose.Frame, movementOnly.Pose.Frame) || !imagesEqual(fireRecovery.Pose.Frame, movementOnly.Pose.Frame))
        {
            std::cerr << "Task 71 regression failed: firing during movement does not start/recover to the exact underlying posture.\n";
            return 1;
        }
        if (!imagesEqual(fireRecoil.Pose.Frame, fireRecoilRepeat.Pose.Frame) || fireRecoil.Diagnostics.ResultLayer != ShipAnimationPoseLayer::TRANSIENT_EVENT || fireRecoil.Diagnostics.TransientEvent != ShipAnimationType::FIRE || fireRecoil.Diagnostics.EventOverriddenWeaponComponents.empty())
        {
            std::cerr << "Task 71 regression failed: transient event composition/ownership is invalid or non-deterministic.\n";
            return 1;
        }
        if (!protectedPropulsionMatches(transitionShip, movementOnly.Pose.Frame, fireRecoil.Pose.Frame))
        {
            std::cerr << "Task 71 regression failed: FIRE reset or modified the underlying movement propulsion posture.\n";
            return 1;
        }
    }

    // Exercise the exact moved-weapon composition path: firing must operate at the movement-adjusted
    // component location rather than erasing/recoiling the raw static coordinates.
    bool exercisedMovedWeaponOverride = false;
    for (ShipAnimationType movementType : MovementTypes)
    {
        const ShipAnimationPose movementPose = evaluateMovementPose(transitionShip, movementType, ShipMovementAnimationPhase::SUSTAIN, 0.31, movementSettings);
        const ShipAnimationComponentTransform* transform = findAnimationComponentTransform(movementPose, ShipAnimationSemanticComponentType::WEAPON, firingTarget.WeaponComponentIndex);
        if (transform == nullptr || (transform->OffsetX == 0 && transform->OffsetY == 0)) { continue; }

        ShipAnimationStateRequest request;
        request.UnderlyingMovementType = movementType;
        request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        request.MovementNormalizedTime = 0.31;
        request.FireActive = true;
        request.FiringTarget = firingTarget;
        request.FiringNormalizedTime = 0.28;
        const ShipAnimationStateEvaluation fired = coordinator.evaluate(transitionShip, request, movementSettings);
        if (imagesEqual(fired.Pose.Frame, movementPose.Frame))
        {
            std::cerr << "Task 71 regression failed: moved weapon component did not receive the event override.\n";
            return 1;
        }
        exercisedMovedWeaponOverride = true;
        break;
    }
    if (!exercisedMovedWeaponOverride)
    {
        std::cerr << "Task 71 regression failed: compatibility fixture no longer exercises a movement-adjusted firing component.\n";
        return 1;
    }

    // A different movement sample remains a different underlying base while the same FIRE event phase
    // is applied, proving movement time and event time are independently composable rather than baked
    // into pair-specific clips.
    ShipAnimationStateRequest movingFireA;
    movingFireA.UnderlyingMovementType = ShipAnimationType::MOVE_UP;
    movingFireA.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
    movingFireA.MovementNormalizedTime = 0.18;
    movingFireA.FireActive = true;
    movingFireA.FiringTarget = firingTarget;
    movingFireA.FiringNormalizedTime = 0.28;
    ShipAnimationStateRequest movingFireB = movingFireA;
    movingFireB.MovementNormalizedTime = 0.63;
    const ShipAnimationStateEvaluation composedA = coordinator.evaluate(transitionShip, movingFireA, movementSettings);
    const ShipAnimationStateEvaluation composedB = coordinator.evaluate(transitionShip, movingFireB, movementSettings);
    const ShipAnimationPose underlyingA = evaluateMovementPose(transitionShip, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.18, movementSettings);
    const ShipAnimationPose underlyingB = evaluateMovementPose(transitionShip, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.63, movementSettings);
    if (!imagesEqual(underlyingA.Frame, underlyingB.Frame) && imagesEqual(composedA.Pose.Frame, composedB.Pose.Frame))
    {
        std::cerr << "Task 71 regression failed: transient FIRE composition discarded independently advancing movement state.\n";
        return 1;
    }

    const GeneratedShip rectangularShip = generator.generate(makeSettings(0x7100000000000048ull, { 80u,48u }, ShipStyle::DELTA, ShipFactionType::CORPORATE));
    for (ShipAnimationType movementType : MovementTypes)
    {
        ShipAnimationStateRequest request;
        request.UnderlyingMovementType = movementType;
        request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        request.MovementNormalizedTime = 0.41;
        const ShipAnimationStateEvaluation evaluated = coordinator.evaluate(rectangularShip, request, movementSettings);
        if (evaluated.Pose.Frame.getWidth() != 80u || evaluated.Pose.Frame.getHeight() != 48u)
        {
            std::cerr << "Task 71 regression failed: rectangular composed pose dimensions changed.\n";
            return 1;
        }
    }

    constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
    constexpr std::array<ShipFactionType, 6u> Factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
    uint64_t coverageSeed = 0x7100000000001000ull;
    uint32_t movementIndex = 0u;
    for (ShipStyle style : Styles)
    {
        for (ShipFactionType faction : Factions)
        {
            const GeneratedShip ship = generator.generate(makeSettings(coverageSeed++, { 44u,44u }, style, faction));
            const ShipAnimationType movementType = MovementTypes[movementIndex++ % MovementTypes.size()];
            ShipAnimationStateRequest request;
            request.UnderlyingMovementType = movementType;
            request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
            request.MovementNormalizedTime = 0.37;
            const ShipAnimationStateEvaluation first = coordinator.evaluate(ship, request, movementSettings);
            const ShipAnimationStateEvaluation second = coordinator.evaluate(ship, request, movementSettings);
            if (!imagesEqual(first.Pose.Frame, second.Pose.Frame) || first.Pose.Frame.getWidth() != ship.FinalImage.getWidth() || first.Pose.Frame.getHeight() != ship.FinalImage.getHeight())
            {
                std::cerr << "Task 71 regression failed: style/faction state evaluation is invalid or non-deterministic.\n";
                return 1;
            }
        }
    }

    if (!imagesEqual(transitionShip.FinalImage, staticSnapshot))
    {
        std::cerr << "Task 71 regression failed: animation state evaluation permanently mutated the GeneratedShip base image.\n";
        return 1;
    }

    std::cout << "Task 71 animation state compatibility regression passed.\n";
    return 0;
}
