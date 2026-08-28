#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <queue>
#include <utility>
#include <vector>

#include "CockpitGenerator.h"
#include "GenerationComplexityBudget.h"
#include "HullGenerator.h"
#include "MacroAsymmetryPlanner.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerator.h"

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

    bool isFourConnected(const PixelMask& mask)
    {
        const uint32_t total = PixelMaskUtils::getMaskPixelCount(mask);
        if (total == 0u) { return false; }
        std::vector<uint8_t> visited(static_cast<std::size_t>(mask.getWidth()) * mask.getHeight(), 0u);
        std::queue<std::pair<uint32_t, uint32_t>> pending;
        bool found = false;
        for (uint32_t y = 0u; y < mask.getHeight() && !found; ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                pending.push({ x, y });
                visited[static_cast<std::size_t>(y) * mask.getWidth() + x] = 1u;
                found = true;
                break;
            }
        }

        uint32_t count = 0u;
        constexpr int32_t Directions[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
        while (!pending.empty())
        {
            const auto [x, y] = pending.front();
            pending.pop();
            ++count;
            for (const auto& direction : Directions)
            {
                const int32_t nx = static_cast<int32_t>(x) + direction[0];
                const int32_t ny = static_cast<int32_t>(y) + direction[1];
                if (nx < 0 || ny < 0 || nx >= static_cast<int32_t>(mask.getWidth()) || ny >= static_cast<int32_t>(mask.getHeight())) { continue; }
                const std::size_t index = static_cast<std::size_t>(ny) * mask.getWidth() + static_cast<uint32_t>(nx);
                if (visited[index] != 0u || !mask.get(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny))) { continue; }
                visited[index] = 1u;
                pending.push({ static_cast<uint32_t>(nx), static_cast<uint32_t>(ny) });
            }
        }
        return count == total;
    }

    struct Snapshot
    {
        PixelMask HullMask;
        PixelMask WingMask;
        PixelMask CockpitMask;
        CockpitData Cockpit;
        ShipGenerationDebugInfo Debug;
        bool Valid = false;
    };

    Snapshot generateCockpit(uint64_t seed, ShipDimensions dimensions, ShipStyle style, ShipFactionType faction)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        const ShipGenerationSeeds seeds = deriveShipGenerationSeeds(seed);
        const ShipGenerationProfile& profile = getShipGenerationProfile(style);
        Snapshot snapshot;
        ShipGenerationContext context(settings, profile, seeds, &snapshot.Debug);
        HullGenerator hullGenerator;
        CockpitGenerator cockpitGenerator;
        MacroAsymmetryPlanner asymmetryPlanner;

        for (uint32_t attempt = 0u; attempt < 24u; ++attempt)
        {
            context.Ship.clear();
            context.resetComplexityBudget();
            hullGenerator.generate(context);
            if (!hullGenerator.validate(context)) { continue; }
            context.resetSpatialBudget();
            context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::SILHOUETTE);
            asymmetryPlanner.createPlan(context);
            cockpitGenerator.generate(context);
            context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::COCKPIT_STRUCTURE);
            if (!snapshot.Debug.CockpitPlacementSucceeded) { continue; }
            context.updateComplexityBudgetDebugInfo();
            context.updateSpatialBudgetDebugInfo();
            snapshot.HullMask = context.Ship.HullMask;
            snapshot.WingMask = context.WingRegions.WingMask;
            snapshot.CockpitMask = context.Ship.CockpitMask;
            snapshot.Cockpit = context.Cockpit;
            snapshot.Valid = true;
            return snapshot;
        }
        return snapshot;
    }

    bool validateSnapshot(const Snapshot& snapshot)
    {
        if (!snapshot.Valid || !isFourConnected(snapshot.CockpitMask)) { return false; }
        uint32_t semanticCount = 0u;
        for (uint32_t y = 0u; y < snapshot.CockpitMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < snapshot.CockpitMask.getWidth(); ++x)
            {
                if (snapshot.CockpitMask.get(x, y) && (!snapshot.HullMask.get(x, y) || snapshot.WingMask.get(x, y))) { return false; }
                const bool glass = snapshot.Cockpit.GlassMask.get(x, y);
                const bool frame = snapshot.Cockpit.FrameMask.get(x, y);
                const bool base = snapshot.Cockpit.BaseMask.get(x, y);
                if (glass || frame || base)
                {
                    if (!snapshot.CockpitMask.get(x, y)) { return false; }
                    ++semanticCount;
                }
                if ((glass && frame) || (glass && base) || (frame && base)) { return false; }
                if (snapshot.Cockpit.UpperSectionMask.get(x, y) && !glass) { return false; }
            }
        }
        if (semanticCount != PixelMaskUtils::getMaskPixelCount(snapshot.CockpitMask)) { return false; }
        if (PixelMaskUtils::getMaskPixelCount(snapshot.Cockpit.GlassMask) == 0u || PixelMaskUtils::getMaskPixelCount(snapshot.Cockpit.FrameMask) == 0u) { return false; }

        const std::size_t cockpitCategory = static_cast<std::size_t>(GenerationComplexityCategory::COCKPIT_STRUCTURE);
        if (snapshot.Cockpit.ComplexityCost != snapshot.Debug.CockpitComplexityCost) { return false; }
        if (snapshot.Cockpit.ComplexityCost > 0u && snapshot.Debug.ComplexityCategoryConsumed[cockpitCategory] != snapshot.Cockpit.ComplexityCost) { return false; }
        if (snapshot.Cockpit.ComplexityCost > 0u)
        {
            const uint32_t centralLoad = snapshot.Debug.SpatialRegionLoads[static_cast<std::size_t>(GenerationSpatialRegion::NOSE)]
                + snapshot.Debug.SpatialRegionLoads[static_cast<std::size_t>(GenerationSpatialRegion::FRONT_FUSELAGE)]
                + snapshot.Debug.SpatialRegionLoads[static_cast<std::size_t>(GenerationSpatialRegion::MID_FUSELAGE)];
            if (centralLoad == 0u) { return false; }
        }
        return true;
    }
}

