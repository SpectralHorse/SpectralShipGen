#include <SpectralShipGen/ShipStructuralNegativeSpaceType.h>

namespace SpectralShipGen
{
    const char* getShipStructuralNegativeSpaceTypeName(ShipStructuralNegativeSpaceType type)
    {
        switch (type)
        {
        case ShipStructuralNegativeSpaceType::WING_CHANNEL: return "WING_CHANNEL";
        case ShipStructuralNegativeSpaceType::REAR_FORK: return "REAR_FORK";
        case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return "SHOULDER_GAP";
        case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return "OPEN_FRAME_BAY";
        case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return "NACELLE_CHANNEL";
        default: return "UNKNOWN";
        }
    }
}
