#include <iostream>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>

int main()
{
    const auto structural = SpectralShipGen::getBuiltInStructuralPresetProfile(SpectralShipGen::ShipStyle::DELTA);
    const auto faction = SpectralShipGen::getBuiltInFactionPresetProfile(SpectralShipGen::ShipFactionType::XENO);

    SpectralShipGen::ExplicitShipGenerationConfiguration generatedConfiguration;
    generatedConfiguration.Seed = 0x1040000000000003ull;
    generatedConfiguration.Dimensions = { 96u, 64u };
    generatedConfiguration.PaletteConfiguration.Mode = SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
    generatedConfiguration.PaletteConfiguration.Generated =
        SpectralShipGen::getBuiltInPalettePresetProfile(SpectralShipGen::ShipFactionType::ASCENDANT);
    generatedConfiguration.PaletteConfiguration.Generated.Ranges.HullHue = { 165u, 220u };
    generatedConfiguration.PaletteConfiguration.Generated.Ranges.Accent.Saturation = { 80u, 100u };

    if (!SpectralShipGen::validateShipPaletteGenerationProfile(generatedConfiguration.PaletteConfiguration.Generated).isValid())
    {
        return 1;
    }

    const auto generatedPaletteShip = SpectralShipGen::ShipGenerator{}.generate(generatedConfiguration, structural, faction);

    SpectralShipGen::ExplicitShipGenerationConfiguration fixedConfiguration = generatedConfiguration;
    fixedConfiguration.Seed = 0x1040000000000004ull;
    fixedConfiguration.PaletteConfiguration.Mode = SpectralShipGen::ShipPaletteSourceMode::FIXED;
    auto& fixed = fixedConfiguration.PaletteConfiguration.Fixed;
    fixed.HullBase = { 46u, 58u, 78u, 255u };
    fixed.HullShadow = { 28u, 36u, 52u, 255u };
    fixed.HullHighlight = { 82u, 98u, 124u, 255u };
    fixed.HullAccent = { 232u, 92u, 62u, 255u };
    fixed.CockpitBase = { 52u, 170u, 204u, 255u };
    fixed.LightBase = { 68u, 218u, 198u, 255u };
    fixed.LightHighlight = { 176u, 255u, 236u, 255u };

    const auto fixedPaletteShip = SpectralShipGen::ShipGenerator{}.generate(fixedConfiguration, structural, faction);
    std::cout << "Generated both an explicit generated palette and an exact fixed semantic palette.\n";
    return generatedPaletteShip.FinalImage.empty() || fixedPaletteShip.FinalImage.empty() ? 1 : 0;
}
