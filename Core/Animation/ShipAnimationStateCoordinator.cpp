#include <SpectralShipGen/ShipAnimationStateCoordinator.h>

#include <stdexcept>

#include <SpectralShipGen/ShipFiringAnimator.h>
#include <SpectralShipGen/ShipLateralMovementAnimator.h>
#include <SpectralShipGen/ShipLongitudinalMovementAnimator.h>

namespace
{
    bool isLateralMovement(SpectralShipGen::ShipAnimationType type)
    {
        return type == SpectralShipGen::ShipAnimationType::MOVE_LEFT || type == SpectralShipGen::ShipAnimationType::MOVE_RIGHT;
    }

    bool isLongitudinalMovement(SpectralShipGen::ShipAnimationType type)
    {
        return type == SpectralShipGen::ShipAnimationType::MOVE_UP || type == SpectralShipGen::ShipAnimationType::MOVE_DOWN;
    }

    bool isMovement(SpectralShipGen::ShipAnimationType type)
    {
        return isLateralMovement(type) || isLongitudinalMovement(type);
    }

    SpectralShipGen::ShipAnimationPose createNeutralPose(const SpectralShipGen::GeneratedShip& ship)
    {
        SpectralShipGen::ShipAnimationPose pose;
        pose.Frame = ship.FinalImage;
        pose.Layer = SpectralShipGen::ShipAnimationPoseLayer::STATIC_NEUTRAL;
        pose.UnderlyingAnimationType = SpectralShipGen::ShipAnimationType::IDLE;
        return pose;
    }

    std::vector<uint32_t> resolveFiringOverrides(const SpectralShipGen::GeneratedShip& ship, const SpectralShipGen::ShipFiringAnimationTarget& target)
    {
        std::vector<uint32_t> indices;
        const auto& components = ship.IdleAnimationMetadata.WeaponComponents;
        if (target.WeaponComponentIndex >= components.size()) { return indices; }

        const auto& selected = components[target.WeaponComponentIndex];
        if (!target.IncludeSymmetryGroup || selected.SymmetryGroup == 0u)
        {
            indices.push_back(target.WeaponComponentIndex);
            return indices;
        }

        for (uint32_t index = 0u; index < components.size(); ++index)
        {
            if (components[index].SymmetryGroup == selected.SymmetryGroup) { indices.push_back(index); }
        }
        return indices;
    }
}

namespace SpectralShipGen
{
    ShipMovementTransitionPlan ShipAnimationStateCoordinator::planMovementTransition(ShipAnimationType from, ShipAnimationType to, const ShipMovementAnimationSettings& settings) const
    {
        if ((from != ShipAnimationType::IDLE && !isMovement(from)) || (to != ShipAnimationType::IDLE && !isMovement(to)))
        {
            throw std::invalid_argument("ShipAnimationStateCoordinator movement transitions require IDLE or a movement animation type.");
        }

        ShipMovementTransitionPlan plan;
        plan.From = from;
        plan.To = to;
        if (from == to) { return plan; }

        if (from == ShipAnimationType::IDLE)
        {
            plan.Policy = ShipMovementTransitionPolicy::ENTER_FROM_NEUTRAL;
            plan.EnterTargetMovement = true;
            plan.UsesNeutralIntermediate = true;
            plan.DurationMilliseconds = settings.EnterDurationMilliseconds;
            return plan;
        }

        if (to == ShipAnimationType::IDLE)
        {
            plan.Policy = ShipMovementTransitionPolicy::EXIT_TO_NEUTRAL;
            plan.ExitCurrentMovement = true;
            plan.UsesNeutralIntermediate = true;
            plan.DurationMilliseconds = settings.ExitDurationMilliseconds;
            return plan;
        }

        plan.Policy = ShipMovementTransitionPolicy::EXIT_THEN_ENTER_VIA_NEUTRAL;
        plan.ExitCurrentMovement = true;
        plan.EnterTargetMovement = true;
        plan.UsesNeutralIntermediate = true;
        plan.DurationMilliseconds = settings.ExitDurationMilliseconds + settings.EnterDurationMilliseconds;
        return plan;
    }

    ShipAnimationStateEvaluation ShipAnimationStateCoordinator::evaluate(const GeneratedShip& ship, const ShipAnimationStateRequest& request, const ShipMovementAnimationSettings& movementSettings, const ShipFiringAnimationSettings& firingSettings) const
    {
        ShipAnimationStateEvaluation result;
        result.Diagnostics.UnderlyingMovementState = request.UnderlyingMovementType;
        result.Diagnostics.MovementPhase = request.MovementPhase;

        if (request.UnderlyingMovementType == ShipAnimationType::IDLE)
        {
            result.Pose = createNeutralPose(ship);
        }
        else if (isLateralMovement(request.UnderlyingMovementType))
        {
            ShipLateralMovementAnimator animator;
            result.Pose = animator.evaluatePoseAtNormalizedTime(ship, request.UnderlyingMovementType, request.MovementPhase, request.MovementNormalizedTime, movementSettings);
        }
        else if (isLongitudinalMovement(request.UnderlyingMovementType))
        {
            ShipLongitudinalMovementAnimator animator;
            result.Pose = animator.evaluatePoseAtNormalizedTime(ship, request.UnderlyingMovementType, request.MovementPhase, request.MovementNormalizedTime, movementSettings);
        }
        else
        {
            throw std::invalid_argument("ShipAnimationStateCoordinator underlying state must be IDLE or a movement animation type.");
        }

        if (!request.FireActive)
        {
            result.Diagnostics.ResultLayer = result.Pose.Layer;
            return result;
        }

        ShipFiringAnimator firingAnimator;
        result.Pose.Frame = firingAnimator.evaluateFrameAtNormalizedTime(ship, result.Pose, request.FiringTarget, request.FiringNormalizedTime, firingSettings);
        result.Pose.Layer = ShipAnimationPoseLayer::TRANSIENT_EVENT;
        result.Diagnostics.TransientEvent = ShipAnimationType::FIRE;
        result.Diagnostics.EventPhase = getFiringAnimationPhase(request.FiringNormalizedTime);
        result.Diagnostics.ResultLayer = result.Pose.Layer;
        result.Diagnostics.EventOverriddenWeaponComponents = resolveFiringOverrides(ship, request.FiringTarget);
        return result;
    }
} // namespace SpectralShipGen
