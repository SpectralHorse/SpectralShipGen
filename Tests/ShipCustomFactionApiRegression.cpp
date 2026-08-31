#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <PixelShipGenerator/ShipAnimationStateCoordinator.h>
#include <PixelShipGenerator/ShipFactionProfile.h>
#include <PixelShipGenerator/ShipFactionProfileValidation.h>
#include <PixelShipGenerator/ShipFiringAnimator.h>
#include <PixelShipGenerator/ShipGenerationProfileValidation.h>
#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipIdleAnimator.h>
#include <PixelShipGenerator/ShipLateralMovementAnimator.h>
#include <PixelShipGenerator/ShipLongitudinalMovementAnimator.h>

namespace
{
    using namespace PixelShipGenerator;

    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y)) { return false; }
            }
        }
        return true;
    }

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool palettesEqual(const ShipPalette& first, const ShipPalette& second)
    {
        return first.Transparent == second.Transparent && first.Outline == second.Outline &&
            first.HullDeepShadow == second.HullDeepShadow && first.HullShadow == second.HullShadow &&
            first.HullBase == second.HullBase && first.HullHighlight == second.HullHighlight &&
            first.HullSecondary == second.HullSecondary && first.HullEdgeHighlight == second.HullEdgeHighlight &&
            first.CockpitDark == second.CockpitDark && first.CockpitBase == second.CockpitBase &&
            first.CockpitHighlight == second.CockpitHighlight && first.CockpitGlint == second.CockpitGlint &&
            first.EngineDark == second.EngineDark && first.EngineBase == second.EngineBase &&
            first.EngineHighlight == second.EngineHighlight && first.EngineHotCore == second.EngineHotCore &&
            first.ExhaustBase == second.ExhaustBase && first.ExhaustHighlight == second.ExhaustHighlight &&
            first.ExhaustHotCore == second.ExhaustHotCore && first.HullAccentDark == second.HullAccentDark &&
            first.HullAccent == second.HullAccent && first.HullAccentHighlight == second.HullAccentHighlight &&
            first.MechanicalDark == second.MechanicalDark && first.MechanicalBase == second.MechanicalBase &&
            first.LightBase == second.LightBase && first.LightHighlight == second.LightHighlight;
    }

    bool staticBehaviorEqual(const GeneratedShip& first, const GeneratedShip& second)
    {
        return first.Seed == second.Seed && first.Seeds == second.Seeds && first.DomainSeeds.Values == second.DomainSeeds.Values &&
            palettesEqual(first.Palette, second.Palette) && masksEqual(first.HullMask, second.HullMask) &&
            masksEqual(first.CockpitMask, second.CockpitMask) && masksEqual(first.EngineMask, second.EngineMask) &&
            masksEqual(first.EngineExhaustMask, second.EngineExhaustMask) && masksEqual(first.AttachmentMask, second.AttachmentMask) &&
            masksEqual(first.AccentMask, second.AccentMask) && masksEqual(first.MechanicalDetailMask, second.MechanicalDetailMask) &&
            masksEqual(first.LightMask, second.LightMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponOccupiedMask, second.IdleAnimationMetadata.WeaponOccupiedMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponMovableMask, second.IdleAnimationMetadata.WeaponMovableMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponMuzzleMask, second.IdleAnimationMetadata.WeaponMuzzleMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponEmissiveMask, second.IdleAnimationMetadata.WeaponEmissiveMask) &&
            first.AttachmentPlacements.size() == second.AttachmentPlacements.size() &&
            first.IdleAnimationMetadata.EngineComponents.size() == second.IdleAnimationMetadata.EngineComponents.size() &&
            first.IdleAnimationMetadata.WeaponComponents.size() == second.IdleAnimationMetadata.WeaponComponents.size() &&
            first.IdleAnimationMetadata.MajorFeatureComponents.size() == second.IdleAnimationMetadata.MajorFeatureComponents.size() &&
            imagesEqual(first.FinalImage, second.FinalImage);
    }

    bool debugBehaviorEqual(const ShipGenerationDebugInfo& first, const ShipGenerationDebugInfo& second)
    {
        return first.ComplexityInitialBudget == second.ComplexityInitialBudget &&
            first.ComplexityConsumedBudget == second.ComplexityConsumedBudget &&
            first.ComplexityUnusedBudget == second.ComplexityUnusedBudget &&
            first.ComplexityCategoryAllocations == second.ComplexityCategoryAllocations &&
            first.ComplexityCategoryConsumed == second.ComplexityCategoryConsumed &&
            first.PrimaryVisualAnchor == second.PrimaryVisualAnchor && first.SecondaryVisualAnchor == second.SecondaryVisualAnchor &&
            first.VisualAnchorTargetRegion == second.VisualAnchorTargetRegion &&
            first.HullGenerationAttemptCount == second.HullGenerationAttemptCount &&
            first.HullValidationRejectionCount == second.HullValidationRejectionCount &&
            first.WeaponCount == second.WeaponCount && first.EngineCount == second.EngineCount &&
            first.MajorFeatureCount == second.MajorFeatureCount && first.AttachmentPlacedGroupCount == second.AttachmentPlacedGroupCount &&
            masksEqual(first.ReservedNegativeSpaceMask, second.ReservedNegativeSpaceMask) &&
            masksEqual(first.MaterialSecondaryHullMask, second.MaterialSecondaryHullMask) &&
            masksEqual(first.MaterialMechanicalMask, second.MaterialMechanicalMask) &&
            masksEqual(first.LiveryPrimaryMask, second.LiveryPrimaryMask) && masksEqual(first.LiverySecondaryMask, second.LiverySecondaryMask) &&
            masksEqual(first.WeaponOccupiedMask, second.WeaponOccupiedMask);
    }

    ExplicitShipGenerationConfiguration explicitConfigurationFrom(const ShipGenerationSettings& settings)
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

    bool animationEvaluationEqual(const GeneratedShip& first, const GeneratedShip& second)
    {
        ShipIdleAnimator idleAnimator;
        ShipLateralMovementAnimator lateralAnimator;
        ShipLongitudinalMovementAnimator longitudinalAnimator;
        ShipFiringAnimator firingAnimator;
        ShipAnimationStateCoordinator coordinator;

        if (!imagesEqual(idleAnimator.evaluateFrameAtNormalizedTime(first, 0.0), first.FinalImage) ||
            !imagesEqual(idleAnimator.evaluateFrameAtNormalizedTime(first, 0.37), idleAnimator.evaluateFrameAtNormalizedTime(second, 0.37)))
        {
            return false;
        }

        for (const ShipAnimationType type : { ShipAnimationType::MOVE_LEFT, ShipAnimationType::MOVE_RIGHT })
        {
            if (!imagesEqual(lateralAnimator.evaluateFrameAtNormalizedTime(first, type, ShipMovementAnimationPhase::ENTER, 0.41), lateralAnimator.evaluateFrameAtNormalizedTime(second, type, ShipMovementAnimationPhase::ENTER, 0.41)) ||
                !imagesEqual(lateralAnimator.evaluateFrameAtNormalizedTime(first, type, ShipMovementAnimationPhase::SUSTAIN, 0.33), lateralAnimator.evaluateFrameAtNormalizedTime(second, type, ShipMovementAnimationPhase::SUSTAIN, 0.33)) ||
                !imagesEqual(lateralAnimator.evaluateFrameAtNormalizedTime(first, type, ShipMovementAnimationPhase::EXIT, 0.58), lateralAnimator.evaluateFrameAtNormalizedTime(second, type, ShipMovementAnimationPhase::EXIT, 0.58)))
            {
                return false;
            }
        }

        for (const ShipAnimationType type : { ShipAnimationType::MOVE_UP, ShipAnimationType::MOVE_DOWN })
        {
            if (!imagesEqual(longitudinalAnimator.evaluateFrameAtNormalizedTime(first, type, ShipMovementAnimationPhase::ENTER, 0.41), longitudinalAnimator.evaluateFrameAtNormalizedTime(second, type, ShipMovementAnimationPhase::ENTER, 0.41)) ||
                !imagesEqual(longitudinalAnimator.evaluateFrameAtNormalizedTime(first, type, ShipMovementAnimationPhase::SUSTAIN, 0.33), longitudinalAnimator.evaluateFrameAtNormalizedTime(second, type, ShipMovementAnimationPhase::SUSTAIN, 0.33)) ||
                !imagesEqual(longitudinalAnimator.evaluateFrameAtNormalizedTime(first, type, ShipMovementAnimationPhase::EXIT, 0.58), longitudinalAnimator.evaluateFrameAtNormalizedTime(second, type, ShipMovementAnimationPhase::EXIT, 0.58)))
            {
                return false;
            }
        }

        const std::vector<ShipFiringAnimationTarget> firstTargets = firingAnimator.getAvailableTargets(first);
        const std::vector<ShipFiringAnimationTarget> secondTargets = firingAnimator.getAvailableTargets(second);
        if (firstTargets.size() != secondTargets.size()) { return false; }
        if (!firstTargets.empty())
        {
            const ShipFiringAnimationTarget target = firstTargets.front();
            if (target.WeaponComponentIndex != secondTargets.front().WeaponComponentIndex || target.IncludeSymmetryGroup != secondTargets.front().IncludeSymmetryGroup) { return false; }
            if (!imagesEqual(firingAnimator.evaluateFrameAtNormalizedTime(first, target, 0.28), firingAnimator.evaluateFrameAtNormalizedTime(second, secondTargets.front(), 0.28)) ||
                !imagesEqual(firingAnimator.evaluateFrameAtNormalizedTime(first, target, 0.56), firingAnimator.evaluateFrameAtNormalizedTime(second, secondTargets.front(), 0.56)))
            {
                return false;
            }

            ShipAnimationStateRequest request;
            request.UnderlyingMovementType = ShipAnimationType::MOVE_LEFT;
            request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
            request.MovementNormalizedTime = 0.31;
            request.FireActive = true;
            request.FiringTarget = target;
            request.FiringNormalizedTime = 0.28;
            const ShipAnimationStateEvaluation firstComposed = coordinator.evaluate(first, request);
            request.FiringTarget = secondTargets.front();
            const ShipAnimationStateEvaluation secondComposed = coordinator.evaluate(second, request);
            if (!imagesEqual(firstComposed.Pose.Frame, secondComposed.Pose.Frame) || firstComposed.Pose.Layer != secondComposed.Pose.Layer ||
                firstComposed.Diagnostics.EventPhase != secondComposed.Diagnostics.EventPhase || firstComposed.Diagnostics.ResultLayer != secondComposed.Diagnostics.ResultLayer)
            {
                return false;
            }
        }

        return true;
    }

    bool checkBuiltInEquivalence()
    {
        constexpr std::array<ShipFactionType, 6u> Factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
        constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::FIGHTER, ShipStyle::SPEARHEAD, ShipStyle::DELTA, ShipStyle::INDUSTRIAL, ShipStyle::SLEEK, ShipStyle::HEAVY };
        constexpr std::array<ShipDimensions, 6u> Dimensions = { ShipDimensions{ 64u, 64u }, ShipDimensions{ 96u, 64u }, ShipDimensions{ 64u, 96u }, ShipDimensions{ 96u, 96u }, ShipDimensions{ 80u, 64u }, ShipDimensions{ 64u, 80u } };

        ShipGenerator generator;
        for (std::size_t index = 0u; index < Factions.size(); ++index)
        {
            ShipGenerationSettings presetSettings;
            presetSettings.Seed = 0x8500000000000000ull + static_cast<uint64_t>(index) * 0x10001ull;
            presetSettings.Dimensions = Dimensions[index];
            presetSettings.Style = Styles[index];
            presetSettings.Faction = Factions[index];
            presetSettings.DetailDensity = 61u;
            presetSettings.AsymmetricDetailChance = 17u;
            presetSettings.AttachmentsEnabled = true;
            presetSettings.DomainSeedOverrides.set(GenerationDomain::DETAILS, presetSettings.Seed ^ 0x94D049BB133111EBull);

            ShipGenerationDebugInfo presetDebug;
            ShipGenerationDebugInfo explicitDebug;
            const GeneratedShip preset = generator.generate(presetSettings, &presetDebug);
            const GeneratedShip explicitShip = generator.generate(explicitConfigurationFrom(presetSettings), getShipGenerationProfile(Styles[index]), getShipFactionProfile(Factions[index]), &explicitDebug);

            if (preset.Style != Styles[index] || preset.Faction != Factions[index] || explicitShip.Style != ShipStyle::SHIP_STYLE_END || explicitShip.Faction != ShipFactionType::SHIP_FACTION_TYPE_END)
            {
                std::cerr << "Task 85 regression failed: built-in/custom provenance semantics are not truthful.\n";
                return false;
            }
            if (!staticBehaviorEqual(preset, explicitShip) || !debugBehaviorEqual(presetDebug, explicitDebug) || !animationEvaluationEqual(preset, explicitShip))
            {
                std::cerr << "Task 85 regression failed: built-in faction enum/profile paths differ for faction index " << index << ".\n";
                return false;
            }

            GeneratedShip withoutFactionProvenance = preset;
            withoutFactionProvenance.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
            if (!animationEvaluationEqual(preset, withoutFactionProvenance))
            {
                std::cerr << "Task 85 regression failed: animation still depends on GeneratedShip::Faction provenance.\n";
                return false;
            }
        }
        return true;
    }

    ShipFactionProfile makeCustomFactionA()
    {
        ShipFactionProfile profile = getShipFactionProfile(ShipFactionType::CORPORATE);
        profile.Palette.Light.HueOffset = { 120, 180 };
        profile.Animation.Idle.TechPulseStrength = 3u;
        profile.Animation.LateralMovement.ResponseStrengthScale = { 7u, 10u };
        return profile;
    }

    ShipFactionProfile makeCustomFactionB()
    {
        ShipFactionProfile profile = getShipFactionProfile(ShipFactionType::RELIC);
        profile.Weapons.ChancePercent = 110u;
        profile.Animation.Idle.ExhaustAmplitudeScale = { 4u, 5u };
        profile.Animation.Firing.ResponseStrengthScale = { 7u, 10u };
        return profile;
    }

    ShipFactionProfile makeCustomFactionC()
    {
        ShipFactionProfile profile;
        profile.Palette.HullHue = { 250u, 300u };
        profile.Palette.HullSaturation = { 25u, 55u };
        profile.Palette.HullValue = { 35u, 65u };
        profile.SurfaceDetails.DetailDensityPercent = 65u;
        profile.Attachments.AttachmentChancePercent = 135u;
        profile.Animation.Idle.AsynchronousEngines = ShipFactionAnimationBooleanOverride::ENABLE;
        profile.Animation.LongitudinalMovement.Staggered = ShipFactionAnimationBooleanOverride::ENABLE;
        profile.Animation.Firing.DurationAdditionMilliseconds = 25;
        return profile;
    }

    bool checkCustomGeneration()
    {
        ShipGenerator generator;
        std::array<ShipGenerationProfile, 3u> structuralProfiles = {
            getShipGenerationProfile(ShipStyle::INDUSTRIAL),
            getShipGenerationProfile(ShipStyle::DELTA),
            getShipGenerationProfile(ShipStyle::FIGHTER)
        };
        structuralProfiles[0].LargeWeaponChance = 100u;
        structuralProfiles[0].LargeWeaponScalePercent = 145u;
        structuralProfiles[1].DetailDensityPercent = 35u;
        structuralProfiles[2].BroadWingWeight += 90u;

        const std::array<ShipFactionProfile, 3u> factionProfiles = { makeCustomFactionA(), makeCustomFactionB(), makeCustomFactionC() };
        const std::array<ShipDimensions, 3u> dimensions = { ShipDimensions{ 128u, 96u }, ShipDimensions{ 96u, 128u }, ShipDimensions{ 96u, 64u } };

        for (std::size_t index = 0u; index < factionProfiles.size(); ++index)
        {
            if (!validateShipGenerationProfile(structuralProfiles[index]).isValid() || !validateShipFactionProfile(factionProfiles[index]).isValid())
            {
                std::cerr << "Task 85 regression setup contains an invalid custom profile.\n";
                return false;
            }

            ExplicitShipGenerationConfiguration configuration;
            configuration.Seed = 0x85C0000000000000ull + static_cast<uint64_t>(index) * 0x10101ull;
            configuration.Dimensions = dimensions[index];
            configuration.DetailDensity = 57u;
            configuration.AsymmetricDetailChance = 23u;
            configuration.AttachmentsEnabled = true;

            ShipGenerationDebugInfo debugInfo;
            const GeneratedShip first = generator.generate(configuration, structuralProfiles[index], factionProfiles[index], &debugInfo);
            const GeneratedShip second = generator.generate(configuration, structuralProfiles[index], factionProfiles[index]);
            if (first.Style != ShipStyle::SHIP_STYLE_END || first.Faction != ShipFactionType::SHIP_FACTION_TYPE_END || !staticBehaviorEqual(first, second) || !animationEvaluationEqual(first, second))
            {
                std::cerr << "Task 85 regression failed: custom structural + custom faction generation is not deterministic/animatable.\n";
                return false;
            }
            if (debugInfo.HullGenerationAttemptCount == 0u)
            {
                std::cerr << "Task 85 regression failed: explicit custom-faction generation did not populate Core debug diagnostics.\n";
                return false;
            }
        }

        ExplicitShipGenerationConfiguration baseConfiguration;
        baseConfiguration.Seed = 0x85D0A11E77E00001ull;
        baseConfiguration.Dimensions = { 96u, 64u };
        const ShipGenerationProfile structuralProfile = structuralProfiles[0];
        const ShipFactionProfile factionProfile = factionProfiles[0];
        const GeneratedShip base = generator.generate(baseConfiguration, structuralProfile, factionProfile);
        ExplicitShipGenerationConfiguration paletteReroll = baseConfiguration;
        paletteReroll.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x85D0A11E77E0F00Dull);
        const GeneratedShip paletteChanged = generator.generate(paletteReroll, structuralProfile, factionProfile);
        if (base.DomainSeeds.get(GenerationDomain::PALETTE) == paletteChanged.DomainSeeds.get(GenerationDomain::PALETTE) ||
            !masksEqual(base.HullMask, paletteChanged.HullMask) || !masksEqual(base.CockpitMask, paletteChanged.CockpitMask) ||
            !masksEqual(base.EngineMask, paletteChanged.EngineMask) || !masksEqual(base.AttachmentMask, paletteChanged.AttachmentMask) ||
            !masksEqual(base.IdleAnimationMetadata.WeaponOccupiedMask, paletteChanged.IdleAnimationMetadata.WeaponOccupiedMask))
        {
            std::cerr << "Task 85 regression failed: palette-domain reroll changed custom-profile structural ownership.\n";
            return false;
        }

        ShipFactionProfile invalidFaction = factionProfile;
        invalidFaction.Animation.Firing.DurationScale.Denominator = 0u;
        bool threw = false;
        try { (void)generator.generate(baseConfiguration, structuralProfile, invalidFaction); }
        catch (const std::invalid_argument&) { threw = true; }
        if (!threw)
        {
            std::cerr << "Task 85 regression failed: invalid explicit ShipFactionProfile was not rejected.\n";
            return false;
        }

        // Common workflow: a canonical built-in structural profile value combined
        // with a custom faction still uses the explicit path and carries no fake IDs.
        const GeneratedShip builtInStructureCustomFaction = generator.generate(baseConfiguration, getShipGenerationProfile(ShipStyle::FIGHTER), factionProfile);
        if (builtInStructureCustomFaction.Style != ShipStyle::SHIP_STYLE_END || builtInStructureCustomFaction.Faction != ShipFactionType::SHIP_FACTION_TYPE_END)
        {
            std::cerr << "Task 85 regression failed: built-in structural profile + custom faction fabricated provenance.\n";
            return false;
        }

        return true;
    }
}

int PixelShipGeneratorTests::runCustomFactionApiRegression()
{
    if (!checkBuiltInEquivalence() || !checkCustomGeneration()) { return 1; }
    std::cout << "Task 85 custom faction API / animation routing regression passed.\n";
    return 0;
}
