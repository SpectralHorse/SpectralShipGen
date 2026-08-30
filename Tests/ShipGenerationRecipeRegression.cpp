#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include <PixelShipGenerator/ShipGenerationRecipeSerializer.h>
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipFactionProfile.h"
#include "ShipPaletteGenerationProfile.h"
#include "ShipGenerator.h"
#include "ShipIdleAnimator.h"

namespace
{
    using PixelShipGenerator::ShipGenerationRecipe;
    using PixelShipGenerator::ShipGenerationRecipeDocument;

    struct RegressionCase
    {
        uint64_t MasterSeed;
        PixelShipGenerator::ShipDimensions Dimensions;
        PixelShipGenerator::ShipStyle Style;
        PixelShipGenerator::ShipFactionType Faction;
        bool AttachmentsEnabled;
        bool IncludeAnimation;
    };

    constexpr std::array<RegressionCase, 16u> Cases = { {
        { 0x1000000000000024ull, { 24u, 24u }, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::FRONTIER, true, true },
        { 0x2000000000000032ull, { 32u, 32u }, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::ASCENDANT, false, true },
        { 0x3000000000000044ull, { 44u, 44u }, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::MILITARY, true, false },
        { 0x4000000000000096ull, { 96u, 96u }, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::XENO, false, true },
        { 0x5000000000000160ull, { 160u, 160u }, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::ASCENDANT, true, true },
        { 0x6000000000000040ull, { 40u, 40u }, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY, true, true },
        { 0x7000000000000072ull, { 72u, 72u }, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::FRONTIER, true, false },
        { 0x8000000000000256ull, { 256u, 256u }, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::XENO, false, true },
        { 0x9100000000320044ull, { 32u, 44u }, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY, true, true },
        { 0x9200000000560040ull, { 56u, 40u }, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::ASCENDANT, true, false },
        { 0x9300000000640096ull, { 64u, 96u }, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::FRONTIER, true, true },
        { 0x9400000001280096ull, { 128u, 96u }, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::XENO, false, true },
        { 0x9500000000480064ull, { 48u, 64u }, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipFactionType::MILITARY, true, true },
        { 0x9600000000640048ull, { 64u, 48u }, PixelShipGenerator::ShipStyle::DELTA, PixelShipGenerator::ShipFactionType::FRONTIER, true, true },
        { 0x9700000000640064ull, { 64u, 64u }, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::CORPORATE, true, true },
        { 0x9800000000960064ull, { 96u, 64u }, PixelShipGenerator::ShipStyle::DELTA, PixelShipGenerator::ShipFactionType::RELIC, true, true }
    } };

    ShipGenerationRecipe createRecipe(const RegressionCase& regressionCase)
    {
        ShipGenerationRecipe recipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(regressionCase.MasterSeed);
        recipe.Dimensions = regressionCase.Dimensions;
        recipe.Style = regressionCase.Style;
        recipe.Faction = regressionCase.Faction;
        recipe.DetailDensity = 63u;
        recipe.AsymmetricDetailChance = 17u;
        recipe.AttachmentsEnabled = regressionCase.AttachmentsEnabled;
        return recipe;
    }

