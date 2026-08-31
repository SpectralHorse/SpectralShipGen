#include <iostream>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

int main()
{
    SpectralShipGen::ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0x1040000000000005ull;
    configuration.Dimensions = { 96u, 64u };
    configuration.PaletteConfiguration.Mode = SpectralShipGen::ShipPaletteSourceMode::FIXED;
    configuration.PaletteConfiguration.Fixed.HullBase = { 40u, 55u, 82u, 255u };
    configuration.PaletteConfiguration.Fixed.HullShadow = { 23u, 31u, 49u, 255u };
    configuration.PaletteConfiguration.Fixed.HullHighlight = { 85u, 110u, 150u, 255u };
    configuration.PaletteConfiguration.Fixed.HullAccent = { 230u, 80u, 135u, 255u };
    configuration.PaletteConfiguration.Fixed.CockpitBase = { 40u, 190u, 220u, 255u };
    configuration.PaletteConfiguration.Fixed.LightBase = { 110u, 255u, 180u, 255u };

    SpectralShipGen::ShipGenerationProfile structural =
        SpectralShipGen::getBuiltInStructuralPresetProfile(SpectralShipGen::ShipStyle::INDUSTRIAL);
    structural.LargeWeaponChance = 88u;
    structural.StructuralNegativeSpaceChance = 45u;

    SpectralShipGen::ShipFactionProfile faction =
        SpectralShipGen::getBuiltInFactionPresetProfile(SpectralShipGen::ShipFactionType::RELIC);
    faction.Weapons.EmissiveChance = 55u;
    faction.SurfaceDetails.DetailDensityPercent = 120u;

    if (!SpectralShipGen::validateShipGenerationProfile(structural).isValid() ||
        !SpectralShipGen::validateShipFactionProfile(faction).isValid())
    {
        return 1;
    }

    // The generation call receives public values, not ShipStyle/ShipFactionType selectors.
    const auto ship = SpectralShipGen::ShipGenerator{}.generate(configuration, structural, faction);
    std::cout << "Generated from custom structural + custom faction + fixed palette values.\n";
    return ship.FinalImage.empty() ? 1 : 0;
}
