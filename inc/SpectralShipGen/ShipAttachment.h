#pragma once

#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipAttachmentType : uint32_t
    {
        WEAPON_MOUNT = 0,
        SENSOR_ARRAY,
        AUXILIARY_POD,
        RADIATOR,
        ARMOR_FIN,
        TECHNOLOGY_NODE,

        SHIP_ATTACHMENT_TYPE_END
    };

    enum class ShipAttachmentRegion : uint32_t
    {
        FRONT = 0,
        FRONT_SIDE,
        MIDDLE_SIDE,
        WING_OUTER_SIDE,
        REAR_SIDE,
        REAR,

        SHIP_ATTACHMENT_REGION_END
    };

    enum class ShipAttachmentDirection : uint32_t
    {
        UP = 0,
        DOWN,
        LEFT,
        RIGHT
    };

    struct ShipAttachmentPlacement
    {
        ShipAttachmentType Type = ShipAttachmentType::WEAPON_MOUNT;
        ShipAttachmentRegion Region = ShipAttachmentRegion::MIDDLE_SIDE;
        ShipAttachmentDirection Direction = ShipAttachmentDirection::LEFT;

        uint32_t AnchorX = 0u;
        uint32_t AnchorY = 0u;
        uint32_t MinimumX = 0u;
        uint32_t MaximumX = 0u;
        uint32_t MinimumY = 0u;
        uint32_t MaximumY = 0u;
        uint32_t SymmetryGroup = 0u;
    };
}