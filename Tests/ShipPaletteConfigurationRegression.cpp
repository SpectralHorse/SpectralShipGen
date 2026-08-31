#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <SpectralShipGen/GenerationDomain.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipPaletteGenerationProfile.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>

namespace
{
    using namespace SpectralShipGen;

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

    bool palettesEqual(const ShipPalette& a, const ShipPalette& b)
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

    bool imagesEqual(const Image& a, const Image& b)
    {
        return a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight() && a.getPixels() == b.getPixels();
    }

    bool geometryEqual(const GeneratedShip& a, const GeneratedShip& b, const ShipGenerationDebugInfo& debugA, const ShipGenerationDebugInfo& debugB)
    {
        return masksEqual(a.HullMask, b.HullMask) && masksEqual(a.CockpitMask, b.CockpitMask) &&
            masksEqual(a.EngineMask, b.EngineMask) && masksEqual(a.EngineExhaustMask, b.EngineExhaustMask) &&
            masksEqual(a.AttachmentMask, b.AttachmentMask) && masksEqual(a.AccentMask, b.AccentMask) &&
            masksEqual(a.MechanicalDetailMask, b.MechanicalDetailMask) && masksEqual(a.LightMask, b.LightMask) &&
            masksEqual(a.IdleAnimationMetadata.WeaponOccupiedMask, b.IdleAnimationMetadata.WeaponOccupiedMask) &&
            masksEqual(a.IdleAnimationMetadata.WeaponMovableMask, b.IdleAnimationMetadata.WeaponMovableMask) &&
            a.AttachmentPlacements.size() == b.AttachmentPlacements.size() &&
            a.IdleAnimationMetadata.EngineComponents.size() == b.IdleAnimationMetadata.EngineComponents.size() &&
            a.IdleAnimationMetadata.WeaponComponents.size() == b.IdleAnimationMetadata.WeaponComponents.size() &&
            masksEqual(debugA.MaterialSecondaryHullMask, debugB.MaterialSecondaryHullMask) &&
            masksEqual(debugA.MaterialMechanicalMask, debugB.MaterialMechanicalMask) &&
            masksEqual(debugA.LiveryPrimaryMask, debugB.LiveryPrimaryMask) &&
            masksEqual(debugA.LiverySecondaryMask, debugB.LiverySecondaryMask) &&
            masksEqual(debugA.ReservedNegativeSpaceMask, debugB.ReservedNegativeSpaceMask) &&
            masksEqual(debugA.HullLayerMask, debugB.HullLayerMask) && masksEqual(debugA.CoreRegionMask, debugB.CoreRegionMask) &&
            masksEqual(debugA.WeaponOccupiedMask, debugB.WeaponOccupiedMask) &&
            debugA.MaterialZoneTypeCounts == debugB.MaterialZoneTypeCounts && debugA.LiveryTypeCounts == debugB.LiveryTypeCounts &&
            debugA.WeaponTypeCounts == debugB.WeaponTypeCounts && debugA.EngineLayout == debugB.EngineLayout && debugA.WingShape == debugB.WingShape;
    }

    ExplicitShipGenerationConfiguration makeConfiguration(uint64_t seed, ShipDimensions dimensions)
    {
        ExplicitShipGenerationConfiguration configuration;
        configuration.Seed = seed;
        configuration.Dimensions = dimensions;
        configuration.DetailDensity = 61u;
        configuration.AsymmetricDetailChance = 19u;
        configuration.AttachmentsEnabled = true;
        return configuration;
    }

