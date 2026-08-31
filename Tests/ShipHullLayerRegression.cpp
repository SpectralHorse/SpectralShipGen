#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>

#include <PixelShipGenerator/GenerationComplexityBudget.h>
#include <PixelShipGenerator/PixelMask.h>
#include <PixelShipGenerator/ShipGenerationDebugInfo.h>
#include <PixelShipGenerator/ShipGenerationSettings.h>
#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipHullLayerType.h>

namespace
{
    uint32_t countPixels(const PixelShipGenerator::PixelMask& mask)
    {
        uint32_t count = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x) { if (mask.get(x, y)) { ++count; } }
        }
        return count;
    }

    uint64_t hashImage(const PixelShipGenerator::GeneratedShip& ship)
    {
        uint64_t hash = 14695981039346656037ull;
        for (const PixelShipGenerator::Color& color : ship.FinalImage.getPixels())
        {
            for (uint8_t value : { color.R, color.G, color.B, color.A }) { hash ^= value; hash *= 1099511628211ull; }
        }
        return hash;
    }

    bool validateLayerGeometry(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipGenerationDebugInfo& debug)
    {
        if (debug.HullLayerMask.getWidth() != ship.HullMask.getWidth() || debug.HullLayerMask.getHeight() != ship.HullMask.getHeight()) { return debug.HullLayerCount == 0u; }
        if (debug.HullLayerUpperMask.getWidth() != ship.HullMask.getWidth() || debug.HullLayerUpperMask.getHeight() != ship.HullMask.getHeight()) { return false; }
        if (debug.HullLayerLowerCount + debug.HullLayerUpperCount != debug.HullLayerCount) { return false; }
        if (countPixels(debug.HullLayerMask) != debug.HullLayerPixelCount) { return false; }
        if (countPixels(debug.HullLayerUpperMask) != debug.HullLayerUpperPixelCount) { return false; }
        if (debug.HullLayerLowerPixelCount > debug.HullLayerPixelCount || debug.HullLayerUpperPixelCount > debug.HullLayerPixelCount) { return false; }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (!debug.HullLayerMask.get(x, y)) { continue; }
                if (!ship.HullMask.get(x, y)) { return false; }
                if (ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y)) { return false; }
                if (ship.FinalImage.getPixel(x, y) == ship.Palette.Outline) { return false; }
            }
        }
        return true;
    }

    PixelShipGenerator::ShipGenerationSettings makeSettings(uint64_t seed, uint32_t width, uint32_t height, PixelShipGenerator::ShipStyle style, PixelShipGenerator::ShipFactionType faction)
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = { width, height };
        settings.Style = style;
        settings.Faction = faction;
        settings.AttachmentsEnabled = true;
        return settings;
    }
}

int PixelShipGeneratorTests::runHullLayerRegression()
{
    using namespace PixelShipGenerator;
    ShipGenerator generator;
    bool success = true;

    constexpr std::array<std::pair<uint32_t, uint32_t>, 10u> Dimensions =
    { {
        { 24u, 24u }, { 32u, 32u }, { 44u, 44u }, { 64u, 64u }, { 96u, 96u }, { 160u, 160u },
        { 32u, 44u }, { 44u, 32u }, { 64u, 96u }, { 96u, 64u }
    } };

    uint32_t smallLayerTotal = 0u;
    uint32_t mediumLayerTotal = 0u;
    uint32_t largeLayerTotal = 0u;
    std::array<uint32_t, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)> observedTypes = {};
    uint32_t generatedLayerShips = 0u;

    for (std::size_t dimensionIndex = 0u; dimensionIndex < Dimensions.size(); ++dimensionIndex)
    {
        const auto [width, height] = Dimensions[dimensionIndex];
        for (uint32_t sample = 0u; sample < 32u; ++sample)
        {
            const ShipStyle style = static_cast<ShipStyle>(sample % static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END));
            const ShipFactionType faction = static_cast<ShipFactionType>((sample / 4u) % static_cast<uint32_t>(ShipFactionType::SHIP_FACTION_TYPE_END));
            const uint64_t seed = 0xA54FF53A5F1D36F1ull ^ (static_cast<uint64_t>(dimensionIndex) << 40u) ^ (static_cast<uint64_t>(sample) * 0x9E3779B97F4A7C15ull);
            const ShipGenerationSettings settings = makeSettings(seed, width, height, style, faction);
            ShipGenerationDebugInfo firstDebug;
            ShipGenerationDebugInfo secondDebug;

            try
            {
                const GeneratedShip first = generator.generate(settings, &firstDebug);
                const GeneratedShip second = generator.generate(settings, &secondDebug);
                if (hashImage(first) != hashImage(second) || firstDebug.HullLayerCount != secondDebug.HullLayerCount || firstDebug.HullLayerTypeCounts != secondDebug.HullLayerTypeCounts)
                {
                    std::cerr << width << 'x' << height << " sample " << sample << " is not deterministic.\n";
                    success = false;
                }
                if (!validateLayerGeometry(first, firstDebug))
                {
                    std::cerr << width << 'x' << height << " sample " << sample << " produced invalid hull-layer geometry.\n";
                    success = false;
                }
                if (firstDebug.HullLayerCount > 3u)
                {
                    std::cerr << width << 'x' << height << " exceeded the maximum layer count.\n";
                    success = false;
                }

                const std::size_t layerCategory = static_cast<std::size_t>(GenerationComplexityCategory::HULL_LAYER);
                if (firstDebug.HullLayerCount > 0u)
                {
                    ++generatedLayerShips;
                    if (firstDebug.ComplexityCategoryConsumed[layerCategory] == 0u)
                    {
                        std::cerr << width << 'x' << height << " generated layers without consuming the global layer budget.\n";
                        success = false;
                    }
                    bool hasSpatialLoad = false;
                    for (uint32_t load : firstDebug.SpatialRegionLoads) { hasSpatialLoad = hasSpatialLoad || load > 0u; }
                    if (!hasSpatialLoad)
                    {
                        std::cerr << width << 'x' << height << " generated layers without contributing semantic spatial load.\n";
                        success = false;
                    }
                }

                for (std::size_t index = 0u; index < observedTypes.size(); ++index) { observedTypes[index] += firstDebug.HullLayerTypeCounts[index]; }
                if (width == 24u && height == 24u) { smallLayerTotal += firstDebug.HullLayerCount; }
                if (width == 64u && height == 64u) { mediumLayerTotal += firstDebug.HullLayerCount; }
                if (width == 160u && height == 160u) { largeLayerTotal += firstDebug.HullLayerCount; }
            }
            catch (const std::exception& exception)
            {
                std::cerr << width << 'x' << height << " sample " << sample << " failed: " << exception.what() << '\n';
                success = false;
            }
        }
    }

    if (!(smallLayerTotal <= mediumLayerTotal && mediumLayerTotal <= largeLayerTotal))
    {
        std::cerr << "Layer complexity did not progress with scale: " << smallLayerTotal << " -> " << mediumLayerTotal << " -> " << largeLayerTotal << '\n';
        success = false;
    }
    if (generatedLayerShips == 0u)
    {
        std::cerr << "No hull layers were generated in the regression sample.\n";
        success = false;
    }

    uint32_t observedTypeCount = 0u;
    for (uint32_t count : observedTypes) { if (count > 0u) { ++observedTypeCount; } }
    if (observedTypeCount < 4u)
    {
        std::cerr << "Hull-layer variation is too narrow in the regression sample (" << observedTypeCount << " types observed).\n";
        success = false;
    }

    return success ? 0 : 1;
}
