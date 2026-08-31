#include <PixelShipGenerator/ShipFactionPaletteProfile.h>

#include <PixelShipGenerator/ShipFactionProfile.h>

namespace PixelShipGenerator
{
    const ShipFactionPaletteProfile& getShipFactionPaletteProfile(ShipFactionType faction)
    {
        return getShipFactionProfile(faction).Palette;
    }
}
