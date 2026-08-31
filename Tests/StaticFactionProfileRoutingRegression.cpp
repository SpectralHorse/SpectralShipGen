#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace SpectralShipGen
{
    struct ShipGeneratorStaticFactionRegressionAccess
    {
        static GeneratedShip generate(ShipGenerator& generator,
            const ShipGenerationConfiguration& configuration,
            const ShipGenerationProfile& structuralProfile,
            const ShipFactionProfile& factionProfile,
            ShipStyle styleProvenance,
            ShipGenerationDebugInfo* debugInfo)
        {
            ExplicitShipGenerationConfiguration explicitConfiguration;
            explicitConfiguration.Seed = configuration.Seed;
            explicitConfiguration.Dimensions = configuration.Dimensions;
            explicitConfiguration.DetailDensity = configuration.DetailDensity;
            explicitConfiguration.AsymmetricDetailChance = configuration.AsymmetricDetailChance;
            explicitConfiguration.AttachmentsEnabled = configuration.AttachmentsEnabled;
            explicitConfiguration.SeedOverrides = configuration.SeedOverrides;
            explicitConfiguration.DomainSeedOverrides = configuration.DomainSeedOverrides;
            explicitConfiguration.RandomStreamMode = configuration.RandomStreamMode;
            return generator.generateInternal(explicitConfiguration, structuralProfile, factionProfile, styleProvenance, configuration.Faction, nullptr, debugInfo, nullptr);
        }
    };
}

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

    bool attachmentPlacementsEqual(const std::vector<ShipAttachmentPlacement>& first, const std::vector<ShipAttachmentPlacement>& second)
    {
        if (first.size() != second.size()) { return false; }
        for (std::size_t index = 0u; index < first.size(); ++index)
        {
            const ShipAttachmentPlacement& a = first[index];
            const ShipAttachmentPlacement& b = second[index];
            if (a.Type != b.Type || a.Region != b.Region || a.Direction != b.Direction ||
                a.AnchorX != b.AnchorX || a.AnchorY != b.AnchorY || a.MinimumX != b.MinimumX ||
                a.MaximumX != b.MaximumX || a.MinimumY != b.MinimumY || a.MaximumY != b.MaximumY ||
                a.SymmetryGroup != b.SymmetryGroup)
            {
                return false;
            }
        }
        return true;
    }

    bool staticShipEqual(const GeneratedShip& first, const GeneratedShip& second)
    {
        return first.Seed == second.Seed && first.Seeds == second.Seeds && first.DomainSeeds.Values == second.DomainSeeds.Values &&
            first.Palette.Transparent == second.Palette.Transparent && first.Palette.Outline == second.Palette.Outline &&
            first.Palette.HullDeepShadow == second.Palette.HullDeepShadow && first.Palette.HullShadow == second.Palette.HullShadow &&
            first.Palette.HullBase == second.Palette.HullBase && first.Palette.HullHighlight == second.Palette.HullHighlight &&
            first.Palette.HullSecondary == second.Palette.HullSecondary && first.Palette.HullEdgeHighlight == second.Palette.HullEdgeHighlight &&
            first.Palette.CockpitDark == second.Palette.CockpitDark && first.Palette.CockpitBase == second.Palette.CockpitBase &&
            first.Palette.CockpitHighlight == second.Palette.CockpitHighlight && first.Palette.CockpitGlint == second.Palette.CockpitGlint &&
            first.Palette.EngineDark == second.Palette.EngineDark && first.Palette.EngineBase == second.Palette.EngineBase &&
            first.Palette.EngineHighlight == second.Palette.EngineHighlight && first.Palette.EngineHotCore == second.Palette.EngineHotCore &&
            first.Palette.ExhaustBase == second.Palette.ExhaustBase && first.Palette.ExhaustHighlight == second.Palette.ExhaustHighlight &&
            first.Palette.ExhaustHotCore == second.Palette.ExhaustHotCore && first.Palette.HullAccentDark == second.Palette.HullAccentDark &&
            first.Palette.HullAccent == second.Palette.HullAccent && first.Palette.HullAccentHighlight == second.Palette.HullAccentHighlight &&
            first.Palette.MechanicalDark == second.Palette.MechanicalDark && first.Palette.MechanicalBase == second.Palette.MechanicalBase &&
            first.Palette.LightBase == second.Palette.LightBase && first.Palette.LightHighlight == second.Palette.LightHighlight &&
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

    bool staticDebugEqual(const ShipGenerationDebugInfo& first, const ShipGenerationDebugInfo& second)
    {
        return first.ComplexityInitialBudget == second.ComplexityInitialBudget &&
            first.ComplexityConsumedBudget == second.ComplexityConsumedBudget &&
            first.ComplexityUnusedBudget == second.ComplexityUnusedBudget &&
            first.ComplexityCategoryAllocations == second.ComplexityCategoryAllocations &&
            first.ComplexityCategoryConsumed == second.ComplexityCategoryConsumed &&
            first.PrimaryVisualAnchor == second.PrimaryVisualAnchor && first.SecondaryVisualAnchor == second.SecondaryVisualAnchor &&
            first.VisualAnchorTargetRegion == second.VisualAnchorTargetRegion &&
            first.MacroAsymmetryPlanned == second.MacroAsymmetryPlanned && first.MacroAsymmetryFulfilled == second.MacroAsymmetryFulfilled &&
            first.StructuralNegativeSpaceTypeCounts == second.StructuralNegativeSpaceTypeCounts &&
            first.MaterialZoneTypeCounts == second.MaterialZoneTypeCounts && first.LiveryTypeCounts == second.LiveryTypeCounts &&
            first.CoreTreatmentTypeCounts == second.CoreTreatmentTypeCounts && first.HullLayerTypeCounts == second.HullLayerTypeCounts &&
            first.MajorFeatureTypeCounts == second.MajorFeatureTypeCounts && first.WeaponTypeCounts == second.WeaponTypeCounts &&
            first.EngineLayout == second.EngineLayout && first.EngineCount == second.EngineCount &&
            first.CockpitSize == second.CockpitSize && first.CockpitShape == second.CockpitShape &&
            first.AttachmentPlacedGroupCount == second.AttachmentPlacedGroupCount &&
            first.AccentPatternCount == second.AccentPatternCount && first.MechanicalPatternCount == second.MechanicalPatternCount &&
            first.LightPatternCount == second.LightPatternCount &&
            masksEqual(first.ReservedNegativeSpaceMask, second.ReservedNegativeSpaceMask) &&
            masksEqual(first.MaterialSecondaryHullMask, second.MaterialSecondaryHullMask) &&
            masksEqual(first.MaterialMechanicalMask, second.MaterialMechanicalMask) &&
            masksEqual(first.LiveryPrimaryMask, second.LiveryPrimaryMask) && masksEqual(first.LiverySecondaryMask, second.LiverySecondaryMask) &&
            masksEqual(first.CoreRaisedMask, second.CoreRaisedMask) && masksEqual(first.CoreRecessedMask, second.CoreRecessedMask) &&
            masksEqual(first.HullLayerMask, second.HullLayerMask) && masksEqual(first.WeaponOccupiedMask, second.WeaponOccupiedMask);
    }

    ShipGenerationConfiguration toConfiguration(const ShipGenerationSettings& settings)
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

    ShipFactionType differentFaction(ShipFactionType faction)
    {
        return faction == ShipFactionType::FRONTIER ? ShipFactionType::RELIC : ShipFactionType::FRONTIER;
    }
}

