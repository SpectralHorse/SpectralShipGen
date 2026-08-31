#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>

namespace SpectralShipGen
{
    namespace
    {
        ExplicitShipGenerationConfiguration copyCommon(const ShipGenerationSettings& settings)
        {
            ExplicitShipGenerationConfiguration result;
            result.Seed = settings.Seed;
            result.Dimensions = settings.Dimensions;
            result.DetailDensity = settings.DetailDensity;
            result.AsymmetricDetailChance = settings.AsymmetricDetailChance;
            result.AttachmentsEnabled = settings.AttachmentsEnabled;
            result.SeedOverrides = settings.SeedOverrides;
            result.DomainSeedOverrides = settings.DomainSeedOverrides;
            result.RandomStreamMode = settings.RandomStreamMode;
            return result;
        }

        ExplicitShipGenerationConfiguration copyCommon(const ShipGenerationConfiguration& settings)
        {
            ExplicitShipGenerationConfiguration result;
            result.Seed = settings.Seed;
            result.Dimensions = settings.Dimensions;
            result.DetailDensity = settings.DetailDensity;
            result.AsymmetricDetailChance = settings.AsymmetricDetailChance;
            result.AttachmentsEnabled = settings.AttachmentsEnabled;
            result.SeedOverrides = settings.SeedOverrides;
            result.DomainSeedOverrides = settings.DomainSeedOverrides;
            result.RandomStreamMode = settings.RandomStreamMode;
            result.PaletteConfiguration = settings.PaletteConfiguration;
            return result;
        }

        ExplicitShipGenerationConfiguration copyCommon(const ShipGenerationRecipe& recipe)
        {
            ExplicitShipGenerationConfiguration result;
            result.Seed = recipe.Seeds.Master;
            result.Dimensions = recipe.Dimensions;
            result.DetailDensity = recipe.DetailDensity;
            result.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
            result.AttachmentsEnabled = recipe.AttachmentsEnabled;
            result.SeedOverrides.Structure = recipe.Seeds.Structure;
            result.SeedOverrides.Palette = recipe.Seeds.Palette;
            result.SeedOverrides.Details = recipe.Seeds.Details;
            result.SeedOverrides.Attachments = recipe.Seeds.Attachments;
            result.DomainSeedOverrides = recipe.DomainSeedOverrides;
            result.RandomStreamMode = recipe.RandomStreamMode;
            result.PaletteConfiguration = recipe.PaletteConfiguration;
            return result;
        }

