#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipHullLayerType : uint32_t
    {
        CENTRAL_DORSAL_PLATE = 0u,
        FORWARD_ARMOR,
        WING_ARMOR,
        SHOULDER_ARMOR,
        REAR_ENGINE_COVER,
        SHIP_HULL_LAYER_TYPE_END
    };

    inline const char* getShipHullLayerTypeName(ShipHullLayerType type)
    {
        switch (type)
        {
        case ShipHullLayerType::CENTRAL_DORSAL_PLATE: return "CENTRAL_DORSAL_PLATE";
        case ShipHullLayerType::FORWARD_ARMOR: return "FORWARD_ARMOR";
        case ShipHullLayerType::WING_ARMOR: return "WING_ARMOR";
        case ShipHullLayerType::SHOULDER_ARMOR: return "SHOULDER_ARMOR";
        case ShipHullLayerType::REAR_ENGINE_COVER: return "REAR_ENGINE_COVER";
        default: return "UNKNOWN";
        }
    }
}
