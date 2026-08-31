#include <stdexcept>
#include <string>

#include <PixelShipGenerator/ShipGenerationRecipe.h>
#include <PixelShipGenerator/ShipGenerationRecipeSerializer.h>
#include <PixelShipGenerator/ShipGenerationSettings.h>
#include <PixelShipGenerator/ShipGenerator.h>

using namespace PixelShipGenerator;

GeneratedShip generateFromPortableBuiltInRecipe()
{
    ShipGenerationSettings settings;
    settings.Seed = 0x123456789ABCDEF0ull;
    settings.Dimensions = { 64u, 64u };
    settings.Style = ShipStyle::FIGHTER;
    settings.Faction = ShipFactionType::MILITARY;

    ShipGenerationRecipeDocument document;
    document.Recipe = makeShipGenerationRecipe(settings);

    const std::string json = serializeShipGenerationRecipe(document);
    const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(json);
    if (!loaded.Success)
    {
        throw std::runtime_error(loaded.Error);
    }

    return ShipGenerator{}.generate(loaded.Document.Recipe);
}

ShipGenerationRecipeDocument makeFullyCustomPortableRecipe()
{
    ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0x8700000000000087ull;
    configuration.Dimensions = { 96u, 64u };
    configuration.DetailDensity = 68u;
    configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
    configuration.PaletteConfiguration.Fixed.HullBase = Color(44u, 77u, 109u, 255u);
    configuration.PaletteConfiguration.Fixed.HullAccent = Color(211u, 82u, 141u, 255u);
    configuration.PaletteConfiguration.Fixed.CockpitBase = Color(29u, 181u, 207u, 255u);
    configuration.PaletteConfiguration.Fixed.LightBase = Color(105u, 255u, 176u, 255u);

    ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
    structuralProfile.LargeWeaponChance = 82u;
    structuralProfile.LargeWeaponScalePercent = 145u;

    ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::RELIC);
    factionProfile.Weapons.EmissiveChance = 47u;
    factionProfile.SurfaceDetails.DetailDensityPercent = 91u;

    ShipGenerationRecipeDocument document;
    document.Recipe = makeShipGenerationRecipe(configuration, structuralProfile, factionProfile);
    return document;
}

GeneratedShip roundTripFullyCustomRecipe()
{
    const ShipGenerationRecipeDocument document = makeFullyCustomPortableRecipe();
    const std::string json = serializeShipGenerationRecipe(document);

    const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(json);
    if (!loaded.Success)
    {
        throw std::runtime_error(loaded.Error);
    }

    // The embedded profiles are sufficient by themselves. No local custom-preset
    // database and no fabricated ShipStyle/ShipFactionType identity are required.
    return ShipGenerator{}.generate(loaded.Document.Recipe);
}
