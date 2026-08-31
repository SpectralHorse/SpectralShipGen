#include "CoreRegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "PixelMaskUtils.h"
#include <SpectralShipGen/ShipDetailMotifType.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

using namespace SpectralShipGen;

namespace
{
    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
                if (first.get(x, y) != second.get(x, y)) { return false; }
        return true;
    }

    bool validateMotifMask(const GeneratedShip& ship, const ShipGenerationDebugInfo& debug, const PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                if (!ship.HullMask.get(x, y)) { std::cerr << "Detail motif escaped HullMask.\n"; return false; }
                if (ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y) || debug.WeaponOccupiedMask.get(x, y))
                {
                    std::cerr << "Detail motif overlaps excluded component geometry.\n";
                    return false;
                }
                if (debug.LiveryPrimaryMask.get(x, y) || debug.LiverySecondaryMask.get(x, y))
                {
                    std::cerr << "Detail motif overwrote Task-60 livery.\n";
                    return false;
                }
                if (debug.CoreLuminousMask.get(x, y) || debug.ReservedNegativeSpaceMask.get(x, y))
                {
                    std::cerr << "Detail motif overlaps luminous/negative-space geometry.\n";
                    return false;
                }
            }
        }
        return true;
    }

    bool checkDeterminismAndRerollOwnership()
    {
        ShipGenerator generator;
        ShipGenerationSettings settings;
        settings.Seed = 0x61A11C0DE1234567ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::INDUSTRIAL;
        settings.Faction = ShipFactionType::CORPORATE;

        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        const GeneratedShip first = generator.generate(settings, &firstDebug);
        const GeneratedShip second = generator.generate(settings, &secondDebug);
        if (firstDebug.PrimaryDetailMotif != secondDebug.PrimaryDetailMotif || firstDebug.SecondaryDetailMotif != secondDebug.SecondaryDetailMotif
            || firstDebug.PrimaryDetailMotifRegion != secondDebug.PrimaryDetailMotifRegion || firstDebug.SecondaryDetailMotifRegion != secondDebug.SecondaryDetailMotifRegion
            || !masksEqual(firstDebug.PrimaryDetailMotifMask, secondDebug.PrimaryDetailMotifMask) || !masksEqual(firstDebug.SecondaryDetailMotifMask, secondDebug.SecondaryDetailMotifMask)
            || first.FinalImage.getPixels() != second.FinalImage.getPixels())
        {
            std::cerr << "Detail motif planning/placement is not deterministic.\n";
            return false;
        }
        if (!validateMotifMask(first, firstDebug, firstDebug.PrimaryDetailMotifMask) || !validateMotifMask(first, firstDebug, firstDebug.SecondaryDetailMotifMask)) { return false; }
        if (firstDebug.PrimaryDetailMotif == ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END || firstDebug.PrimaryDetailMotifOccurrenceCount == 0u)
        {
            std::cerr << "Fixed Details-domain fixture did not realize a primary motif.\n";
            return false;
        }

        const GenerationDomainSeeds domains = resolveGenerationDomainSeeds(deriveShipGenerationSeeds(settings.Seed), settings.DomainSeedOverrides);
        ShipGenerationSettings paletteReroll = settings;
        paletteReroll.DomainSeedOverrides.set(GenerationDomain::PALETTE, deriveGenerationDomainRerollSeed(0x61B22D1EF2345678ull, GenerationDomain::PALETTE, domains.get(GenerationDomain::PALETTE)));
        ShipGenerationDebugInfo paletteDebug;
        const GeneratedShip paletteShip = generator.generate(paletteReroll, &paletteDebug);
        if (!masksEqual(firstDebug.PrimaryDetailMotifMask, paletteDebug.PrimaryDetailMotifMask) || !masksEqual(firstDebug.SecondaryDetailMotifMask, paletteDebug.SecondaryDetailMotifMask)
            || firstDebug.PrimaryDetailMotif != paletteDebug.PrimaryDetailMotif || firstDebug.SecondaryDetailMotif != paletteDebug.SecondaryDetailMotif)
        {
            std::cerr << "Palette reroll changed detail motif geometry.\n";
            return false;
        }
        if (first.Palette.HullBase == paletteShip.Palette.HullBase && first.Palette.HullAccent == paletteShip.Palette.HullAccent)
        {
            std::cerr << "Palette reroll did not recolor the ship.\n";
            return false;
        }

        ShipGenerationSettings detailsReroll = settings;
        detailsReroll.DomainSeedOverrides.set(GenerationDomain::DETAILS, deriveGenerationDomainRerollSeed(0x61C33E2FA3456789ull, GenerationDomain::DETAILS, domains.get(GenerationDomain::DETAILS)));
        ShipGenerationDebugInfo detailsDebug;
        const GeneratedShip detailsShip = generator.generate(detailsReroll, &detailsDebug);
        if (!masksEqual(first.HullMask, detailsShip.HullMask) || !masksEqual(first.CockpitMask, detailsShip.CockpitMask) || !masksEqual(first.EngineMask, detailsShip.EngineMask)
            || !masksEqual(firstDebug.MaterialSecondaryHullMask, detailsDebug.MaterialSecondaryHullMask) || !masksEqual(firstDebug.MaterialMechanicalMask, detailsDebug.MaterialMechanicalMask))
        {
            std::cerr << "Details reroll changed structural/material-zone geometry.\n";
            return false;
        }
        if (firstDebug.PrimaryDetailMotif == detailsDebug.PrimaryDetailMotif && firstDebug.SecondaryDetailMotif == detailsDebug.SecondaryDetailMotif
            && masksEqual(firstDebug.PrimaryDetailMotifMask, detailsDebug.PrimaryDetailMotifMask) && masksEqual(firstDebug.SecondaryDetailMotifMask, detailsDebug.SecondaryDetailMotifMask))
        {
            std::cerr << "Details reroll did not change the fixed motif fixture.\n";
            return false;
        }
        return true;
    }

    bool checkStylesFactionsAndDimensions()
    {
        const std::array<ShipStyle, 6u> styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
        const std::array<ShipFactionType, 6u> factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
        const std::array<ShipDimensions, 7u> dimensions = { ShipDimensions{24u,24u}, {32u,32u}, {44u,44u}, {48u,64u}, {64u,48u}, {64u,64u}, {96u,96u} };
        ShipGenerator generator;
        uint64_t planned = 0u;
        uint64_t realized = 0u;

        for (std::size_t styleIndex = 0u; styleIndex < styles.size(); ++styleIndex)
        {
            for (std::size_t dimensionIndex = 0u; dimensionIndex < dimensions.size(); ++dimensionIndex)
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x6100000000000000ull + static_cast<uint64_t>(styleIndex) * 0x100000ull + static_cast<uint64_t>(dimensionIndex) * 0x1000ull;
                settings.Dimensions = dimensions[dimensionIndex];
                settings.Style = styles[styleIndex];
                settings.Faction = factions[(styleIndex + dimensionIndex) % factions.size()];
                ShipGenerationDebugInfo debug;
                const GeneratedShip ship = generator.generate(settings, &debug);
                if (!validateMotifMask(ship, debug, debug.PrimaryDetailMotifMask) || !validateMotifMask(ship, debug, debug.SecondaryDetailMotifMask)) { return false; }
                if (debug.PrimaryDetailMotif != ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END) { ++planned; }
                if (debug.PrimaryDetailMotifOccurrenceCount > 0u) { ++realized; }
                if (settings.Dimensions.Width == 24u && settings.Dimensions.Height == 24u)
                {
                    if (debug.PrimaryDetailMotifOccurrenceCount + debug.SecondaryDetailMotifOccurrenceCount > 1u || debug.SecondaryDetailMotif != ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END)
                    {
                        std::cerr << "24x24 detail grammar was not restrained.\n";
                        return false;
                    }
                    if (debug.PrimaryDetailMotif == ShipDetailMotifType::TRIPLE_VENT_BANK || debug.PrimaryDetailMotif == ShipDetailMotifType::THREE_NODE_LIGHTS || debug.PrimaryDetailMotif == ShipDetailMotifType::RECESSED_SLOT)
                    {
                        std::cerr << "24x24 selected an overly complex motif.\n";
                        return false;
                    }
                }
            }
        }
        if (planned == 0u || realized * 100u < planned * 55u)
        {
            std::cerr << "Detail motif planning rarely realizes visible occurrences.\n";
            return false;
        }
        return true;
    }

    bool checkCoherentDistributionAndStyleLanguage()
    {
        ShipGenerator generator;
        std::array<uint64_t, static_cast<std::size_t>(ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END)> primaryCounts = {};
        uint64_t primaryOccurrences = 0u;
        uint64_t secondaryOccurrences = 0u;
        uint64_t industrialMechanical = 0u;
        uint64_t industrialPlans = 0u;
        uint64_t sleekMechanical = 0u;
        uint64_t sleekPlans = 0u;
        uint64_t spearWingRegions = 0u;
        uint64_t spearPlans = 0u;
        uint64_t deltaWingRegions = 0u;
        uint64_t deltaPlans = 0u;
        uint64_t corporateOccurrences = 0u;
        uint64_t ascendantOccurrences = 0u;

        for (uint64_t index = 0u; index < 96u; ++index)
        {
            for (const ShipStyle style : { ShipStyle::SLEEK, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA })
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x61D0000000000000ull + index * 0x9E3779B97F4A7C15ull + static_cast<uint64_t>(style) * 0x10000ull;
                settings.Dimensions = { 64u, 64u };
                settings.Style = style;
                settings.Faction = ShipFactionType::MILITARY;
                ShipGenerationDebugInfo debug;
                (void)generator.generate(settings, &debug);
                if (debug.PrimaryDetailMotif == ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END) { continue; }
                ++primaryCounts[static_cast<std::size_t>(debug.PrimaryDetailMotif)];
                primaryOccurrences += debug.PrimaryDetailMotifOccurrenceCount;
                secondaryOccurrences += debug.SecondaryDetailMotifOccurrenceCount;
                const bool mechanicalMotif = debug.PrimaryDetailMotif == ShipDetailMotifType::PAIRED_VENTS || debug.PrimaryDetailMotif == ShipDetailMotifType::TRIPLE_VENT_BANK || debug.PrimaryDetailMotif == ShipDetailMotifType::RECESSED_SLOT;
                if (style == ShipStyle::INDUSTRIAL) { ++industrialPlans; if (mechanicalMotif) { ++industrialMechanical; } }
                if (style == ShipStyle::SLEEK) { ++sleekPlans; if (mechanicalMotif) { ++sleekMechanical; } }
                if (style == ShipStyle::SPEARHEAD)
                {
                    ++spearPlans;
                    if (debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::LEFT_WING_ROOT || debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::RIGHT_WING_ROOT || debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::LEFT_OUTER_WING || debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::RIGHT_OUTER_WING) { ++spearWingRegions; }
                }
                if (style == ShipStyle::DELTA)
                {
                    ++deltaPlans;
                    if (debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::LEFT_WING_ROOT || debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::RIGHT_WING_ROOT || debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::LEFT_OUTER_WING || debug.PrimaryDetailMotifRegion == GenerationSpatialRegion::RIGHT_OUTER_WING) { ++deltaWingRegions; }
                }
            }

            for (const ShipFactionType faction : { ShipFactionType::CORPORATE, ShipFactionType::ASCENDANT })
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x61E0000000000000ull + index * 0xD1B54A32D192ED03ull;
                settings.Dimensions = { 64u, 64u };
                settings.Style = ShipStyle::FIGHTER;
                settings.Faction = faction;
                ShipGenerationDebugInfo debug;
                (void)generator.generate(settings, &debug);
                if (faction == ShipFactionType::CORPORATE) { corporateOccurrences += debug.PrimaryDetailMotifOccurrenceCount + debug.SecondaryDetailMotifOccurrenceCount; }
                else { ascendantOccurrences += debug.PrimaryDetailMotifOccurrenceCount + debug.SecondaryDetailMotifOccurrenceCount; }
            }
        }

        const uint64_t totalPlans = [&]() { uint64_t total = 0u; for (uint64_t value : primaryCounts) { total += value; } return total; }();
        uint64_t maximumCount = 0u; for (uint64_t value : primaryCounts) { maximumCount = std::max(maximumCount, value); }
        if (totalPlans == 0u || maximumCount * 100u >= totalPlans * 55u) { std::cerr << "One detail motif dominates the primary vocabulary.\n"; return false; }
        if (primaryOccurrences <= secondaryOccurrences) { std::cerr << "Secondary motif occurrences compete with primary motifs.\n"; return false; }
        if (industrialPlans >= 20u && industrialMechanical * 100u <= industrialPlans * 35u) { std::cerr << "INDUSTRIAL lacks a mechanical motif bias.\n"; return false; }
        if (sleekPlans >= 20u && sleekMechanical * 100u >= sleekPlans * 45u) { std::cerr << "SLEEK became too mechanically motif-heavy.\n"; return false; }
        if (spearPlans > 0u && spearWingRegions != 0u) { std::cerr << "SPEARHEAD detail grammar left its axial regions.\n"; return false; }
        if (deltaPlans >= 20u && deltaWingRegions * 100u < deltaPlans * 65u) { std::cerr << "DELTA detail grammar is not sufficiently lateral.\n"; return false; }
        if (corporateOccurrences <= ascendantOccurrences) { std::cerr << "CORPORATE detail rhythm is not more repetitive than ASCENDANT.\n"; return false; }
        return true;
    }
}

int SpectralShipGenTests::runDetailMotifRegression()
{
    bool success = true;
    success = checkDeterminismAndRerollOwnership() && success;
    success = checkStylesFactionsAndDimensions() && success;
    success = checkCoherentDistributionAndStyleLanguage() && success;
    if (!success) { return 1; }
    std::cout << "Ship detail motif regression passed.\n";
    return 0;
}