    ShipPalette makeFixedPalette()
    {
        ShipPalette p;
        p.Transparent = Color(1u, 2u, 3u, 0u);
        p.Outline = Color(4u, 5u, 6u, 255u);
        p.HullDeepShadow = Color(7u, 8u, 9u, 255u);
        p.HullShadow = Color(10u, 11u, 12u, 255u);
        p.HullBase = Color(13u, 14u, 15u, 255u);
        p.HullHighlight = Color(16u, 17u, 18u, 255u);
        p.HullSecondary = Color(19u, 20u, 21u, 255u);
        p.HullEdgeHighlight = Color(22u, 23u, 24u, 255u);
        p.CockpitDark = Color(25u, 26u, 27u, 255u);
        p.CockpitBase = Color(28u, 29u, 30u, 255u);
        p.CockpitHighlight = Color(31u, 32u, 33u, 255u);
        p.CockpitGlint = Color(34u, 35u, 36u, 255u);
        p.EngineDark = Color(37u, 38u, 39u, 255u);
        p.EngineBase = Color(40u, 41u, 42u, 255u);
        p.EngineHighlight = Color(43u, 44u, 45u, 255u);
        p.EngineHotCore = Color(46u, 47u, 48u, 255u);
        p.ExhaustBase = Color(49u, 50u, 51u, 255u);
        p.ExhaustHighlight = Color(52u, 53u, 54u, 255u);
        p.ExhaustHotCore = Color(55u, 56u, 57u, 255u);
        p.HullAccentDark = Color(58u, 59u, 60u, 255u);
        p.HullAccent = Color(61u, 62u, 63u, 255u);
        p.HullAccentHighlight = Color(64u, 65u, 66u, 255u);
        p.MechanicalDark = Color(67u, 68u, 69u, 255u);
        p.MechanicalBase = Color(70u, 71u, 72u, 255u);
        p.LightBase = Color(73u, 74u, 75u, 255u);
        p.LightHighlight = Color(76u, 77u, 78u, 255u);
        return p;
    }

    bool checkBuiltInGeneratedEquivalence()
    {
        ShipGenerator generator;
        const std::array<ShipFactionType, 6u> factions = {
            ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT,
            ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC
        };
        const std::array<ShipStyle, 2u> styles = { ShipStyle::FIGHTER, ShipStyle::DELTA };

        for (std::size_t factionIndex = 0u; factionIndex < factions.size(); ++factionIndex)
        {
            const ShipFactionType faction = factions[factionIndex];
            const ShipStyle style = styles[factionIndex % styles.size()];
            const ShipDimensions dimensions = factionIndex % 2u == 0u ? ShipDimensions{ 64u, 64u } : ShipDimensions{ 96u, 64u };
            const uint64_t seed = 0x8600000000000000ull + static_cast<uint64_t>(factionIndex) * 0x1001ull;

            ShipGenerationSettings preset;
            preset.Seed = seed;
            preset.Dimensions = dimensions;
            preset.Style = style;
            preset.Faction = faction;
            preset.DetailDensity = 61u;
            preset.AsymmetricDetailChance = 19u;

            ShipGenerationDebugInfo presetDebug;
            const GeneratedShip presetShip = generator.generate(preset, &presetDebug);

            ExplicitShipGenerationConfiguration explicitConfiguration = makeConfiguration(seed, dimensions);
            explicitConfiguration.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
            explicitConfiguration.PaletteConfiguration.Generated = getShipPaletteGenerationProfile(faction);

            ShipGenerationDebugInfo explicitDebug;
            const GeneratedShip explicitShip = generator.generate(explicitConfiguration, getShipGenerationProfile(style), getShipFactionProfile(faction), &explicitDebug);

            if (!palettesEqual(presetShip.Palette, explicitShip.Palette) || !imagesEqual(presetShip.FinalImage, explicitShip.FinalImage) ||
                !geometryEqual(presetShip, explicitShip, presetDebug, explicitDebug))
            {
                std::cerr << "Built-in faction palette path differed from explicit generated palette path for faction index " << factionIndex << ".\n";
                return false;
            }
        }
        return true;
    }

    bool checkGeneratedPaletteRerollAndCustomCombination()
    {
        ShipGenerator generator;
        ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        structuralProfile.LargeWeaponChance = 83u;
        ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::ASCENDANT);
        factionProfile.SurfaceDetails.DetailDensityPercent = 72u;