    PixelShipGenerator::ShipGenerationSettings createSettings(const ShipGenerationRecipe& recipe)
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = recipe.Seeds.Master;
        settings.Dimensions.Width = recipe.Dimensions.Width;
        settings.Dimensions.Height = recipe.Dimensions.Height;
        settings.Style = recipe.Style;
        settings.Faction = recipe.Faction;
        settings.DetailDensity = recipe.DetailDensity;
        settings.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        settings.SeedOverrides.Structure = recipe.Seeds.Structure;
        settings.SeedOverrides.Palette = recipe.Seeds.Palette;
        settings.SeedOverrides.Details = recipe.Seeds.Details;
        settings.SeedOverrides.Attachments = recipe.Seeds.Attachments;
        settings.DomainSeedOverrides = recipe.DomainSeedOverrides;
        settings.RandomStreamMode = recipe.RandomStreamMode;
        return settings;
    }

    PixelShipGenerator::ShipIdleAnimationSettings createAnimationSettings(uint64_t seed)
    {
        PixelShipGenerator::ShipIdleAnimationSettings settings;
        settings.AnimationDurationMilliseconds = 1500u;
        settings.FrameCount = 12u;
        settings.MinimumFrameCount = 8u;
        settings.MaximumFrameCount = 48u;
        settings.SamplingMode = PixelShipGenerator::AnimationSamplingMode::ADAPTIVE;
        settings.EngineFlicker = true;
        settings.LightBlinking = true;
        settings.MechanicalMicroMovement = true;
        settings.HoverOffset = true;
        settings.SmallDetailVariation = true;
        settings.Seed = seed ^ 0xD6E8FEB86659FD93ull;
        return settings;
    }

    bool animationSettingsEqual(const PixelShipGenerator::ShipIdleAnimationSettings& first, const PixelShipGenerator::ShipIdleAnimationSettings& second)
    {
        return first.AnimationDurationMilliseconds == second.AnimationDurationMilliseconds && first.FrameCount == second.FrameCount && first.MinimumFrameCount == second.MinimumFrameCount && first.MaximumFrameCount == second.MaximumFrameCount && first.SamplingMode == second.SamplingMode && first.EngineFlicker == second.EngineFlicker && first.LightBlinking == second.LightBlinking && first.MechanicalMicroMovement == second.MechanicalMicroMovement && first.HoverOffset == second.HoverOffset && first.SmallDetailVariation == second.SmallDetailVariation && first.Seed == second.Seed;
    }


    ShipGenerationRecipe createCustomRecipe(PixelShipGenerator::ShipPaletteSourceMode paletteMode)
    {
        using namespace PixelShipGenerator;
        ShipGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x8700000000000087ull);
        recipe.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x8700000000001001ull);
        recipe.DomainSeedOverrides.set(GenerationDomain::DETAILS, 0x8700000000001002ull);
        recipe.RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        recipe.Dimensions = { 96u, 64u };
        recipe.DetailDensity = 68u;
        recipe.AsymmetricDetailChance = 21u;
        recipe.AttachmentsEnabled = true;

        recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        recipe.StructuralProfile.LargeWeaponChance = 82u;
        recipe.StructuralProfile.LargeWeaponScalePercent = 145u;
        recipe.StructuralProfile.BroadWingWeight = 47u;
        recipe.StructuralProfile.DetailMotifRepeatPercent = 73u;
        recipe.StructuralProfile.PaletteHullValueOffset = 7;
        recipe.StructuralProfile.AnimationTraits.Firing.ResponseStrengthPercent = 137u;

        recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = getShipFactionProfile(ShipFactionType::RELIC);
        recipe.FactionProfile.Weapons.EmissiveChance = 47u;
        recipe.FactionProfile.SurfaceDetails.DetailDensityPercent = 91u;
        recipe.FactionProfile.Livery.AsymmetricChanceOffset = 6;
        recipe.FactionProfile.Animation.Firing.DurationAdditionMilliseconds = 35;
        recipe.FactionProfile.Finish.ForceAxialRidgeEdgeHighlight = true;

        recipe.PaletteConfiguration.Mode = paletteMode;
        if (paletteMode == ShipPaletteSourceMode::EXPLICIT_GENERATED)
        {
            recipe.PaletteConfiguration.Generated = getShipPaletteGenerationProfile(ShipFactionType::CORPORATE);
            recipe.PaletteConfiguration.Generated.Ranges.HullHue = { 205u, 228u };
            recipe.PaletteConfiguration.Generated.Ranges.HullSaturation = { 42u, 67u };
            recipe.PaletteConfiguration.Generated.Behavior.MinimumAccentHueDistance = 74u;
        }
        else if (paletteMode == ShipPaletteSourceMode::FIXED)
        {
            ShipPalette& palette = recipe.PaletteConfiguration.Fixed;
            palette.Outline = Color(9u, 11u, 17u, 255u);
            palette.HullBase = Color(44u, 77u, 109u, 255u);
            palette.HullSecondary = Color(73u, 55u, 102u, 255u);
            palette.HullAccent = Color(211u, 82u, 141u, 255u);
            palette.CockpitBase = Color(29u, 181u, 207u, 255u);
            palette.EngineHotCore = Color(255u, 121u, 47u, 255u);
            palette.LightBase = Color(105u, 255u, 176u, 255u);
        }
        return recipe;
    }

    bool imagesEqual(const PixelShipGenerator::GeneratedShip& first, const PixelShipGenerator::GeneratedShip& second)
    {
        return first.FinalImage.getWidth() == second.FinalImage.getWidth() && first.FinalImage.getHeight() == second.FinalImage.getHeight() && first.FinalImage.getPixels() == second.FinalImage.getPixels();
    }

    bool animationsEqual(PixelShipGenerator::ShipIdleAnimator& animator, const PixelShipGenerator::GeneratedShip& first, const PixelShipGenerator::GeneratedShip& second, const PixelShipGenerator::ShipIdleAnimationSettings& settings)
    {
        const PixelShipGenerator::ShipIdleAnimation firstAnimation = animator.generate(first, settings);
        const PixelShipGenerator::ShipIdleAnimation secondAnimation = animator.generate(second, settings);
        if (firstAnimation.Frames.size() != secondAnimation.Frames.size()) { return false; }
        for (std::size_t index = 0u; index < firstAnimation.Frames.size(); ++index)
        {
            if (firstAnimation.Frames[index].getPixels() != secondAnimation.Frames[index].getPixels()) { return false; }
        }
        return true;
    }

    bool replaceNumberAfterKey(std::string& json, const std::string& key, const std::string& replacement)
    {
        const std::string token = "\"" + key + "\": ";
        const std::size_t keyPosition = json.find(token);
        if (keyPosition == std::string::npos) { return false; }
        const std::size_t valueStart = keyPosition + token.size();
        std::size_t valueEnd = valueStart;
        if (valueEnd < json.size() && json[valueEnd] == '-') { ++valueEnd; }
        while (valueEnd < json.size() && json[valueEnd] >= '0' && json[valueEnd] <= '9') { ++valueEnd; }
        if (valueEnd == valueStart) { return false; }
        json.replace(valueStart, valueEnd - valueStart, replacement);
        return true;
    }
}

