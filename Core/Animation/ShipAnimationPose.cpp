#include <PixelShipGenerator/ShipAnimationPose.h>

#include <algorithm>

namespace PixelShipGenerator
{
    const ShipAnimationComponentTransform* findAnimationComponentTransform(const ShipAnimationPose& pose, ShipAnimationSemanticComponentType type, uint32_t componentIndex)
    {
        const auto iterator = std::find_if(pose.ComponentTransforms.begin(), pose.ComponentTransforms.end(), [&](const ShipAnimationComponentTransform& transform)
        {
            return transform.Type == type && transform.ComponentIndex == componentIndex;
        });
        return iterator == pose.ComponentTransforms.end() ? nullptr : &(*iterator);
    }
} // namespace PixelShipGenerator