        ShipPaletteGenerationProfile paletteProfile = getShipPaletteGenerationProfile(ShipFactionType::CORPORATE);
        paletteProfile.Ranges.HullHue = { 15u, 330u };
        paletteProfile.Ranges.HullSaturation = { 45u, 90u };
        paletteProfile.Ranges.HullValue = { 35u, 82u };
        paletteProfile.Ranges.Accent.HueOffset = { -180, 180 };
        paletteProfile.Behavior.MinimumAccentHueDistance = 80u;
        paletteProfile.Behavior.AccentHueSeparationShiftA = 120;
        paletteProfile.Behavior.AccentHueSeparationShiftB = -120;

        if (!validateShipPaletteGenerationProfile(paletteProfile).isValid())
        {
            std::cerr << "Custom generated palette profile unexpectedly failed validation.\n";
            return false;
        }

        ExplicitShipGenerationConfiguration configuration = makeConfiguration(0x86ABCDEF12345678ull, { 96u, 64u });
        configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        configuration.PaletteConfiguration.Generated = paletteProfile;
        configuration.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x1111222233334444ull);

        ShipGenerationDebugInfo firstDebug;
        const GeneratedShip first = generator.generate(configuration, structuralProfile, factionProfile, &firstDebug);
        ShipGenerationDebugInfo repeatedDebug;
        const GeneratedShip repeated = generator.generate(configuration, structuralProfile, factionProfile, &repeatedDebug);
        if (!palettesEqual(first.Palette, repeated.Palette) || !imagesEqual(first.FinalImage, repeated.FinalImage) ||
            !geometryEqual(first, repeated, firstDebug, repeatedDebug))
        {
            std::cerr << "Custom generated palette path is not deterministic.\n";
            return false;
        }

        ExplicitShipGenerationConfiguration rerolledConfiguration = configuration;
        rerolledConfiguration.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x9999AAAABBBBCCCCull);
        ShipGenerationDebugInfo rerolledDebug;
        const GeneratedShip rerolled = generator.generate(rerolledConfiguration, structuralProfile, factionProfile, &rerolledDebug);
        if (!geometryEqual(first, rerolled, firstDebug, rerolledDebug))
        {
            std::cerr << "Generated palette-domain reroll changed geometry/material/livery masks.\n";
            return false;
        }
        if (palettesEqual(first.Palette, rerolled.Palette) || imagesEqual(first.FinalImage, rerolled.FinalImage))
        {
            std::cerr << "Generated palette-domain reroll did not recolor the ship.\n";
            return false;
        }
        if (first.Provenance.StructuralPreset.has_value() || first.Provenance.FactionPreset.has_value())
        {
            std::cerr << "Explicit structural/faction/palette generation fabricated built-in provenance.\n";
            return false;
        }

        ShipIdleAnimator animator;
        const ShipIdleAnimation animation = animator.generate(first);
        if (animation.Frames.empty() || !imagesEqual(animation.Frames.front(), first.FinalImage))
        {
            std::cerr << "Custom generated palette ship failed exact IDLE frame-0 semantics.\n";
            return false;
        }
        return true;
    }

    bool checkFixedPaletteMode()
    {
        ShipGenerator generator;
        ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::DELTA);
        structuralProfile.DetailDensityPercent = 73u;
        ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::XENO);
        factionProfile.SurfaceDetails.DetailDensityPercent = 82u;
        const ShipPalette fixedPalette = makeFixedPalette();

        ExplicitShipGenerationConfiguration configuration = makeConfiguration(0x8600F1E0D2C3B4A5ull, { 64u, 96u });
        configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
        configuration.PaletteConfiguration.Fixed = fixedPalette;
        // Dormant generated data is deliberately invalid; FIXED mode must not infer
        // or validate/use it as a hidden palette source.
        configuration.PaletteConfiguration.Generated.Ranges.HullHue = { 300u, 10u };
        configuration.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x1010101010101010ull);

        ShipGenerationDebugInfo firstDebug;
        const GeneratedShip first = generator.generate(configuration, structuralProfile, factionProfile, &firstDebug);
        if (!palettesEqual(first.Palette, fixedPalette))
        {
            std::cerr << "Fixed palette was not preserved exactly on GeneratedShip.\n";
            return false;
        }
        if (first.Provenance.StructuralPreset.has_value() || first.Provenance.FactionPreset.has_value() || first.PaletteSourceMode != ShipPaletteSourceMode::FIXED)
        {
            std::cerr << "Fixed custom generation fabricated built-in provenance or lost palette-source provenance.\n";
            return false;
        }

        ExplicitShipGenerationConfiguration rerolledConfiguration = configuration;
        rerolledConfiguration.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0xEFEFEFEFEFEFEFEFull);
        ShipGenerationDebugInfo rerolledDebug;
        const GeneratedShip rerolled = generator.generate(rerolledConfiguration, structuralProfile, factionProfile, &rerolledDebug);
        const bool fixedPaletteEqual = palettesEqual(rerolled.Palette, fixedPalette);
        const bool fixedImageEqual = imagesEqual(first.FinalImage, rerolled.FinalImage);
        const bool fixedGeometryEqual = geometryEqual(first, rerolled, firstDebug, rerolledDebug);
        if (!fixedPaletteEqual || !fixedImageEqual || !fixedGeometryEqual)
        {
            std::cerr << "Palette-domain reroll changed a fixed-palette ship (palette=" << fixedPaletteEqual
                << ", image=" << fixedImageEqual << ", geometry=" << fixedGeometryEqual << ").\n";
            return false;
        }

        ShipIdleAnimator animator;
        const ShipIdleAnimation animation = animator.generate(first);
        if (animation.Frames.empty() || !imagesEqual(animation.Frames.front(), first.FinalImage))
        {
            std::cerr << "Fixed-palette ship failed exact IDLE frame-0 semantics.\n";
            return false;
        }
        return true;
    }

    bool checkValidation()
    {
        ShipPaletteGenerationProfile valid = getShipPaletteGenerationProfile(ShipFactionType::FRONTIER);
        if (!validateShipPaletteGenerationProfile(valid).isValid())
        {
            std::cerr << "Built-in palette generation profile failed validation.\n";
            return false;
        }

        ShipPaletteGenerationProfile invalid = valid;
        invalid.Ranges.HullSaturation = { 70u, 40u };
        invalid.Ranges.Light.Value.Max = 101u;
        invalid.Behavior.MinimumAccentHueDistance = 181u;
        if (validateShipPaletteGenerationProfile(invalid).isValid())
        {
            std::cerr << "Invalid generated palette ranges/relationships were accepted.\n";
            return false;
        }

        ShipGenerator generator;
        ExplicitShipGenerationConfiguration configuration = makeConfiguration(0x86123456789ABCDEull, { 64u, 64u });
        configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        configuration.PaletteConfiguration.Generated = invalid;
        try
        {
            (void)generator.generate(configuration, getShipGenerationProfile(ShipStyle::FIGHTER), getShipFactionProfile(ShipFactionType::FRONTIER));
            std::cerr << "Generation accepted an invalid explicit palette-generation profile.\n";
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }

        configuration.PaletteConfiguration.Mode = static_cast<ShipPaletteSourceMode>(999u);
        try
        {
            (void)generator.generate(configuration, getShipGenerationProfile(ShipStyle::FIGHTER), getShipFactionProfile(ShipFactionType::FRONTIER));
            std::cerr << "Generation accepted an invalid palette source mode.\n";
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        return true;
    }
}

int SpectralShipGenTests::runPaletteConfigurationRegression()
{
    bool success = true;
    success = checkBuiltInGeneratedEquivalence() && success;
    success = checkGeneratedPaletteRerollAndCustomCombination() && success;
    success = checkFixedPaletteMode() && success;
    success = checkValidation() && success;

    if (!success)
    {
        return 1;
    }

    std::cout << "Palette configuration regression passed.\n";
    return 0;
}
