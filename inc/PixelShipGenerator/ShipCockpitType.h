#pragma once

#include <cstdint>

namespace PixelShipGenerator
{
    enum class CockpitSizeClass : uint32_t
    {
        COMPACT = 0u,
        STANDARD,
        LARGE,
        MASSIVE,
        COCKPIT_SIZE_CLASS_END
    };

    enum class CockpitShapeType : uint32_t
    {
        COMPACT_CANOPY = 0u,
        ELONGATED_CANOPY,
        WIDE_COMMAND_DECK,
        SPLIT_CANOPY,
        DORSAL_BRIDGE,
        LAYERED_BRIDGE,
        COCKPIT_SHAPE_TYPE_END
    };

    inline const char* getCockpitSizeClassName(CockpitSizeClass sizeClass)
    {
        switch (sizeClass)
        {
        case CockpitSizeClass::COMPACT: return "COMPACT";
        case CockpitSizeClass::STANDARD: return "STANDARD";
        case CockpitSizeClass::LARGE: return "LARGE";
        case CockpitSizeClass::MASSIVE: return "MASSIVE";
        default: return "UNKNOWN";
        }
    }

    inline const char* getCockpitShapeTypeName(CockpitShapeType shapeType)
    {
        switch (shapeType)
        {
        case CockpitShapeType::COMPACT_CANOPY: return "COMPACT_CANOPY";
        case CockpitShapeType::ELONGATED_CANOPY: return "ELONGATED_CANOPY";
        case CockpitShapeType::WIDE_COMMAND_DECK: return "WIDE_COMMAND_DECK";
        case CockpitShapeType::SPLIT_CANOPY: return "SPLIT_CANOPY";
        case CockpitShapeType::DORSAL_BRIDGE: return "DORSAL_BRIDGE";
        case CockpitShapeType::LAYERED_BRIDGE: return "LAYERED_BRIDGE";
        default: return "UNKNOWN";
        }
    }
}
