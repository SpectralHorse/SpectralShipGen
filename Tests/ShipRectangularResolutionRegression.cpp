#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

#include "ShipGenerator.h"
#include "ShipIdleAnimator.h"
#include "ShipSpritesheetUtils.h"

namespace
{
    using PixelShipGenerator::ShipDimensions;

    constexpr std::array<ShipDimensions, 14u> Dimensions = { {
        { 32u, 44u },
        { 44u, 32u },
        { 40u, 56u },
        { 56u, 40u },
        { 44u, 64u },
        { 64u, 44u },
        { 64u, 96u },
        { 96u, 64u },
        { 96u, 128u },
        { 128u, 96u },
        { 32u, 64u },
        { 64u, 32u },
        { 48u, 64u },
        { 64u, 48u }
    } };

    constexpr std::array<PixelShipGenerator::ShipStyle, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> Styles = {
        PixelShipGenerator::ShipStyle::SLEEK,
        PixelShipGenerator::ShipStyle::FIGHTER,
        PixelShipGenerator::ShipStyle::HEAVY,
        PixelShipGenerator::ShipStyle::INDUSTRIAL,
        PixelShipGenerator::ShipStyle::SPEARHEAD,
        PixelShipGenerator::ShipStyle::DELTA
    };

    constexpr std::array<PixelShipGenerator::ShipFactionType, static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = {
        PixelShipGenerator::ShipFactionType::FRONTIER,
        PixelShipGenerator::ShipFactionType::MILITARY,
        PixelShipGenerator::ShipFactionType::ASCENDANT,
        PixelShipGenerator::ShipFactionType::XENO,
        PixelShipGenerator::ShipFactionType::CORPORATE,
        PixelShipGenerator::ShipFactionType::RELIC
    };

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

    bool isHullConnected(const PixelShipGenerator::PixelMask& mask)
    {
        const uint32_t width = mask.getWidth();
        const uint32_t height = mask.getHeight();
        std::vector<uint8_t> visited(static_cast<std::size_t>(width) * height, 0u);
        std::vector<std::pair<uint32_t, uint32_t>> pending;

        for (uint32_t y = 0u; y < height && pending.empty(); ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (mask.get(x, y)) { pending.emplace_back(x, y); break; }
            }
        }
        if (pending.empty()) { return false; }

        visited[static_cast<std::size_t>(pending.front().second) * width + pending.front().first] = 1u;
        std::size_t pendingIndex = 0u;
        uint32_t visitedCount = 0u;
        constexpr std::array<int32_t, 4u> OffsetX = { -1, 1, 0, 0 };
        constexpr std::array<int32_t, 4u> OffsetY = { 0, 0, -1, 1 };

        while (pendingIndex < pending.size())
        {
            const auto [x, y] = pending[pendingIndex++];
            ++visitedCount;
            for (std::size_t direction = 0u; direction < OffsetX.size(); ++direction)
            {
                const int32_t nx = static_cast<int32_t>(x) + OffsetX[direction];
                const int32_t ny = static_cast<int32_t>(y) + OffsetY[direction];
                if (nx < 0 || ny < 0 || nx >= static_cast<int32_t>(width) || ny >= static_cast<int32_t>(height)) { continue; }
                const std::size_t index = static_cast<std::size_t>(ny) * width + static_cast<uint32_t>(nx);
                if (visited[index] != 0u || !mask.get(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny))) { continue; }
                visited[index] = 1u;
                pending.emplace_back(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny));
            }
        }

        uint32_t totalCount = 0u;
        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x) { totalCount += mask.get(x, y) ? 1u : 0u; }
        }
        return visitedCount == totalCount;
    }

    bool hasExpectedDimensions(const PixelShipGenerator::GeneratedShip& ship, const ShipDimensions& dimensions)
    {
        const auto matches = [&](const PixelShipGenerator::PixelMask& mask) { return mask.getWidth() == dimensions.Width && mask.getHeight() == dimensions.Height; };
        return ship.FinalImage.getWidth() == dimensions.Width && ship.FinalImage.getHeight() == dimensions.Height &&
            matches(ship.HullMask) && matches(ship.CockpitMask) && matches(ship.EngineMask) && matches(ship.EngineExhaustMask) &&
            matches(ship.AttachmentMask) && matches(ship.AccentMask) && matches(ship.MechanicalDetailMask) && matches(ship.LightMask);
    }
}

