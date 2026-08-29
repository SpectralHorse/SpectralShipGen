#include <PixelShipGenerator/ShipLiveryType.h>

namespace PixelShipGenerator
{
    const char* getShipLiveryTypeName(ShipLiveryType type)
    {
        switch (type)
        {
        case ShipLiveryType::CENTER_STRIPE: return "CENTER_STRIPE";
        case ShipLiveryType::DOUBLE_CENTER_STRIPE: return "DOUBLE_CENTER_STRIPE";
        case ShipLiveryType::WING_BAND: return "WING_BAND";
        case ShipLiveryType::SHOULDER_BLOCK: return "SHOULDER_BLOCK";
        case ShipLiveryType::NOSE_BAND: return "NOSE_BAND";
        case ShipLiveryType::CHEVRON: return "CHEVRON";
        case ShipLiveryType::ID_PANEL: return "ID_PANEL";
        case ShipLiveryType::GEOMETRIC_INSIGNIA: return "GEOMETRIC_INSIGNIA";
        default: return "UNKNOWN";
        }
    }
}
