#include <SpectralShipGen/ShipMovementAnimation.h>

namespace SpectralShipGen
{
    const ShipMovementAnimationClip& getMovementAnimationClip(const ShipMovementAnimation& animation, ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case ShipMovementAnimationPhase::ENTER: return animation.Enter;
        case ShipMovementAnimationPhase::SUSTAIN: return animation.Sustain;
        case ShipMovementAnimationPhase::EXIT: return animation.Exit;
        default: return animation.Enter;
        }
    }

    ShipMovementAnimationClip& getMovementAnimationClip(ShipMovementAnimation& animation, ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case ShipMovementAnimationPhase::ENTER: return animation.Enter;
        case ShipMovementAnimationPhase::SUSTAIN: return animation.Sustain;
        case ShipMovementAnimationPhase::EXIT: return animation.Exit;
        default: return animation.Enter;
        }
    }
}
