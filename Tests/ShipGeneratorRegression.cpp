#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ShipGenerator.h"

namespace
{
    struct RegressionRecipe
    {
        const char* Name;
        uint64_t Seed;
        uint32_t Width;
        uint32_t Height;
        PixelShipGenerator::ShipStyle Style;
        PixelShipGenerator::ShipFactionType Faction;
        bool AttachmentsEnabled;
        uint64_t ExpectedHash;
    };

    constexpr std::array<RegressionRecipe, 12u> Recipes =
    { {
        { "fighter_frontier_64", 0x0123456789ABCDEFull, 64u, 64u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::FRONTIER, true, 0x4D111F4E7018B22Cull },
        { "fighter_xeno_96", 0xBF58476D1CE4E5B9ull, 96u, 96u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::XENO, true, 0x3AE228FD6CA0E937ull },
        { "sleek_ascendant_64", 0x8C3C010CB4754C91ull, 64u, 64u, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::ASCENDANT, true, 0xF7A0D53BF0001BAEull },
        { "sleek_military_96_no_attachments", 0x243F6A8885A308D3ull, 96u, 96u, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::MILITARY, false, 0x74BDB8D9223D65D8ull },
        { "heavy_military_64", 0xD1B54A32D192ED03ull, 64u, 64u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::MILITARY, true, 0x59F1600FBB86A6EEull },
        { "heavy_ascendant_96", 0x13198A2E03707344ull, 96u, 96u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::ASCENDANT, true, 0xB810857B111B0393ull },
        { "industrial_xeno_64_no_attachments", 0x94D049BB133111EBull, 64u, 64u, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::XENO, false, 0xA7D842A2DBB40AE1ull },
        { "industrial_frontier_96", 0xA4093822299F31D0ull, 96u, 96u, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::FRONTIER, true, 0x2D887A69CD8DF0B1ull },
        { "sleek_frontier_64", 0x6A09E667F3BCC909ull, 64u, 64u, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::FRONTIER, true, 0x38FB92401D2F7784ull },
        { "fighter_military_96", 0xBB67AE8584CAA73Bull, 96u, 96u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY, false, 0xAA064B287EAC1FCAull },
        { "heavy_xeno_64", 0x3C6EF372FE94F82Bull, 64u, 64u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::XENO, true, 0xE7B591604EB2097Eull },
        { "industrial_ascendant_96", 0xA54FF53A5F1D36F1ull, 96u, 96u, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::ASCENDANT, true, 0x3579B48694389F2Cull }
    } };

    void hashByte(uint64_t& hash, uint8_t value)
    {
        hash ^= static_cast<uint64_t>(value);
        hash *= 1099511628211ull;
    }

    void hashUInt32(uint64_t& hash, uint32_t value)
    {
        hashByte(hash, static_cast<uint8_t>(value & 0xFFu));
        hashByte(hash, static_cast<uint8_t>((value >> 8u) & 0xFFu));
        hashByte(hash, static_cast<uint8_t>((value >> 16u) & 0xFFu));
        hashByte(hash, static_cast<uint8_t>((value >> 24u) & 0xFFu));
    }

    uint64_t calculateImageHash(const PixelShipGenerator::GeneratedShip& ship, uint32_t width, uint32_t height)
    {
        uint64_t hash = 14695981039346656037ull;
        hashUInt32(hash, width);
        hashUInt32(hash, height);

        for (const PixelShipGenerator::Color& color : ship.FinalImage.getPixels())
        {
            hashByte(hash, color.R);
            hashByte(hash, color.G);
            hashByte(hash, color.B);
            hashByte(hash, color.A);
        }

        return hash;
    }

