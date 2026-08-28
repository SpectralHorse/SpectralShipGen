#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Image.h"

namespace PixelShipGenerator
{
    struct ShipIdleAnimationSettings
    {
        uint32_t FrameCount = 10u;
        bool EngineFlicker = true;
        bool LightBlinking = true;
        bool MechanicalMicroMovement = true;
        bool HoverOffset = true;
        bool SmallDetailVariation = true;
        std::optional<uint64_t> Seed;
    };

    struct ShipIdleAnimation
    {
        std::vector<Image> Frames;
        uint32_t FrameWidth = 0u;
        uint32_t FrameHeight = 0u;
        uint64_t Seed = 0u;
    };
}
