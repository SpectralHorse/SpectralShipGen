#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include <SpectralShipGen/MacroAsymmetry.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    using namespace SpectralShipGen;

    struct Case
    {
        uint64_t Seed;
        MacroAsymmetryCategory Category;
    };

    uint32_t countMaskPixels(const PixelMask& mask)
    {
        uint32_t count = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x) { count += mask.get(x, y) ? 1u : 0u; }
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

    bool maskIsSubset(const PixelMask& subset, const PixelMask& superset)
    {
        if (subset.getWidth() != superset.getWidth() || subset.getHeight() != superset.getHeight()) { return false; }
        for (uint32_t y = 0u; y < subset.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < subset.getWidth(); ++x)
            {
                if (subset.get(x, y) && !superset.get(x, y)) { return false; }
            }
        }
        return true;
    }

    bool isHorizontallySymmetric(const PixelMask& mask)
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

    bool macroMaskLivesOnDominantSide(const ShipGenerationDebugInfo& debug)
    {
        const PixelMask& mask = debug.MacroAsymmetryMask;
        const uint32_t total = countMaskPixels(mask);
        if (total == 0u) { return false; }
        uint32_t dominant = 0u;
        const uint32_t half = mask.getWidth() / 2u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                const bool isDominant = debug.MacroAsymmetryDominantSide == MacroAsymmetrySide::LEFT ? x < half : x >= half;
                dominant += isDominant ? 1u : 0u;
            }
        }
        return dominant * 4u >= total * 3u;
    }

    bool isMaskConnected(const PixelMask& mask)
    {
        const uint32_t total = countMaskPixels(mask);
        if (total == 0u) { return false; }

        uint32_t startX = 0u;
        uint32_t startY = 0u;
        bool foundStart = false;
        for (uint32_t y = 0u; y < mask.getHeight() && !foundStart; ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                startX = x;
                startY = y;
                foundStart = true;
                break;
            }
        }

        constexpr std::array<int32_t, 4u> OffsetX = { -1, 1, 0, 0 };
        constexpr std::array<int32_t, 4u> OffsetY = { 0, 0, -1, 1 };
        std::vector<uint8_t> visited(static_cast<std::size_t>(mask.getWidth()) * mask.getHeight(), 0u);
        std::vector<std::pair<uint32_t, uint32_t>> pending;
        pending.reserve(total);
        pending.emplace_back(startX, startY);
        visited[static_cast<std::size_t>(startY) * mask.getWidth() + startX] = 1u;

        uint32_t visitedCount = 0u;
        for (std::size_t index = 0u; index < pending.size(); ++index)
        {
            const auto [x, y] = pending[index];
            ++visitedCount;
            for (std::size_t direction = 0u; direction < OffsetX.size(); ++direction)
            {
                const int32_t nx = static_cast<int32_t>(x) + OffsetX[direction];
                const int32_t ny = static_cast<int32_t>(y) + OffsetY[direction];
                if (nx < 0 || ny < 0 || nx >= static_cast<int32_t>(mask.getWidth()) || ny >= static_cast<int32_t>(mask.getHeight())) { continue; }
                const uint32_t ux = static_cast<uint32_t>(nx);
                const uint32_t uy = static_cast<uint32_t>(ny);
                const std::size_t pixelIndex = static_cast<std::size_t>(uy) * mask.getWidth() + ux;
                if (visited[pixelIndex] != 0u || !mask.get(ux, uy)) { continue; }
                visited[pixelIndex] = 1u;
                pending.emplace_back(ux, uy);
            }
        }

        return visitedCount == total;
    }

    bool maskTouchesHull(const PixelMask& mask, const PixelMask& hull)
    {
        constexpr std::array<std::pair<int32_t, int32_t>, 5u> Offsets = { std::pair<int32_t, int32_t>{ 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                for (const auto& [dx, dy] : Offsets)
                {
                    const int32_t nx = static_cast<int32_t>(x) + dx;
                    const int32_t ny = static_cast<int32_t>(y) + dy;
                    if (nx >= 0 && ny >= 0 && nx < static_cast<int32_t>(hull.getWidth()) && ny < static_cast<int32_t>(hull.getHeight()) && hull.get(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny))) { return true; }
                }
            }
        }
        return false;
    }

    ShipGenerationSettings makeSettings(uint64_t seed, ShipDimensions dimensions = { 64u, 64u })
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = ShipStyle::INDUSTRIAL;
        settings.Faction = ShipFactionType::FRONTIER;
        settings.AttachmentsEnabled = true;
        return settings;
    }

    bool validateKnownCategory(ShipGenerator& generator, const Case& testCase)
    {
        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        const ShipGenerationSettings settings = makeSettings(testCase.Seed);
        const GeneratedShip first = generator.generate(settings, &firstDebug);
        const GeneratedShip second = generator.generate(settings, &secondDebug);

        if (!firstDebug.MacroAsymmetryPlanned || !firstDebug.MacroAsymmetryFulfilled || firstDebug.MacroAsymmetryRejected) { return false; }
        if (firstDebug.MacroAsymmetryFeatureCategory != testCase.Category || secondDebug.MacroAsymmetryFeatureCategory != testCase.Category) { return false; }
        if (firstDebug.MacroAsymmetryBalanceScore < 42u || firstDebug.MacroAsymmetryActualVisualWeight == 0u) { return false; }
        if (!macroMaskLivesOnDominantSide(firstDebug) || !isMaskConnected(firstDebug.MacroAsymmetryMask)) { return false; }
        const std::size_t targetRegion = static_cast<std::size_t>(firstDebug.MacroAsymmetryTargetRegion);
        if (targetRegion >= firstDebug.SpatialRegionDominantCounts.size() || firstDebug.SpatialRegionDominantCounts[targetRegion] == 0u) { return false; }
        if (!isHorizontallySymmetric(first.HullMask)) { return false; }
        if (first.FinalImage.getPixels() != second.FinalImage.getPixels() || !masksEqual(firstDebug.MacroAsymmetryMask, secondDebug.MacroAsymmetryMask)) { return false; }
        if (firstDebug.ComplexityConsumedBudget > firstDebug.ComplexityInitialBudget) { return false; }

        switch (testCase.Category)
        {
        case MacroAsymmetryCategory::HULL_LAYER:
            if (!maskIsSubset(firstDebug.MacroAsymmetryMask, firstDebug.HullLayerMask) || !maskIsSubset(firstDebug.MacroAsymmetryMask, first.HullMask)) { return false; }
            break;
        case MacroAsymmetryCategory::LARGE_WEAPON:
            if (!maskIsSubset(firstDebug.MacroAsymmetryMask, first.IdleAnimationMetadata.WeaponOccupiedMask) || !maskTouchesHull(firstDebug.MacroAsymmetryMask, first.HullMask)) { return false; }
            break;
        case MacroAsymmetryCategory::ATTACHMENT:
            if (!maskIsSubset(firstDebug.MacroAsymmetryMask, first.AttachmentMask) || !maskTouchesHull(firstDebug.MacroAsymmetryMask, first.HullMask)) { return false; }
            break;
        default:
            return false;
        }

        return true;
    }

    uint32_t countPlans(ShipGenerator& generator, ShipDimensions dimensions, uint32_t sampleCount)
    {
        uint32_t result = 0u;
        for (uint32_t index = 0u; index < sampleCount; ++index)
        {
            ShipGenerationSettings settings = makeSettings(0xD1B54A32D192ED03ull + static_cast<uint64_t>(index) * 0x9E3779B97F4A7C15ull, dimensions);
            settings.Faction = ShipFactionType::XENO;
            ShipGenerationDebugInfo debug;
            const GeneratedShip ship = generator.generate(settings, &debug);
            if (!isHorizontallySymmetric(ship.HullMask)) { return UINT32_MAX; }
            result += debug.MacroAsymmetryPlanned ? 1u : 0u;
        }
        return result;
    }
}

