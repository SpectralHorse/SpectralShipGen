#include "ShipFactionPaletteProfile.h"

#include "ShipFactionProfile.h"

namespace PixelShipGenerator
{
    const ShipFactionPaletteProfile& getShipFactionPaletteProfile(ShipFactionType faction)
    {
        if (faction >= ShipFactionType::SHIP_FACTION_TYPE_END)
        {
            return getShipFactionProfile(ShipFactionType::FRONTIER).Palette;
        }
        return getShipFactionProfile(faction).Palette;
    }
}
