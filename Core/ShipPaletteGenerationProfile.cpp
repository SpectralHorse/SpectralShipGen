#include <SpectralShipGen/ShipPaletteGenerationProfile.h>

namespace SpectralShipGen
{
    ShipPaletteGenerationProfile getShipPaletteGenerationProfile(const ShipFactionProfile& factionProfile)
    {
        ShipPaletteGenerationProfile profile;
        profile.Ranges = factionProfile.Palette;
        profile.Behavior = factionProfile.PaletteBehavior;
        return profile;
    }

    ShipPaletteGenerationProfile getShipPaletteGenerationProfile(ShipFactionType faction)
    {
        return getShipPaletteGenerationProfile(getShipFactionProfile(faction));
    }
}
