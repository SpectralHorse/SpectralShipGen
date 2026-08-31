#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "AttachmentGenerator.h"
#include "CockpitGenerator.h"
#include "DetailGenerator.h"
#include "EngineGenerator.h"
#include "HullGenerator.h"
#include "MajorFeatureGenerator.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include "WeaponGenerator.h"

namespace
{
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    constexpr std::array<SpectralShipGen::ShipStyle, static_cast<std::size_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)> Styles = { SpectralShipGen::ShipStyle::SLEEK, SpectralShipGen::ShipStyle::FIGHTER, SpectralShipGen::ShipStyle::HEAVY, SpectralShipGen::ShipStyle::INDUSTRIAL, SpectralShipGen::ShipStyle::SPEARHEAD, SpectralShipGen::ShipStyle::DELTA };
    constexpr std::array<SpectralShipGen::ShipFactionType, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { SpectralShipGen::ShipFactionType::FRONTIER, SpectralShipGen::ShipFactionType::MILITARY, SpectralShipGen::ShipFactionType::ASCENDANT, SpectralShipGen::ShipFactionType::XENO, SpectralShipGen::ShipFactionType::CORPORATE, SpectralShipGen::ShipFactionType::RELIC };
    constexpr uint32_t SamplesPerConfiguration = 10u;
    constexpr uint32_t MaximumHullAttempts = 8u;

    bool masksEqual(const SpectralShipGen::PixelMask& first, const SpectralShipGen::PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight())
        {
            return false;
        }

        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool isSubset(const SpectralShipGen::PixelMask& subset, const SpectralShipGen::PixelMask& superset)
    {
        for (uint32_t y = 0u; y < subset.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < subset.getWidth(); ++x)
            {
                if (subset.get(x, y) && !superset.get(x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool generatePipeline(SpectralShipGen::ShipGenerationContext& context)
    {
        SpectralShipGen::HullGenerator hullGenerator;
        SpectralShipGen::CockpitGenerator cockpitGenerator;
        SpectralShipGen::EngineGenerator engineGenerator;
        SpectralShipGen::MajorFeatureGenerator majorFeatureGenerator;
        SpectralShipGen::WeaponGenerator weaponGenerator;
        SpectralShipGen::AttachmentGenerator attachmentGenerator;
        SpectralShipGen::DetailGenerator detailGenerator;

        for (uint32_t attempt = 0u; attempt < MaximumHullAttempts; ++attempt)
        {
            context.Ship.clear();
            hullGenerator.generate(context);

            if (!hullGenerator.validate(context))
            {
                continue;
            }

            cockpitGenerator.generate(context);
            engineGenerator.generate(context);
            majorFeatureGenerator.generate(context);
            weaponGenerator.generate(context);

            if (context.Settings.AttachmentsEnabled)
            {
                attachmentGenerator.generate(context);
            }

            detailGenerator.generate(context);
            return true;
        }

        return false;
    }

    bool placementsEqual(const std::vector<SpectralShipGen::WeaponPlacement>& first, const std::vector<SpectralShipGen::WeaponPlacement>& second)
    {
        if (first.size() != second.size())
        {
            return false;
        }

        for (std::size_t index = 0u; index < first.size(); ++index)
        {
            const SpectralShipGen::WeaponPlacement& a = first[index];
            const SpectralShipGen::WeaponPlacement& b = second[index];
            if (a.Type != b.Type || a.Region != b.Region || a.Direction != b.Direction || a.AnchorX != b.AnchorX || a.AnchorY != b.AnchorY || a.BodyMinX != b.BodyMinX || a.BodyMaxX != b.BodyMaxX || a.BodyMinY != b.BodyMinY || a.BodyMaxY != b.BodyMaxY || a.BarrelMinX != b.BarrelMinX || a.BarrelMaxX != b.BarrelMaxX || a.BarrelMinY != b.BarrelMinY || a.BarrelMaxY != b.BarrelMaxY || a.MuzzleX != b.MuzzleX || a.MuzzleY != b.MuzzleY || a.SymmetryGroup != b.SymmetryGroup || a.MovableBarrel != b.MovableBarrel || a.Emissive != b.Emissive)
            {
                return false;
            }
        }

        return true;
    }

    bool validateWeapons(const SpectralShipGen::ShipGenerationContext& context)
    {
        const SpectralShipGen::WeaponData& weapons = context.Weapons;

        if (!isSubset(weapons.RootMask, weapons.OccupiedMask) || !isSubset(weapons.BodyMask, weapons.OccupiedMask) || !isSubset(weapons.BarrelMask, weapons.OccupiedMask) || !isSubset(weapons.MuzzleMask, weapons.OccupiedMask) || !isSubset(weapons.MovableMask, weapons.OccupiedMask) || !isSubset(weapons.EmissiveMask, weapons.OccupiedMask))
        {
            return false;
        }

        if (!isSubset(weapons.RootMask, context.Ship.HullMask))
        {
            return false;
        }

        if (SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.CockpitMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.EngineMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.EngineExhaustMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.MajorFeatures.OccupiedMask))
        {
            return false;
        }

        if (SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.AttachmentMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.AccentMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.MechanicalDetailMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(weapons.OccupiedMask, context.Ship.LightMask))
        {
            return false;
        }

        std::map<uint32_t, std::vector<const SpectralShipGen::WeaponPlacement*>> symmetryGroups;

        for (const SpectralShipGen::WeaponPlacement& placement : weapons.Placements)
        {
            if (!weapons.RootMask.get(placement.AnchorX, placement.AnchorY) || !weapons.MuzzleMask.get(placement.MuzzleX, placement.MuzzleY) || placement.MuzzleY >= placement.AnchorY)
            {
                return false;
            }

            if (placement.BodyMinX > placement.BodyMaxX || placement.BodyMinY > placement.BodyMaxY || placement.BarrelMinX > placement.BarrelMaxX || placement.BarrelMinY > placement.BarrelMaxY)
            {
                return false;
            }

            if (placement.Region == SpectralShipGen::ShipWeaponHardpointRegion::WING_ROOT && !context.WingRegions.WingRootMask.get(placement.AnchorX, placement.AnchorY))
            {
                return false;
            }

            if (placement.Region == SpectralShipGen::ShipWeaponHardpointRegion::OUTER_WING && !context.WingRegions.OuterWingMask.get(placement.AnchorX, placement.AnchorY))
            {
                return false;
            }

            if (placement.SymmetryGroup != 0u)
            {
                symmetryGroups[placement.SymmetryGroup].push_back(&placement);
            }
        }

        for (const auto& group : symmetryGroups)
        {
            if (group.second.size() != 2u)
            {
                return false;
            }

            const SpectralShipGen::WeaponPlacement& first = *group.second[0];
            const SpectralShipGen::WeaponPlacement& second = *group.second[1];
            const uint32_t width = weapons.OccupiedMask.getWidth();

            if (first.Type != second.Type || first.Region != second.Region || first.AnchorY != second.AnchorY || first.AnchorX != width - 1u - second.AnchorX || first.MuzzleY != second.MuzzleY || first.MuzzleX != width - 1u - second.MuzzleX || first.MovableBarrel != second.MovableBarrel || first.Emissive != second.Emissive)
            {
                return false;
            }
        }

        for (const SpectralShipGen::ShipAttachmentPlacement& placement : context.Ship.AttachmentPlacements)
        {
            if (weapons.OccupiedMask.get(placement.AnchorX, placement.AnchorY))
            {
                return false;
            }
        }

        return true;
    }
}

int SpectralShipGenTests::runWeaponGeometryRegression()
{
    std::array<uint32_t, Resolutions.size()> shipsWithWeapons = {};
    std::array<uint64_t, Resolutions.size()> weaponPixelsByResolution = {};
    std::array<uint64_t, Resolutions.size()> maximumAssemblyAreaByResolution = {};
    std::array<uint32_t, static_cast<std::size_t>(SpectralShipGen::ShipWeaponType::SHIP_WEAPON_TYPE_END)> observedTypeCounts = {};
    uint32_t shipsWithoutWeapons = 0u;

    for (std::size_t resolutionIndex = 0u; resolutionIndex < Resolutions.size(); ++resolutionIndex)
    {
        const uint32_t resolution = Resolutions[resolutionIndex];

        for (SpectralShipGen::ShipStyle style : Styles)
        {
            for (SpectralShipGen::ShipFactionType faction : Factions)
            {
                for (uint32_t sample = 0u; sample < SamplesPerConfiguration; ++sample)
                {
                    SpectralShipGen::ShipGenerationSettings settings;
                    settings.Dimensions.Width = resolution;
                    settings.Dimensions.Height = resolution;
                    settings.Style = style;
                    settings.Faction = faction;
                    settings.Seed = 0x37000000ull + static_cast<uint64_t>(resolution) * 100000ull + static_cast<uint64_t>(static_cast<uint32_t>(style)) * 10000ull + static_cast<uint64_t>(static_cast<uint32_t>(faction)) * 1000ull + sample;
                    const SpectralShipGen::ShipGenerationSeeds seeds = SpectralShipGen::deriveShipGenerationSeeds(settings.Seed);
                    const SpectralShipGen::ShipGenerationProfile& profile = SpectralShipGen::getShipGenerationProfile(style);
                    SpectralShipGen::ShipGenerationDebugInfo firstDebug;
                    SpectralShipGen::ShipGenerationDebugInfo secondDebug;
                    SpectralShipGen::ShipGenerationContext firstContext(settings, profile, seeds, &firstDebug);
                    SpectralShipGen::ShipGenerationContext secondContext(settings, profile, seeds, &secondDebug);

                    if (!generatePipeline(firstContext) || !generatePipeline(secondContext))
                    {
                        std::cerr << "Pipeline generation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!masksEqual(firstContext.Weapons.OccupiedMask, secondContext.Weapons.OccupiedMask) || !masksEqual(firstContext.Weapons.RootMask, secondContext.Weapons.RootMask) || !masksEqual(firstContext.Weapons.BodyMask, secondContext.Weapons.BodyMask) || !masksEqual(firstContext.Weapons.BarrelMask, secondContext.Weapons.BarrelMask) || !masksEqual(firstContext.Weapons.MuzzleMask, secondContext.Weapons.MuzzleMask) || !masksEqual(firstContext.Weapons.MovableMask, secondContext.Weapons.MovableMask) || !masksEqual(firstContext.Weapons.EmissiveMask, secondContext.Weapons.EmissiveMask) || !placementsEqual(firstContext.Weapons.Placements, secondContext.Weapons.Placements))
                    {
                        std::cerr << "Weapon determinism failure at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!validateWeapons(firstContext))
                    {
                        std::cerr << "Weapon validation failure at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (firstDebug.WeaponCount != firstContext.Weapons.Placements.size() || firstDebug.WeaponPixelCount != SpectralShipGen::PixelMaskUtils::getMaskPixelCount(firstContext.Weapons.OccupiedMask) || firstDebug.WeaponRealizedGroupCount > firstDebug.WeaponRequestedGroupCount)
                    {
                        std::cerr << "Weapon debug metadata mismatch at resolution " << resolution << ".\n";
                        return 1;
                    }
                    if ((firstDebug.WeaponCount == 0u) != (firstDebug.WeaponCoveragePermille == 0u))
                    {
                        std::cerr << "Weapon coverage diagnostics are inconsistent at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (firstContext.Weapons.Placements.empty())
                    {
                        ++shipsWithoutWeapons;
                        continue;
                    }

                    ++shipsWithWeapons[resolutionIndex];
                    weaponPixelsByResolution[resolutionIndex] += firstDebug.WeaponPixelCount;
                    uint64_t maximumAssemblyArea = 0u;
                    for (const SpectralShipGen::WeaponPlacement& placement : firstContext.Weapons.Placements)
                    {
                        ++observedTypeCounts[static_cast<std::size_t>(placement.Type)];
                        const uint32_t minimumX = std::min(placement.BodyMinX, placement.BarrelMinX);
                        const uint32_t maximumX = std::max(placement.BodyMaxX, placement.BarrelMaxX);
                        const uint32_t minimumY = std::min(placement.BodyMinY, placement.BarrelMinY);
                        const uint32_t maximumY = std::max(placement.BodyMaxY, placement.BarrelMaxY);
                        maximumAssemblyArea = std::max<uint64_t>(maximumAssemblyArea, static_cast<uint64_t>(maximumX - minimumX + 1u) * (maximumY - minimumY + 1u));
                    }
                    maximumAssemblyAreaByResolution[resolutionIndex] += maximumAssemblyArea;
                }
            }
        }

        if (resolution >= 32u && shipsWithWeapons[resolutionIndex] == 0u)
        {
            std::cerr << "No large weapons observed at resolution " << resolution << ".\n";
            return 1;
        }
    }

    if (shipsWithoutWeapons == 0u)
    {
        std::cerr << "Large weapons were forced onto every sampled ship.\n";
        return 1;
    }

    uint32_t observedTypes = 0u;
    for (uint32_t count : observedTypeCounts) { if (count > 0u) { ++observedTypes; } }

    if (observedTypes < 4u)
    {
        std::cerr << "Insufficient large-weapon type variety observed.\n";
        return 1;
    }

    const auto averagePixels = [&](std::size_t index) -> double
        { return shipsWithWeapons[index] == 0u ? 0.0 : static_cast<double>(weaponPixelsByResolution[index]) / shipsWithWeapons[index]; };
    const auto averageAssemblyArea = [&](std::size_t index) -> double
        { return shipsWithWeapons[index] == 0u ? 0.0 : static_cast<double>(maximumAssemblyAreaByResolution[index]) / shipsWithWeapons[index]; };
    const std::size_t index32 = 1u;
    const std::size_t index96 = 4u;
    const std::size_t index160 = 6u;
    if (!(averagePixels(index96) > averagePixels(index32) * 2.0 && averagePixels(index160) > averagePixels(index96) * 1.8 && averageAssemblyArea(index160) > averageAssemblyArea(index96) * 1.5))
    {
        std::cerr << "Large-weapon scale growth is not visible across native resolutions.\n";
        return 1;
    }

    constexpr std::array<SpectralShipGen::ShipDimensions, 2u> RectangularDimensions = {{{ 128u, 96u }, { 96u, 128u }}};
    constexpr std::array<SpectralShipGen::ShipStyle, 2u> RectangularStyles = { SpectralShipGen::ShipStyle::DELTA, SpectralShipGen::ShipStyle::SPEARHEAD };
    for (std::size_t caseIndex = 0u; caseIndex < RectangularDimensions.size(); ++caseIndex)
    {
        bool observedWeapons = false;
        for (uint32_t sample = 0u; sample < 16u && !observedWeapons; ++sample)
        {
            SpectralShipGen::ShipGenerationSettings settings;
            settings.Dimensions = RectangularDimensions[caseIndex];
            settings.Style = RectangularStyles[caseIndex];
            settings.Faction = caseIndex == 0u ? SpectralShipGen::ShipFactionType::MILITARY : SpectralShipGen::ShipFactionType::ASCENDANT;
            settings.Seed = 0x7600760000000000ull + static_cast<uint64_t>(caseIndex) * 0x1000ull + sample;
            const SpectralShipGen::ShipGenerationSeeds seeds = SpectralShipGen::deriveShipGenerationSeeds(settings.Seed);
            const SpectralShipGen::ShipGenerationProfile& profile = SpectralShipGen::getShipGenerationProfile(settings.Style);
            SpectralShipGen::ShipGenerationDebugInfo firstDebug;
            SpectralShipGen::ShipGenerationDebugInfo secondDebug;
            SpectralShipGen::ShipGenerationContext firstContext(settings, profile, seeds, &firstDebug);
            SpectralShipGen::ShipGenerationContext secondContext(settings, profile, seeds, &secondDebug);

            if (!generatePipeline(firstContext) || !generatePipeline(secondContext) || !validateWeapons(firstContext) || !validateWeapons(secondContext))
            {
                std::cerr << "Rectangular weapon generation/validation failure.\n";
                return 1;
            }
            if (!masksEqual(firstContext.Weapons.OccupiedMask, secondContext.Weapons.OccupiedMask) || !placementsEqual(firstContext.Weapons.Placements, secondContext.Weapons.Placements))
            {
                std::cerr << "Rectangular weapon determinism failure.\n";
                return 1;
            }
            observedWeapons = !firstContext.Weapons.Placements.empty();
        }
        if (!observedWeapons)
        {
            std::cerr << "No weapon realization observed for rectangular regression case.\n";
            return 1;
        }
    }

    SpectralShipGen::ShipGenerator generator;
    uint32_t anchorOpportunities = 0u;
    uint32_t anchorRealizations = 0u;
    for (uint32_t sample = 0u; sample < 96u && anchorOpportunities < 12u; ++sample)
    {
        SpectralShipGen::ShipGenerationSettings settings;
        settings.Seed = 0x7600000000000000ull + sample;
        settings.Dimensions = { 96u,96u };
        settings.Style = static_cast<SpectralShipGen::ShipStyle>(sample % static_cast<uint32_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END));
        settings.Faction = static_cast<SpectralShipGen::ShipFactionType>((sample / static_cast<uint32_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)) % static_cast<uint32_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END));
        SpectralShipGen::ShipGenerationDebugInfo debug;
        generator.generate(settings, &debug);
        if (!debug.WeaponVisualAnchorOpportunity) { continue; }
        ++anchorOpportunities;
        if (debug.WeaponVisualAnchorRealized) { ++anchorRealizations; }
    }
    if (anchorOpportunities < 3u || anchorRealizations * 100u < anchorOpportunities * 85u)
    {
        std::cerr << "WEAPONS visual-anchor realization is not reliable when a valid hardpoint opportunity exists.\n";
        return 1;
    }

    std::cout << "Ship large weapon regression passed.\n";
    return 0;
}
