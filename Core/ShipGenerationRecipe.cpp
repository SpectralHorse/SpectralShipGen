#include <SpectralShipGen/ShipGenerationRecipe.h>

#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>

namespace SpectralShipGen
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

    ValidationResult validateShipGenerationRecipe(const ShipGenerationRecipe& recipe)
    {
        ValidationResult result;
        if (recipe.Dimensions.Width < 16u || recipe.Dimensions.Height < 16u)
        {
            result.Errors.push_back({ "Dimensions", "Width and height must both be at least 16 pixels." });
        }
        if (recipe.Dimensions.Width > 4096u || recipe.Dimensions.Height > 4096u)
        {
            result.Errors.push_back({ "Dimensions", "Width and height must not exceed 4096 pixels." });
        }
        if (recipe.DetailDensity > 100u)
        {
            result.Errors.push_back({ "DetailDensity", "Probability must be in the range 0-100." });
        }
        if (recipe.AsymmetricDetailChance > 100u)
        {
            result.Errors.push_back({ "AsymmetricDetailChance", "Probability must be in the range 0-100." });
        }
        if (recipe.RandomStreamMode >= GenerationRandomStreamMode::GENERATION_RANDOM_STREAM_MODE_END)
        {
            result.Errors.push_back({ "RandomStreamMode", "Random stream mode is outside the supported range." });
        }

        if (recipe.StructuralSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
        {
            if (recipe.Style >= ShipStyle::SHIP_STYLE_END) { result.Errors.push_back({ "StructuralSource", "Built-in structural source requires a valid preset." }); }
        }
        else if (recipe.StructuralSource == ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM)
        {
            appendValidationIssues(result, validateShipGenerationProfile(recipe.StructuralProfile), "StructuralProfile.");
        }
        else
        {
            result.Errors.push_back({ "StructuralSource", "Structural recipe source is outside the supported range." });
        }

        if (recipe.FactionSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
        {
            if (recipe.Faction >= ShipFactionType::SHIP_FACTION_TYPE_END) { result.Errors.push_back({ "FactionSource", "Built-in faction source requires a valid preset." }); }
        }
        else if (recipe.FactionSource == ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM)
        {
            appendValidationIssues(result, validateShipFactionProfile(recipe.FactionProfile), "FactionProfile.");
        }
        else
        {
            result.Errors.push_back({ "FactionSource", "Faction recipe source is outside the supported range." });
        }

        if (recipe.PaletteConfiguration.Mode >= ShipPaletteSourceMode::SHIP_PALETTE_SOURCE_MODE_END)
        {
            result.Errors.push_back({ "PaletteConfiguration.Mode", "Palette source mode is outside the supported range." });
        }
        else if (recipe.PaletteConfiguration.Mode == ShipPaletteSourceMode::EXPLICIT_GENERATED)
        {
            appendValidationIssues(result, validateShipPaletteGenerationProfile(recipe.PaletteConfiguration.Generated), "PaletteConfiguration.Generated.");
        }
        return result;
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
    ShipGenerationRecipe makeShipGenerationRecipe(const ShipResolvedGenerationConfiguration& configuration)
    {
        ShipGenerationRecipe recipe;
        const auto& settings = configuration.Generation;
        copyCommon(recipe, settings.Seed, settings.Dimensions, settings.DetailDensity, settings.AsymmetricDetailChance, settings.AttachmentsEnabled, settings.SeedOverrides, settings.DomainSeedOverrides, settings.RandomStreamMode);
        if (configuration.Provenance.StructuralPreset.has_value())
        {
            recipe.StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
            recipe.Style = *configuration.Provenance.StructuralPreset;
        }
        else
        {
            recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
            recipe.Style = ShipStyle::SHIP_STYLE_END;
            recipe.StructuralProfile = configuration.StructuralProfile;
        }
        if (configuration.Provenance.FactionPreset.has_value())
        {
            recipe.FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
            recipe.Faction = *configuration.Provenance.FactionPreset;
        }
        else
        {
            recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
            recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
            recipe.FactionProfile = configuration.FactionProfile;
        }
        recipe.PaletteConfiguration = settings.PaletteConfiguration;
        return recipe;
    }

}
