#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include "ShipGenerationRecipeSerializer.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"
#include "ShipIdleAnimator.h"

namespace
{
    using PixelShipGeneratorPreview::PreviewGenerationRecipe;
    using PixelShipGeneratorPreview::ShipGenerationRecipeDocument;

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

    PreviewGenerationRecipe createRecipe(const RegressionCase& regressionCase)
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(regressionCase.MasterSeed);
        recipe.Dimensions = regressionCase.Dimensions;
        recipe.Style = regressionCase.Style;
        recipe.Faction = regressionCase.Faction;
        recipe.DetailDensity = 63u;
        recipe.AsymmetricDetailChance = 17u;
        recipe.AttachmentsEnabled = regressionCase.AttachmentsEnabled;
        return recipe;
    }

    PixelShipGenerator::ShipGenerationSettings createSettings(const PreviewGenerationRecipe& recipe)
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
        settings.FrameCount = 10u;
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
        return first.FrameCount == second.FrameCount && first.EngineFlicker == second.EngineFlicker && first.LightBlinking == second.LightBlinking && first.MechanicalMicroMovement == second.MechanicalMicroMovement && first.HoverOffset == second.HoverOffset && first.SmallDetailVariation == second.SmallDetailVariation && first.Seed == second.Seed;
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

        const std::string jsonText = PixelShipGeneratorPreview::serializeShipGenerationRecipe(original);
        const PixelShipGeneratorPreview::ShipGenerationRecipeLoadResult loaded = PixelShipGeneratorPreview::deserializeShipGenerationRecipe(jsonText);

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
            const PixelShipGenerator::GeneratedShip firstShip = generator.generate(createSettings(original.Recipe));
            const PixelShipGenerator::GeneratedShip secondShip = generator.generate(createSettings(loaded.Document.Recipe));
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
    const auto legacyObject = PixelShipGeneratorPreview::deserializeShipGenerationRecipe(legacyObjectRecipe);
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
    const auto legacyScalar = PixelShipGeneratorPreview::deserializeShipGenerationRecipe(legacyScalarRecipe);
    if (!legacyScalar.Success || legacyScalar.Document.Recipe.Dimensions != PixelShipGenerator::ShipDimensions{ 44u, 44u })
    {
        success = false;
        std::cerr << "Legacy scalar resolution recipe did not migrate to square dimensions.\n";
    }

    const auto unsupportedVersion = PixelShipGeneratorPreview::deserializeShipGenerationRecipe("{ \"format_version\": 999, \"ship\": {} }");
    if (unsupportedVersion.Success) { success = false; std::cerr << "Unsupported format version was not rejected.\n"; }

    return success ? 0 : 1;
}