int PixelShipGeneratorTests::runCockpitGeometryRegression()
{
    using namespace PixelShipGenerator;

    const std::array<ShipDimensions, 7u> representative = { ShipDimensions{24u, 24u}, {32u, 32u}, {44u, 44u}, {48u, 64u}, {64u, 48u}, {64u, 64u}, {96u, 96u} };
    for (std::size_t index = 0u; index < representative.size(); ++index)
    {
        const Snapshot first = generateCockpit(0x5200000000000000ull + index * 0x9E3779B97F4A7C15ull, representative[index], ShipStyle::FIGHTER, ShipFactionType::MILITARY);
        const Snapshot second = generateCockpit(0x5200000000000000ull + index * 0x9E3779B97F4A7C15ull, representative[index], ShipStyle::FIGHTER, ShipFactionType::MILITARY);
        if (!validateSnapshot(first) || !validateSnapshot(second) || !masksEqual(first.CockpitMask, second.CockpitMask) || first.Cockpit.SizeClass != second.Cockpit.SizeClass || first.Cockpit.ShapeType != second.Cockpit.ShapeType)
        {
            std::cerr << "Cockpit semantic/determinism regression failed at " << representative[index].Width << 'x' << representative[index].Height << ".\n";
            return 1;
        }
    }

    std::array<bool, static_cast<std::size_t>(CockpitSizeClass::COCKPIT_SIZE_CLASS_END)> sizeSeen = {};
    std::array<bool, static_cast<std::size_t>(CockpitShapeType::COCKPIT_SHAPE_TYPE_END)> shapeSeen = {};
    const std::array<ShipStyle, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
    const std::array<ShipFactionType, static_cast<std::size_t>(ShipFactionType::SHIP_FACTION_TYPE_END)> factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
    for (uint32_t sample = 0u; sample < 384u; ++sample)
    {
        const ShipDimensions dimensions = sample % 4u == 0u ? ShipDimensions{ 32u, 32u } : (sample % 4u == 1u ? ShipDimensions{ 44u, 44u } : (sample % 4u == 2u ? ShipDimensions{ 96u, 96u } : ShipDimensions{ 160u, 160u }));
        const Snapshot snapshot = generateCockpit(0xA5A5A5A500000000ull + sample * 0xD1B54A32D192ED03ull, dimensions, styles[sample % styles.size()], factions[(sample / styles.size()) % factions.size()]);
        if (!snapshot.Valid) { continue; }
        sizeSeen[static_cast<std::size_t>(snapshot.Cockpit.SizeClass)] = true;
        shapeSeen[static_cast<std::size_t>(snapshot.Cockpit.ShapeType)] = true;
    }

    for (uint32_t index = 0u; index < static_cast<uint32_t>(CockpitSizeClass::COCKPIT_SIZE_CLASS_END); ++index)
    {
        if (!sizeSeen[index])
        {
            std::cerr << "Cockpit size class was never observed: " << getCockpitSizeClassName(static_cast<CockpitSizeClass>(index)) << ".\n";
            return 1;
        }
    }
    for (uint32_t index = 0u; index < static_cast<uint32_t>(CockpitShapeType::COCKPIT_SHAPE_TYPE_END); ++index)
    {
        if (!shapeSeen[index])
        {
            std::cerr << "Cockpit shape was never observed: " << getCockpitShapeTypeName(static_cast<CockpitShapeType>(index)) << ".\n";
            return 1;
        }
    }

    ShipGenerationSettings baseSettings;
    baseSettings.Seed = 0x123456789ABC5200ull;
    baseSettings.Dimensions = { 64u, 64u };
    baseSettings.Style = ShipStyle::HEAVY;
    baseSettings.Faction = ShipFactionType::FRONTIER;
    ShipGenerator generator;
    const GeneratedShip base = generator.generate(baseSettings);
    const ShipGenerationSeeds topSeeds = deriveShipGenerationSeeds(baseSettings.Seed);
    const GenerationDomainSeeds domainSeeds = resolveGenerationDomainSeeds(topSeeds, baseSettings.DomainSeedOverrides, baseSettings.RandomStreamMode);
    ShipGenerationSettings rerolledSettings = baseSettings;
    rerolledSettings.DomainSeedOverrides.set(GenerationDomain::COCKPIT, deriveGenerationDomainRerollSeed(0xCAFEBABE52005200ull, GenerationDomain::COCKPIT, domainSeeds.get(GenerationDomain::COCKPIT)));
    const GeneratedShip rerolled = generator.generate(rerolledSettings);
    if (!masksEqual(base.HullMask, rerolled.HullMask))
    {
        std::cerr << "Cockpit-domain reroll changed upstream Hull geometry.\n";
        return 1;
    }
    const GeneratedShip rerolledAgain = generator.generate(rerolledSettings);
    if (!masksEqual(rerolled.CockpitMask, rerolledAgain.CockpitMask) || rerolled.FinalImage.getPixels() != rerolledAgain.FinalImage.getPixels())
    {
        std::cerr << "Cockpit-domain reroll is not exactly reproducible.\n";
        return 1;
    }

    std::cout << "Advanced cockpit geometry regression passed.\n";
    return 0;
}
