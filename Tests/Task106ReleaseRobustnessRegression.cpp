#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <SpectralShipGen/Diagnostics/DiagnosticsRunner.h>
#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/GenerationDomain.h>
#include <SpectralShipGen/ShipAnimationStateCoordinator.h>
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipFiringAnimator.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipLateralMovementAnimator.h>
#include <SpectralShipGen/ShipLongitudinalMovementAnimator.h>
#include <SpectralShipGen/ShipPaletteGenerationProfile.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenDiagnostics;

    uint64_t imageSignature(const Image& image)
    {
        uint64_t hash = 1469598103934665603ull;
        auto mix = [&](uint8_t value) { hash ^= value; hash *= 1099511628211ull; };
        for (const Color& pixel : image.getPixels()) { mix(pixel.R); mix(pixel.G); mix(pixel.B); mix(pixel.A); }
        return hash;
    }

    struct CustomCase
    {
        const char* Name;
        ShipResolvedGenerationConfiguration Configuration;
    };

    ShipResolvedGenerationConfiguration makeResolved(ShipStyle style, ShipFactionType faction, ShipDimensions dimensions, uint64_t seed)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        return resolveShipGenerationConfiguration(settings);
    }

    std::vector<CustomCase> makeCustomCases()
    {
        std::vector<CustomCase> cases;

        auto sparse = makeResolved(ShipStyle::SLEEK, ShipFactionType::FRONTIER, { 64u, 64u }, 0x1060000000000001ull);
        sparse.Provenance.StructuralPreset.reset();
        sparse.StructuralProfile.MajorFeatureChance = 0u;
        sparse.StructuralProfile.MaximumMajorFeatures = 0u;
        sparse.StructuralProfile.LargeWeaponChance = 0u;
        sparse.StructuralProfile.MaximumLargeWeaponGroups = 0u;
        sparse.StructuralProfile.AttachmentChance = 0u;
        sparse.StructuralProfile.MaximumAttachmentGroups = 0u;
        sparse.StructuralProfile.DetailMotifChance = 0u;
        sparse.StructuralProfile.SecondaryDetailMotifChance = 0u;
        sparse.StructuralProfile.MaterialCompositionChance = 0u;
        sparse.StructuralProfile.MaximumMaterialZones = 0u;
        sparse.StructuralProfile.LiveryChance = 0u;
        sparse.StructuralProfile.MaximumLiveryMarkings = 0u;
        cases.push_back({ "sparse", sparse });

        auto broad = makeResolved(ShipStyle::DELTA, ShipFactionType::CORPORATE, { 96u, 64u }, 0x1060000000000002ull);
        broad.Provenance.StructuralPreset.reset();
        broad.StructuralProfile.NoWingWeight = 0u;
        broad.StructuralProfile.SmallWingWeight = 0u;
        broad.StructuralProfile.SweptWingWeight = 0u;
        broad.StructuralProfile.BroadWingWeight = 1000u;
        broad.StructuralProfile.BroadWingWidthPercent = { 96u, 100u };
        broad.StructuralProfile.LargeWeaponScalePercent = 140u;
        cases.push_back({ "broad", broad });

        auto weapons = makeResolved(ShipStyle::HEAVY, ShipFactionType::MILITARY, { 96u, 96u }, 0x1060000000000003ull);
        weapons.Provenance.StructuralPreset.reset();
        weapons.StructuralProfile.LargeWeaponChance = 100u;
        weapons.StructuralProfile.MaximumLargeWeaponGroups = 4u;
        weapons.StructuralProfile.LargeWeaponSymmetryChance = 100u;
        weapons.StructuralProfile.LargeWeaponScalePercent = 150u;
        cases.push_back({ "weapon-heavy", weapons });

        auto wingless = makeResolved(ShipStyle::SPEARHEAD, ShipFactionType::ASCENDANT, { 44u, 64u }, 0x1060000000000004ull);
        wingless.Provenance.StructuralPreset.reset();
        wingless.StructuralProfile.NoWingWeight = 1000u;
        wingless.StructuralProfile.SmallWingWeight = 0u;
        wingless.StructuralProfile.SweptWingWeight = 0u;
        wingless.StructuralProfile.BroadWingWeight = 0u;
        wingless.StructuralProfile.HullHorizontalPaddingPercent = { 20u, 28u };
        cases.push_back({ "compact-wingless", wingless });

        auto asymmetric = makeResolved(ShipStyle::INDUSTRIAL, ShipFactionType::XENO, { 64u, 96u }, 0x1060000000000005ull);
        asymmetric.Provenance.StructuralPreset.reset();
        asymmetric.Generation.AsymmetricDetailChance = 100u;
        asymmetric.StructuralProfile.MacroAsymmetryChance = 100u;
        asymmetric.StructuralProfile.MacroAsymmetryVisualWeightPercent = 180u;
        asymmetric.StructuralProfile.LiveryAsymmetricChance = 100u;
        cases.push_back({ "strong-asymmetry", asymmetric });

        auto unusualFaction = makeResolved(ShipStyle::FIGHTER, ShipFactionType::RELIC, { 96u, 64u }, 0x1060000000000006ull);
        unusualFaction.Provenance.FactionPreset.reset();
        unusualFaction.Provenance.PaletteFactionPreset.reset();
        unusualFaction.FactionProfile.SurfaceDetails.DetailDensityPercent = 170u;
        unusualFaction.FactionProfile.Weapons.ChancePercent = 145u;
        unusualFaction.FactionProfile.Materials.ZoneWeightMultipliersPercent.AxialBand = 175u;
        unusualFaction.FactionProfile.Livery.ChancePercent = 140u;
        cases.push_back({ "unusual-faction", unusualFaction });

        auto generatedPalette = makeResolved(ShipStyle::DELTA, ShipFactionType::CORPORATE, { 96u, 160u }, 0x1060000000000007ull);
        generatedPalette.Generation.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        generatedPalette.Generation.PaletteConfiguration.Generated = getShipPaletteGenerationProfile(ShipFactionType::XENO);
        generatedPalette.Generation.PaletteConfiguration.Generated.Ranges.HullHue = { 0u, 359u };
        generatedPalette.Provenance.PaletteSource = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        generatedPalette.Provenance.PaletteFactionPreset.reset();
        cases.push_back({ "unusual-generated-palette", generatedPalette });

        auto fixedPalette = makeResolved(ShipStyle::SLEEK, ShipFactionType::CORPORATE, { 160u, 96u }, 0x1060000000000008ull);
        fixedPalette.Generation.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
        fixedPalette.Generation.PaletteConfiguration.Fixed.HullDeepShadow = Color(0u, 0u, 0u, 255u);
        fixedPalette.Generation.PaletteConfiguration.Fixed.HullBase = Color(255u, 255u, 255u, 255u);
        fixedPalette.Generation.PaletteConfiguration.Fixed.HullAccent = Color(255u, 0u, 255u, 255u);
        fixedPalette.Generation.PaletteConfiguration.Fixed.LightBase = Color(0u, 255u, 0u, 255u);
        fixedPalette.Provenance.PaletteSource = ShipPaletteSourceMode::FIXED;
        fixedPalette.Provenance.PaletteFactionPreset.reset();
        cases.push_back({ "fixed-palette", fixedPalette });

        return cases;
    }

    bool sameSampleIdentity(const DiagnosticsRawSampleResult& first, const DiagnosticsRawSampleResult& second)
    {
        return first.WorkItem.Seed == second.WorkItem.Seed &&
            first.Success == second.Success &&
            first.ErrorMessage == second.ErrorMessage &&
            first.FinalImageSignature == second.FinalImageSignature &&
            first.HullAttemptCount == second.HullAttemptCount &&
            first.HullValidationRejectionCount == second.HullValidationRejectionCount &&
            first.WeaponPlacementAttemptCount == second.WeaponPlacementAttemptCount &&
            first.AttachmentPlacementAttemptCount == second.AttachmentPlacementAttemptCount;
    }

    int fail(const std::string& message)
    {
        std::cerr << "Task-106 release robustness regression failed: " << message << '\n';
        return 1;
    }
}

