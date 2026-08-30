#include "RegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <utility>

#include "GenerationComplexityBudget.h"
#include "GenerationDomain.h"
#include "CockpitGenerator.h"
#include "CoreTreatmentGenerator.h"
#include "EngineGenerator.h"
#include "HullGenerator.h"
#include "MacroAsymmetryPlanner.h"
#include "ShipGenerationContext.h"
#include "ShipGenerationProfile.h"
#include "ShipPainter.h"
#include "ShipPaletteGenerator.h"
#include "PixelMask.h"
#include "ShipCoreTreatmentType.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{
    using namespace PixelShipGenerator;

    uint32_t countPixels(const PixelMask& mask)
    {
        uint32_t count = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x) { if (mask.get(x, y)) { ++count; } }
        }
        return count;
    }

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

    uint64_t hashImage(const GeneratedShip& ship)
    {
        uint64_t hash = 14695981039346656037ull;
        for (const Color& color : ship.FinalImage.getPixels())
        {
            for (uint8_t value : { color.R, color.G, color.B, color.A }) { hash ^= value; hash *= 1099511628211ull; }
        }
        return hash;
    }

    ShipGenerationSettings makeSettings(uint64_t seed, ShipDimensions dimensions, ShipStyle style, ShipFactionType faction)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        settings.AttachmentsEnabled = true;
        return settings;
    }

    bool validateMasks(const GeneratedShip& ship, const ShipGenerationDebugInfo& debug)
    {
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();
        const PixelMask* masks[] = { &debug.CoreRegionMask, &debug.CoreRaisedMask, &debug.CoreRecessedMask, &debug.CoreSecondaryMaterialMask, &debug.CoreLuminousMask };
        for (const PixelMask* mask : masks)
        {
            if (mask->getWidth() != width || mask->getHeight() != height) { return false; }
        }
        if (countPixels(debug.CoreRegionMask) != debug.CoreRegionPixelCount) { return false; }
        if (countPixels(debug.CoreRaisedMask) != debug.CoreRaisedPixelCount) { return false; }
        if (countPixels(debug.CoreRecessedMask) != debug.CoreRecessedPixelCount) { return false; }
        if (countPixels(debug.CoreSecondaryMaterialMask) != debug.CoreSecondaryMaterialPixelCount) { return false; }
        if (countPixels(debug.CoreLuminousMask) != debug.CoreLuminousPixelCount) { return false; }

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (debug.CoreRegionMask.get(x, y) && !ship.HullMask.get(x, y)) { return false; }
                const bool treatment = debug.CoreRaisedMask.get(x, y) || debug.CoreRecessedMask.get(x, y) || debug.CoreSecondaryMaterialMask.get(x, y) || debug.CoreLuminousMask.get(x, y);
                if (!treatment) { continue; }
                if (!debug.CoreRegionMask.get(x, y) || !ship.HullMask.get(x, y)) { return false; }
                if (ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y)) { return false; }
            }
        }
        return true;
    }
    bool validateIsolatedCorePainting()
    {
        ShipGenerationSettings settings = makeSettings(0x5300000000400001ull, { 64u, 64u }, ShipStyle::HEAVY, ShipFactionType::MILITARY);
        const ShipGenerationSeeds seeds = deriveShipGenerationSeeds(settings.Seed);
        const ShipGenerationProfile& profile = getShipGenerationProfile(settings.Style);
        ShipGenerationDebugInfo debug;
        ShipGenerationContext context(settings, profile, seeds, &debug);
        context.Ship.Palette = ShipPaletteGenerator::generate(context.DomainSeeds.get(GenerationDomain::PALETTE), settings.Faction, profile);
        HullGenerator hullGenerator;
        CockpitGenerator cockpitGenerator;
        EngineGenerator engineGenerator;
        CoreTreatmentGenerator coreGenerator;
        MacroAsymmetryPlanner asymmetryPlanner;
        ShipPainter painter;

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
            engineGenerator.generate(context);
            coreGenerator.generate(context);
            if (context.CoreTreatment.empty()) { continue; }
            painter.paint(context);
            for (uint32_t y = 0u; y < context.Ship.HullMask.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < context.Ship.HullMask.getWidth(); ++x)
                {
                    const bool corePixel = context.CoreTreatment.RaisedMask.get(x, y) || context.CoreTreatment.RecessedMask.get(x, y) || context.CoreTreatment.SecondaryMaterialMask.get(x, y) || context.CoreTreatment.LuminousMask.get(x, y);
                    if (corePixel && context.Ship.FinalImage.getPixel(x, y) == context.Ship.Palette.Outline) { return false; }
                }
            }
            return true;
        }
        return false;
    }

}

