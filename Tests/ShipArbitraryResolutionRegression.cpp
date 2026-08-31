#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <vector>

#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipIdleAnimator.h>
#include <PixelShipGenerator/ShipSpritesheetUtils.h>

namespace
{
    constexpr std::array<uint32_t, 14u> Resolutions = { 24u, 26u, 28u, 30u, 36u, 40u, 42u, 48u, 56u, 72u, 80u, 112u, 192u, 256u };
    constexpr std::array<PixelShipGenerator::ShipStyle, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> Styles = { PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipStyle::DELTA };
    constexpr std::array<PixelShipGenerator::ShipFactionType, static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { PixelShipGenerator::ShipFactionType::FRONTIER, PixelShipGenerator::ShipFactionType::MILITARY, PixelShipGenerator::ShipFactionType::ASCENDANT, PixelShipGenerator::ShipFactionType::XENO, PixelShipGenerator::ShipFactionType::CORPORATE, PixelShipGenerator::ShipFactionType::RELIC };

    bool isHullHorizontallySymmetric(const PixelShipGenerator::PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) != mask.get(mask.getWidth() - 1u - x, y)) { return false; }
            }
        }
        return true;
    }
}

int PixelShipGeneratorTests::runArbitraryResolutionRegression()
{
    PixelShipGenerator::ShipGenerator generator;
    PixelShipGenerator::ShipIdleAnimator animator;
    bool success = true;

    for (std::size_t index = 0u; index < Resolutions.size(); ++index)
    {
        const uint32_t resolution = Resolutions[index];
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = 0x9E3779B97F4A7C15ull ^ (static_cast<uint64_t>(resolution) * 0xBF58476D1CE4E5B9ull);
        settings.Dimensions.Width = resolution;
        settings.Dimensions.Height = resolution;
        settings.Style = Styles[index % Styles.size()];
        settings.Faction = Factions[(index * 3u) % Factions.size()];

        try
        {
            const PixelShipGenerator::GeneratedShip ship = generator.generate(settings);
            if (ship.FinalImage.getWidth() != resolution || ship.FinalImage.getHeight() != resolution || ship.HullMask.getWidth() != resolution || ship.HullMask.getHeight() != resolution)
            {
                success = false;
                std::cerr << resolution << "x" << resolution << " returned incorrect dimensions.\n";
                continue;
            }
            if (!isHullHorizontallySymmetric(ship.HullMask))
            {
                success = false;
                std::cerr << resolution << "x" << resolution << " produced a non-symmetric hull.\n";
            }

            PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
            animationSettings.FrameCount = 6u;
            animationSettings.Seed = 0xA24BAED4963EE407ull ^ resolution;
            const PixelShipGenerator::ShipIdleAnimation animation = animator.generate(ship, animationSettings);
            if (animation.FrameWidth != resolution || animation.FrameHeight != resolution || animation.Frames.size() != animation.Sampling.ActualFrameCount)
            {
                success = false;
                std::cerr << resolution << "x" << resolution << " animation dimensions/frame count are incorrect.\n";
                continue;
            }
            if (animation.Frames.empty() || animation.Frames.front().getPixels() != ship.FinalImage.getPixels())
            {
                success = false;
                std::cerr << resolution << "x" << resolution << " animation frame 0 differs from the static image.\n";
            }

            const PixelShipGenerator::Image spritesheet = PixelShipGenerator::createHorizontalSpritesheet(animation);
            if (spritesheet.getWidth() != resolution * animation.Sampling.ActualFrameCount || spritesheet.getHeight() != resolution)
            {
                success = false;
                std::cerr << resolution << "x" << resolution << " spritesheet dimensions are incorrect.\n";
            }
        }
        catch (const std::exception& exception)
        {
            success = false;
            std::cerr << resolution << "x" << resolution << " failed: " << exception.what() << '\n';
        }
    }

    return success ? 0 : 1;
}
