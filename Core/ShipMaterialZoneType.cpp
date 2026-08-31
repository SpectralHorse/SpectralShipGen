#include <SpectralShipGen/ShipMaterialZoneType.h>

namespace SpectralShipGen
{
    const char* getShipMaterialZoneTypeName(ShipMaterialZoneType type)
    {
        switch (type)
        {
        case ShipMaterialZoneType::WING_SURFACE: return "WING_SURFACE";
        case ShipMaterialZoneType::SHOULDER_SURFACE: return "SHOULDER_SURFACE";
        case ShipMaterialZoneType::AXIAL_BAND: return "AXIAL_BAND";
        case ShipMaterialZoneType::REAR_MECHANICAL: return "REAR_MECHANICAL";
        case ShipMaterialZoneType::COCKPIT_COLLAR: return "COCKPIT_COLLAR";
        case ShipMaterialZoneType::HARDPOINT_SURROUND: return "HARDPOINT_SURROUND";
        default: return "UNKNOWN";
        }
    }
}