int PixelShipGeneratorTests::runCoreTreatmentRegression()
{
    using namespace PixelShipGenerator;
    ShipGenerator generator;
    bool success = true;

    constexpr std::array<ShipDimensions, 10u> Dimensions =
    { {
        {24u, 24u}, {32u, 32u}, {44u, 44u}, {48u, 64u}, {64u, 48u},
        {64u, 64u}, {96u, 96u}, {160u, 160u}, {64u, 96u}, {96u, 64u}
    } };
    const std::array<ShipStyle, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
    const std::array<ShipFactionType, static_cast<std::size_t>(ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };

    std::array<uint32_t, static_cast<std::size_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)> observedTypes = {};
    uint32_t smallCount = 0u;
    uint32_t mediumCount = 0u;
    uint32_t largeCount = 0u;
    uint32_t treatmentShips = 0u;

    for (std::size_t dimensionIndex = 0u; dimensionIndex < Dimensions.size(); ++dimensionIndex)
    {
        for (uint32_t sample = 0u; sample < 32u; ++sample)
        {
            const uint64_t seed = 0x53C0AE0000000000ull ^ (static_cast<uint64_t>(dimensionIndex) << 40u) ^ (static_cast<uint64_t>(sample) * 0x9E3779B97F4A7C15ull);
            const ShipGenerationSettings settings = makeSettings(seed, Dimensions[dimensionIndex], Styles[sample % Styles.size()], Factions[(sample / Styles.size()) % Factions.size()]);
            ShipGenerationDebugInfo firstDebug;
            ShipGenerationDebugInfo secondDebug;
            try
            {
                const GeneratedShip first = generator.generate(settings, &firstDebug);
                const GeneratedShip second = generator.generate(settings, &secondDebug);
                if (hashImage(first) != hashImage(second) || firstDebug.CoreTreatmentTypeCounts != secondDebug.CoreTreatmentTypeCounts || firstDebug.CoreTreatmentCount != secondDebug.CoreTreatmentCount)
                {
                    std::cerr << "Core treatment is not deterministic at " << Dimensions[dimensionIndex].Width << 'x' << Dimensions[dimensionIndex].Height << ".\n";
                    success = false;
                }
                if (!validateMasks(first, firstDebug))
                {
                    std::cerr << "Invalid core-treatment masks at " << Dimensions[dimensionIndex].Width << 'x' << Dimensions[dimensionIndex].Height << ".\n";
                    success = false;
                }

                if (firstDebug.CoreTreatmentCount > 0u)
                {
                    ++treatmentShips;
                    const std::size_t layerBudget = static_cast<std::size_t>(GenerationComplexityCategory::HULL_LAYER);
                    if (firstDebug.CoreTreatmentComplexityCost == 0u || firstDebug.ComplexityCategoryConsumed[layerBudget] < firstDebug.CoreTreatmentComplexityCost)
                    {
                        std::cerr << "Core treatment did not consume the shared hull-layer complexity budget.\n";
                        success = false;
                    }
                    const uint32_t centralLoad = firstDebug.SpatialRegionLoads[static_cast<std::size_t>(GenerationSpatialRegion::FRONT_FUSELAGE)]
                        + firstDebug.SpatialRegionLoads[static_cast<std::size_t>(GenerationSpatialRegion::MID_FUSELAGE)]
                        + firstDebug.SpatialRegionLoads[static_cast<std::size_t>(GenerationSpatialRegion::REAR_FUSELAGE)];
                    if (centralLoad == 0u)
                    {
                        std::cerr << "Core treatment did not contribute semantic central load.\n";
                        success = false;
                    }
                }

                for (std::size_t index = 0u; index < observedTypes.size(); ++index) { observedTypes[index] += firstDebug.CoreTreatmentTypeCounts[index]; }
                if (Dimensions[dimensionIndex].Width == 24u && Dimensions[dimensionIndex].Height == 24u) { smallCount += firstDebug.CoreTreatmentCount; }
                if (Dimensions[dimensionIndex].Width == 64u && Dimensions[dimensionIndex].Height == 64u) { mediumCount += firstDebug.CoreTreatmentCount; }
                if (Dimensions[dimensionIndex].Width == 160u && Dimensions[dimensionIndex].Height == 160u) { largeCount += firstDebug.CoreTreatmentCount; }
            }
            catch (const std::exception& exception)
            {
                std::cerr << "Core-treatment sample failed: " << exception.what() << '\n';
                success = false;
            }
        }
    }

    if (treatmentShips == 0u) { std::cerr << "No core treatments were generated.\n"; success = false; }
    if (!(smallCount <= mediumCount && mediumCount <= largeCount))
    {
        std::cerr << "Core complexity did not progress with scale: " << smallCount << " -> " << mediumCount << " -> " << largeCount << ".\n";
        success = false;
    }
    uint32_t observedTypeCount = 0u;
    for (uint32_t count : observedTypes) { if (count > 0u) { ++observedTypeCount; } }
    if (observedTypeCount < 5u)
    {
        std::cerr << "Core treatment variation is too narrow (" << observedTypeCount << " types observed).\n";
        success = false;
    }

    ShipGenerationSettings baseSettings = makeSettings(0x5300C0DEC0FFEE00ull, { 64u, 64u }, ShipStyle::HEAVY, ShipFactionType::MILITARY);
    ShipGenerationDebugInfo baseDebug;
    const GeneratedShip base = generator.generate(baseSettings, &baseDebug);
    const ShipGenerationSeeds topSeeds = deriveShipGenerationSeeds(baseSettings.Seed);
    const GenerationDomainSeeds domains = resolveGenerationDomainSeeds(topSeeds, baseSettings.DomainSeedOverrides, baseSettings.RandomStreamMode);
    ShipGenerationSettings rerolledSettings = baseSettings;
    rerolledSettings.DomainSeedOverrides.set(GenerationDomain::HULL_LAYERS, deriveGenerationDomainRerollSeed(0x5353535353535353ull, GenerationDomain::HULL_LAYERS, domains.get(GenerationDomain::HULL_LAYERS)));
    ShipGenerationDebugInfo rerolledDebug;
    const GeneratedShip rerolled = generator.generate(rerolledSettings, &rerolledDebug);
    if (!masksEqual(base.HullMask, rerolled.HullMask) || !masksEqual(base.CockpitMask, rerolled.CockpitMask) || !masksEqual(base.EngineMask, rerolled.EngineMask))
    {
        std::cerr << "Hull-layer/core reroll changed upstream Hull/Cockpit/Engine geometry.\n";
        success = false;
    }
    const GeneratedShip rerolledAgain = generator.generate(rerolledSettings);
    if (rerolled.FinalImage.getPixels() != rerolledAgain.FinalImage.getPixels())
    {
        std::cerr << "Hull-layer/core reroll is not reproducible.\n";
        success = false;
    }
    if (!validateIsolatedCorePainting())
    {
        std::cerr << "Core treatment introduced an internal outline artifact in isolated painting.\n";
        success = false;
    }

    return success ? 0 : 1;
}
