#pragma once

#include "Image.h"
#include "ShipFiringAnimation.h"
#include "ShipIdleAnimation.h"
#include "ShipMovementAnimation.h"

namespace PixelShipGenerator
{
    Image createHorizontalSpritesheet(const ShipIdleAnimation& animation);
    Image createHorizontalSpritesheet(const ShipMovementAnimationClip& animation);
    Image createHorizontalSpritesheet(const ShipFiringAnimation& animation);
}
