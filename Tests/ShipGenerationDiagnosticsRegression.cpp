#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>

#include <PixelShipGenerator/ShipGenerationDebugInfo.h>
#include <PixelShipGenerator/ShipGenerationSettings.h>
#include <PixelShipGenerator/ShipGenerator.h>

namespace
{
    bool imagesEqual(const PixelShipGenerator::Image& first, const PixelShipGenerator::Image& second, uint32_t width, uint32_t height)
    {
        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (first.getPixel(x, y) != second.getPixel(x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool masksEqual(const PixelShipGenerator::PixelMask& first, const PixelShipGenerator::PixelMask& second)
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
}

int PixelShipGeneratorTests::runGenerationDiagnosticsRegression()
{
    PixelShipGenerator::ShipGenerator generator;
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };

    for (std::size_t index = 0u; index < Resolutions.size(); ++index)
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = 0x9E3779B97F4A7C15ull + static_cast<uint64_t>(index) * 0x10001ull;
        settings.Dimensions.Width = Resolutions[index];
        settings.Dimensions.Height = Resolutions[index];
        settings.Style = static_cast<PixelShipGenerator::ShipStyle>(index % static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END));
        settings.Faction = static_cast<PixelShipGenerator::ShipFactionType>(index % static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END));

        const PixelShipGenerator::GeneratedShip normalShip = generator.generate(settings);
        PixelShipGenerator::ShipGenerationDebugInfo debugInfo;
        const PixelShipGenerator::GeneratedShip diagnosticShip = generator.generate(settings, &debugInfo);

        if (!imagesEqual(normalShip.FinalImage, diagnosticShip.FinalImage, settings.Dimensions.Width, settings.Dimensions.Height))
        {
            std::cerr << "Diagnostic capture changed generated pixels at " << settings.Dimensions.Width << "x" << settings.Dimensions.Height << ".\n";
            return 1;
        }

        if (debugInfo.HullGenerationAttemptCount == 0u || debugInfo.HullStages.size() != 5u)
        {
            std::cerr << "Diagnostic hull metadata missing at " << settings.Dimensions.Width << "x" << settings.Dimensions.Height << ".\n";
            return 1;
        }

        if (!debugInfo.HasSurfaceDetailProfile)
        {
            std::cerr << "Resolved detail profile was not captured.\n";
            return 1;
        }

        if (!masksEqual(debugInfo.HullStages.back().HullMask, diagnosticShip.HullMask))
        {
            std::cerr << "Final captured hull stage does not match the generated hull.\n";
            return 1;
        }

        for (const PixelShipGenerator::ShipGenerationDebugStage& stage : debugInfo.HullStages)
        {
            if (stage.HullMask.getWidth() != settings.Dimensions.Width || stage.HullMask.getHeight() != settings.Dimensions.Height)
            {
                std::cerr << "Diagnostic stage dimensions are incorrect.\n";
                return 1;
            }
        }
    }

    std::cout << "Ship generation diagnostic regression passed.\n";
    return 0;
}
