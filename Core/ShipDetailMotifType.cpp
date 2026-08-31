#include <SpectralShipGen/ShipDetailMotifType.h>

namespace SpectralShipGen
{
    const char* getShipDetailMotifTypeName(ShipDetailMotifType type)
    {
        switch (type)
        {
        case ShipDetailMotifType::PAIRED_VENTS: return "PAIRED_VENTS";
        case ShipDetailMotifType::TRIPLE_VENT_BANK: return "TRIPLE_VENT_BANK";
        case ShipDetailMotifType::PAIRED_LIGHTS: return "PAIRED_LIGHTS";
        case ShipDetailMotifType::THREE_NODE_LIGHTS: return "THREE_NODE_LIGHTS";
        case ShipDetailMotifType::PARALLEL_SEAMS: return "PARALLEL_SEAMS";
        case ShipDetailMotifType::REPEATED_DASHES: return "REPEATED_DASHES";
        case ShipDetailMotifType::RECESSED_SLOT: return "RECESSED_SLOT";
        default: return "NONE";
        }
    }
}