int SpectralShipGenTests::runMacroAsymmetryRegression()
{
    using namespace SpectralShipGen;
    ShipGenerator generator;
    bool success = true;

    constexpr std::array<Case, 3u> Cases = {
        Case{ 15253433902508948837ull, MacroAsymmetryCategory::HULL_LAYER },
        Case{ 3460057330748747529ull, MacroAsymmetryCategory::LARGE_WEAPON },
        Case{ 18054082321272548793ull, MacroAsymmetryCategory::ATTACHMENT }
    };

    for (const Case& testCase : Cases)
    {
        if (!validateKnownCategory(generator, testCase))
        {
            success = false;
            std::cerr << "Known macro-asymmetry category validation failed for category " << static_cast<uint32_t>(testCase.Category) << ".\n";
        }
    }

    constexpr uint32_t Samples = 48u;
    const uint32_t tinyPlans = countPlans(generator, { 24u, 24u }, Samples);
    const uint32_t mediumPlans = countPlans(generator, { 64u, 64u }, Samples);
    const uint32_t largePlans = countPlans(generator, { 160u, 160u }, Samples);
    if (tinyPlans == UINT32_MAX || mediumPlans == UINT32_MAX || largePlans == UINT32_MAX || tinyPlans > mediumPlans || mediumPlans > largePlans || largePlans >= Samples / 2u)
    {
        success = false;
        std::cerr << "Macro-asymmetry rarity/scale trend failed: tiny=" << tinyPlans << ", medium=" << mediumPlans << ", large=" << largePlans << ".\n";
    }

    constexpr std::array<ShipDimensions, 4u> Rectangular = { ShipDimensions{ 48u, 64u }, { 64u, 48u }, { 64u, 96u }, { 96u, 64u } };
    for (std::size_t index = 0u; index < Rectangular.size(); ++index)
    {
        const ShipGenerationSettings settings = makeSettings(0xA4093822299F31D0ull + index * 0x10001ull, Rectangular[index]);
        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        const GeneratedShip first = generator.generate(settings, &firstDebug);
        const GeneratedShip second = generator.generate(settings, &secondDebug);
        if (first.FinalImage.getWidth() != Rectangular[index].Width || first.FinalImage.getHeight() != Rectangular[index].Height || first.FinalImage.getPixels() != second.FinalImage.getPixels() || !isHorizontallySymmetric(first.HullMask))
        {
            success = false;
            std::cerr << "Rectangular macro-asymmetry determinism/symmetry validation failed.\n";
        }
    }

    return success ? 0 : 1;
}
