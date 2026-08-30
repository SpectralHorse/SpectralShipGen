#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "GenerationTuningProfile.h"
#include "ShipAnimationStateCoordinator.h"
#include "ShipFiringAnimator.h"
#include "ShipGenerationProfileValidation.h"
#include "ShipGenerator.h"
#include "ShipIdleAnimator.h"
#include "ShipLateralMovementAnimator.h"
#include "ShipLongitudinalMovementAnimator.h"

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

    bool paletteEqual(const ShipPalette& first, const ShipPalette& second)
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

    bool attachmentPlacementsEqual(const std::vector<ShipAttachmentPlacement>& first, const std::vector<ShipAttachmentPlacement>& second)
    {
        if (first.size() != second.size()) { return false; }
        for (std::size_t index = 0u; index < first.size(); ++index)
        {
            const auto& a = first[index];
            const auto& b = second[index];
            if (a.Type != b.Type || a.Region != b.Region || a.Direction != b.Direction ||
                a.AnchorX != b.AnchorX || a.AnchorY != b.AnchorY ||
                a.MinimumX != b.MinimumX || a.MaximumX != b.MaximumX ||
                a.MinimumY != b.MinimumY || a.MaximumY != b.MaximumY ||
                a.SymmetryGroup != b.SymmetryGroup)
            {
                return false;
            }
        }
        return true;
    }

    bool animationTraitsEqual(const ShipAnimationTraits& first, const ShipAnimationTraits& second)
    {
        return first.Idle.EnginePulseStrength == second.Idle.EnginePulseStrength &&
            first.Idle.ExhaustAmplitudePercent == second.Idle.ExhaustAmplitudePercent &&
            first.Idle.EngineMechanicalChance == second.Idle.EngineMechanicalChance &&
            first.Idle.WeaponMechanicalChance == second.Idle.WeaponMechanicalChance &&
            first.Idle.VentActivityChance == second.Idle.VentActivityChance &&
            first.Idle.SynchronizeEngines == second.Idle.SynchronizeEngines &&
            first.Idle.AsynchronousEngines == second.Idle.AsynchronousEngines &&
            first.Idle.AlternateEnginePhases == second.Idle.AlternateEnginePhases &&
            first.Idle.SlowMechanicalCycle == second.Idle.SlowMechanicalCycle &&
            first.LateralMovement.ResponseStrengthPercent == second.LateralMovement.ResponseStrengthPercent &&
            first.LateralMovement.EngineTravelLimit == second.LateralMovement.EngineTravelLimit &&
            first.LateralMovement.WeaponTravelLimit == second.LateralMovement.WeaponTravelLimit &&
            first.LateralMovement.AttachmentTravelLimit == second.LateralMovement.AttachmentTravelLimit &&
            first.LateralMovement.Synchronized == second.LateralMovement.Synchronized &&
            first.LateralMovement.Staggered == second.LateralMovement.Staggered &&
            first.LateralMovement.HeavyResponse == second.LateralMovement.HeavyResponse &&
            first.LateralMovement.Responsive == second.LateralMovement.Responsive &&
            first.LongitudinalMovement.ResponseStrengthPercent == second.LongitudinalMovement.ResponseStrengthPercent &&
            first.LongitudinalMovement.AccelerationExtensionPercent == second.LongitudinalMovement.AccelerationExtensionPercent &&
            first.LongitudinalMovement.BrakingContractionPercent == second.LongitudinalMovement.BrakingContractionPercent &&
            first.LongitudinalMovement.ExhaustVariationLimit == second.LongitudinalMovement.ExhaustVariationLimit &&
            first.LongitudinalMovement.WeaponTravelLimit == second.LongitudinalMovement.WeaponTravelLimit &&
            first.LongitudinalMovement.AttachmentTravelLimit == second.LongitudinalMovement.AttachmentTravelLimit &&
            first.LongitudinalMovement.BrakingTravelLimit == second.LongitudinalMovement.BrakingTravelLimit &&
            first.LongitudinalMovement.Synchronized == second.LongitudinalMovement.Synchronized &&
            first.LongitudinalMovement.Staggered == second.LongitudinalMovement.Staggered &&
            first.LongitudinalMovement.HeavyResponse == second.LongitudinalMovement.HeavyResponse &&
            first.LongitudinalMovement.Responsive == second.LongitudinalMovement.Responsive &&
            first.Firing.ResponseStrengthPercent == second.Firing.ResponseStrengthPercent &&
            first.Firing.DurationAdditionMilliseconds == second.Firing.DurationAdditionMilliseconds &&
            first.Firing.AdditionalRecoilLimit == second.Firing.AdditionalRecoilLimit &&
            first.Firing.RailWeaponAdditionalRecoilLimit == second.Firing.RailWeaponAdditionalRecoilLimit &&
            first.Firing.MaximumRecoilLimit == second.Firing.MaximumRecoilLimit &&
            first.Firing.MinimumPreFireExtensionLimit == second.Firing.MinimumPreFireExtensionLimit &&
            first.Firing.HeavyResponse == second.Firing.HeavyResponse &&
            first.Firing.Responsive == second.Firing.Responsive;
    }

    bool generatedBehaviorEqual(const GeneratedShip& first, const GeneratedShip& second)
    {
        return first.Seed == second.Seed && first.Seeds == second.Seeds && first.DomainSeeds.Values == second.DomainSeeds.Values &&
            paletteEqual(first.Palette, second.Palette) && animationTraitsEqual(first.AnimationTraits, second.AnimationTraits) &&
            masksEqual(first.HullMask, second.HullMask) && masksEqual(first.CockpitMask, second.CockpitMask) &&
            masksEqual(first.EngineMask, second.EngineMask) && masksEqual(first.EngineExhaustMask, second.EngineExhaustMask) &&
            masksEqual(first.AttachmentMask, second.AttachmentMask) && attachmentPlacementsEqual(first.AttachmentPlacements, second.AttachmentPlacements) &&
            masksEqual(first.AccentMask, second.AccentMask) && masksEqual(first.MechanicalDetailMask, second.MechanicalDetailMask) &&
            masksEqual(first.LightMask, second.LightMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponOccupiedMask, second.IdleAnimationMetadata.WeaponOccupiedMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponMovableMask, second.IdleAnimationMetadata.WeaponMovableMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponMuzzleMask, second.IdleAnimationMetadata.WeaponMuzzleMask) &&
            masksEqual(first.IdleAnimationMetadata.WeaponEmissiveMask, second.IdleAnimationMetadata.WeaponEmissiveMask) &&
            masksEqual(first.IdleAnimationMetadata.MajorFeatureMechanicalMask, second.IdleAnimationMetadata.MajorFeatureMechanicalMask) &&
            masksEqual(first.IdleAnimationMetadata.MajorFeatureEmissiveMask, second.IdleAnimationMetadata.MajorFeatureEmissiveMask) &&
            first.IdleAnimationMetadata.EngineComponents.size() == second.IdleAnimationMetadata.EngineComponents.size() &&
            first.IdleAnimationMetadata.WeaponComponents.size() == second.IdleAnimationMetadata.WeaponComponents.size() &&
            first.IdleAnimationMetadata.MajorFeatureComponents.size() == second.IdleAnimationMetadata.MajorFeatureComponents.size() &&
            first.FinalImage.getPixels() == second.FinalImage.getPixels();
    }

    bool debugBehaviorEqual(const ShipGenerationDebugInfo& first, const ShipGenerationDebugInfo& second)
    {
        return first.ComplexityInitialBudget == second.ComplexityInitialBudget &&
            first.ComplexityConsumedBudget == second.ComplexityConsumedBudget &&
            first.ComplexityUnusedBudget == second.ComplexityUnusedBudget &&
            first.PrimaryVisualAnchor == second.PrimaryVisualAnchor && first.SecondaryVisualAnchor == second.SecondaryVisualAnchor &&
            first.VisualAnchorTargetRegion == second.VisualAnchorTargetRegion &&
            first.ComplexityCategoryAllocations == second.ComplexityCategoryAllocations &&
            first.ComplexityCategoryConsumed == second.ComplexityCategoryConsumed &&
            first.SpatialRegionAreas == second.SpatialRegionAreas && first.SpatialRegionCapacities == second.SpatialRegionCapacities &&
            first.SpatialRegionLoads == second.SpatialRegionLoads && first.SpatialRegionMap == second.SpatialRegionMap &&
            first.HullGenerationAttemptCount == second.HullGenerationAttemptCount &&
            first.HullValidationRejectionCount == second.HullValidationRejectionCount &&
            first.WingShape == second.WingShape && first.WingPixelCount == second.WingPixelCount &&
            first.CoreTreatmentCount == second.CoreTreatmentCount && first.HullLayerCount == second.HullLayerCount &&
            first.MajorFeatureCount == second.MajorFeatureCount && first.WeaponCount == second.WeaponCount &&
            first.EngineCount == second.EngineCount && first.EngineLayout == second.EngineLayout &&
            first.CockpitPlacementSucceeded == second.CockpitPlacementSucceeded && first.CockpitShape == second.CockpitShape &&
            first.AttachmentPlacedGroupCount == second.AttachmentPlacedGroupCount &&
            first.MaterialZoneCount == second.MaterialZoneCount && first.LiveryMarkingCount == second.LiveryMarkingCount &&
            first.PrimaryDetailMotif == second.PrimaryDetailMotif && first.SecondaryDetailMotif == second.SecondaryDetailMotif &&
            masksEqual(first.ReservedNegativeSpaceMask, second.ReservedNegativeSpaceMask) &&
            masksEqual(first.CoreRaisedMask, second.CoreRaisedMask) && masksEqual(first.HullLayerMask, second.HullLayerMask) &&
            masksEqual(first.WeaponOccupiedMask, second.WeaponOccupiedMask) &&
            first.WeaponUnits.size() == second.WeaponUnits.size() && first.EngineUnits.size() == second.EngineUnits.size();
    }

    ShipGenerationConfiguration makeExplicitConfiguration(const ShipGenerationSettings& settings)
    {
        ShipGenerationConfiguration configuration;
        configuration.Seed = settings.Seed;
        configuration.Dimensions = settings.Dimensions;
        configuration.Faction = settings.Faction;
        configuration.DetailDensity = settings.DetailDensity;
        configuration.AsymmetricDetailChance = settings.AsymmetricDetailChance;
        configuration.AttachmentsEnabled = settings.AttachmentsEnabled;
        configuration.SeedOverrides = settings.SeedOverrides;
        configuration.DomainSeedOverrides = settings.DomainSeedOverrides;
        configuration.RandomStreamMode = settings.RandomStreamMode;
        return configuration;
    }

    ShipGenerationSettings makePresetSettings(ShipStyle style, ShipDimensions dimensions, uint64_t seed)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = ShipFactionType::MILITARY;
        settings.DetailDensity = 63u;
        settings.AsymmetricDetailChance = 17u;
        settings.AttachmentsEnabled = true;
        settings.SeedOverrides.Palette = seed ^ 0xA5A5A5A5A5A5A5A5ull;
        settings.DomainSeedOverrides.set(GenerationDomain::DETAILS, seed ^ 0xC6BC279692B5CC83ull);
        settings.RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        return settings;
    }

    bool checkValidationContracts()
    {
        bool success = true;

        const ShipGenerationProfile defaultProfile;
        if (!validateShipGenerationProfile(defaultProfile).isValid())
        {
            std::cerr << "Default-constructed ShipGenerationProfile is not a valid public baseline.\n";
            success = false;
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END); ++index)
        {
            if (!validateShipGenerationProfile(getShipGenerationProfile(static_cast<ShipStyle>(index))).isValid())
            {
                std::cerr << "Built-in profile failed public validation for style index " << index << ".\n";
                success = false;
            }
        }

        ShipGenerationProfile invalidRange = defaultProfile;
        invalidRange.NoseEndPercent = { 30u, 20u };
        if (validateShipGenerationProfile(invalidRange).isValid())
        {
            std::cerr << "Reversed UIntRange was accepted.\n";
            success = false;
        }

        ShipGenerationProfile invalidProbability = defaultProfile;
        invalidProbability.LargeWeaponChance = 101u;
        if (validateShipGenerationProfile(invalidProbability).isValid())
        {
            std::cerr << "Out-of-range probability was accepted.\n";
            success = false;
        }

        ShipGenerationProfile invalidMandatoryWeights = defaultProfile;
        invalidMandatoryWeights.CentralEngineWeight = 0u;
        invalidMandatoryWeights.TwinEngineWeight = 0u;
        invalidMandatoryWeights.QuadEngineWeight = 0u;
        invalidMandatoryWeights.CentralAuxiliaryEngineWeight = 0u;
        invalidMandatoryWeights.EngineBankWeight = 0u;
        if (validateShipGenerationProfile(invalidMandatoryWeights).isValid())
        {
            std::cerr << "Zero-total mandatory engine-layout weights were accepted.\n";
            success = false;
        }

        ShipGenerationProfile dormantWeapons = defaultProfile;
        dormantWeapons.LargeWeaponChance = 0u;
        dormantWeapons.MaximumLargeWeaponGroups = 0u;
        dormantWeapons.LargeWeaponWeights = {};
        dormantWeapons.LargeWeaponHardpointWeights = {};
        dormantWeapons.LargeWeaponScalePercent = 240u;
        if (!validateShipGenerationProfile(dormantWeapons).isValid())
        {
            std::cerr << "Dormant all-zero weapon weights or an extreme valid scale multiplier were rejected.\n";
            success = false;
        }

        ShipGenerationProfile extremeValid = defaultProfile;
        extremeValid.LargeWeaponScalePercent = 180u;
        extremeValid.BroadWingWeight = 100000u;
        extremeValid.DetailDensityPercent = 5u;
        if (!validateShipGenerationProfile(extremeValid).isValid())
        {
            std::cerr << "Extreme but mechanically valid profile was rejected.\n";
            success = false;
        }

        ShipGenerator generator;
        ShipGenerationConfiguration configuration;
        configuration.Seed = 0x81A0B0C0D0E0F001ull;
        configuration.Dimensions = { 64u, 64u };
        bool threw = false;
        try { (void)generator.generate(configuration, invalidProbability); }
        catch (const std::invalid_argument&) { threw = true; }
        if (!threw)
        {
            std::cerr << "Generation did not fail safely for an invalid explicit profile.\n";
            success = false;
        }

        return success;
    }

    bool checkBuiltInEquivalence()
    {
        constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
        constexpr std::array<ShipDimensions, 6u> Dimensions = { ShipDimensions{ 64u, 64u }, ShipDimensions{ 96u, 64u }, ShipDimensions{ 64u, 96u }, ShipDimensions{ 44u, 44u }, ShipDimensions{ 96u, 96u }, ShipDimensions{ 128u, 96u } };
        ShipGenerator generator;
        ShipIdleAnimator idleAnimator;

        for (std::size_t index = 0u; index < Styles.size(); ++index)
        {
            const ShipStyle style = Styles[index];
            const ShipGenerationSettings presetSettings = makePresetSettings(style, Dimensions[index], 0xB17B17B170000000ull + index * 0x10001ull);
            const ShipGenerationConfiguration explicitConfiguration = makeExplicitConfiguration(presetSettings);
            const ShipGenerationProfile& profile = getShipGenerationProfile(style);
            ShipGenerationDebugInfo presetDebug;
            ShipGenerationDebugInfo explicitDebug;

            const GeneratedShip presetShip = generator.generate(presetSettings, &presetDebug);
            const GeneratedShip explicitShip = generator.generate(explicitConfiguration, profile, &explicitDebug);

            if (presetShip.Style != style || explicitShip.Style != ShipStyle::SHIP_STYLE_END)
            {
                std::cerr << "Structural provenance is not truthful for style index " << index << ".\n";
                return false;
            }
            if (!generatedBehaviorEqual(presetShip, explicitShip) || !debugBehaviorEqual(presetDebug, explicitDebug))
            {
                std::cerr << "Built-in convenience and explicit-profile generation diverged for style index " << index << ".\n";
                return false;
            }

            ShipIdleAnimationSettings animationSettings;
            animationSettings.Seed = 0xA4093822299F31D0ull + index;
            const Image presetFrame = idleAnimator.evaluateFrameAtNormalizedTime(presetShip, 0.375, animationSettings);
            const Image explicitFrame = idleAnimator.evaluateFrameAtNormalizedTime(explicitShip, 0.375, animationSettings);
            if (presetFrame.getPixels() != explicitFrame.getPixels())
            {
                std::cerr << "Built-in and explicit-profile animation diverged for style index " << index << ".\n";
                return false;
            }
        }

        return true;
    }

    bool checkCalibrationEquivalence()
    {
        constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
        GenerationTuningProfile tuning = createDefaultGenerationTuningProfile();
        ShipGenerator generator;

        for (std::size_t index = 0u; index < Styles.size(); ++index)
        {
            const ShipStyle style = Styles[index];
            const uint32_t oldWeight = getGenerationTuningWeight(tuning, style, GenerationWeightGroup::ENGINE_LAYOUT, 0u);
            setGenerationTuningWeight(tuning, style, GenerationWeightGroup::ENGINE_LAYOUT, 0u, oldWeight + 7u);

            const ShipGenerationSettings presetSettings = makePresetSettings(style, { 72u, 64u }, 0xCA11B2A710000000ull + index);
            const ShipGenerationConfiguration explicitConfiguration = makeExplicitConfiguration(presetSettings);

            GenerationCalibrationSettings presetCalibration;
            presetCalibration.TuningProfile = &tuning;
            presetCalibration.IsolatedGroup = GenerationWeightGroup::ENGINE_LAYOUT;
            presetCalibration.IsolationSalt = 0x9E3779B97F4A7C15ull + index;

            GenerationCalibrationSettings explicitCalibration = presetCalibration;
            explicitCalibration.TuningProfile = nullptr;
            explicitCalibration.ExplicitTuningProfile = &tuning.Styles[static_cast<std::size_t>(style)];

            const GeneratedShip presetShip = generator.generateCalibrated(presetSettings, presetCalibration);
            const GeneratedShip explicitShip = generator.generateCalibrated(explicitConfiguration, getShipGenerationProfile(style), explicitCalibration);
            if (!generatedBehaviorEqual(presetShip, explicitShip))
            {
                std::cerr << "Calibrated built-in and explicit-profile paths diverged for style index " << index << ".\n";
                return false;
            }
        }

        ShipGenerationConfiguration configuration;
        configuration.Seed = 0xCCAA551122334455ull;
        configuration.Dimensions = { 64u, 64u };
        GenerationCalibrationSettings invalidCalibration;
        invalidCalibration.TuningProfile = &tuning;
        bool threw = false;
        try { (void)generator.generateCalibrated(configuration, ShipGenerationProfile{}, invalidCalibration); }
        catch (const std::invalid_argument&) { threw = true; }
        if (!threw)
        {
            std::cerr << "Custom calibration silently accepted a style-indexed tuning catalog.\n";
            return false;
        }

        return true;
    }

    bool checkCustomProfiles()
    {
        ShipGenerator generator;
        ShipIdleAnimator idleAnimator;
        ShipLateralMovementAnimator lateralAnimator;
        ShipLongitudinalMovementAnimator longitudinalAnimator;
        ShipFiringAnimator firingAnimator;
        ShipAnimationStateCoordinator coordinator;

        struct CustomCase
        {
            ShipGenerationProfile Profile;
            ShipDimensions Dimensions;
            uint64_t Seed;
        };

        std::array<CustomCase, 4u> cases;

        cases[0].Profile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        cases[0].Profile.LargeWeaponChance = 85u;
        cases[0].Profile.MaximumLargeWeaponGroups = 2u;
        cases[0].Profile.LargeWeaponScalePercent = 140u;
        cases[0].Dimensions = { 96u, 64u };
        cases[0].Seed = 0xC001000000000001ull;

        cases[1].Profile = getShipGenerationProfile(ShipStyle::FIGHTER);
        cases[1].Profile.NoWingWeight = 100u;
        cases[1].Profile.SmallWingWeight = 0u;
        cases[1].Profile.SweptWingWeight = 0u;
        cases[1].Profile.BroadWingWeight = 0u;
        cases[1].Profile.DetailDensityPercent = 45u;
        cases[1].Dimensions = { 64u, 96u };
        cases[1].Seed = 0xC001000000000002ull;

        cases[2].Profile = getShipGenerationProfile(ShipStyle::DELTA);
        cases[2].Profile.NoWingWeight = 5u;
        cases[2].Profile.SmallWingWeight = 5u;
        cases[2].Profile.SweptWingWeight = 20u;
        cases[2].Profile.BroadWingWeight = 250u;
        cases[2].Dimensions = { 128u, 96u };
        cases[2].Seed = 0xC001000000000003ull;

        cases[3].Profile = getShipGenerationProfile(ShipStyle::SLEEK);
        cases[3].Profile.DetailDensityPercent = 20u;
        cases[3].Profile.DetailMotifChance = 0u;
        cases[3].Profile.LiveryChance = 0u;
        cases[3].Profile.AttachmentChance = 0u;
        cases[3].Dimensions = { 48u, 64u };
        cases[3].Seed = 0xC001000000000004ull;

        for (std::size_t index = 0u; index < cases.size(); ++index)
        {
            const auto validation = validateShipGenerationProfile(cases[index].Profile);
            if (!validation.isValid())
            {
                std::cerr << "Custom profile " << index << " failed validation.\n";
                return false;
            }

            ShipGenerationConfiguration configuration;
            configuration.Seed = cases[index].Seed;
            configuration.Dimensions = cases[index].Dimensions;
            configuration.Faction = ShipFactionType::FRONTIER;
            configuration.DetailDensity = 57u;
            configuration.AsymmetricDetailChance = 13u;
            configuration.AttachmentsEnabled = true;

            const GeneratedShip first = generator.generate(configuration, cases[index].Profile);
            const GeneratedShip second = generator.generate(configuration, cases[index].Profile);
            if (first.Style != ShipStyle::SHIP_STYLE_END || second.Style != ShipStyle::SHIP_STYLE_END || !generatedBehaviorEqual(first, second))
            {
                std::cerr << "Custom profile " << index << " was not deterministic or acquired fake preset provenance.\n";
                return false;
            }

            const Image idleFirst = idleAnimator.evaluateFrameAtNormalizedTime(first, 0.42);
            const Image idleSecond = idleAnimator.evaluateFrameAtNormalizedTime(second, 0.42);
            const Image lateralFirst = lateralAnimator.evaluateFrameAtNormalizedTime(first, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::SUSTAIN, 0.55);
            const Image lateralSecond = lateralAnimator.evaluateFrameAtNormalizedTime(second, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::SUSTAIN, 0.55);
            const Image longitudinalFirst = longitudinalAnimator.evaluateFrameAtNormalizedTime(first, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.55);
            const Image longitudinalSecond = longitudinalAnimator.evaluateFrameAtNormalizedTime(second, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.55);
            if (idleFirst.getPixels() != idleSecond.getPixels() || lateralFirst.getPixels() != lateralSecond.getPixels() || longitudinalFirst.getPixels() != longitudinalSecond.getPixels())
            {
                std::cerr << "Custom profile " << index << " animation was not deterministic.\n";
                return false;
            }

            const auto firingTargets = firingAnimator.getAvailableTargets(first);
            if (!firingTargets.empty())
            {
                const Image firingFrame = firingAnimator.evaluateFrameAtNormalizedTime(first, firingTargets.front(), 0.5);
                if (firingFrame.getWidth() != cases[index].Dimensions.Width || firingFrame.getHeight() != cases[index].Dimensions.Height)
                {
                    std::cerr << "Custom profile " << index << " firing animation dimensions are invalid.\n";
                    return false;
                }

                ShipAnimationStateRequest request;
                request.UnderlyingMovementType = ShipAnimationType::MOVE_RIGHT;
                request.MovementPhase = ShipMovementAnimationPhase::SUSTAIN;
                request.MovementNormalizedTime = 0.45;
                request.FireActive = true;
                request.FiringTarget = firingTargets.front();
                request.FiringNormalizedTime = 0.5;
                const ShipAnimationStateEvaluation composed = coordinator.evaluate(first, request);
                if (composed.Pose.Frame.getWidth() != cases[index].Dimensions.Width || composed.Pose.Frame.getHeight() != cases[index].Dimensions.Height)
                {
                    std::cerr << "Custom profile " << index << " composed movement+firing animation dimensions are invalid.\n";
                    return false;
                }
            }
        }

        // Copy-before-edit must not mutate the canonical built-in preset.
        const uint32_t canonicalIndustrialChance = getShipGenerationProfile(ShipStyle::INDUSTRIAL).LargeWeaponChance;
        ShipGenerationProfile copy = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        copy.LargeWeaponChance = canonicalIndustrialChance == 100u ? 99u : canonicalIndustrialChance + 1u;
        if (getShipGenerationProfile(ShipStyle::INDUSTRIAL).LargeWeaponChance != canonicalIndustrialChance)
        {
            std::cerr << "Mutating a copied built-in profile changed the canonical preset.\n";
            return false;
        }

        return true;
    }

    bool checkCustomDomainOwnership()
    {
        ShipGenerator generator;
        ShipGenerationProfile profile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        profile.LargeWeaponScalePercent = 135u;

        ShipGenerationConfiguration base;
        base.Seed = 0xD0A1A2A3A4A5A6A7ull;
        base.Dimensions = { 96u, 64u };
        base.Faction = ShipFactionType::CORPORATE;
        const GeneratedShip original = generator.generate(base, profile);

        ShipGenerationConfiguration paletteReroll = base;
        const uint64_t newPaletteSeed = deriveGenerationDomainRerollSeed(0x123456789ABCDEF0ull, GenerationDomain::PALETTE, original.DomainSeeds.get(GenerationDomain::PALETTE));
        paletteReroll.DomainSeedOverrides.set(GenerationDomain::PALETTE, newPaletteSeed);
        const GeneratedShip recolored = generator.generate(paletteReroll, profile);

        if (recolored.DomainSeeds.get(GenerationDomain::PALETTE) == original.DomainSeeds.get(GenerationDomain::PALETTE) ||
            !masksEqual(original.HullMask, recolored.HullMask) || !masksEqual(original.CockpitMask, recolored.CockpitMask) ||
            !masksEqual(original.EngineMask, recolored.EngineMask) || !masksEqual(original.AttachmentMask, recolored.AttachmentMask) ||
            !masksEqual(original.IdleAnimationMetadata.WeaponOccupiedMask, recolored.IdleAnimationMetadata.WeaponOccupiedMask))
        {
            std::cerr << "Explicit profile altered Palette-domain reroll ownership.\n";
            return false;
        }

        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            if (domain == GenerationDomain::PALETTE) { continue; }
            if (original.DomainSeeds.Values[index] != recolored.DomainSeeds.Values[index])
            {
                std::cerr << "Palette reroll changed an unrelated domain seed under explicit-profile generation.\n";
                return false;
            }
        }

        return true;
    }
}

int PixelShipGeneratorTests::runCustomProfileApiRegression()
{
    bool success = true;
    success = checkValidationContracts() && success;
    success = checkBuiltInEquivalence() && success;
    success = checkCalibrationEquivalence() && success;
    success = checkCustomProfiles() && success;
    success = checkCustomDomainOwnership() && success;

    if (success)
    {
        std::cout << "Custom ShipGenerationProfile API regression passed.\n";
        return 0;
    }

    return 1;
}
