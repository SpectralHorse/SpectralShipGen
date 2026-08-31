#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>
#include <SpectralShipGen/ShipGenerator.h>

#include <iostream>

int main()
{
    using namespace SpectralShipGen;

    ShipGenerationSettings settings;
    settings.Seed = 0x103103103ull;
    settings.Dimensions = { 64u, 64u };
    settings.Style = ShipStyle::FIGHTER;
    settings.Faction = ShipFactionType::MILITARY;

    ShipGenerator generator;
    const GeneratedShip first = generator.generate(settings);
    const GeneratedShip second = generator.generate(settings);
    if (first.FinalImage.getPixels() != second.FinalImage.getPixels())
    {
        std::cerr << "Deterministic built-in generation mismatch.\n";
        return 1;
    }

    ExplicitShipGenerationConfiguration explicitConfiguration;
    explicitConfiguration.Seed = 0xC0570C0FFEEull;
    explicitConfiguration.Dimensions = { 96u, 64u };
    ShipGenerationProfile structural = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
    structural.LargeWeaponChance = 80u;
    ShipFactionProfile faction = getShipFactionProfile(ShipFactionType::CORPORATE);

    const GeneratedShip custom = generator.generate(explicitConfiguration, structural, faction);
    if (custom.FinalImage.empty())
    {
        std::cerr << "Explicit custom configuration produced no image.\n";
        return 2;
    }

    ShipGenerationRecipeDocument document;
    document.Recipe = makeShipGenerationRecipe(settings);
    const std::string json = serializeShipGenerationRecipe(document);
    const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(json);
    if (!loaded.Success)
    {
        std::cerr << "Recipe round-trip failed: " << loaded.Error << '\n';
        return 3;
    }

    const GeneratedShip reproduced = generator.generate(loaded.Document.Recipe);
    if (first.FinalImage.getPixels() != reproduced.FinalImage.getPixels())
    {
        std::cerr << "Recipe reproduction mismatch.\n";
        return 4;
    }

    return 0;
}
