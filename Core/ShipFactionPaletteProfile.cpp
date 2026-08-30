#include "ShipFactionPaletteProfile.h"

#include "ShipFactionProfile.h"

namespace PixelShipGenerator
{
    const ShipFactionPaletteProfile& getShipFactionPaletteProfile(ShipFactionType faction)
    {
        return getShipFactionProfile(faction).Palette;
    }
}
