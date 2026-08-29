#include "ShipSpritesheetUtils.h"

#include <cstdint>

namespace
{
    PixelShipGenerator::Image createSpritesheet(const std::vector<PixelShipGenerator::Image>& frames, uint32_t frameWidth, uint32_t frameHeight)
    {
        PixelShipGenerator::Image spritesheet;
        if (frames.empty() || frameWidth == 0u || frameHeight == 0u) { return spritesheet; }

        spritesheet.reset(frameWidth * static_cast<uint32_t>(frames.size()), frameHeight);
        for (uint32_t frameIndex = 0u; frameIndex < frames.size(); ++frameIndex)
        {
            const PixelShipGenerator::Image& frame = frames[frameIndex];
            const uint32_t frameOffsetX = frameIndex * frameWidth;
            for (uint32_t y = 0u; y < frameHeight; ++y)
            {
                for (uint32_t x = 0u; x < frameWidth; ++x) { spritesheet.setPixel(frameOffsetX + x, y, frame.getPixel(x, y)); }
            }
        }
        return spritesheet;
    }
}

namespace PixelShipGenerator
{
    Image createHorizontalSpritesheet(const ShipIdleAnimation& animation)
    {
        return createSpritesheet(animation.Frames, animation.FrameWidth, animation.FrameHeight);
    }

    Image createHorizontalSpritesheet(const ShipMovementAnimationClip& animation)
    {
        return createSpritesheet(animation.Frames, animation.FrameWidth, animation.FrameHeight);
    }
}
