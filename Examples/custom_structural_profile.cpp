#include <iostream>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

int main()
{
    SpectralShipGen::ShipGenerationConfiguration configuration;
    configuration.Seed = 0x1040000000000001ull;
    configuration.Dimensions = { 96u, 64u };
    configuration.Faction = SpectralShipGen::ShipFactionType::MILITARY;

    SpectralShipGen::ShipGenerationProfile structural =
        SpectralShipGen::getBuiltInStructuralPresetProfile(SpectralShipGen::ShipStyle::INDUSTRIAL);
    structural.LargeWeaponChance = 82u;
    structural.MaximumLargeWeaponGroups = 2u;
    structural.LargeWeaponScalePercent = 135u;

    const auto validation = SpectralShipGen::validateShipGenerationProfile(structural);
    if (!validation.isValid())
    {
        std::cerr << validation.Errors.front().Field << ": " << validation.Errors.front().Message << '\n';
        return 1;
    }

    const auto ship = SpectralShipGen::ShipGenerator{}.generate(configuration, structural);
    std::cout << "Generated a validated custom structural profile without assigning a custom ShipStyle identity.\n";
    return ship.FinalImage.empty() ? 1 : 0;
}
