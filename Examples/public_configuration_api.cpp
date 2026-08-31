#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>
#include <SpectralShipGen/ShipGenerator.h>

using namespace SpectralShipGen;

void builtInConvenience()
{
    ShipGenerationSettings settings;
    settings.Seed = 1234u;
    settings.Dimensions = { 64u, 64u };
    settings.Style = ShipStyle::FIGHTER;
    settings.Faction = ShipFactionType::MILITARY;
    const GeneratedShip ship = ShipGenerator().generate(settings);
    (void)ship;
}

ShipResolvedGenerationConfiguration fullyExplicitConfiguration()
{
    ExplicitShipGenerationConfiguration settings;
    settings.Seed = 5678u;
    settings.Dimensions = { 96u, 64u };

    ShipGenerationProfile structural = getBuiltInStructuralPresetProfile(ShipStyle::INDUSTRIAL);
    structural.LargeWeaponChance = 85u;
    structural.LargeWeaponScalePercent = 140u;

    ShipFactionProfile faction = getBuiltInFactionPresetProfile(ShipFactionType::RELIC);
    faction.SurfaceDetails.DetailDensityPercent = 70u;

    settings.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
    settings.PaletteConfiguration.Generated = getBuiltInPalettePresetProfile(ShipFactionType::ASCENDANT);
    settings.PaletteConfiguration.Generated.Ranges.HullHue = { 280u, 320u };

    // This resolved configuration has no built-in structural/faction identity.
    return resolveShipGenerationConfiguration(settings, structural, faction);
}

void fixedPaletteAndPortableRecipe()
{
    ShipResolvedGenerationConfiguration configuration = fullyExplicitConfiguration();
    configuration.Generation.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
    configuration.Generation.PaletteConfiguration.Fixed.HullBase = { 70u, 90u, 120u, 255u };
    configuration.Generation.PaletteConfiguration.Fixed.HullAccent = { 230u, 80u, 40u, 255u };
    configuration.Provenance.PaletteSource = ShipPaletteSourceMode::FIXED;
    configuration.Provenance.PaletteFactionPreset.reset();

    ShipGenerator generator;
    const GeneratedShip ship = generator.generate(configuration);

    ShipGenerationRecipeDocument document;
    document.Recipe = makeShipGenerationRecipe(configuration);
    const std::string json = serializeShipGenerationRecipe(document);
    const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(json);
    if (loaded.Success)
    {
        const GeneratedShip reproduced = generator.generate(loaded.Document.Recipe);
        (void)reproduced;
    }
    (void)ship;
}
