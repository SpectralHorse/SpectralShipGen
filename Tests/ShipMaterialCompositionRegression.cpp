#include "CoreRegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "PixelMaskUtils.h"
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipMaterialZoneType.h>

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

    bool validateMaterialMasks(const GeneratedShip& ship, const ShipGenerationDebugInfo& debug)
    {
        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                const bool material = debug.MaterialSecondaryHullMask.get(x, y) || debug.MaterialMechanicalMask.get(x, y);
                if (!material) { continue; }
                if (!ship.HullMask.get(x, y)) { std::cerr << "Material zone escaped HullMask.\n"; return false; }
                if (ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y) || debug.WeaponOccupiedMask.get(x, y))
                {
                    std::cerr << "Material zone overlaps a component mask.\n";
                    return false;
                }
                if (debug.CoreRaisedMask.get(x, y) || debug.CoreRecessedMask.get(x, y) || debug.CoreSecondaryMaterialMask.get(x, y) || debug.CoreLuminousMask.get(x, y) || debug.HullLayerMask.get(x, y))
                {
                    std::cerr << "Material zone overlaps authoritative Core/HullLayer material geometry.\n";
                    return false;
                }
            }
        }
        if (PixelMaskUtils::masksOverlap(debug.MaterialSecondaryHullMask, debug.MaterialMechanicalMask))
        {
            std::cerr << "Secondary and mechanical material masks overlap.\n";
            return false;
        }
        return true;
    }

    bool checkDeterminismAndPaletteIsolation()
    {
        ShipGenerator generator;
        ShipGenerationSettings settings;
        settings.Seed = 0x59A11C0DE1234567ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::DELTA;
        settings.Faction = ShipFactionType::CORPORATE;
        settings.AttachmentsEnabled = true;

        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        const GeneratedShip first = generator.generate(settings, &firstDebug);
        const GeneratedShip second = generator.generate(settings, &secondDebug);
        if (!masksEqual(firstDebug.MaterialSecondaryHullMask, secondDebug.MaterialSecondaryHullMask) || !masksEqual(firstDebug.MaterialMechanicalMask, secondDebug.MaterialMechanicalMask) || first.FinalImage.getPixels() != second.FinalImage.getPixels())
        {
            std::cerr << "Material composition is not deterministic.\n";
            return false;
        }
        if (!validateMaterialMasks(first, firstDebug)) { return false; }

        const GenerationDomainSeeds domains = resolveGenerationDomainSeeds(deriveShipGenerationSeeds(settings.Seed), settings.DomainSeedOverrides, settings.RandomStreamMode);
        ShipGenerationSettings rerolled = settings;
        rerolled.DomainSeedOverrides.set(GenerationDomain::PALETTE, deriveGenerationDomainRerollSeed(0x59B22D1EF2345678ull, GenerationDomain::PALETTE, domains.get(GenerationDomain::PALETTE)));
        ShipGenerationDebugInfo paletteDebug;
        const GeneratedShip paletteShip = generator.generate(rerolled, &paletteDebug);

        if (!masksEqual(first.HullMask, paletteShip.HullMask) || !masksEqual(first.CockpitMask, paletteShip.CockpitMask) || !masksEqual(first.EngineMask, paletteShip.EngineMask) || !masksEqual(first.EngineExhaustMask, paletteShip.EngineExhaustMask) || !masksEqual(firstDebug.MaterialSecondaryHullMask, paletteDebug.MaterialSecondaryHullMask) || !masksEqual(firstDebug.MaterialMechanicalMask, paletteDebug.MaterialMechanicalMask))
        {
            std::cerr << "Palette reroll changed structural/material-zone geometry.\n";
            return false;
        }
        if (first.Palette.HullBase == paletteShip.Palette.HullBase && first.Palette.HullSecondary == paletteShip.Palette.HullSecondary)
        {
            std::cerr << "Palette reroll failed to recolor the material vocabulary.\n";
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
        uint64_t materialShips = 0u;
        uint64_t generatedShips = 0u;

        for (std::size_t styleIndex = 0u; styleIndex < styles.size(); ++styleIndex)
        {
            for (std::size_t dimensionIndex = 0u; dimensionIndex < dimensions.size(); ++dimensionIndex)
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x5900000000000000ull + static_cast<uint64_t>(styleIndex) * 0x100000ull + static_cast<uint64_t>(dimensionIndex) * 0x1000ull;
                settings.Dimensions = dimensions[dimensionIndex];
                settings.Style = styles[styleIndex];
                settings.Faction = factions[(styleIndex + dimensionIndex) % factions.size()];
                ShipGenerationDebugInfo debug;
                const GeneratedShip ship = generator.generate(settings, &debug);
                ++generatedShips;
                if (!validateMaterialMasks(ship, debug)) { return false; }
                if (debug.MaterialZoneCount > getShipGenerationProfile(settings.Style).MaximumMaterialZones)
                {
                    std::cerr << "Material zone count exceeds style profile maximum.\n";
                    return false;
                }
                if (settings.Dimensions.Width == 24u && debug.MaterialZoneCount > 1u)
                {
                    std::cerr << "Tiny ship received excessive material zoning.\n";
                    return false;
                }
                if (debug.MaterialZoneCount > 0u) { ++materialShips; }
            }
        }
        if (materialShips < generatedShips / 2u)
        {
            std::cerr << "Material zoning is too rare to affect whole-ship composition.\n";
            return false;
        }
        return true;
    }

    bool checkAnchorReinforcement()
    {
        ShipGenerator generator;
        uint64_t cockpitAnchors = 0u;
        uint64_t cockpitCollars = 0u;
        uint64_t wingAnchors = 0u;
        uint64_t wingZones = 0u;
        uint64_t engineAnchors = 0u;
        uint64_t rearZones = 0u;
        uint64_t weaponAnchors = 0u;
        uint64_t hardpointZones = 0u;

        for (uint64_t seedIndex = 0u; seedIndex < 320u; ++seedIndex)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x59C0000000000000ull + seedIndex * 0x9E3779B97F4A7C15ull;
            settings.Dimensions = { 64u, 64u };
            settings.Style = ShipStyle::FIGHTER;
            settings.Faction = ShipFactionType::MILITARY;
            ShipGenerationDebugInfo debug;
            (void)generator.generate(settings, &debug);
            const auto count = [&](ShipMaterialZoneType type) { return debug.MaterialZoneTypeCounts[static_cast<std::size_t>(type)] > 0u; };
            if (debug.PrimaryVisualAnchor == ShipVisualAnchorType::COCKPIT) { ++cockpitAnchors; if (count(ShipMaterialZoneType::COCKPIT_COLLAR)) { ++cockpitCollars; } }
            if (debug.PrimaryVisualAnchor == ShipVisualAnchorType::WINGS) { ++wingAnchors; if (count(ShipMaterialZoneType::WING_SURFACE) || count(ShipMaterialZoneType::SHOULDER_SURFACE)) { ++wingZones; } }
            if (debug.PrimaryVisualAnchor == ShipVisualAnchorType::ENGINES) { ++engineAnchors; if (count(ShipMaterialZoneType::REAR_MECHANICAL)) { ++rearZones; } }
            if (debug.PrimaryVisualAnchor == ShipVisualAnchorType::WEAPONS) { ++weaponAnchors; if (count(ShipMaterialZoneType::HARDPOINT_SURROUND)) { ++hardpointZones; } }
        }

        if ((cockpitAnchors >= 8u && cockpitCollars * 100u < cockpitAnchors * 35u) || (wingAnchors >= 8u && wingZones * 100u < wingAnchors * 40u) || (engineAnchors >= 8u && rearZones * 100u < engineAnchors * 30u) || (weaponAnchors >= 8u && hardpointZones * 100u < weaponAnchors * 30u))
        {
            std::cerr << "Material composition does not sufficiently reinforce Task-58 focal regions.\n";
            return false;
        }
        return true;
    }
}

int SpectralShipGenTests::runMaterialCompositionRegression()
{
    bool success = true;
    success = checkDeterminismAndPaletteIsolation() && success;
    success = checkStylesFactionsAndDimensions() && success;
    success = checkAnchorReinforcement() && success;
    if (!success) { return 1; }
    std::cout << "Ship material composition regression passed.\n";
    return 0;
}
