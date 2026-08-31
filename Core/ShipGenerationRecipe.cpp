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
            const GenerationDomainSeedOverrides& domainSeedOverrides)
        {
            recipe.Seeds = resolveRecipeSeeds(seed, seedOverrides);
            recipe.DomainSeedOverrides = domainSeedOverrides;
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

        if (recipe.StructuralPreset.has_value())
        {
            if (*recipe.StructuralPreset >= ShipStyle::SHIP_STYLE_END)
            {
                result.Errors.push_back({ "StructuralPreset", "Optional built-in structural preset must reference a valid preset." });
            }
        }
        else
        {
            appendValidationIssues(result, validateShipGenerationProfile(recipe.StructuralProfile), "StructuralProfile.");
        }

        if (recipe.FactionPreset.has_value())
        {
            if (*recipe.FactionPreset >= ShipFactionType::SHIP_FACTION_TYPE_END)
            {
                result.Errors.push_back({ "FactionPreset", "Optional built-in faction preset must reference a valid preset." });
            }
        }
        else
        {
            appendValidationIssues(result, validateShipFactionProfile(recipe.FactionProfile), "FactionProfile.");
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
        copyCommon(recipe, settings.Seed, settings.Dimensions, settings.DetailDensity, settings.AsymmetricDetailChance, settings.AttachmentsEnabled, settings.SeedOverrides, settings.DomainSeedOverrides);
        recipe.StructuralPreset = settings.Style;
        recipe.FactionPreset = settings.Faction;
        recipe.PaletteConfiguration.Mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;
        return recipe;
    }

    ShipGenerationRecipe makeShipGenerationRecipe(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile)
    {
        ShipGenerationRecipe recipe;
        copyCommon(recipe, configuration.Seed, configuration.Dimensions, configuration.DetailDensity, configuration.AsymmetricDetailChance, configuration.AttachmentsEnabled, configuration.SeedOverrides, configuration.DomainSeedOverrides);
        recipe.StructuralPreset.reset();
        recipe.StructuralProfile = profile;
        recipe.FactionPreset = configuration.Faction;
        recipe.PaletteConfiguration = configuration.PaletteConfiguration;
        return recipe;
    }

    ShipGenerationRecipe makeShipGenerationRecipe(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile)
    {
        ShipGenerationRecipe recipe;
        copyCommon(recipe, configuration.Seed, configuration.Dimensions, configuration.DetailDensity, configuration.AsymmetricDetailChance, configuration.AttachmentsEnabled, configuration.SeedOverrides, configuration.DomainSeedOverrides);
        recipe.StructuralPreset.reset();
        recipe.StructuralProfile = profile;
        recipe.FactionPreset.reset();
        recipe.FactionProfile = factionProfile;
        recipe.PaletteConfiguration = configuration.PaletteConfiguration;
        return recipe;
    }

    ShipGenerationRecipe makeShipGenerationRecipe(const ShipResolvedGenerationConfiguration& configuration)
    {
        ShipGenerationRecipe recipe;
        const auto& settings = configuration.Generation;
        copyCommon(recipe, settings.Seed, settings.Dimensions, settings.DetailDensity, settings.AsymmetricDetailChance, settings.AttachmentsEnabled, settings.SeedOverrides, settings.DomainSeedOverrides);
        recipe.StructuralPreset = configuration.Provenance.StructuralPreset;
        if (!recipe.StructuralPreset.has_value()) { recipe.StructuralProfile = configuration.StructuralProfile; }
        recipe.FactionPreset = configuration.Provenance.FactionPreset;
        if (!recipe.FactionPreset.has_value()) { recipe.FactionProfile = configuration.FactionProfile; }
        recipe.PaletteConfiguration = settings.PaletteConfiguration;
        return recipe;
    }
}
