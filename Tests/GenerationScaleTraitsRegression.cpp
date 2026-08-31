#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <vector>

#include <SpectralShipGen/GenerationScaleTraits.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    using namespace SpectralShipGen;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    double collectComplexityAverage(uint32_t width, uint32_t height)
    {
        ShipGenerator generator;
        double total = 0.0;
        constexpr uint32_t Samples = 32u;

        for (uint32_t sample = 0u; sample < Samples; ++sample)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x9E3779B97F4A7C15ull ^ (static_cast<uint64_t>(sample) * 0xBF58476D1CE4E5B9ull);
            settings.Dimensions = { width, height };
            settings.Style = ShipStyle::INDUSTRIAL;
            settings.Faction = ShipFactionType::FRONTIER;
            settings.DetailDensity = 65u;
            settings.AttachmentsEnabled = true;

            ShipGenerationDebugInfo debugInfo;
            const GeneratedShip ship = generator.generate(settings, &debugInfo);
            const double detailPatterns = static_cast<double>(debugInfo.AccentPatternCount + debugInfo.MechanicalPatternCount + debugInfo.LightPatternCount);
            total += static_cast<double>(debugInfo.AppliedHullModifiers.size()) * 2.0;
            total += static_cast<double>(debugInfo.MajorFeatureCount) * 3.0;
            total += static_cast<double>(debugInfo.WeaponCount) * 2.0;
            total += static_cast<double>(debugInfo.AttachmentPlacedGroupCount) * 2.0;
            total += detailPatterns * 0.5;

            if (debugInfo.ScaleTraits.Dimensions != settings.Dimensions || debugInfo.ScaleTraits.CanvasArea != static_cast<uint64_t>(width) * height)
            {
                throw std::runtime_error("Generation debug scale traits do not match requested dimensions.");
            }

            if (ship.FinalImage.getWidth() != width || ship.FinalImage.getHeight() != height)
            {
                throw std::runtime_error("Scale regression generated an unexpected image size.");
            }
        }

        return total / static_cast<double>(Samples);
    }
}

int SpectralShipGenTests::runGenerationScaleTraitsRegression()
{
    using namespace SpectralShipGen;

    try
    {
        constexpr std::array<uint32_t, 6u> SquareDimensions = { 24u, 32u, 44u, 64u, 96u, 160u };
        uint32_t previousCapacity = 0u;

        for (std::size_t index = 0u; index < SquareDimensions.size(); ++index)
        {
            const uint32_t dimension = SquareDimensions[index];
            const GenerationScaleTraits traits = GenerationScaleTraits::fromDimensions({ dimension, dimension });

            if (traits.MinimumDimension != dimension || traits.MaximumDimension != dimension || traits.CanvasArea != static_cast<uint64_t>(dimension) * dimension || traits.AspectRatio != 1.0)
            {
                std::cerr << "Incorrect square scale traits for " << dimension << "x" << dimension << ".\n";
                return 1;
            }

            if (traits.HorizontalCapacity != traits.LongitudinalCapacity || traits.SmallFeatureCapacity != traits.HorizontalCapacity)
            {
                std::cerr << "Square scale traits are not axis-balanced.\n";
                return 1;
            }

            if (index > 0u && traits.SmallFeatureCapacity <= previousCapacity)
            {
                std::cerr << "Square scale capacity did not increase progressively.\n";
                return 1;
            }

            previousCapacity = traits.SmallFeatureCapacity;
        }

        const GenerationScaleTraits wide = GenerationScaleTraits::fromDimensions({ 64u, 32u });
        const GenerationScaleTraits tall = GenerationScaleTraits::fromDimensions({ 32u, 64u });

        if (wide.HorizontalCapacity <= wide.LongitudinalCapacity || tall.LongitudinalCapacity <= tall.HorizontalCapacity)
        {
            std::cerr << "Rectangular scale traits are not axis-aware.\n";
            return 1;
        }

        if (wide.SmallFeatureCapacity != tall.SmallFeatureCapacity || wide.MajorFeatureCapacity != tall.MajorFeatureCapacity || wide.CanvasArea != tall.CanvasArea)
        {
            std::cerr << "Transposed dimensions should share scalar readability capacities.\n";
            return 1;
        }

        if (wide.AspectRatio <= 1.0 || tall.AspectRatio >= 1.0)
        {
            std::cerr << "Rectangular aspect ratios are incorrect.\n";
            return 1;
        }

        const double tinyComplexity = collectComplexityAverage(24u, 24u);
        const double mediumComplexity = collectComplexityAverage(64u, 64u);
        const double largeComplexity = collectComplexityAverage(160u, 160u);

        if (!(tinyComplexity < mediumComplexity && mediumComplexity < largeComplexity))
        {
            std::cerr << "Generated semantic complexity is not progressive: " << tinyComplexity << ", " << mediumComplexity << ", " << largeComplexity << ".\n";
            return 1;
        }

        constexpr std::array<ShipDimensions, 6u> RectangularDimensions =
        { {
            { 32u, 44u }, { 44u, 32u }, { 48u, 64u }, { 64u, 48u }, { 64u, 96u }, { 96u, 64u }
        } };

        ShipGenerator generator;
        for (std::size_t index = 0u; index < RectangularDimensions.size(); ++index)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0xD6E8FEB86659FD93ull ^ static_cast<uint64_t>(index);
            settings.Dimensions = RectangularDimensions[index];
            settings.Style = static_cast<ShipStyle>(index % static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END));
            settings.Faction = static_cast<ShipFactionType>(index % static_cast<std::size_t>(ShipFactionType::SHIP_FACTION_TYPE_END));

            ShipGenerationDebugInfo firstDebug;
            ShipGenerationDebugInfo secondDebug;
            const GeneratedShip first = generator.generate(settings, &firstDebug);
            const GeneratedShip second = generator.generate(settings, &secondDebug);

            if (!imagesEqual(first.FinalImage, second.FinalImage))
            {
                std::cerr << "Scale-aware generation is not deterministic.\n";
                return 1;
            }

            if (firstDebug.ScaleTraits.HorizontalCapacity != GenerationScaleTraits::fromDimensions(settings.Dimensions).HorizontalCapacity || firstDebug.ScaleTraits.LongitudinalCapacity != GenerationScaleTraits::fromDimensions(settings.Dimensions).LongitudinalCapacity)
            {
                std::cerr << "Rectangular debug traits do not match derived traits.\n";
                return 1;
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Generation scale regression failed: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
