#include <SpectralShipGen/ShipFactionPaletteProfile.h>

#include <SpectralShipGen/ShipFactionProfile.h>

namespace SpectralShipGen
{
    const ShipFactionPaletteProfile& getShipFactionPaletteProfile(ShipFactionType faction)
    {
        return getShipFactionProfile(faction).Palette;
    }
}
