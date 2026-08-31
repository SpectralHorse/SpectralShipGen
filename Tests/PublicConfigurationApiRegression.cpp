#include "CoreRegressionSuites.h"

#include <SpectralShipGen/AnimationSamplingPlanner.h>
#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/ShipAnimationStateCoordinator.h>
#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipLateralMovementAnimator.h>
#include <SpectralShipGen/ShipLongitudinalMovementAnimator.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>
#include <SpectralShipGen/ShipFiringAnimator.h>

#include <SpectralShipGen/Diagnostics/DiagnosticsRunner.h>

#include <iostream>
#include <stdexcept>

namespace
{
    using namespace SpectralShipGen;

    bool sameImage(const Image& a, const Image& b)
    {
        return a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight() && a.getPixels() == b.getPixels();
    }

    bool samePalette(const ShipPalette& a, const ShipPalette& b)
    {
        return a.Transparent == b.Transparent && a.Outline == b.Outline &&
            a.HullDeepShadow == b.HullDeepShadow && a.HullShadow == b.HullShadow && a.HullBase == b.HullBase &&
            a.HullHighlight == b.HullHighlight && a.HullSecondary == b.HullSecondary && a.HullEdgeHighlight == b.HullEdgeHighlight &&
            a.CockpitDark == b.CockpitDark && a.CockpitBase == b.CockpitBase && a.CockpitHighlight == b.CockpitHighlight && a.CockpitGlint == b.CockpitGlint &&
            a.EngineDark == b.EngineDark && a.EngineBase == b.EngineBase && a.EngineHighlight == b.EngineHighlight && a.EngineHotCore == b.EngineHotCore &&
            a.ExhaustBase == b.ExhaustBase && a.ExhaustHighlight == b.ExhaustHighlight && a.ExhaustHotCore == b.ExhaustHotCore &&
            a.HullAccentDark == b.HullAccentDark && a.HullAccent == b.HullAccent && a.HullAccentHighlight == b.HullAccentHighlight &&
            a.MechanicalDark == b.MechanicalDark && a.MechanicalBase == b.MechanicalBase &&
            a.LightBase == b.LightBase && a.LightHighlight == b.LightHighlight;
    }

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    ShipGenerationSettings builtInSettings(ShipStyle style, ShipFactionType faction, uint64_t seed, ShipDimensions dimensions)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        settings.DetailDensity = 57u;
        settings.AsymmetricDetailChance = 17u;
        return settings;
    }
}

