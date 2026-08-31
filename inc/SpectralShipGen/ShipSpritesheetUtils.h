#pragma once

#include <SpectralShipGen/Image.h>
#include <SpectralShipGen/ShipFiringAnimation.h>
#include <SpectralShipGen/ShipIdleAnimation.h>
#include <SpectralShipGen/ShipMovementAnimation.h>

namespace SpectralShipGen
{
    Image createHorizontalSpritesheet(const ShipIdleAnimation& animation);
    Image createHorizontalSpritesheet(const ShipMovementAnimationClip& animation);
    Image createHorizontalSpritesheet(const ShipFiringAnimation& animation);
}
