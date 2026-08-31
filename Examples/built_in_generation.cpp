#include <iostream>

#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

int main()
{
    SpectralShipGen::ShipGenerationSettings settings;
    settings.Seed = 0x123456789ABCDEF0ull;
    settings.Dimensions = { 64u, 64u };
    settings.Style = SpectralShipGen::ShipStyle::FIGHTER;
    settings.Faction = SpectralShipGen::ShipFactionType::MILITARY;

    const SpectralShipGen::GeneratedShip ship = SpectralShipGen::ShipGenerator{}.generate(settings);
    std::cout << "Generated " << ship.FinalImage.getWidth() << 'x' << ship.FinalImage.getHeight()
              << " ship from built-in FIGHTER + MILITARY presets.\n";
    return ship.FinalImage.empty() ? 1 : 0;
}