int PixelShipGeneratorTests::runGenerationRecipeRegression()
{
    PixelShipGenerator::ShipGenerator generator;
    PixelShipGenerator::ShipIdleAnimator animator;
    bool success = true;

    for (const RegressionCase& regressionCase : Cases)
    {
        ShipGenerationRecipeDocument original;
        original.Recipe = createRecipe(regressionCase);
        if (regressionCase.IncludeAnimation) { original.AnimationSettings = createAnimationSettings(regressionCase.MasterSeed); }

        const std::string jsonText = PixelShipGenerator::serializeShipGenerationRecipe(original);
        const PixelShipGenerator::ShipGenerationRecipeLoadResult loaded = PixelShipGenerator::deserializeShipGenerationRecipe(jsonText);

        if (!loaded.Success)
        {
            success = false;
            std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " recipe failed to deserialize: " << loaded.Error << '\n';
            continue;
        }

        if (loaded.Document.Recipe != original.Recipe || loaded.Document.AnimationSettings.has_value() != original.AnimationSettings.has_value())
        {
            success = false;
            std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " recipe values changed during round-trip.\n";
            continue;
        }

        if (original.AnimationSettings.has_value() && !animationSettingsEqual(*original.AnimationSettings, *loaded.Document.AnimationSettings))
        {
            success = false;
            std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " animation settings changed during round-trip.\n";
            continue;
        }

        try
        {
            const PixelShipGenerator::GeneratedShip firstShip = generator.generate(original.Recipe);
            const PixelShipGenerator::GeneratedShip secondShip = generator.generate(loaded.Document.Recipe);
            if (firstShip.FinalImage.getPixels() != secondShip.FinalImage.getPixels())
            {
                success = false;
                std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " static image changed during recipe round-trip.\n";
                continue;
            }

            if (original.AnimationSettings.has_value())
            {
                const PixelShipGenerator::ShipIdleAnimation firstAnimation = animator.generate(firstShip, *original.AnimationSettings);
                const PixelShipGenerator::ShipIdleAnimation secondAnimation = animator.generate(secondShip, *loaded.Document.AnimationSettings);
                if (firstAnimation.Frames.size() != secondAnimation.Frames.size())
                {
                    success = false;
                    std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " animation frame count changed during round-trip.\n";
                    continue;
                }
                for (std::size_t frameIndex = 0u; frameIndex < firstAnimation.Frames.size(); ++frameIndex)
                {
                    if (firstAnimation.Frames[frameIndex].getPixels() != secondAnimation.Frames[frameIndex].getPixels())
                    {
                        success = false;
                        std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " animation frame " << frameIndex << " changed during round-trip.\n";
                        break;
                    }
                }
                if (firstAnimation.Frames.empty() || firstAnimation.Frames.front().getPixels() != firstShip.FinalImage.getPixels())
                {
                    success = false;
                    std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " animation frame 0 does not equal the static image.\n";
                }
            }
        }
        catch (const std::exception& exception)
        {
            success = false;
            std::cerr << regressionCase.Dimensions.Width << "x" << regressionCase.Dimensions.Height << " generation failed: " << exception.what() << '\n';
        }
    }

    const std::string legacyObjectRecipe = R"JSON({
  "format_version": 1,
  "ship": {
    "resolution": { "width": 48, "height": 48 },
    "style": "FIGHTER",
    "faction": "FRONTIER",
    "seeds": { "master": 1, "structure": 2, "palette": 3, "details": 4, "attachments": 5 },
    "settings": { "detail_density": 50, "asymmetric_detail_chance": 10, "attachments_enabled": true }
  }
})JSON";
    const auto legacyObject = PixelShipGenerator::deserializeShipGenerationRecipe(legacyObjectRecipe);
    if (!legacyObject.Success || legacyObject.Document.Recipe.Dimensions != PixelShipGenerator::ShipDimensions{ 48u, 48u })
    {
        success = false;
        std::cerr << "Task-41 object resolution recipe did not migrate to dimensions.\n";
    }

    const std::string legacyScalarRecipe = R"JSON({
  "format_version": 1,
  "ship": {
    "resolution": 44,
    "style": "HEAVY",
    "faction": "MILITARY",
    "seeds": { "master": 10, "structure": 11, "palette": 12, "details": 13, "attachments": 14 },
    "settings": { "detail_density": 50, "asymmetric_detail_chance": 10, "attachments_enabled": false }
  }
})JSON";
    const auto legacyScalar = PixelShipGenerator::deserializeShipGenerationRecipe(legacyScalarRecipe);
    if (!legacyScalar.Success || legacyScalar.Document.Recipe.Dimensions != PixelShipGenerator::ShipDimensions{ 44u, 44u })
    {
        success = false;
        std::cerr << "Legacy scalar resolution recipe did not migrate to square dimensions.\n";
    }


    const std::string legacyAnimationRecipe = R"JSON({
  "format_version": 3,
  "ship": {
    "dimensions": { "width": 64, "height": 48 },
    "style": "INDUSTRIAL",
    "faction": "FRONTIER",
    "seeds": { "master": 101, "structure": 102, "palette": 103, "details": 104, "attachments": 105, "rng_mode": "DOMAIN_SUBSTREAMS" },
    "settings": { "detail_density": 50, "asymmetric_detail_chance": 10, "attachments_enabled": true }
  },
  "animation": {
    "seed": 106,
    "frame_count": 20,
    "engine_flicker": true,
    "light_blinking": true,
    "mechanical_micro_movement": true,
    "hover_offset": true,
    "small_detail_variation": true
  }
})JSON";
    const auto legacyAnimation = PixelShipGenerator::deserializeShipGenerationRecipe(legacyAnimationRecipe);
    if (!legacyAnimation.Success || !legacyAnimation.Document.AnimationSettings.has_value())
    {
        success = false;
        std::cerr << "Task-67 legacy animation recipe failed to migrate.\n";
    }
    else
    {
        const PixelShipGenerator::ShipIdleAnimationSettings& migrated = *legacyAnimation.Document.AnimationSettings;
        if (migrated.SamplingMode != PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT || migrated.FrameCount != 20u || migrated.MinimumFrameCount != 20u || migrated.MaximumFrameCount != 20u || migrated.AnimationDurationMilliseconds != 2000u)
        {
            success = false;
            std::cerr << "Task-67 legacy animation recipe did not preserve exact frame-count/100ms timing semantics.\n";
        }
        else
        {
            try
            {
                const PixelShipGenerator::GeneratedShip migratedShip = generator.generate(legacyAnimation.Document.Recipe);
                const PixelShipGenerator::ShipIdleAnimation migratedAnimation = animator.generate(migratedShip, migrated);
                if (migratedAnimation.Frames.size() != 20u || migratedAnimation.DurationMilliseconds != 2000u || migratedAnimation.FrameDurationMilliseconds != 100.0)
                {
                    success = false;
                    std::cerr << "Task-67 migrated legacy animation did not preserve exact output/timing semantics.\n";
                }
            }
            catch (const std::exception& exception)
            {
                success = false;
                std::cerr << "Task-67 migrated legacy animation generation failed: " << exception.what() << '\n';
            }
        }
    }

    // Task 87: a completely custom structural/faction/generated-palette recipe must
    // be self-contained and regenerate without any built-in identity.
    {
        using namespace PixelShipGenerator;
        ShipGenerationRecipeDocument customDocument;
        customDocument.Recipe = createCustomRecipe(ShipPaletteSourceMode::EXPLICIT_GENERATED);
        customDocument.AnimationSettings = createAnimationSettings(0x8700000000002001ull);
        const GeneratedShip before = generator.generate(customDocument.Recipe);
        const std::string serialized = serializeShipGenerationRecipe(customDocument);
        const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(serialized);
        if (!loaded.Success)
        {
            success = false;
            std::cerr << "Fully custom recipe failed to deserialize: " << loaded.Error << '\n';
        }
        else
        {
            const ShipGenerationRecipe& recipe = loaded.Document.Recipe;
            if (recipe.StructuralSource != ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM || recipe.FactionSource != ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM || recipe.Style != ShipStyle::SHIP_STYLE_END || recipe.Faction != ShipFactionType::SHIP_FACTION_TYPE_END || recipe.PaletteConfiguration.Mode != ShipPaletteSourceMode::EXPLICIT_GENERATED)
            {
                success = false;
                std::cerr << "Custom recipe source/provenance semantics changed during round-trip.\n";
            }
            if (recipe.StructuralProfile.LargeWeaponChance != 82u || recipe.StructuralProfile.LargeWeaponScalePercent != 145u || recipe.StructuralProfile.AnimationTraits.Firing.ResponseStrengthPercent != 137u || recipe.FactionProfile.Weapons.EmissiveChance != 47u || recipe.FactionProfile.Animation.Firing.DurationAdditionMilliseconds != 35 || recipe.PaletteConfiguration.Generated.Ranges.HullHue.Min != 205u || recipe.PaletteConfiguration.Generated.Ranges.HullHue.Max != 228u || recipe.PaletteConfiguration.Generated.Behavior.MinimumAccentHueDistance != 74u)
            {
                success = false;
                std::cerr << "Embedded custom profile fields changed during round-trip.\n";
            }
            if (recipe.DomainSeedOverrides.get(GenerationDomain::PALETTE) != customDocument.Recipe.DomainSeedOverrides.get(GenerationDomain::PALETTE) || recipe.DomainSeedOverrides.get(GenerationDomain::DETAILS) != customDocument.Recipe.DomainSeedOverrides.get(GenerationDomain::DETAILS))
            {
                success = false;
                std::cerr << "Custom recipe domain overrides changed during round-trip.\n";
            }
            try
            {
                const GeneratedShip after = generator.generate(recipe);
                if (!imagesEqual(before, after) || before.Style != ShipStyle::SHIP_STYLE_END || after.Style != ShipStyle::SHIP_STYLE_END || before.Faction != ShipFactionType::SHIP_FACTION_TYPE_END || after.Faction != ShipFactionType::SHIP_FACTION_TYPE_END)
                {
                    success = false;
                    std::cerr << "Fully custom recipe did not reproduce static output/provenance.\n";
                }
                if (!loaded.Document.AnimationSettings.has_value() || !animationsEqual(animator, before, after, *loaded.Document.AnimationSettings))
                {
                    success = false;
                    std::cerr << "Fully custom recipe did not reproduce animation behavior.\n";
                }
            }
            catch (const std::exception& exception)
            {
                success = false;
                std::cerr << "Fully custom recipe generation failed: " << exception.what() << '\n';
            }
        }
        if (serialized.find("\"preset\"") != std::string::npos)
        {
            success = false;
            std::cerr << "Fully custom recipe unexpectedly depends on a built-in preset reference.\n";
        }
    }

    // Fixed semantic palettes must survive recipe transport exactly.
    {
        using namespace PixelShipGenerator;
        ShipGenerationRecipeDocument fixedDocument;
        fixedDocument.Recipe = createCustomRecipe(ShipPaletteSourceMode::FIXED);
        const GeneratedShip before = generator.generate(fixedDocument.Recipe);
        const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(serializeShipGenerationRecipe(fixedDocument));
        if (!loaded.Success || loaded.Document.Recipe.PaletteConfiguration.Mode != ShipPaletteSourceMode::FIXED || loaded.Document.Recipe.PaletteConfiguration.Fixed.HullBase != fixedDocument.Recipe.PaletteConfiguration.Fixed.HullBase || loaded.Document.Recipe.PaletteConfiguration.Fixed.LightBase != fixedDocument.Recipe.PaletteConfiguration.Fixed.LightBase)
        {
            success = false;
            std::cerr << "Fixed-palette recipe did not preserve exact semantic colors.\n";
        }
        else
        {
            const GeneratedShip after = generator.generate(loaded.Document.Recipe);
            if (!imagesEqual(before, after) || after.Palette.HullBase != fixedDocument.Recipe.PaletteConfiguration.Fixed.HullBase || after.Palette.LightBase != fixedDocument.Recipe.PaletteConfiguration.Fixed.LightBase)
            {
                success = false;
                std::cerr << "Fixed-palette recipe did not reproduce exact resolved colors/output.\n";
            }
        }
    }

    // Mixed source combinations remain portable as well.
    {
        using namespace PixelShipGenerator;
        ShipGenerationRecipe customStructural = createCustomRecipe(ShipPaletteSourceMode::FACTION_PROFILE_GENERATED);
        customStructural.FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        customStructural.Faction = ShipFactionType::MILITARY;
        ShipGenerationRecipeLoadResult structuralLoad = deserializeShipGenerationRecipe(serializeShipGenerationRecipe({ customStructural, std::nullopt }));
        if (!structuralLoad.Success || !imagesEqual(generator.generate(customStructural), generator.generate(structuralLoad.Document.Recipe)))
        {
            success = false;
            std::cerr << "Custom structural + built-in faction recipe portability failed.\n";
        }

        ShipGenerationRecipe customFaction = createCustomRecipe(ShipPaletteSourceMode::FACTION_PROFILE_GENERATED);
        customFaction.StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        customFaction.Style = ShipStyle::DELTA;
        ShipGenerationRecipeLoadResult factionLoad = deserializeShipGenerationRecipe(serializeShipGenerationRecipe({ customFaction, std::nullopt }));
        if (!factionLoad.Success || !imagesEqual(generator.generate(customFaction), generator.generate(factionLoad.Document.Recipe)))
        {
            success = false;
            std::cerr << "Built-in structural + custom faction recipe portability failed.\n";
        }
    }

    // New built-in recipes stay compact and use stable string preset identifiers.
    {
        ShipGenerationRecipeDocument builtIn;
        builtIn.Recipe = createRecipe(Cases.front());
        const std::string serialized = PixelShipGenerator::serializeShipGenerationRecipe(builtIn);
        if (serialized.find("\"preset\": \"FIGHTER\"") == std::string::npos || serialized.find("\"preset\": \"FRONTIER\"") == std::string::npos || serialized.find("\"LargeWeaponChance\"") != std::string::npos)
        {
            success = false;
            std::cerr << "Built-in recipe is not using compact stable preset references.\n";
        }
    }

    // Embedded invalid configuration is rejected by the same Core validators used by generation.
    {
        ShipGenerationRecipeDocument custom;
        custom.Recipe = createCustomRecipe(PixelShipGenerator::ShipPaletteSourceMode::EXPLICIT_GENERATED);
        std::string structuralJson = PixelShipGenerator::serializeShipGenerationRecipe(custom);
        if (!replaceNumberAfterKey(structuralJson, "LargeWeaponChance", "101") || PixelShipGenerator::deserializeShipGenerationRecipe(structuralJson).Success)
        {
            success = false;
            std::cerr << "Malformed embedded structural profile was not rejected.\n";
        }

        std::string factionJson = PixelShipGenerator::serializeShipGenerationRecipe(custom);
        if (!replaceNumberAfterKey(factionJson, "AsymmetricChanceDivisor", "0") || PixelShipGenerator::deserializeShipGenerationRecipe(factionJson).Success)
        {
            success = false;
            std::cerr << "Malformed embedded faction profile was not rejected.\n";
        }

        std::string paletteJson = PixelShipGenerator::serializeShipGenerationRecipe(custom);
        if (!replaceNumberAfterKey(paletteJson, "MinimumAccentHueDistance", "999") || PixelShipGenerator::deserializeShipGenerationRecipe(paletteJson).Success)
        {
            success = false;
            std::cerr << "Malformed embedded palette profile was not rejected.\n";
        }
    }


    // Public configuration -> recipe helpers must preserve the exact deterministic
    // generation path rather than reconstructing approximate settings.
    {
        using namespace PixelShipGenerator;

        ShipGenerationSettings settings;
        settings.Seed = 0x8700000000003001ull;
        settings.Dimensions = { 64u, 48u };
        settings.Style = ShipStyle::SPEARHEAD;
        settings.Faction = ShipFactionType::CORPORATE;
        settings.DetailDensity = 71u;
        settings.AsymmetricDetailChance = 13u;
        settings.AttachmentsEnabled = true;
        settings.SeedOverrides.Palette = 0x8700000000003002ull;
        settings.DomainSeedOverrides.set(GenerationDomain::DETAILS, 0x8700000000003003ull);

        const ShipGenerationRecipe recipe = makeShipGenerationRecipe(settings);
        const GeneratedShip direct = generator.generate(settings);
        const GeneratedShip viaRecipe = generator.generate(recipe);
        if (!imagesEqual(direct, viaRecipe) || recipe.StructuralSource != ShipGenerationRecipeProfileSource::BUILT_IN_PRESET || recipe.FactionSource != ShipGenerationRecipeProfileSource::BUILT_IN_PRESET || recipe.Seeds.Palette != *settings.SeedOverrides.Palette)
        {
            success = false;
            std::cerr << "Built-in configuration-to-recipe helper changed deterministic generation state.\n";
        }

        ExplicitShipGenerationConfiguration configuration;
        configuration.Seed = 0x8700000000003010ull;
        configuration.Dimensions = { 96u, 64u };
        configuration.DetailDensity = 66u;
        configuration.AsymmetricDetailChance = 19u;
        configuration.AttachmentsEnabled = true;
        configuration.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x8700000000003011ull);
        configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
        configuration.PaletteConfiguration.Fixed.HullBase = Color(53u, 87u, 119u, 255u);
        configuration.PaletteConfiguration.Fixed.HullAccent = Color(219u, 73u, 132u, 255u);
        configuration.PaletteConfiguration.Fixed.CockpitBase = Color(42u, 194u, 221u, 255u);

        ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        structuralProfile.LargeWeaponChance = 79u;
        ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::ASCENDANT);
        factionProfile.Weapons.EmissiveChance = 38u;

        const ShipGenerationRecipe explicitRecipe = makeShipGenerationRecipe(configuration, structuralProfile, factionProfile);
        const GeneratedShip explicitDirect = generator.generate(configuration, structuralProfile, factionProfile);
        const GeneratedShip explicitViaRecipe = generator.generate(explicitRecipe);
        if (!imagesEqual(explicitDirect, explicitViaRecipe) || explicitRecipe.Style != ShipStyle::SHIP_STYLE_END || explicitRecipe.Faction != ShipFactionType::SHIP_FACTION_TYPE_END || explicitRecipe.PaletteConfiguration.Mode != ShipPaletteSourceMode::FIXED)
        {
            success = false;
            std::cerr << "Fully explicit configuration-to-recipe helper changed deterministic generation state.\n";
        }
    }

    // Known schema versions may carry unknown future fields, while required
    // fields and types remain strict.
    {
        ShipGenerationRecipeDocument builtIn;
        builtIn.Recipe = createRecipe(Cases.front());
        std::string withUnknownField = PixelShipGenerator::serializeShipGenerationRecipe(builtIn);
        const std::size_t firstBrace = withUnknownField.find('{');
        if (firstBrace == std::string::npos)
        {
            success = false;
            std::cerr << "Could not prepare forward-compatible recipe fixture.\n";
        }
        else
        {
            withUnknownField.insert(firstBrace + 1u, "\n  \"future_note\": 123,");
            if (!PixelShipGenerator::deserializeShipGenerationRecipe(withUnknownField).Success)
            {
                success = false;
                std::cerr << "Unknown field in a known recipe schema version was not ignored.\n";
            }
        }

        const auto missingSettings = PixelShipGenerator::deserializeShipGenerationRecipe(R"JSON({
          "format_version": 5,
          "ship": {
            "dimensions": { "width": 64, "height": 64 },
            "structural": { "source": "BUILT_IN_PRESET", "preset": "FIGHTER" },
            "faction": { "source": "BUILT_IN_PRESET", "preset": "FRONTIER" },
            "palette": { "source": "FACTION_PROFILE_GENERATED" },
            "seeds": { "master": 1, "structure": 2, "palette": 3, "details": 4, "attachments": 5, "rng_mode": "DOMAIN_SUBSTREAMS" }
          }
        })JSON");
        if (missingSettings.Success)
        {
            success = false;
            std::cerr << "Recipe missing required settings object was not rejected.\n";
        }

        const auto wrongType = PixelShipGenerator::deserializeShipGenerationRecipe(R"JSON({
          "format_version": 5,
          "ship": {
            "dimensions": { "width": "64", "height": 64 },
            "structural": { "source": "BUILT_IN_PRESET", "preset": "FIGHTER" },
            "faction": { "source": "BUILT_IN_PRESET", "preset": "FRONTIER" },
            "palette": { "source": "FACTION_PROFILE_GENERATED" },
            "seeds": { "master": 1, "structure": 2, "palette": 3, "details": 4, "attachments": 5, "rng_mode": "DOMAIN_SUBSTREAMS" },
            "settings": { "detail_density": 50, "asymmetric_detail_chance": 10, "attachments_enabled": true }
          }
        })JSON");
        if (wrongType.Success)
        {
            success = false;
            std::cerr << "Recipe with a wrong required-field type was not rejected.\n";
        }
    }

    const auto unsupportedVersion = PixelShipGenerator::deserializeShipGenerationRecipe("{ \"format_version\": 999, \"ship\": {} }");
    if (unsupportedVersion.Success) { success = false; std::cerr << "Unsupported format version was not rejected.\n"; }

    const auto unknownPreset = PixelShipGenerator::deserializeShipGenerationRecipe(R"JSON({
      "format_version": 5,
      "ship": {
        "dimensions": { "width": 64, "height": 64 },
        "structural": { "source": "BUILT_IN_PRESET", "preset": "UNKNOWN_STYLE" },
        "faction": { "source": "BUILT_IN_PRESET", "preset": "FRONTIER" },
        "palette": { "source": "FACTION_PROFILE_GENERATED" },
        "seeds": { "master": 1, "structure": 2, "palette": 3, "details": 4, "attachments": 5, "rng_mode": "DOMAIN_SUBSTREAMS" },
        "settings": { "detail_density": 50, "asymmetric_detail_chance": 10, "attachments_enabled": true }
      }
    })JSON");
    if (unknownPreset.Success) { success = false; std::cerr << "Unknown built-in preset name was not rejected.\n"; }

    return success ? 0 : 1;
}