int SpectralShipGenTests::runTask106ReleaseRobustnessRegression()
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenDiagnostics;

    const std::vector<CustomCase> customCases = makeCustomCases();
    ShipGenerator generator;

    for (const CustomCase& customCase : customCases)
    {
        const ValidationResult validation = validateShipGenerationConfiguration(customCase.Configuration);
        if (!validation.isValid()) { return fail(std::string(customCase.Name) + " is not valid through the public validation contract"); }

        const GeneratedShip first = generator.generate(customCase.Configuration);
        const GeneratedShip second = generator.generate(customCase.Configuration);
        if (first.FinalImage.empty() || imageSignature(first.FinalImage) != imageSignature(second.FinalImage))
        {
            return fail(std::string(customCase.Name) + " is not deterministic");
        }

        ShipGenerationRecipe recipe = makeShipGenerationRecipe(customCase.Configuration);
        ShipGenerationRecipeDocument document;
        document.Recipe = recipe;
        const std::string serialized = serializeShipGenerationRecipe(document);
        const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(serialized);
        if (!loaded.Success || !validateShipGenerationRecipe(loaded.Document.Recipe).isValid())
        {
            return fail(std::string(customCase.Name) + " recipe round-trip failed");
        }
        const GeneratedShip recipeShip = generator.generate(loaded.Document.Recipe);
        if (imageSignature(first.FinalImage) != imageSignature(recipeShip.FinalImage))
        {
            return fail(std::string(customCase.Name) + " recipe changed generated output");
        }
    }

    DiagnosticsRunConfiguration builtIns;
    builtIns.Dimensions = { { 24u, 24u }, { 96u, 64u }, { 160u, 160u } };
    builtIns.Styles.clear();
    builtIns.Factions.clear();
    for (const auto& entry : getBuiltInStructuralPresetCatalog()) { builtIns.Styles.push_back(entry.Preset); }
    for (const auto& entry : getBuiltInFactionPresetCatalog()) { builtIns.Factions.push_back(entry.Preset); }
    builtIns.SamplesPerConfiguration = 4u;
    builtIns.DiagnosticSeed = 0x106B17B17B17B17Bull;
    builtIns.DetailedPerformanceInstrumentation = true;

    const DiagnosticsResult builtInResult = DiagnosticsRunner().run(builtIns);
    if (!builtInResult.Completed || builtInResult.OverallSummary.FailedSamples != 0u || builtInResult.Samples.size() != 432u)
    {
        return fail("built-in deterministic stress matrix did not complete successfully");
    }
    for (const DiagnosticsRawSampleResult& sample : builtInResult.Samples)
    {
        if (!sample.Success || sample.HullAttemptCount == 0u || sample.HullAttemptCount > 8u)
        {
            return fail("built-in stress sample exceeded the bounded hull retry contract");
        }
    }

    for (const CustomCase& customCase : customCases)
    {
        DiagnosticsRunConfiguration configuration;
        configuration.Dimensions = { { 24u, 24u }, { 96u, 64u }, { 160u, 96u } };
        configuration.SamplesPerConfiguration = 3u;
        configuration.DiagnosticSeed = 0x106C057000000000ull ^ customCase.Configuration.Generation.Seed;
        configuration.DetailedPerformanceInstrumentation = true;
        configuration.ConfigurationLabel = customCase.Name;

        const DiagnosticsResult first = DiagnosticsRunner().run(configuration, customCase.Configuration);
        const DiagnosticsResult second = DiagnosticsRunner().run(configuration, customCase.Configuration);
        if (!first.Completed || !second.Completed || first.Samples.size() != second.Samples.size())
        {
            return fail(std::string(customCase.Name) + " diagnostics stress did not complete");
        }
        for (std::size_t index = 0u; index < first.Samples.size(); ++index)
        {
            if (!sameSampleIdentity(first.Samples[index], second.Samples[index]))
            {
                return fail(std::string(customCase.Name) + " diagnostics repeat changed deterministic result identity");
            }
        }
    }

    auto extreme = customCases.front().Configuration;
    extreme.Provenance.StructuralPreset.reset();
    constexpr std::array<ShipDimensions, 3u> extremeDimensions = { { { 16u, 16u }, { 16u, 4096u }, { 4096u, 16u } } };
    for (const ShipDimensions dimensions : extremeDimensions)
    {
        extreme.Generation.Dimensions = dimensions;
        if (!validateShipGenerationConfiguration(extreme).isValid()) { return fail("extreme boundary dimensions were rejected by public validation"); }

        DiagnosticsRunConfiguration configuration;
        configuration.Dimensions = { dimensions };
        configuration.SamplesPerConfiguration = 2u;
        configuration.DiagnosticSeed = 0x106E000000000000ull ^ (static_cast<uint64_t>(dimensions.Width) << 32u) ^ dimensions.Height;
        configuration.ConfigurationLabel = "extreme-valid";
        const DiagnosticsResult first = DiagnosticsRunner().run(configuration, extreme);
        const DiagnosticsResult second = DiagnosticsRunner().run(configuration, extreme);
        if (!first.Completed || !second.Completed || first.Samples.size() != 2u || second.Samples.size() != 2u)
        {
            return fail("extreme-valid diagnostics did not return coherent results");
        }
        for (std::size_t index = 0u; index < first.Samples.size(); ++index)
        {
            if (!sameSampleIdentity(first.Samples[index], second.Samples[index])) { return fail("extreme-valid success/failure behavior is not deterministic"); }
        }
    }

    ShipResolvedGenerationConfiguration animatedConfiguration = customCases[2u].Configuration;
    animatedConfiguration.Generation.Dimensions = { 160u, 96u };
    GeneratedShip animatedShip;
    ShipFiringAnimator firingAnimator;
    for (uint64_t offset = 0u; offset < 128u; ++offset)
    {
        animatedConfiguration.Generation.Seed = 0x106A000000000000ull + offset;
        animatedShip = generator.generate(animatedConfiguration);
        if (!firingAnimator.getAvailableTargets(animatedShip).empty()) { break; }
    }
    if (animatedShip.FinalImage.empty()) { return fail("could not generate rectangular animation stress ship"); }

    ShipIdleAnimationSettings idleSettings;
    idleSettings.MinimumFrameCount = 10u;
    idleSettings.MaximumFrameCount = 60u;
    const ShipIdleAnimation idle = ShipIdleAnimator{}.generate(animatedShip, idleSettings);
    if (idle.Frames.empty() || idle.Frames.size() < idleSettings.MinimumFrameCount || idle.Frames.size() > idleSettings.MaximumFrameCount || imageSignature(idle.Frames.front()) != imageSignature(animatedShip.FinalImage))
    {
        return fail("adaptive IDLE animation contract failed");
    }

    ShipMovementAnimationSettings movementSettings;
    movementSettings.MaximumFrameCount = 60u;
    const ShipMovementAnimation left = ShipLateralMovementAnimator{}.generate(animatedShip, ShipAnimationType::MOVE_LEFT, movementSettings);
    const ShipMovementAnimation right = ShipLateralMovementAnimator{}.generate(animatedShip, ShipAnimationType::MOVE_RIGHT, movementSettings);
    const ShipMovementAnimation up = ShipLongitudinalMovementAnimator{}.generate(animatedShip, ShipAnimationType::MOVE_UP, movementSettings);
    const ShipMovementAnimation down = ShipLongitudinalMovementAnimator{}.generate(animatedShip, ShipAnimationType::MOVE_DOWN, movementSettings);
    for (const ShipMovementAnimation* animation : { &left, &right, &up, &down })
    {
        if (animation->Enter.Frames.empty() || animation->Sustain.Frames.empty() || animation->Exit.Frames.empty()) { return fail("movement animation produced an empty clip"); }
    }

    const std::vector<ShipFiringAnimationTarget> targets = firingAnimator.getAvailableTargets(animatedShip);
    if (!targets.empty())
    {
        const ShipFiringAnimation fire = firingAnimator.generate(animatedShip, targets.front());
        if (fire.Frames.empty()) { return fail("FIRE animation produced no frames"); }
        ShipAnimationStateRequest stateRequest;
        stateRequest.UnderlyingMovementType = ShipAnimationType::MOVE_LEFT;
        stateRequest.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
        stateRequest.MovementNormalizedTime = 0.5;
        stateRequest.FireActive = true;
        stateRequest.FiringTarget = targets.front();
        stateRequest.FiringNormalizedTime = 0.5;
        const ShipAnimationStateEvaluation combined = ShipAnimationStateCoordinator{}.evaluate(animatedShip, stateRequest);
        if (combined.Diagnostics.TransientEvent != ShipAnimationType::FIRE) { return fail("movement+FIRE state composition did not preserve the transient firing event"); }
    }

    GeneratedShip weaponless = animatedShip;
    weaponless.IdleAnimationMetadata.WeaponComponents.clear();
    if (!firingAnimator.getAvailableTargets(weaponless).empty()) { return fail("weaponless animation stress ship exposed firing targets"); }
    if (ShipIdleAnimator{}.generate(weaponless).Frames.empty()) { return fail("weaponless ship failed IDLE animation"); }

    ShipGenerationRecipe overrideRecipe = makeShipGenerationRecipe(customCases[4u].Configuration);
    overrideRecipe.DomainSeedOverrides.set(GenerationDomain::DETAILS, 0x106D0A1100000001ull);
    overrideRecipe.RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
    ShipGenerationRecipeDocument overrideDocument;
    overrideDocument.Recipe = overrideRecipe;
    const ShipGenerationRecipeLoadResult overrideLoad = deserializeShipGenerationRecipe(serializeShipGenerationRecipe(overrideDocument));
    if (!overrideLoad.Success || overrideLoad.Document.Recipe.DomainSeedOverrides.get(GenerationDomain::DETAILS) != overrideRecipe.DomainSeedOverrides.get(GenerationDomain::DETAILS))
    {
        return fail("domain override recipe semantics did not round-trip");
    }

    ShipGenerationRecipe legacyRecipe = makeShipGenerationRecipe(customCases[0u].Configuration);
    legacyRecipe.RandomStreamMode = GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS;
    if (!validateShipGenerationRecipe(legacyRecipe).isValid()) { return fail("legacy random stream recipe is not valid"); }
    const GeneratedShip legacyFirst = generator.generate(legacyRecipe);
    const GeneratedShip legacySecond = generator.generate(legacyRecipe);
    if (imageSignature(legacyFirst.FinalImage) != imageSignature(legacySecond.FinalImage)) { return fail("legacy random stream mode lost deterministic repeatability"); }

    uint64_t completed = 0u;
    DiagnosticsRunConfiguration cancellation = builtIns;
    cancellation.SamplesPerConfiguration = 8u;
    const DiagnosticsResult cancelled = DiagnosticsRunner().run(cancellation,
        [&](const DiagnosticsProgress& progress) { completed = progress.CompletedWorkItems; },
        [&]() { return completed >= 7u; });
    if (!cancelled.Cancelled || cancelled.Completed || cancelled.CompletedWorkItems != 7u || cancelled.Samples.size() != 7u)
    {
        return fail("heavy-matrix cancellation did not produce a coherent partial result");
    }

    std::cout << "Task-106 release robustness regression passed.\n";
    return 0;
}
