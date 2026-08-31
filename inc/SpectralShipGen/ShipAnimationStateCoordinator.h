#pragma once

#include <cstdint>
#include <vector>

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipAnimationPose.h>
#include <SpectralShipGen/ShipFiringAnimation.h>
#include <SpectralShipGen/ShipMovementAnimation.h>

namespace SpectralShipGen
{
    enum class ShipMovementTransitionPolicy : uint32_t
    {
        NONE = 0u,
        ENTER_FROM_NEUTRAL,
        EXIT_TO_NEUTRAL,
        EXIT_THEN_ENTER_VIA_NEUTRAL,
        SHIP_MOVEMENT_TRANSITION_POLICY_END
    };

    struct ShipMovementTransitionPlan
    {
        ShipAnimationType From = ShipAnimationType::IDLE;
        ShipAnimationType To = ShipAnimationType::IDLE;
        ShipMovementTransitionPolicy Policy = ShipMovementTransitionPolicy::NONE;
        bool ExitCurrentMovement = false;
        bool EnterTargetMovement = false;
        bool UsesNeutralIntermediate = false;
        uint32_t DurationMilliseconds = 0u;
    };

    struct ShipAnimationStateRequest
    {
        // IDLE means there is no movement posture underneath the transient event.
        ShipAnimationType UnderlyingMovementType = ShipAnimationType::IDLE;
        ShipMovementAnimationPhase MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        double MovementNormalizedTime = 0.0;

        bool FireActive = false;
        ShipFiringAnimationTarget FiringTarget;
        double FiringNormalizedTime = 0.0;
    };

    struct ShipAnimationStateDiagnostics
    {
        ShipAnimationType UnderlyingMovementState = ShipAnimationType::IDLE;
        ShipMovementAnimationPhase MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        ShipAnimationType TransientEvent = ShipAnimationType::IDLE;
        ShipFiringAnimationPhase EventPhase = ShipFiringAnimationPhase::REST;
        ShipAnimationPoseLayer ResultLayer = ShipAnimationPoseLayer::STATIC_NEUTRAL;
        std::vector<uint32_t> EventOverriddenWeaponComponents;
    };

    struct ShipAnimationStateEvaluation
    {
        ShipAnimationPose Pose;
        ShipAnimationStateDiagnostics Diagnostics;
    };

    class ShipAnimationStateCoordinator
    {
    public:
        ShipMovementTransitionPlan planMovementTransition(ShipAnimationType from, ShipAnimationType to, const ShipMovementAnimationSettings& settings = {}) const;
        ShipAnimationStateEvaluation evaluate(const GeneratedShip& ship, const ShipAnimationStateRequest& request, const ShipMovementAnimationSettings& movementSettings = {}, const ShipFiringAnimationSettings& firingSettings = {}) const;
    };
} // namespace SpectralShipGen