    PixelShipGenerator::ShipGenerationSettings createSettings(const RegressionRecipe& recipe)
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = recipe.Seed;
        settings.Dimensions.Width = recipe.Width;
        settings.Dimensions.Height = recipe.Height;
        settings.Style = recipe.Style;
        settings.Faction = recipe.Faction;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        return settings;
    }

    bool isHullConnected(const PixelShipGenerator::PixelMask& hullMask)
    {
        const uint32_t width = hullMask.getWidth();
        const uint32_t height = hullMask.getHeight();
        uint32_t totalPixels = 0u;
        uint32_t startX = 0u;
        uint32_t startY = 0u;
        bool foundStart = false;

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (!hullMask.get(x, y))
                {
                    continue;
                }

                ++totalPixels;

                if (!foundStart)
                {
                    startX = x;
                    startY = y;
                    foundStart = true;
                }
            }
        }

        if (!foundStart)
        {
            return false;
        }

        std::vector<uint8_t> visited(static_cast<std::size_t>(width) * height, 0u);
        std::vector<std::pair<uint32_t, uint32_t>> pending;
        pending.reserve(totalPixels);
        pending.emplace_back(startX, startY);
        visited[static_cast<std::size_t>(startY) * width + startX] = 1u;
        uint32_t visitedCount = 0u;
        std::size_t pendingIndex = 0u;
        constexpr std::array<int32_t, 4u> OffsetX = { -1, 1, 0, 0 };
        constexpr std::array<int32_t, 4u> OffsetY = { 0, 0, -1, 1 };

        while (pendingIndex < pending.size())
        {
            const auto [x, y] = pending[pendingIndex++];
            ++visitedCount;

            for (std::size_t direction = 0u; direction < OffsetX.size(); ++direction)
            {
                const int32_t neighbourX = static_cast<int32_t>(x) + OffsetX[direction];
                const int32_t neighbourY = static_cast<int32_t>(y) + OffsetY[direction];

                if (neighbourX < 0 || neighbourY < 0 || neighbourX >= static_cast<int32_t>(width) || neighbourY >= static_cast<int32_t>(height))
                {
                    continue;
                }

                const uint32_t pixelX = static_cast<uint32_t>(neighbourX);
                const uint32_t pixelY = static_cast<uint32_t>(neighbourY);
                const std::size_t index = static_cast<std::size_t>(pixelY) * width + pixelX;

                if (visited[index] != 0u || !hullMask.get(pixelX, pixelY))
                {
                    continue;
                }

                visited[index] = 1u;
                pending.emplace_back(pixelX, pixelY);
            }
        }

        return visitedCount == totalPixels;
    }

    bool isHullHorizontallySymmetric(const PixelShipGenerator::PixelMask& hullMask)
    {
        for (uint32_t y = 0u; y < hullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (hullMask.get(x, y) != hullMask.get(hullMask.getWidth() - 1u - x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }
}

int PixelShipGeneratorTests::runGeneratorRegression()
{
    const bool recordMode = false;
    PixelShipGenerator::ShipGenerator generator;
    bool success = true;

    for (const RegressionRecipe& recipe : Recipes)
    {
        try
        {
            const PixelShipGenerator::ShipGenerationSettings settings = createSettings(recipe);
            const PixelShipGenerator::GeneratedShip firstShip = generator.generate(settings);
            const PixelShipGenerator::GeneratedShip secondShip = generator.generate(settings);
            const uint64_t firstHash = calculateImageHash(firstShip, recipe.Width, recipe.Height);
            const uint64_t secondHash = calculateImageHash(secondShip, recipe.Width, recipe.Height);

            if (firstHash != secondHash)
            {
                success = false;
                std::cerr << recipe.Name << " is not deterministic: first 0x" << std::hex << firstHash << ", second 0x" << secondHash << std::dec << '\n';
            }

            if (recordMode)
            {
                std::cout << recipe.Name << " = 0x" << std::hex << std::uppercase << firstHash << "ull" << std::dec << '\n';
            }
            else if (recipe.ExpectedHash != 0ull && firstHash != recipe.ExpectedHash)
            {
                success = false;
                std::cerr << recipe.Name << " changed: expected 0x" << std::hex << std::uppercase << recipe.ExpectedHash << ", actual 0x" << firstHash << std::dec << '\n';
            }

            if (firstShip.HullMask.getWidth() != recipe.Width || firstShip.HullMask.getHeight() != recipe.Height)
            {
                success = false;
                std::cerr << recipe.Name << " returned an unexpected hull resolution.\n";
            }

            if (!isHullConnected(firstShip.HullMask))
            {
                success = false;
                std::cerr << recipe.Name << " produced a disconnected hull.\n";
            }

            if (!isHullHorizontallySymmetric(firstShip.HullMask))
            {
                success = false;
                std::cerr << recipe.Name << " produced a non-symmetric base hull.\n";
            }
        }
        catch (const std::exception& exception)
        {
            success = false;
            std::cerr << recipe.Name << " generation failed: " << exception.what() << '\n';
        }
    }

    return success ? 0 : 1;
}