        void setPaletteProvenance(ShipResolvedGenerationConfiguration& result)
        {
            result.Provenance.PaletteSource = result.Generation.PaletteConfiguration.Mode;
            if (result.Provenance.PaletteSource == ShipPaletteSourceMode::FACTION_PROFILE_GENERATED && result.Provenance.FactionPreset.has_value())
            {
                result.Provenance.PaletteFactionPreset = result.Provenance.FactionPreset;
            }
            else
            {
                result.Provenance.PaletteFactionPreset.reset();
            }
        }
    }

    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ShipGenerationSettings& settings)
    {
        ShipResolvedGenerationConfiguration result;
        result.Generation = copyCommon(settings);
        result.StructuralProfile = getBuiltInStructuralPresetProfile(settings.Style);
        result.FactionProfile = getBuiltInFactionPresetProfile(settings.Faction);
        result.Provenance.StructuralPreset = settings.Style;
        result.Provenance.FactionPreset = settings.Faction;
        setPaletteProvenance(result);
        return result;
    }

    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& structuralProfile)
    {
        ShipResolvedGenerationConfiguration result;
        result.Generation = copyCommon(configuration);
        result.StructuralProfile = structuralProfile;
        result.FactionProfile = getBuiltInFactionPresetProfile(configuration.Faction);
        result.Provenance.FactionPreset = configuration.Faction;
        setPaletteProvenance(result);
        return result;
    }

    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& structuralProfile, const ShipFactionProfile& factionProfile)
    {
        ShipResolvedGenerationConfiguration result;
        result.Generation = configuration;
        result.StructuralProfile = structuralProfile;
        result.FactionProfile = factionProfile;
        setPaletteProvenance(result);
        return result;
    }

    ShipResolvedGenerationConfiguration resolveShipGenerationConfiguration(const ShipGenerationRecipe& recipe)
    {
        ShipResolvedGenerationConfiguration result;
        result.Generation = copyCommon(recipe);
        if (recipe.StructuralSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
        {
            result.StructuralProfile = getBuiltInStructuralPresetProfile(recipe.Style);
            result.Provenance.StructuralPreset = recipe.Style;
        }
        else if (recipe.StructuralSource == ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM)
        {
            result.StructuralProfile = recipe.StructuralProfile;
        }
        else
        {
            result.StructuralProfile = ShipGenerationProfile();
            result.Generation.Dimensions = {};
        }

        if (recipe.FactionSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
        {
            result.FactionProfile = getBuiltInFactionPresetProfile(recipe.Faction);
            result.Provenance.FactionPreset = recipe.Faction;
        }
        else if (recipe.FactionSource == ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM)
        {
            result.FactionProfile = recipe.FactionProfile;
        }
        else
        {
            result.FactionProfile = ShipFactionProfile();
            result.Generation.Dimensions = {};
        }
        setPaletteProvenance(result);
        return result;
    }

    ValidationResult validateShipGenerationConfiguration(const ShipResolvedGenerationConfiguration& configuration)
    {
        ValidationResult result;
        if (configuration.Generation.Dimensions.Width < 16u || configuration.Generation.Dimensions.Height < 16u)
        {
            result.Errors.push_back({ "Generation.Dimensions", "Width and height must both be at least 16 pixels." });
        }
        if (configuration.Generation.Dimensions.Width > 4096u || configuration.Generation.Dimensions.Height > 4096u)
        {
            result.Errors.push_back({ "Generation.Dimensions", "Width and height must not exceed 4096 pixels." });
        }
        if (configuration.Generation.DetailDensity > 100u)
        {
            result.Errors.push_back({ "Generation.DetailDensity", "Probability must be in the range 0-100." });
        }
        if (configuration.Generation.AsymmetricDetailChance > 100u)
        {
            result.Errors.push_back({ "Generation.AsymmetricDetailChance", "Probability must be in the range 0-100." });
        }
        if (configuration.Generation.RandomStreamMode >= GenerationRandomStreamMode::GENERATION_RANDOM_STREAM_MODE_END)
        {
            result.Errors.push_back({ "Generation.RandomStreamMode", "Random stream mode is outside the supported range." });
        }
        if (configuration.Generation.PaletteConfiguration.Mode >= ShipPaletteSourceMode::SHIP_PALETTE_SOURCE_MODE_END)
        {
            result.Errors.push_back({ "Generation.PaletteConfiguration.Mode", "Palette source mode is outside the supported range." });
        }
        else if (configuration.Generation.PaletteConfiguration.Mode == ShipPaletteSourceMode::EXPLICIT_GENERATED)
        {
            appendValidationIssues(result, validateShipPaletteGenerationProfile(configuration.Generation.PaletteConfiguration.Generated), "Generation.PaletteConfiguration.Generated.");
        }

        appendValidationIssues(result, validateShipGenerationProfile(configuration.StructuralProfile), "StructuralProfile.");
        appendValidationIssues(result, validateShipFactionProfile(configuration.FactionProfile), "FactionProfile.");

        if (configuration.Provenance.PaletteSource != configuration.Generation.PaletteConfiguration.Mode)
        {
            result.Errors.push_back({ "Provenance.PaletteSource", "Palette provenance must match the active palette source mode." });
        }

        if (configuration.Provenance.StructuralPreset.has_value() && *configuration.Provenance.StructuralPreset >= ShipStyle::SHIP_STYLE_END)
        {
            result.Errors.push_back({ "Provenance.StructuralPreset", "Optional built-in structural provenance must reference a valid preset." });
        }
        if (configuration.Provenance.FactionPreset.has_value() && *configuration.Provenance.FactionPreset >= ShipFactionType::SHIP_FACTION_TYPE_END)
        {
            result.Errors.push_back({ "Provenance.FactionPreset", "Optional built-in faction provenance must reference a valid preset." });
        }
        if (configuration.Provenance.PaletteFactionPreset.has_value())
        {
            if (configuration.Provenance.PaletteSource != ShipPaletteSourceMode::FACTION_PROFILE_GENERATED)
            {
                result.Errors.push_back({ "Provenance.PaletteFactionPreset", "Built-in palette provenance is only valid for faction-profile-generated palettes." });
            }
            else if (*configuration.Provenance.PaletteFactionPreset >= ShipFactionType::SHIP_FACTION_TYPE_END)
            {
                result.Errors.push_back({ "Provenance.PaletteFactionPreset", "Optional built-in palette provenance must reference a valid faction palette preset." });
            }
        }
        return result;
    }
}
