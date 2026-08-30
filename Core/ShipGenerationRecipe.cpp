#include "ShipGenerationRecipe.h"

#include "ShipGenerationSettings.h"

namespace PixelShipGenerator
{
    namespace
    {
        ShipGenerationSeeds resolveRecipeSeeds(uint64_t seed, const ShipGenerationSeedOverrides& overrides)
        {
            return applyShipGenerationSeedOverrides(deriveShipGenerationSeeds(seed), overrides);
        }

        void copyCommon(ShipGenerationRecipe& recipe,
            uint64_t seed,
            ShipDimensions dimensions,
            uint32_t detailDensity,
            uint32_t asymmetricDetailChance,
            bool attachmentsEnabled,
            const ShipGenerationSeedOverrides& seedOverrides,
            const GenerationDomainSeedOverrides& domainSeedOverrides,
            GenerationRandomStreamMode randomStreamMode)
        {
            recipe.Seeds = resolveRecipeSeeds(seed, seedOverrides);
            recipe.DomainSeedOverrides = domainSeedOverrides;
            recipe.RandomStreamMode = randomStreamMode;
            recipe.Dimensions = dimensions;
            recipe.DetailDensity = detailDensity;
            recipe.AsymmetricDetailChance = asymmetricDetailChance;
            recipe.AttachmentsEnabled = attachmentsEnabled;
        }
    }

    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationSettings& settings)
    {
        ShipGenerationRecipe recipe;
        copyCommon(recipe, settings.Seed, settings.Dimensions, settings.DetailDensity, settings.AsymmetricDetailChance, settings.AttachmentsEnabled, settings.SeedOverrides, settings.DomainSeedOverrides, settings.RandomStreamMode);
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        recipe.Style = settings.Style;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        recipe.Faction = settings.Faction;
        recipe.PaletteConfiguration.Mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;
        return recipe;
    }

    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile)
    {
        ShipGenerationRecipe recipe;
        copyCommon(recipe, configuration.Seed, configuration.Dimensions, configuration.DetailDensity, configuration.AsymmetricDetailChance, configuration.AttachmentsEnabled, configuration.SeedOverrides, configuration.DomainSeedOverrides, configuration.RandomStreamMode);
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = profile;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        recipe.Faction = configuration.Faction;
        recipe.PaletteConfiguration = configuration.PaletteConfiguration;
        return recipe;
    }

    ShipGenerationRecipe makeShipGenerationRecipe(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile)
    {
        ShipGenerationRecipe recipe;
        copyCommon(recipe, configuration.Seed, configuration.Dimensions, configuration.DetailDensity, configuration.AsymmetricDetailChance, configuration.AttachmentsEnabled, configuration.SeedOverrides, configuration.DomainSeedOverrides, configuration.RandomStreamMode);
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = profile;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = factionProfile;
        recipe.PaletteConfiguration = configuration.PaletteConfiguration;
        return recipe;
    }
}