namespace SpectralShipGenTests
{
    int runPublicConfigurationApiRegression()
    {
        try
        {
            const auto& structuralCatalog = getBuiltInStructuralPresetCatalog();
            const auto& factionCatalog = getBuiltInFactionPresetCatalog();
            const auto& paletteCatalog = getBuiltInPalettePresetCatalog();
            require(structuralCatalog.size() == 6u && factionCatalog.size() == 6u && paletteCatalog.size() == 6u, "Built-in preset catalog size mismatch.");

            for (const auto& entry : structuralCatalog)
            {
                ShipStyle parsed = ShipStyle::SHIP_STYLE_END;
                require(tryGetBuiltInStructuralPreset(entry.StableId, parsed) && parsed == entry.Preset, "Structural stable-id round-trip failed.");
                require(&getBuiltInStructuralPresetProfile(entry.Preset) == &getShipGenerationProfile(entry.Preset), "Structural catalog must expose the canonical public profile.");
            }
            for (const auto& entry : factionCatalog)
            {
                ShipFactionType parsed = ShipFactionType::SHIP_FACTION_TYPE_END;
                require(tryGetBuiltInFactionPreset(entry.StableId, parsed) && parsed == entry.Preset, "Faction stable-id round-trip failed.");
                require(&getBuiltInFactionPresetProfile(entry.Preset) == &getShipFactionProfile(entry.Preset), "Faction catalog must expose the canonical public profile.");
            }
            for (const auto& entry : paletteCatalog)
            {
                ShipFactionType parsed = ShipFactionType::SHIP_FACTION_TYPE_END;
                require(tryGetBuiltInPalettePreset(entry.StableId, parsed) && parsed == entry.FactionPreset, "Palette stable-id round-trip failed.");
                require(getBuiltInPalettePresetId(entry.FactionPreset) == entry.StableId, "Palette stable-id lookup failed.");
                require(validateShipPaletteGenerationProfile(getBuiltInPalettePresetProfile(entry.FactionPreset)).isValid(), "Built-in palette preset validation failed.");
            }

            ShipGenerator generator;
            const ShipDimensions dimensions{ 64u, 48u };
            const uint64_t seed = 0x88A11CE5D00D1234ull;
            for (const auto& structural : structuralCatalog)
            {
                for (const auto& faction : factionCatalog)
                {
                    const ShipGenerationSettings settings = builtInSettings(structural.Preset, faction.Preset, seed ^ (static_cast<uint64_t>(structural.Preset) << 12u) ^ static_cast<uint64_t>(faction.Preset), dimensions);
                    const GeneratedShip convenience = generator.generate(settings);
                    const ShipResolvedGenerationConfiguration resolved = resolveShipGenerationConfiguration(settings);
                    require(validateShipGenerationConfiguration(resolved).isValid(), "Resolved built-in configuration validation failed.");
                    const GeneratedShip explicitShip = generator.generate(resolved);
                    require(sameImage(convenience.FinalImage, explicitShip.FinalImage), "Built-in convenience/resolved configuration image mismatch.");
                    require(samePalette(convenience.Palette, explicitShip.Palette), "Built-in convenience/resolved palette mismatch.");
                    require(explicitShip.Provenance.StructuralPreset == resolved.Provenance.StructuralPreset, "Structural provenance mismatch.");
                    require(explicitShip.Provenance.FactionPreset == resolved.Provenance.FactionPreset, "Faction provenance mismatch.");
                }
            }

            ExplicitShipGenerationConfiguration customSettings;
            customSettings.Seed = 0x88C0570F15C0FFEEull;
            customSettings.Dimensions = { 72u, 52u };
            customSettings.DetailDensity = 31u;
            customSettings.AsymmetricDetailChance = 23u;
            customSettings.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
            customSettings.PaletteConfiguration.Generated = getBuiltInPalettePresetProfile(ShipFactionType::ASCENDANT);
            customSettings.PaletteConfiguration.Generated.Ranges.HullHue.Min = 280u;
            customSettings.PaletteConfiguration.Generated.Ranges.HullHue.Max = 320u;

            ShipGenerationProfile customStructural = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
            customStructural.LargeWeaponChance = 87u;
            customStructural.LargeWeaponScalePercent = 145u;
            ShipFactionProfile customFaction = getShipFactionProfile(ShipFactionType::RELIC);
            customFaction.SurfaceDetails.DetailDensityPercent = 73u;
            customFaction.Weapons.ChancePercent = 133u;

            ShipResolvedGenerationConfiguration custom = resolveShipGenerationConfiguration(customSettings, customStructural, customFaction);
            require(!custom.Provenance.StructuralPreset.has_value() && !custom.Provenance.FactionPreset.has_value(), "Custom resolved configuration must not fabricate preset provenance.");
            require(validateShipGenerationConfiguration(custom).isValid(), "Custom resolved configuration validation failed.");

            const GeneratedShip customShip = generator.generate(custom);
            const GeneratedShip customShipAgain = generator.generate(custom);
            require(sameImage(customShip.FinalImage, customShipAgain.FinalImage), "Custom canonical generation is not deterministic.");
            require(!customShip.Provenance.StructuralPreset.has_value() && !customShip.Provenance.FactionPreset.has_value(), "Generated custom ship fabricated preset provenance.");
            require(customShip.Style == ShipStyle::SHIP_STYLE_END && customShip.Faction == ShipFactionType::SHIP_FACTION_TYPE_END, "Legacy provenance mirrors must remain truthful for custom generation.");

            ShipResolvedGenerationConfiguration fixedPaletteConfiguration = custom;
            fixedPaletteConfiguration.Generation.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
            fixedPaletteConfiguration.Generation.PaletteConfiguration.Fixed = customShip.Palette;
            fixedPaletteConfiguration.Provenance.PaletteSource = ShipPaletteSourceMode::FIXED;
            fixedPaletteConfiguration.Provenance.PaletteFactionPreset.reset();
            require(validateShipGenerationConfiguration(fixedPaletteConfiguration).isValid(), "Fixed-palette canonical configuration validation failed.");
            const GeneratedShip fixedPaletteShip = generator.generate(fixedPaletteConfiguration);
            ShipResolvedGenerationConfiguration fixedPaletteReroll = fixedPaletteConfiguration;
            fixedPaletteReroll.Generation.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x88F1EED123456789ull);
            const GeneratedShip fixedPaletteRerolledShip = generator.generate(fixedPaletteReroll);
            require(samePalette(fixedPaletteShip.Palette, fixedPaletteConfiguration.Generation.PaletteConfiguration.Fixed), "Fixed palette did not preserve exact semantic colors.");
            require(sameImage(fixedPaletteShip.FinalImage, fixedPaletteRerolledShip.FinalImage), "Fixed palette PALETTE-domain reroll changed final pixels.");

            ShipIdleAnimationSettings idleSettings;
            idleSettings.Seed = 0x12345678u;
            idleSettings.MinimumFrameCount = 4u;
            idleSettings.MaximumFrameCount = 8u;
            ShipIdleAnimator idleAnimator;
            const auto idle = idleAnimator.generate(customShip, idleSettings);
            require(!idle.Frames.empty() && sameImage(idle.Frames.front(), customShip.FinalImage), "Custom IDLE frame 0 invariant failed.");

            ShipLateralMovementAnimator lateral;
            ShipLongitudinalMovementAnimator longitudinal;
            require(lateral.evaluateFrameAtNormalizedTime(customShip, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::ENTER, 0.41).getWidth() == customShip.FinalImage.getWidth(), "Custom lateral animation failed.");
            require(longitudinal.evaluateFrameAtNormalizedTime(customShip, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.33).getHeight() == customShip.FinalImage.getHeight(), "Custom longitudinal animation failed.");

            ShipFiringAnimator firingAnimator;
            const auto firingTargets = firingAnimator.getAvailableTargets(customShip);
            if (!firingTargets.empty())
            {
                require(firingAnimator.evaluateFrameAtNormalizedTime(customShip, firingTargets.front(), 0.56).getWidth() == customShip.FinalImage.getWidth(), "Custom firing animation failed.");

                ShipAnimationStateRequest request;
                request.UnderlyingMovementType = ShipAnimationType::MOVE_LEFT;
                request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
                request.MovementNormalizedTime = 0.31;
                request.FireActive = true;
                request.FiringTarget = firingTargets.front();
                request.FiringNormalizedTime = 0.28;
                const ShipAnimationStateEvaluation composed = ShipAnimationStateCoordinator().evaluate(customShip, request);
                require(composed.Pose.Frame.getWidth() == customShip.FinalImage.getWidth(), "Custom composed animation state failed.");
            }

            const ShipGenerationRecipe recipe = makeShipGenerationRecipe(custom);
            ShipGenerationRecipeDocument document;
            document.Recipe = recipe;
            const auto loaded = deserializeShipGenerationRecipe(serializeShipGenerationRecipe(document));
            require(loaded.Success, "Portable custom recipe round-trip failed.");
            require(sameImage(generator.generate(loaded.Document.Recipe).FinalImage, customShip.FinalImage), "Portable custom recipe did not reproduce canonical custom ship.");

            SpectralShipGenDiagnostics::DiagnosticsRunConfiguration diagnostics;
            diagnostics.Dimensions = { custom.Generation.Dimensions };
            diagnostics.SamplesPerConfiguration = 2u;
            diagnostics.DiagnosticSeed = 0x8899AABBCCDDEEFFull;
            diagnostics.ConfigurationLabel = "task88-custom";
            const auto diagnosticsResult = SpectralShipGenDiagnostics::DiagnosticsRunner().run(diagnostics, custom);
            require(diagnosticsResult.Completed && diagnosticsResult.OverallSummary.SuccessfulSamples == 2u, "Custom resolved diagnostics run failed.");
            require(diagnosticsResult.Configuration.PaletteSourceMode == ShipPaletteSourceMode::EXPLICIT_GENERATED, "Diagnostics palette identity was not preserved.");
            require(diagnosticsResult.Configuration.Styles.size() == 1u && diagnosticsResult.Configuration.Styles.front() == ShipStyle::SHIP_STYLE_END, "Diagnostics fabricated structural preset identity.");
            require(diagnosticsResult.Configuration.Factions.size() == 1u && diagnosticsResult.Configuration.Factions.front() == ShipFactionType::SHIP_FACTION_TYPE_END, "Diagnostics fabricated faction preset identity.");

            ShipResolvedGenerationConfiguration invalid = custom;
            invalid.Generation.Dimensions.Width = 8u;
            require(!validateShipGenerationConfiguration(invalid).isValid(), "Combined configuration validation accepted unsafe dimensions.");

            std::cout << "Public configuration API regression passed.\n";
            return 0;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Public configuration API regression failed: " << exception.what() << '\n';
            return 1;
        }
    }
}
