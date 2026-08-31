#include <iostream>
#include <string>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>
#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    bool imagesEqual(const SpectralShipGen::Image& a, const SpectralShipGen::Image& b)
    {
        return a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight() && a.getPixels() == b.getPixels();
    }
}

int main()
{
    SpectralShipGen::ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0x1040000000000006ull;
    configuration.Dimensions = { 96u, 64u };

    auto structural = SpectralShipGen::getBuiltInStructuralPresetProfile(SpectralShipGen::ShipStyle::HEAVY);
    auto faction = SpectralShipGen::getBuiltInFactionPresetProfile(SpectralShipGen::ShipFactionType::FRONTIER);
    structural.LargeWeaponScalePercent = 125u;
    faction.Weapons.EmissiveChance = 35u;

    SpectralShipGen::ShipGenerationRecipeDocument document;
    document.Recipe = SpectralShipGen::makeShipGenerationRecipe(configuration, structural, faction);

    const std::string json = SpectralShipGen::serializeShipGenerationRecipe(document);
    const auto loaded = SpectralShipGen::deserializeShipGenerationRecipe(json);
    if (!loaded.Success)
    {
        std::cerr << loaded.Error << '\n';
        return 1;
    }

    SpectralShipGen::ShipGenerator generator;
    const auto first = generator.generate(document.Recipe);
    const auto second = generator.generate(loaded.Document.Recipe);
    std::cout << "Portable recipe round trip reproduced the same pixels without a GUI preset database.\n";
    return imagesEqual(first.FinalImage, second.FinalImage) ? 0 : 1;
}
