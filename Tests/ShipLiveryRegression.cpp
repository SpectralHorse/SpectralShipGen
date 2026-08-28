#include "RegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "PixelMaskUtils.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"
#include "ShipLiveryType.h"
#include "ShipIdleAnimator.h"

using namespace PixelShipGenerator;

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

    bool isSymmetric(const PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                if (mask.get(x, y) != mask.get(mask.getWidth() - 1u - x, y)) { return false; }
        return true;
    }

    bool validateLiveryMasks(const GeneratedShip& ship, const ShipGenerationDebugInfo& debug)
    {
        if (PixelMaskUtils::masksOverlap(debug.LiveryPrimaryMask, debug.LiverySecondaryMask))
        {
            std::cerr << "Primary and secondary livery masks overlap.\n";
            return false;
        }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                const bool livery = debug.LiveryPrimaryMask.get(x, y) || debug.LiverySecondaryMask.get(x, y);
                if (!livery) { continue; }
                if (!ship.HullMask.get(x, y)) { std::cerr << "Livery escaped HullMask.\n"; return false; }
                if (ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y) || debug.WeaponOccupiedMask.get(x, y))
                {
                    std::cerr << "Livery overlaps excluded component geometry.\n";
                    return false;
                }
                if (debug.CoreRecessedMask.get(x, y) || debug.CoreLuminousMask.get(x, y) || debug.ReservedNegativeSpaceMask.get(x, y))
                {
                    std::cerr << "Livery overlaps excluded recessed/luminous/negative-space geometry.\n";
                    return false;
                }
            }
        }
        return true;
    }

    bool checkDeterminismAndPaletteIsolation()
    {
        ShipGenerator generator;
        ShipGenerationSettings settings;
        settings.Seed = 0x60A11C0DE1234567ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::DELTA;
        settings.Faction = ShipFactionType::CORPORATE;

        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        const GeneratedShip first = generator.generate(settings, &firstDebug);
        const GeneratedShip second = generator.generate(settings, &secondDebug);
        if (!masksEqual(firstDebug.LiveryPrimaryMask, secondDebug.LiveryPrimaryMask) || !masksEqual(firstDebug.LiverySecondaryMask, secondDebug.LiverySecondaryMask) || first.FinalImage.getPixels() != second.FinalImage.getPixels())
        {
            std::cerr << "Livery generation is not deterministic.\n";
            return false;
        }
        if (!validateLiveryMasks(first, firstDebug)) { return false; }
        const ShipIdleAnimation animation = ShipIdleAnimator().generate(first);
        if (animation.Frames.empty() || animation.Frames.front().getPixels() != first.FinalImage.getPixels())
        {
            std::cerr << "Livery was not preserved in idle-animation Frame 0.\n";
            return false;
        }
        if (firstDebug.LiveryMarkingCount == 0u)
        {
            std::cerr << "Palette-isolation fixture did not generate livery.\n";
            return false;
        }

        const GenerationDomainSeeds domains = resolveGenerationDomainSeeds(deriveShipGenerationSeeds(settings.Seed), settings.DomainSeedOverrides, settings.RandomStreamMode);
        ShipGenerationSettings paletteReroll = settings;
        paletteReroll.DomainSeedOverrides.set(GenerationDomain::PALETTE, deriveGenerationDomainRerollSeed(0x60B22D1EF2345678ull, GenerationDomain::PALETTE, domains.get(GenerationDomain::PALETTE)));
        ShipGenerationDebugInfo paletteDebug;
        const GeneratedShip paletteShip = generator.generate(paletteReroll, &paletteDebug);

        if (!masksEqual(first.HullMask, paletteShip.HullMask) || !masksEqual(firstDebug.LiveryPrimaryMask, paletteDebug.LiveryPrimaryMask) || !masksEqual(firstDebug.LiverySecondaryMask, paletteDebug.LiverySecondaryMask))
        {
            std::cerr << "Palette reroll changed livery geometry.\n";
            return false;
        }
        if (first.Palette.HullAccent == paletteShip.Palette.HullAccent && first.Palette.HullAccentHighlight == paletteShip.Palette.HullAccentHighlight)
        {
            std::cerr << "Palette reroll failed to recolor livery palette roles.\n";
            return false;
        }

        ShipGenerationSettings detailReroll = settings;
        detailReroll.DomainSeedOverrides.set(GenerationDomain::DETAILS, deriveGenerationDomainRerollSeed(0x60C33E2FA3456789ull, GenerationDomain::DETAILS, domains.get(GenerationDomain::DETAILS)));
        ShipGenerationDebugInfo detailDebug;
        const GeneratedShip detailShip = generator.generate(detailReroll, &detailDebug);
        if (!masksEqual(first.HullMask, detailShip.HullMask) || !masksEqual(first.CockpitMask, detailShip.CockpitMask) || !masksEqual(first.EngineMask, detailShip.EngineMask))
        {
            std::cerr << "Details reroll changed structural geometry.\n";
            return false;
        }
        if (masksEqual(firstDebug.LiveryPrimaryMask, detailDebug.LiveryPrimaryMask) && masksEqual(firstDebug.LiverySecondaryMask, detailDebug.LiverySecondaryMask))
        {
            std::cerr << "Details reroll did not affect livery geometry for the fixed fixture.\n";
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
        uint64_t markedShips = 0u;
        uint64_t generatedShips = 0u;

        for (std::size_t styleIndex = 0u; styleIndex < styles.size(); ++styleIndex)
        {
            for (std::size_t dimensionIndex = 0u; dimensionIndex < dimensions.size(); ++dimensionIndex)
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x6000000000000000ull + static_cast<uint64_t>(styleIndex) * 0x100000ull + static_cast<uint64_t>(dimensionIndex) * 0x1000ull;
                settings.Dimensions = dimensions[dimensionIndex];
                settings.Style = styles[styleIndex];
                settings.Faction = factions[(styleIndex + dimensionIndex) % factions.size()];
                ShipGenerationDebugInfo debug;
                const GeneratedShip ship = generator.generate(settings, &debug);
                ++generatedShips;
                if (!validateLiveryMasks(ship, debug)) { return false; }
                if (debug.LiveryMarkingCount > getShipGenerationProfile(settings.Style).MaximumLiveryMarkings)
                {
                    std::cerr << "Livery count exceeds style profile maximum.\n";
                    return false;
                }
                if (settings.Dimensions.Width == 24u && settings.Dimensions.Height == 24u)
                {
                    if (debug.LiveryMarkingCount > 1u) { std::cerr << "24x24 ship received multiple large markings.\n"; return false; }
                    if (debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::DOUBLE_CENTER_STRIPE)] > 0u || debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::CHEVRON)] > 0u || debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::GEOMETRIC_INSIGNIA)] > 0u)
                    {
                        std::cerr << "24x24 ship received an overly complex livery type.\n";
                        return false;
                    }
                }
                if (debug.LiveryMarkingCount > 0u) { ++markedShips; }
            }
        }

        if (markedShips == 0u || markedShips == generatedShips)
        {
            std::cerr << "Livery density lost zero-or-one procedural variation.\n";
            return false;
        }
        return true;
    }

    bool checkCorporateAndStyleLanguage()
    {
        ShipGenerator generator;
        uint64_t corporateMarked = 0u;
        uint64_t ascendantMarked = 0u;
        uint64_t spearAxial = 0u;
        uint64_t spearMarked = 0u;
        uint64_t deltaLateral = 0u;
        uint64_t deltaMarked = 0u;

        for (uint64_t i = 0u; i < 80u; ++i)
        {
            for (const ShipFactionType faction : { ShipFactionType::CORPORATE, ShipFactionType::ASCENDANT })
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x60C0000000000000ull + i * 0x9E3779B97F4A7C15ull;
                settings.Dimensions = { 64u, 64u };
                settings.Style = ShipStyle::FIGHTER;
                settings.Faction = faction;
                ShipGenerationDebugInfo debug;
                (void)generator.generate(settings, &debug);
                if (debug.LiveryMarkingCount > 0u)
                {
                    if (faction == ShipFactionType::CORPORATE) { ++corporateMarked; }
                    else { ++ascendantMarked; }
                }
            }

            for (const ShipStyle style : { ShipStyle::SPEARHEAD, ShipStyle::DELTA })
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x60D0000000000000ull + i * 0xD1B54A32D192ED03ull;
                settings.Dimensions = { 64u, 64u };
                settings.Style = style;
                settings.Faction = ShipFactionType::CORPORATE;
                ShipGenerationDebugInfo debug;
                (void)generator.generate(settings, &debug);
                if (debug.LiveryMarkingCount == 0u) { continue; }
                if (style == ShipStyle::SPEARHEAD)
                {
                    ++spearMarked;
                    if (debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::CENTER_STRIPE)] > 0u || debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::DOUBLE_CENTER_STRIPE)] > 0u || debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::NOSE_BAND)] > 0u) { ++spearAxial; }
                }
                else
                {
                    ++deltaMarked;
                    if (debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::WING_BAND)] > 0u || debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::SHOULDER_BLOCK)] > 0u || debug.LiveryTypeCounts[static_cast<std::size_t>(ShipLiveryType::ID_PANEL)] > 0u) { ++deltaLateral; }
                }
            }
        }

        if (corporateMarked <= ascendantMarked + 8u)
        {
            std::cerr << "Corporate livery occurrence is not meaningfully stronger than Ascendant.\n";
            return false;
        }
        if (spearMarked >= 8u && spearAxial * 100u < spearMarked * 55u)
        {
            std::cerr << "SPEARHEAD livery does not preserve axial marking language.\n";
            return false;
        }
        if (deltaMarked >= 8u && deltaLateral * 100u < deltaMarked * 55u)
        {
            std::cerr << "DELTA livery does not use broad lateral surfaces sufficiently.\n";
            return false;
        }
        return true;
    }

    bool checkMilitarySymmetry()
    {
        ShipGenerator generator;
        uint64_t checked = 0u;
        for (uint64_t i = 0u; i < 60u; ++i)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x60E0000000000000ull + i * 0x94D049BB133111EBull;
            settings.Dimensions = { 64u, 64u };
            settings.Style = ShipStyle::FIGHTER;
            settings.Faction = ShipFactionType::MILITARY;
            ShipGenerationDebugInfo debug;
            (void)generator.generate(settings, &debug);
            if (debug.LiveryMarkingCount == 0u) { continue; }
            ++checked;
            PixelMask combined = debug.LiveryPrimaryMask;
            PixelMaskUtils::mergeMask(combined, debug.LiverySecondaryMask);
            if (!isSymmetric(combined))
            {
                std::cerr << "Military livery violated disciplined symmetric presentation.\n";
                return false;
            }
        }
        return checked > 5u;
    }
}

int PixelShipGeneratorTests::runLiveryRegression()
{
    bool success = true;
    success = checkDeterminismAndPaletteIsolation() && success;
    success = checkStylesFactionsAndDimensions() && success;
    success = checkCorporateAndStyleLanguage() && success;
    success = checkMilitarySymmetry() && success;
    if (!success) { return 1; }
    std::cout << "Ship livery regression passed.\n";
    return 0;
}
