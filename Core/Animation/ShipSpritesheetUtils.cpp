#include "ShipSpritesheetUtils.h"

#include <cstdint>

namespace PixelShipGenerator
{
    Image createHorizontalSpritesheet(const ShipIdleAnimation& animation)
    {
        Image spritesheet;

        if (animation.Frames.empty() || animation.FrameWidth == 0u || animation.FrameHeight == 0u)
        {
            return spritesheet;
        }

        const uint32_t spritesheetWidth = animation.FrameWidth * static_cast<uint32_t>(animation.Frames.size());
        spritesheet.reset(spritesheetWidth, animation.FrameHeight);

        for (uint32_t frameIndex = 0u; frameIndex < animation.Frames.size(); ++frameIndex)
        {
            const Image& frame = animation.Frames[frameIndex];
            const uint32_t frameOffsetX = frameIndex * animation.FrameWidth;

            for (uint32_t y = 0u; y < animation.FrameHeight; ++y)
            {
                for (uint32_t x = 0u; x < animation.FrameWidth; ++x)
                {
                    spritesheet.setPixel(frameOffsetX + x, y, frame.getPixel(x, y));
                }
            }
        }

        return spritesheet;
    }
}
