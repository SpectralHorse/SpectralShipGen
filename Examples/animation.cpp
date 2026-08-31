#include <iostream>

#include <SpectralShipGen/ShipAnimationStateCoordinator.h>
#include <SpectralShipGen/ShipFiringAnimator.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipLateralMovementAnimator.h>

int main()
{
    SpectralShipGen::ShipGenerationSettings settings;
    settings.Seed = 0x7100000000000001ull;
    settings.Dimensions = { 96u, 96u };
    settings.Style = SpectralShipGen::ShipStyle::INDUSTRIAL;
    settings.Faction = SpectralShipGen::ShipFactionType::MILITARY;

    const auto ship = SpectralShipGen::ShipGenerator{}.generate(settings);
    const auto idle = SpectralShipGen::ShipIdleAnimator{}.generate(ship);
    const auto movement = SpectralShipGen::ShipLateralMovementAnimator{}.generate(
        ship, SpectralShipGen::ShipAnimationType::MOVE_LEFT);

    std::cout << "IDLE frames: " << idle.Sampling.ActualFrameCount
              << ", MOVE_LEFT sustain frames: " << movement.Sustain.Sampling.ActualFrameCount << '\n';

    SpectralShipGen::ShipFiringAnimator firingAnimator;
    const auto targets = firingAnimator.getAvailableTargets(ship);
    if (targets.empty())
    {
        std::cout << "This deterministic ship has no applicable firing target; FIRE is unavailable truthfully.\n";
        return 0;
    }

    SpectralShipGen::ShipAnimationStateRequest request;
    request.UnderlyingMovementType = SpectralShipGen::ShipAnimationType::MOVE_LEFT;
    request.MovementPhase = SpectralShipGen::ShipMovementAnimationPhase::SUSTAIN;
    request.MovementNormalizedTime = 0.31;
    request.FireActive = true;
    request.FiringTarget = targets.front();
    request.FiringNormalizedTime = 0.28;

    const auto composed = SpectralShipGen::ShipAnimationStateCoordinator{}.evaluate(ship, request);
    std::cout << "Composed MOVE_LEFT + FIRE at semantic time without creating a combined clip type.\n";
    return composed.Pose.Frame.empty() ? 1 : 0;
}
