#include "RegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"
#include "SilhouetteQualityMetrics.h"

namespace
{
    using namespace PixelShipGenerator;

    constexpr std::array<ShipDimensions, 7u> Dimensions = { { {24u,24u}, {32u,32u}, {44u,44u}, {48u,64u}, {64u,48u}, {64u,64u}, {96u,96u} } };
    constexpr std::array<uint64_t, 7u> SpearheadRgbHashes = { {
        0x528750c29806f4c7ull, 0x747d3c0ac7cf1e79ull, 0x3aa32565a403baa7ull, 0xc99d5a85f88b6145ull,
        0x34079b708fa3e800ull, 0x94faad2e64a63204ull, 0xc22da157d70dfd86ull
    } };

    uint64_t rgbHash(const Image& image)
    {
        uint64_t hash = 1469598103934665603ull;
        for (const Color& color : image.getPixels())
        {
            for (uint8_t value : { color.R, color.G, color.B })
            {
                hash ^= value;
                hash *= 1099511628211ull;
            }
        }
        return hash;
    }

    void setSymmetricRow(PixelMask& mask, uint32_t y, uint32_t width)
    {
        const uint32_t left = (mask.getWidth() - width) / 2u;
        for (uint32_t x = left; x < left + width; ++x) { mask.set(x, y, true); }
    }

    bool metricsRegression()
    {
        PixelMask blob(32u, 32u, false);
        PixelMask articulated(32u, 32u, false);
        for (uint32_t y = 4u; y < 28u; ++y)
        {
            const uint32_t local = y - 4u;
            const uint32_t blobWidth = local <= 11u ? 4u + local : 4u + (23u - local);
            setSymmetricRow(blob, y, blobWidth | 1u);

            uint32_t width = 5u;
            if (local < 5u) width = 5u + local * 2u;
            else if (local < 9u) width = 19u;
            else if (local < 13u) width = 11u;
            else if (local < 18u) width = 23u;
            else width = 17u - (local - 18u) * 2u;
            setSymmetricRow(articulated, y, std::max(3u, width | 1u));
        }

        const auto first = calculateSilhouetteQualityMetrics(blob);
        const auto repeat = calculateSilhouetteQualityMetrics(blob);
        const auto shaped = calculateSilhouetteQualityMetrics(articulated);
        return first.ArticulationCount == repeat.ArticulationCount && first.BoundingFillPercent == repeat.BoundingFillPercent &&
            shaped.ArticulationCount > first.ArticulationCount && shaped.InteriorContractionPercent > first.InteriorContractionPercent;
    }

    bool isSymmetric(const PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                if (mask.get(x, y) != mask.get(mask.getWidth() - 1u - x, y)) return false;
        return true;
    }
}

int PixelShipGeneratorTests::runSilhouetteArticulationRegression()
{
    using namespace PixelShipGenerator;
    if (!metricsRegression()) { std::cerr << "Silhouette metric articulation regression failed.\n"; return 1; }

    ShipGenerator generator;
    for (uint32_t styleIndex = 0u; styleIndex < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END); ++styleIndex)
    {
        for (std::size_t dimensionIndex = 0u; dimensionIndex < Dimensions.size(); ++dimensionIndex)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x5600000000000000ull + styleIndex * 0x100000ull + dimensionIndex;
            settings.Dimensions = Dimensions[dimensionIndex];
            settings.Style = static_cast<ShipStyle>(styleIndex);
            settings.Faction = static_cast<ShipFactionType>((styleIndex + dimensionIndex) % static_cast<uint32_t>(ShipFactionType::SHIP_FACTION_TYPE_END));
            ShipGenerationDebugInfo firstDebug;
            ShipGenerationDebugInfo secondDebug;
            const GeneratedShip first = generator.generate(settings, &firstDebug);
            const GeneratedShip second = generator.generate(settings, &secondDebug);
            if (first.FinalImage.getPixels() != second.FinalImage.getPixels() || firstDebug.SilhouetteMetrics.ArticulationCount != secondDebug.SilhouetteMetrics.ArticulationCount || firstDebug.SilhouetteValidationFailureCounts != secondDebug.SilhouetteValidationFailureCounts || !isSymmetric(first.HullMask))
            {
                std::cerr << "Deterministic/symmetric silhouette regression failed.\n"; return 1;
            }
        }
    }

    for (std::size_t index = 0u; index < Dimensions.size(); ++index)
    {
        ShipGenerationSettings settings;
        settings.Seed = 0x7A56D3914B8C102Full;
        settings.Dimensions = Dimensions[index];
        settings.Style = ShipStyle::SPEARHEAD;
        settings.Faction = ShipFactionType::MILITARY;
        const GeneratedShip ship = generator.generate(settings);
        if (rgbHash(ship.FinalImage) != SpearheadRgbHashes[index]) { std::cerr << "SPEARHEAD Task-54 fixture changed at review dimension.\n"; return 1; }
    }

    constexpr uint32_t Samples = 48u;
    std::array<double, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> averageWidth = {};
    std::array<double, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> averageHeight = {};
    std::array<double, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> averageArticulation = {};
    for (uint32_t styleIndex = 0u; styleIndex < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END); ++styleIndex)
    {
        uint64_t rejections = 0u;
        for (uint32_t sample = 0u; sample < Samples; ++sample)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x56A11E0000000000ull + styleIndex * 0x10000ull + sample;
            settings.Dimensions = { 64u,64u };
            settings.Style = static_cast<ShipStyle>(styleIndex);
            settings.Faction = static_cast<ShipFactionType>(sample % static_cast<uint32_t>(ShipFactionType::SHIP_FACTION_TYPE_END));
            ShipGenerationDebugInfo debug;
            generator.generate(settings, &debug);
            rejections += debug.HullValidationRejectionCount;
            averageWidth[styleIndex] += debug.SilhouetteMetrics.NormalizedWidthPercent;
            averageHeight[styleIndex] += debug.SilhouetteMetrics.NormalizedHeightPercent;
            averageArticulation[styleIndex] += debug.SilhouetteMetrics.ArticulationCount;
        }
        averageWidth[styleIndex] /= Samples;
        averageHeight[styleIndex] /= Samples;
        averageArticulation[styleIndex] /= Samples;
        if (rejections * 100u > Samples * 20u) { std::cerr << "Pathological silhouette rejection rate detected.\n"; return 1; }
    }

    if (!(averageHeight[static_cast<std::size_t>(ShipStyle::SPEARHEAD)] > averageHeight[static_cast<std::size_t>(ShipStyle::FIGHTER)] + 5.0 &&
        averageWidth[static_cast<std::size_t>(ShipStyle::DELTA)] > averageWidth[static_cast<std::size_t>(ShipStyle::HEAVY)] + 4.0 &&
        averageArticulation[static_cast<std::size_t>(ShipStyle::INDUSTRIAL)] >= averageArticulation[static_cast<std::size_t>(ShipStyle::SLEEK)]))
    {
        std::cerr << "Style-specific silhouette utilization/articulation regressed.\n"; return 1;
    }

    std::cout << "Task 56 silhouette articulation regression passed.\n";
    return 0;
}
