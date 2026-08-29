#pragma once

#include "Image.h"
#include "ShipIdleAnimation.h"
#include "ShipMovementAnimation.h"

namespace PixelShipGenerator
{
    Image createHorizontalSpritesheet(const ShipIdleAnimation& animation);
    Image createHorizontalSpritesheet(const ShipMovementAnimationClip& animation);
}
