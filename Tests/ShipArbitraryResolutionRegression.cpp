#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <vector>

#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipSpritesheetUtils.h>

namespace
{
    constexpr std::array<uint32_t, 14u> Resolutions = { 24u, 26u, 28u, 30u, 36u, 40u, 42u, 48u, 56u, 72u, 80u, 112u, 192u, 256u };
    constexpr std::array<SpectralShipGen::ShipStyle, static_cast<std::size_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)> Styles = { SpectralShipGen::ShipStyle::SLEEK, SpectralShipGen::ShipStyle::FIGHTER, SpectralShipGen::ShipStyle::HEAVY, SpectralShipGen::ShipStyle::INDUSTRIAL, SpectralShipGen::ShipStyle::SPEARHEAD, SpectralShipGen::ShipStyle::DELTA };
    constexpr std::array<SpectralShipGen::ShipFactionType, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { SpectralShipGen::ShipFactionType::FRONTIER, SpectralShipGen::ShipFactionType::MILITARY, SpectralShipGen::ShipFactionType::ASCENDANT, SpectralShipGen::ShipFactionType::XENO, SpectralShipGen::ShipFactionType::CORPORATE, SpectralShipGen::ShipFactionType::RELIC };

    bool isHullHorizontallySymmetric(const SpectralShipGen::PixelMask& mask)
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

int SpectralShipGenTests::runArbitraryResolutionRegression()
{
    SpectralShipGen::ShipGenerator generator;
    SpectralShipGen::ShipIdleAnimator animator;
    bool success = true;

    for (std::size_t index = 0u; index < Resolutions.size(); ++index)
    {
        const uint32_t resolution = Resolutions[index];
        SpectralShipGen::ShipGenerationSettings settings;
        settings.Seed = 0x9E3779B97F4A7C15ull ^ (static_cast<uint64_t>(resolution) * 0xBF58476D1CE4E5B9ull);
        settings.Dimensions.Width = resolution;
        settings.Dimensions.Height = resolution;
        settings.Style = Styles[index % Styles.size()];
        settings.Faction = Factions[(index * 3u) % Factions.size()];

        try
        {
            const SpectralShipGen::GeneratedShip ship = generator.generate(settings);
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

            SpectralShipGen::ShipIdleAnimationSettings animationSettings;
            animationSettings.FrameCount = 6u;
            animationSettings.Seed = 0xA24BAED4963EE407ull ^ resolution;
            const SpectralShipGen::ShipIdleAnimation animation = animator.generate(ship, animationSettings);
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

            const SpectralShipGen::Image spritesheet = SpectralShipGen::createHorizontalSpritesheet(animation);
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
