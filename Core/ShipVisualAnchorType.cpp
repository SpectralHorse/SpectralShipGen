#include <PixelShipGenerator/ShipVisualAnchorType.h>

namespace PixelShipGenerator
{
    const char* getShipVisualAnchorTypeName(ShipVisualAnchorType type)
    {
        switch (type)
        {
        case ShipVisualAnchorType::SILHOUETTE: return "SILHOUETTE";
        case ShipVisualAnchorType::COCKPIT: return "COCKPIT";
        case ShipVisualAnchorType::WINGS: return "WINGS";
        case ShipVisualAnchorType::ENGINES: return "ENGINES";
        case ShipVisualAnchorType::WEAPONS: return "WEAPONS";
        case ShipVisualAnchorType::MAJOR_FEATURE: return "MAJOR_FEATURE";
        case ShipVisualAnchorType::HULL_LAYERS: return "HULL_LAYERS";
        case ShipVisualAnchorType::CENTRAL_CORE: return "CENTRAL_CORE";
        case ShipVisualAnchorType::MACRO_ASYMMETRY: return "MACRO_ASYMMETRY";
        case ShipVisualAnchorType::NEGATIVE_SPACE: return "NEGATIVE_SPACE";
        default: return "NONE";
        }
    }
}
