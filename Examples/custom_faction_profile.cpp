#include <iostream>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

int main()
{
    SpectralShipGen::ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0x1040000000000002ull;
    configuration.Dimensions = { 96u, 64u };

    SpectralShipGen::ShipGenerationProfile structural =
        SpectralShipGen::getBuiltInStructuralPresetProfile(SpectralShipGen::ShipStyle::SPEARHEAD);
    SpectralShipGen::ShipFactionProfile faction =
        SpectralShipGen::getBuiltInFactionPresetProfile(SpectralShipGen::ShipFactionType::CORPORATE);

    faction.Weapons.EmissiveChance = 65u;
    faction.SurfaceDetails.DetailDensityPercent = 125u;
    faction.Materials.ZoneWeightMultipliersPercent.RearMechanical = 150u;

    const auto validation = SpectralShipGen::validateShipFactionProfile(faction);
    if (!validation.isValid())
    {
        std::cerr << validation.Errors.front().Field << ": " << validation.Errors.front().Message << '\n';
        return 1;
    }

    const auto ship = SpectralShipGen::ShipGenerator{}.generate(configuration, structural, faction);
    std::cout << "Generated with a custom faction profile; no built-in faction identity is required by this call.\n";
    return ship.FinalImage.empty() ? 1 : 0;
}
