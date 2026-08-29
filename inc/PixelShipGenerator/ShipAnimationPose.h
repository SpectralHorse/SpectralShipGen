#pragma once

#include <cstdint>
#include <vector>

#include "Image.h"
#include "ShipAnimationType.h"

namespace PixelShipGenerator
{
    enum class ShipAnimationPoseLayer : uint32_t
    {
        STATIC_NEUTRAL = 0u,
        IDLE_AMBIENT,
        MOVEMENT,
        TRANSIENT_EVENT,
        SHIP_ANIMATION_POSE_LAYER_END
    };

    enum class ShipAnimationSemanticComponentType : uint32_t
    {
        ENGINE = 0u,
        WEAPON,
        ATTACHMENT,
        SHIP_ANIMATION_SEMANTIC_COMPONENT_TYPE_END
    };

    struct ShipAnimationComponentTransform
    {
        ShipAnimationSemanticComponentType Type = ShipAnimationSemanticComponentType::ENGINE;
        uint32_t ComponentIndex = 0u;
        int32_t OffsetX = 0;
        int32_t OffsetY = 0;
    };

    struct ShipAnimationPose
    {
        Image Frame;
        ShipAnimationPoseLayer Layer = ShipAnimationPoseLayer::STATIC_NEUTRAL;
        ShipAnimationType UnderlyingAnimationType = ShipAnimationType::IDLE;
        std::vector<ShipAnimationComponentTransform> ComponentTransforms;
    };

    const ShipAnimationComponentTransform* findAnimationComponentTransform(const ShipAnimationPose& pose, ShipAnimationSemanticComponentType type, uint32_t componentIndex);
} // namespace PixelShipGenerator