int PixelShipGeneratorTests::runRectangularResolutionRegression()
{
    PixelShipGenerator::ShipGenerator generator;
    PixelShipGenerator::ShipIdleAnimator animator;
    bool success = true;

    for (std::size_t dimensionIndex = 0u; dimensionIndex < Dimensions.size(); ++dimensionIndex)
    {
        const ShipDimensions dimensions = Dimensions[dimensionIndex];
        for (uint32_t sample = 0u; sample < 2u; ++sample)
        {
            PixelShipGenerator::ShipGenerationSettings settings;
            settings.Dimensions = dimensions;
            settings.Seed = 0x4200000000000000ull ^ (static_cast<uint64_t>(dimensions.Width) << 32u) ^ (static_cast<uint64_t>(dimensions.Height) << 16u) ^ sample;
            settings.Style = Styles[(dimensionIndex + sample) % Styles.size()];
            settings.Faction = Factions[(dimensionIndex * 3u + sample) % Factions.size()];

            try
            {
                const PixelShipGenerator::GeneratedShip first = generator.generate(settings);
                const PixelShipGenerator::GeneratedShip second = generator.generate(settings);

                if (!hasExpectedDimensions(first, dimensions))
                {
                    success = false;
                    std::cerr << dimensions.Width << 'x' << dimensions.Height << " returned incorrect image/mask dimensions.\n";
                    continue;
                }
                if (first.FinalImage.getPixels() != second.FinalImage.getPixels())
                {
                    success = false;
                    std::cerr << dimensions.Width << 'x' << dimensions.Height << " is not deterministic.\n";
                }
                if (!isHullHorizontallySymmetric(first.HullMask) || !isHullConnected(first.HullMask))
                {
                    success = false;
                    std::cerr << dimensions.Width << 'x' << dimensions.Height << " produced invalid hull symmetry/connectivity.\n";
                }

                PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
                animationSettings.FrameCount = 6u;
                animationSettings.Seed = settings.Seed ^ 0xA24BAED4963EE407ull;
                const PixelShipGenerator::ShipIdleAnimation animation = animator.generate(first, animationSettings);
                if (animation.FrameWidth != dimensions.Width || animation.FrameHeight != dimensions.Height || animation.Frames.size() != animationSettings.FrameCount)
                {
                    success = false;
                    std::cerr << dimensions.Width << 'x' << dimensions.Height << " idle animation dimensions are incorrect.\n";
                    continue;
                }
                if (animation.Frames.empty() || animation.Frames.front().getPixels() != first.FinalImage.getPixels())
                {
                    success = false;
                    std::cerr << dimensions.Width << 'x' << dimensions.Height << " animation Frame 0 differs from static output.\n";
                }
                for (const PixelShipGenerator::Image& frame : animation.Frames)
                {
                    if (frame.getWidth() != dimensions.Width || frame.getHeight() != dimensions.Height)
                    {
                        success = false;
                        std::cerr << dimensions.Width << 'x' << dimensions.Height << " animation frame changed dimensions.\n";
                        break;
                    }
                }

                const PixelShipGenerator::Image spritesheet = PixelShipGenerator::createHorizontalSpritesheet(animation);
                if (spritesheet.getWidth() != dimensions.Width * animationSettings.FrameCount || spritesheet.getHeight() != dimensions.Height)
                {
                    success = false;
                    std::cerr << dimensions.Width << 'x' << dimensions.Height << " horizontal spritesheet dimensions are incorrect.\n";
                }
            }
            catch (const std::exception& exception)
            {
                success = false;
                std::cerr << dimensions.Width << 'x' << dimensions.Height << " generation failed: " << exception.what() << '\n';
            }
        }
    }

    return success ? 0 : 1;
}