namespace SpectralShipGenTests
{
    int runStaticFactionProfileRoutingRegression()
    {
        using namespace SpectralShipGen;

        const std::array<ShipFactionType, 6u> factions = {
            ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT,
            ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC
        };
        const std::array<ShipStyle, 3u> styles = { ShipStyle::FIGHTER, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
        const std::array<ShipDimensions, 3u> dimensions = { ShipDimensions{ 44u, 44u }, ShipDimensions{ 64u, 48u }, ShipDimensions{ 96u, 64u } };

        ShipGenerator generator;
        uint64_t seed = 0x8400FACC10A50001ull;
        for (std::size_t factionIndex = 0u; factionIndex < factions.size(); ++factionIndex)
        {
            ShipGenerationSettings settings;
            settings.Seed = seed + static_cast<uint64_t>(factionIndex) * 0x10001ull;
            settings.Style = styles[factionIndex % styles.size()];
            settings.Faction = factions[factionIndex];
            settings.Dimensions = dimensions[factionIndex % dimensions.size()];
            settings.DetailDensity = 87u;
            settings.AsymmetricDetailChance = 23u;
            settings.AttachmentsEnabled = true;

            ShipGenerationDebugInfo presetDebug;
            const GeneratedShip presetShip = generator.generate(settings, &presetDebug);

            ShipGenerationConfiguration routedConfiguration = toConfiguration(settings);
            routedConfiguration.Faction = differentFaction(settings.Faction);
            ShipGenerationDebugInfo routedDebug;
            const GeneratedShip routedShip = ShipGeneratorStaticFactionRegressionAccess::generate(
                generator, routedConfiguration, getShipGenerationProfile(settings.Style), getShipFactionProfile(settings.Faction), settings.Style, &routedDebug);

            if (routedShip.Faction != routedConfiguration.Faction)
            {
                std::cerr << "Static faction routing regression did not preserve the intentionally mismatched provenance field.\n";
                return 1;
            }
            if (!staticShipEqual(presetShip, routedShip) || !staticDebugEqual(presetDebug, routedDebug))
            {
                std::cerr << "Static generation still depends on ShipFactionType after resolved profile routing for faction index " << factionIndex << ".\n";
                return 1;
            }
        }

        ShipGenerationConfiguration customConfiguration;
        customConfiguration.Seed = 0x8400FACC10A5C057ull;
        customConfiguration.Dimensions = { 80u, 56u };
        customConfiguration.Faction = ShipFactionType::CORPORATE;
        customConfiguration.DetailDensity = 72u;
        customConfiguration.AsymmetricDetailChance = 31u;
        customConfiguration.AttachmentsEnabled = true;
        ShipGenerationProfile customProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        customProfile.LargeWeaponChance = 83u;
        customProfile.LargeWeaponScalePercent = 135u;
        customProfile.DetailDensityPercent = 74u;

        ShipGenerationDebugInfo customPresetDebug;
        const GeneratedShip customPresetShip = generator.generate(customConfiguration, customProfile, &customPresetDebug);
        ShipGenerationConfiguration customRoutedConfiguration = customConfiguration;
        customRoutedConfiguration.Faction = ShipFactionType::FRONTIER;
        ShipGenerationDebugInfo customRoutedDebug;
        const GeneratedShip customRoutedShip = ShipGeneratorStaticFactionRegressionAccess::generate(
            generator, customRoutedConfiguration, customProfile, getShipFactionProfile(ShipFactionType::CORPORATE), ShipStyle::SHIP_STYLE_END, &customRoutedDebug);

        if (!staticShipEqual(customPresetShip, customRoutedShip) || !staticDebugEqual(customPresetDebug, customRoutedDebug))
        {
            std::cerr << "Custom structural profile + resolved built-in faction static routing changed output when faction provenance changed.\n";
            return 1;
        }

        std::cout << "Static faction profile routing regression passed.\n";
        return 0;
    }
}
