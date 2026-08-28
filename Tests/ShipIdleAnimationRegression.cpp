#include "RegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "ShipGenerator.h"
#include "ShipIdleAnimator.h"
#include "ShipSpritesheetUtils.h"

namespace
{
    struct AnimationRegressionRecipe
    {
        const char* Name;
        uint64_t Seed;
        uint32_t Resolution;
        PixelShipGenerator::ShipStyle Style;
        PixelShipGenerator::ShipFactionType Faction;
        bool AttachmentsEnabled;
    };

    struct PropulsionLayoutRecipe
    {
        const char* Name;
        uint64_t Seed;
        uint32_t Resolution;
        PixelShipGenerator::ShipStyle Style;
        PixelShipGenerator::ShipFactionType Faction;
        PixelShipGenerator::EngineLayoutType ExpectedLayout;
    };

    constexpr uint64_t SharedResolutionSeed = 0x9E3779B97F4A7C15ull;

    constexpr std::array<AnimationRegressionRecipe, 9u> Recipes =
    { {
        { "idle_24_frontier_sleek", SharedResolutionSeed, 24u, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::FRONTIER, true },
        { "idle_32_military_fighter", SharedResolutionSeed, 32u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY, true },
        { "idle_44_ascendant_heavy", SharedResolutionSeed, 44u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::ASCENDANT, true },
        { "idle_64_xeno_industrial", SharedResolutionSeed, 64u, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::XENO, true },
        { "idle_96_frontier_sleek", SharedResolutionSeed, 96u, PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipFactionType::FRONTIER, true },
        { "idle_128_military_fighter", SharedResolutionSeed, 128u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY, true },
        { "idle_160_ascendant_heavy", SharedResolutionSeed, 160u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::ASCENDANT, true },
        { "idle_64_corporate_delta", SharedResolutionSeed, 64u, PixelShipGenerator::ShipStyle::DELTA, PixelShipGenerator::ShipFactionType::CORPORATE, true },
        { "idle_64_relic_spearhead", SharedResolutionSeed, 64u, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipFactionType::RELIC, true }
    } };

    constexpr std::array<PropulsionLayoutRecipe, 5u> PropulsionLayoutRecipes =
    { {
        { "propulsion_twin_32", 1u, 32u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY, PixelShipGenerator::EngineLayoutType::TWIN },
        { "propulsion_quad_44", 0u, 44u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::XENO, PixelShipGenerator::EngineLayoutType::QUAD },
        { "propulsion_central_auxiliary_44", 0u, 44u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::ASCENDANT, PixelShipGenerator::EngineLayoutType::CENTRAL_AUXILIARY },
        { "propulsion_central_64", 7u, 64u, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipFactionType::FRONTIER, PixelShipGenerator::EngineLayoutType::CENTRAL },
        { "propulsion_wide_bank_64", 0u, 64u, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::FRONTIER, PixelShipGenerator::EngineLayoutType::WIDE_BANK }
    } };

    PixelShipGenerator::ShipGenerationSettings createSettings(const AnimationRegressionRecipe& recipe)
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = recipe.Seed;
        settings.Dimensions.Width = recipe.Resolution;
        settings.Dimensions.Height = recipe.Resolution;
        settings.Style = recipe.Style;
        settings.Faction = recipe.Faction;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        return settings;
    }

    bool imagesEqual(const PixelShipGenerator::Image& first, const PixelShipGenerator::Image& second)
    {
        return first.getPixels() == second.getPixels();
    }

    bool isExhaustColor(const PixelShipGenerator::Color& color, const PixelShipGenerator::ShipPalette& palette)
    {
        return color == palette.ExhaustBase || color == palette.ExhaustHighlight || color == palette.ExhaustHotCore;
    }

    uint32_t getOpaquePixelCount(const PixelShipGenerator::Image& image)
    {
        uint32_t count = 0u;
        for (const PixelShipGenerator::Color& color : image.getPixels()) { if (color.A != 0u) { ++count; } }
        return count;
    }

    bool hasBinaryAlpha(const PixelShipGenerator::Image& image)
    {
        for (const PixelShipGenerator::Color& color : image.getPixels())
        {
            if (color.A != 0u && color.A != 255u)
            {
                return false;
            }
        }

        return true;
    }

    bool validateSpritesheetOrder(const PixelShipGenerator::ShipIdleAnimation& animation, const PixelShipGenerator::Image& spritesheet)
    {
        if (animation.Frames.empty())
        {
            return false;
        }

        for (uint32_t frameIndex = 0u; frameIndex < animation.Frames.size(); ++frameIndex)
        {
            for (uint32_t y = 0u; y < animation.FrameHeight; ++y)
            {
                for (uint32_t x = 0u; x < animation.FrameWidth; ++x)
                {
                    if (spritesheet.getPixel(frameIndex * animation.FrameWidth + x, y) != animation.Frames[frameIndex].getPixel(x, y))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool isOpaqueImageEightConnected(const PixelShipGenerator::Image& image, uint32_t width, uint32_t height)
    {
        std::vector<uint8_t> visited(static_cast<std::size_t>(width) * height, 0u);
        std::queue<std::pair<uint32_t, uint32_t>> pending;
        uint32_t opaqueCount = 0u;
        bool foundStart = false;

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (image.getPixel(x, y).A == 0u)
                {
                    continue;
                }

                ++opaqueCount;

                if (!foundStart)
                {
                    foundStart = true;
                    pending.push({ x, y });
                    visited[static_cast<std::size_t>(y) * width + x] = 1u;
                }
            }
        }

        if (!foundStart)
        {
            return false;
        }

        uint32_t visitedCount = 0u;

        while (!pending.empty())
        {
            const auto [x, y] = pending.front();
            pending.pop();
            ++visitedCount;

            for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
            {
                for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    if (offsetX == 0 && offsetY == 0)
                    {
                        continue;
                    }

                    const int32_t neighbourX = static_cast<int32_t>(x) + offsetX;
                    const int32_t neighbourY = static_cast<int32_t>(y) + offsetY;

                    if (neighbourX < 0 || neighbourY < 0 || neighbourX >= static_cast<int32_t>(width) || neighbourY >= static_cast<int32_t>(height))
                    {
                        continue;
                    }

                    const uint32_t pixelX = static_cast<uint32_t>(neighbourX);
                    const uint32_t pixelY = static_cast<uint32_t>(neighbourY);
                    const std::size_t index = static_cast<std::size_t>(pixelY) * width + pixelX;

                    if (visited[index] != 0u || image.getPixel(pixelX, pixelY).A == 0u)
                    {
                        continue;
                    }

                    visited[index] = 1u;
                    pending.push({ pixelX, pixelY });
                }
            }
        }

        return visitedCount == opaqueCount;
    }

    bool hasIsolatedOpaquePixel(const PixelShipGenerator::Image& image, uint32_t width, uint32_t height)
    {
        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (image.getPixel(x, y).A == 0u)
                {
                    continue;
                }

                bool hasNeighbour = false;

                for (int32_t offsetY = -1; offsetY <= 1 && !hasNeighbour; ++offsetY)
                {
                    for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
                    {
                        if (offsetX == 0 && offsetY == 0)
                        {
                            continue;
                        }

                        const int32_t neighbourX = static_cast<int32_t>(x) + offsetX;
                        const int32_t neighbourY = static_cast<int32_t>(y) + offsetY;

                        if (neighbourX >= 0 && neighbourY >= 0 && neighbourX < static_cast<int32_t>(width) && neighbourY < static_cast<int32_t>(height) && image.getPixel(static_cast<uint32_t>(neighbourX), static_cast<uint32_t>(neighbourY)).A != 0u)
                        {
                            hasNeighbour = true;
                            break;
                        }
                    }
                }

                if (!hasNeighbour)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool validateTaperedExhaust(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship)
    {
        for (const PixelShipGenerator::ShipEngineAnimationComponent& component : ship.IdleAnimationMetadata.EngineComponents)
        {
            uint32_t previousWidth = component.NozzleWidth;
            bool foundRow = false;
            bool encounteredGap = false;
            const uint32_t maximumY = std::min(ship.EngineExhaustMask.getHeight() - 1u, component.ExhaustStartY + component.ExhaustLength);
            const uint32_t endX = std::min(ship.EngineExhaustMask.getWidth(), component.HousingStartX + component.HousingWidth);

            for (uint32_t y = component.ExhaustStartY; y <= maximumY; ++y)
            {
                uint32_t rowWidth = 0u;

                for (uint32_t x = component.HousingStartX; x < endX; ++x)
                {
                    if (isExhaustColor(frame.getPixel(x, y), ship.Palette))
                    {
                        ++rowWidth;
                    }
                }

                if (rowWidth == 0u)
                {
                    if (foundRow) { encounteredGap = true; }
                    continue;
                }

                if (encounteredGap || rowWidth > previousWidth)
                {
                    return false;
                }

                previousWidth = rowWidth;
                foundRow = true;
            }

            if (!foundRow)
            {
                return false;
            }
        }

        return true;
    }

    bool maskRegionChanged(const PixelShipGenerator::Image& frame, const PixelShipGenerator::Image& base, const PixelShipGenerator::PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) && frame.getPixel(x, y) != base.getPixel(x, y))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool isInsideExhaustEnvelope(const PixelShipGenerator::ShipEngineAnimationComponent& component, uint32_t x, uint32_t y)
    {
        return x >= component.HousingStartX && x < component.HousingStartX + component.HousingWidth && y >= component.ExhaustStartY && y < component.ExhaustStartY + component.MaximumExhaustLength;
    }

    bool isInsideAnyExhaustEnvelope(const PixelShipGenerator::GeneratedShip& ship, uint32_t x, uint32_t y)
    {
        for (const PixelShipGenerator::ShipEngineAnimationComponent& component : ship.IdleAnimationMetadata.EngineComponents)
        {
            if (isInsideExhaustEnvelope(component, x, y))
            {
                return true;
            }
        }

        return false;
    }

    uint32_t countChangedPixelsInMask(const PixelShipGenerator::Image& frame, const PixelShipGenerator::Image& base, const PixelShipGenerator::PixelMask& mask)
    {
        uint32_t count = 0u;

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) && frame.getPixel(x, y) != base.getPixel(x, y))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    uint32_t countChangedPixelsInExhaustEnvelope(const PixelShipGenerator::Image& frame, const PixelShipGenerator::Image& base, const PixelShipGenerator::GeneratedShip& ship)
    {
        uint32_t count = 0u;

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (isInsideAnyExhaustEnvelope(ship, x, y) && frame.getPixel(x, y) != base.getPixel(x, y))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    uint32_t getAnimatedExhaustLength(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::ShipEngineAnimationComponent& component)
    {
        uint32_t length = 0u;

        for (uint32_t row = 0u; row < component.MaximumExhaustLength; ++row)
        {
            const uint32_t y = component.ExhaustStartY + row;
            bool foundExhaust = false;

            for (uint32_t x = component.HousingStartX; x < component.HousingStartX + component.HousingWidth && x < ship.HullMask.getWidth(); ++x)
            {
                if (isExhaustColor(frame.getPixel(x, y), ship.Palette))
                {
                    foundExhaust = true;
                    break;
                }
            }

            if (!foundExhaust)
            {
                break;
            }

            ++length;
        }

        return length;
    }

    bool validateAnimatedExhaustFrame(const PixelShipGenerator::Image& frame, const PixelShipGenerator::GeneratedShip& ship)
    {
        for (const PixelShipGenerator::ShipEngineAnimationComponent& component : ship.IdleAnimationMetadata.EngineComponents)
        {
            if (component.MinimumExhaustLength == 0u || component.MinimumExhaustLength > component.ExhaustLength || component.ExhaustLength > component.MaximumExhaustLength || component.ExhaustStartY + component.MaximumExhaustLength > ship.HullMask.getHeight())
            {
                return false;
            }

            uint32_t previousWidth = component.NozzleWidth;
            uint32_t firstWidth = 0u;
            uint32_t lastWidth = 0u;
            uint32_t length = 0u;
            bool encounteredGap = false;

            for (uint32_t row = 0u; row < component.MaximumExhaustLength; ++row)
            {
                const uint32_t y = component.ExhaustStartY + row;
                uint32_t rowWidth = 0u;
                uint32_t firstX = 0u;
                uint32_t lastX = 0u;
                bool found = false;

                for (uint32_t x = component.HousingStartX; x < component.HousingStartX + component.HousingWidth && x < ship.HullMask.getWidth(); ++x)
                {
                    if (!isExhaustColor(frame.getPixel(x, y), ship.Palette))
                    {
                        continue;
                    }

                    if (!found) { firstX = x; }
                    lastX = x;
                    found = true;
                    ++rowWidth;

                    if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y) || ship.IdleAnimationMetadata.WeaponOccupiedMask.get(x, y))
                    {
                        return false;
                    }
                }

                if (rowWidth == 0u)
                {
                    if (length > 0u) { encounteredGap = true; }
                    continue;
                }

                if (encounteredGap || rowWidth > previousWidth || lastX - firstX + 1u != rowWidth)
                {
                    return false;
                }

                if (length == 0u)
                {
                    firstWidth = rowWidth;
                    bool connectedToNozzle = false;

                    if (y > 0u)
                    {
                        for (uint32_t x = firstX; x <= lastX; ++x)
                        {
                            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
                            {
                                const int32_t nozzleX = static_cast<int32_t>(x) + offsetX;
                                if (nozzleX >= 0 && nozzleX < static_cast<int32_t>(ship.EngineMask.getWidth()) && ship.EngineMask.get(static_cast<uint32_t>(nozzleX), y - 1u))
                                {
                                    connectedToNozzle = true;
                                }
                            }
                        }
                    }

                    if (!connectedToNozzle)
                    {
                        return false;
                    }
                }

                previousWidth = rowWidth;
                lastWidth = rowWidth;
                ++length;
            }

            if (length < component.MinimumExhaustLength || length > component.MaximumExhaustLength)
            {
                return false;
            }

            if (component.NozzleWidth >= 3u && length >= 3u && lastWidth >= firstWidth)
            {
                return false;
            }
        }

        return true;
    }
}

int PixelShipGeneratorTests::runIdleAnimationRegression()
{
    PixelShipGenerator::ShipGenerator generator;
    PixelShipGenerator::ShipIdleAnimator animator;
    bool success = true;

    for (const AnimationRegressionRecipe& recipe : Recipes)
    {
        try
        {
            const PixelShipGenerator::GeneratedShip ship = generator.generate(createSettings(recipe));
            PixelShipGenerator::ShipIdleAnimationSettings settings;
            settings.Seed = recipe.Seed ^ 0xD6E8FEB86659FD93ull;

            const PixelShipGenerator::ShipIdleAnimation firstAnimation = animator.generate(ship, settings);
            const PixelShipGenerator::ShipIdleAnimation secondAnimation = animator.generate(ship, settings);

            if (firstAnimation.FrameWidth != recipe.Resolution || firstAnimation.FrameHeight != recipe.Resolution || firstAnimation.Frames.size() != settings.FrameCount)
            {
                success = false;
                std::cerr << recipe.Name << " returned unexpected animation dimensions or frame count.\n";
                continue;
            }

            if (firstAnimation.Frames.empty() || !imagesEqual(firstAnimation.Frames.front(), ship.FinalImage))
            {
                success = false;
                std::cerr << recipe.Name << " frame 0 does not exactly equal the static generated image.\n";
            }

            if (!firstAnimation.Frames.empty() && !imagesEqual(firstAnimation.Frames.back(), ship.FinalImage))
            {
                success = false;
                std::cerr << recipe.Name << " does not close its loop cleanly back to the static pose.\n";
            }

            if (firstAnimation.Frames.size() != secondAnimation.Frames.size())
            {
                success = false;
                std::cerr << recipe.Name << " is not deterministic in frame count.\n";
            }
            else
            {
                for (std::size_t frameIndex = 0u; frameIndex < firstAnimation.Frames.size(); ++frameIndex)
                {
                    if (!imagesEqual(firstAnimation.Frames[frameIndex], secondAnimation.Frames[frameIndex]))
                    {
                        success = false;
                        std::cerr << recipe.Name << " is not deterministic at frame " << frameIndex << ".\n";
                        break;
                    }

                    if (firstAnimation.Frames[frameIndex].getPixels().size() != static_cast<std::size_t>(recipe.Resolution) * recipe.Resolution || !hasBinaryAlpha(firstAnimation.Frames[frameIndex]))
                    {
                        success = false;
                        std::cerr << recipe.Name << " contains a frame with invalid dimensions or alpha.\n";
                        break;
                    }
                }
            }

            const PixelShipGenerator::Image spritesheet = PixelShipGenerator::createHorizontalSpritesheet(firstAnimation);
            const std::size_t expectedSpritesheetPixelCount = static_cast<std::size_t>(recipe.Resolution) * firstAnimation.Frames.size() * recipe.Resolution;

            if (spritesheet.getPixels().size() != expectedSpritesheetPixelCount || !validateSpritesheetOrder(firstAnimation, spritesheet))
            {
                success = false;
                std::cerr << recipe.Name << " spritesheet has incorrect dimensions or frame ordering.\n";
            }

            PixelShipGenerator::ShipIdleAnimationSettings structuralSettings = settings;
            structuralSettings.HoverOffset = false;
            const PixelShipGenerator::ShipIdleAnimation structuralAnimation = animator.generate(ship, structuralSettings);
            const uint32_t baseOpaquePixelCount = getOpaquePixelCount(ship.FinalImage);

            for (const PixelShipGenerator::Image& frame : structuralAnimation.Frames)
            {
                const uint32_t opaquePixelCount = getOpaquePixelCount(frame);
                const uint32_t maximumAllowedDelta = std::max(6u, baseOpaquePixelCount / 10u);
                const uint32_t opaqueDelta = opaquePixelCount > baseOpaquePixelCount ? opaquePixelCount - baseOpaquePixelCount : baseOpaquePixelCount - opaquePixelCount;

                if (opaqueDelta > maximumAllowedDelta || !isOpaqueImageEightConnected(frame, recipe.Resolution, recipe.Resolution) || hasIsolatedOpaquePixel(frame, recipe.Resolution, recipe.Resolution) || !validateTaperedExhaust(frame, ship))
                {
                    success = false;
                    std::cerr << recipe.Name << " produced an unreadable/disconnected frame or invalid tapered exhaust.\n";
                    break;
                }
            }

            PixelShipGenerator::ShipIdleAnimationSettings propulsionSettings;
            propulsionSettings.EngineFlicker = true;
            propulsionSettings.LightBlinking = false;
            propulsionSettings.MechanicalMicroMovement = false;
            propulsionSettings.HoverOffset = false;
            propulsionSettings.SmallDetailVariation = false;
            propulsionSettings.Seed = recipe.Seed ^ 0xA0761D6478BD642Full;
            const PixelShipGenerator::ShipIdleAnimation propulsionAnimation = animator.generate(ship, propulsionSettings);
            uint32_t changedExhaustPixels = 0u;
            bool exhaustLengthChanged = false;
            bool hasLengthHeadroom = false;

            for (const PixelShipGenerator::ShipEngineAnimationComponent& component : ship.IdleAnimationMetadata.EngineComponents)
            {
                hasLengthHeadroom = hasLengthHeadroom || component.MinimumExhaustLength < component.ExhaustLength || component.ExhaustLength < component.MaximumExhaustLength;
            }

            for (std::size_t frameIndex = 0u; frameIndex < propulsionAnimation.Frames.size(); ++frameIndex)
            {
                const PixelShipGenerator::Image& frame = propulsionAnimation.Frames[frameIndex];

                if (!validateAnimatedExhaustFrame(frame, ship))
                {
                    success = false;
                    std::cerr << recipe.Name << " produced invalid propulsion exhaust geometry at frame " << frameIndex << ".\n";
                    break;
                }

                changedExhaustPixels += countChangedPixelsInExhaustEnvelope(frame, ship.FinalImage, ship);

                for (const PixelShipGenerator::ShipEngineAnimationComponent& component : ship.IdleAnimationMetadata.EngineComponents)
                {
                    if (getAnimatedExhaustLength(frame, ship, component) != component.ExhaustLength)
                    {
                        exhaustLengthChanged = true;
                    }
                }
            }

            if (hasLengthHeadroom && !exhaustLengthChanged)
            {
                success = false;
                std::cerr << recipe.Name << " has exhaust animation room but never changes flame length.\n";
            }

            PixelShipGenerator::ShipIdleAnimationSettings engineMechanicalSettings;
            engineMechanicalSettings.EngineFlicker = false;
            engineMechanicalSettings.LightBlinking = false;
            engineMechanicalSettings.MechanicalMicroMovement = true;
            engineMechanicalSettings.HoverOffset = false;
            engineMechanicalSettings.SmallDetailVariation = false;
            engineMechanicalSettings.Seed = *propulsionSettings.Seed;
            const PixelShipGenerator::ShipIdleAnimation engineMechanicalAnimation = animator.generate(ship, engineMechanicalSettings);
            uint32_t changedEnginePixels = 0u;

            for (const PixelShipGenerator::Image& frame : engineMechanicalAnimation.Frames)
            {
                changedEnginePixels += countChangedPixelsInMask(frame, ship.FinalImage, ship.EngineMask);
            }

            if (!ship.IdleAnimationMetadata.EngineComponents.empty() && changedExhaustPixels <= changedEnginePixels)
            {
                success = false;
                std::cerr << recipe.Name << " propulsion animation is not visually dominated by exhaust changes.\n";
            }

            PixelShipGenerator::ShipIdleAnimationSettings hoverSettings;
            hoverSettings.EngineFlicker = false;
            hoverSettings.LightBlinking = false;
            hoverSettings.MechanicalMicroMovement = false;
            hoverSettings.HoverOffset = true;
            hoverSettings.SmallDetailVariation = false;
            hoverSettings.Seed = 0ull;

            const PixelShipGenerator::ShipIdleAnimation hoverAnimation = animator.generate(ship, hoverSettings);

            for (const PixelShipGenerator::Image& frame : hoverAnimation.Frames)
            {
                if (getOpaquePixelCount(frame) != baseOpaquePixelCount)
                {
                    success = false;
                    std::cerr << recipe.Name << " hover animation clipped opaque pixels.\n";
                    break;
                }
            }

            if (recipe.Resolution == 24u)
            {
                PixelShipGenerator::ShipIdleAnimationSettings mechanicalOnly;
                mechanicalOnly.EngineFlicker = false;
                mechanicalOnly.LightBlinking = false;
                mechanicalOnly.MechanicalMicroMovement = true;
                mechanicalOnly.HoverOffset = false;
                mechanicalOnly.SmallDetailVariation = false;
                mechanicalOnly.Seed = 0x1234ull;
                const PixelShipGenerator::ShipIdleAnimation restrainedAnimation = animator.generate(ship, mechanicalOnly);

                for (const PixelShipGenerator::Image& frame : restrainedAnimation.Frames)
                {
                    if (!imagesEqual(frame, ship.FinalImage))
                    {
                        success = false;
                        std::cerr << recipe.Name << " performs mechanical geometry animation at 24x24.\n";
                        break;
                    }
                }
            }
        }
        catch (const std::exception& exception)
        {
            success = false;
            std::cerr << recipe.Name << " animation regression failed: " << exception.what() << '\n';
        }
    }

    try
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = 6896045811037514874ull;
        settings.Dimensions.Width = 96u;
        settings.Dimensions.Height = 96u;
        settings.Style = PixelShipGenerator::ShipStyle::INDUSTRIAL;
        settings.Faction = PixelShipGenerator::ShipFactionType::ASCENDANT;
        const PixelShipGenerator::GeneratedShip ship = generator.generate(settings);
        PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
        animationSettings.Seed = 0x12345678ull;
        animationSettings.HoverOffset = false;
        const PixelShipGenerator::ShipIdleAnimation animation = animator.generate(ship, animationSettings);
        bool techChanged = false;
        bool ventChanged = false;
        bool weaponChanged = false;

        for (const PixelShipGenerator::Image& frame : animation.Frames)
        {
            techChanged = techChanged || maskRegionChanged(frame, ship.FinalImage, ship.IdleAnimationMetadata.MajorFeatureEmissiveMask);
            ventChanged = ventChanged || maskRegionChanged(frame, ship.FinalImage, ship.IdleAnimationMetadata.MajorFeatureMechanicalMask);
            weaponChanged = weaponChanged || maskRegionChanged(frame, ship.FinalImage, ship.IdleAnimationMetadata.WeaponOccupiedMask);
        }

        if (!techChanged || !ventChanged || !weaponChanged)
        {
            success = false;
            std::cerr << "animation_component_coverage did not exercise tech-core, vent-bank and large-weapon animation.\n";
        }
    }
    catch (const std::exception& exception)
    {
        success = false;
        std::cerr << "animation_component_coverage failed: " << exception.what() << '\n';
    }

    try
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = 0x6A09E667F3BCC909ull;
        settings.Dimensions.Width = 64u;
        settings.Dimensions.Height = 64u;
        settings.Style = PixelShipGenerator::ShipStyle::FIGHTER;
        settings.Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
        const PixelShipGenerator::GeneratedShip ship = generator.generate(settings);
        constexpr std::array<uint32_t, 5u> FrameCounts = { 2u, 3u, 4u, 6u, 12u };

        for (uint32_t frameCount : FrameCounts)
        {
            PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
            animationSettings.FrameCount = frameCount;
            animationSettings.EngineFlicker = true;
            animationSettings.LightBlinking = false;
            animationSettings.MechanicalMicroMovement = false;
            animationSettings.HoverOffset = false;
            animationSettings.SmallDetailVariation = false;
            animationSettings.Seed = 0xBB67AE8584CAA73Bull;
            const PixelShipGenerator::ShipIdleAnimation first = animator.generate(ship, animationSettings);
            const PixelShipGenerator::ShipIdleAnimation second = animator.generate(ship, animationSettings);
            bool foundAnimatedFrame = false;

            if (first.Frames.size() != frameCount || second.Frames.size() != frameCount || !imagesEqual(first.Frames.front(), ship.FinalImage) || !imagesEqual(first.Frames.back(), ship.FinalImage))
            {
                success = false;
                std::cerr << "variable_frame_count_" << frameCount << " did not preserve frame count or static loop anchors.\n";
                continue;
            }

            for (std::size_t frameIndex = 0u; frameIndex < first.Frames.size(); ++frameIndex)
            {
                if (!imagesEqual(first.Frames[frameIndex], second.Frames[frameIndex]) || !validateAnimatedExhaustFrame(first.Frames[frameIndex], ship))
                {
                    success = false;
                    std::cerr << "variable_frame_count_" << frameCount << " is non-deterministic or produced invalid exhaust geometry.\n";
                    break;
                }

                if (frameIndex > 0u && frameIndex + 1u < first.Frames.size() && !imagesEqual(first.Frames[frameIndex], ship.FinalImage))
                {
                    foundAnimatedFrame = true;
                }
            }

            if (frameCount >= 3u && !foundAnimatedFrame)
            {
                success = false;
                std::cerr << "variable_frame_count_" << frameCount << " did not produce a useful propulsion state.\n";
            }
        }
    }
    catch (const std::exception& exception)
    {
        success = false;
        std::cerr << "variable_frame_count_coverage failed: " << exception.what() << '\n';
    }

    for (const PropulsionLayoutRecipe& recipe : PropulsionLayoutRecipes)
    {
        try
        {
            PixelShipGenerator::ShipGenerationSettings settings;
            settings.Seed = recipe.Seed;
            settings.Dimensions.Width = recipe.Resolution;
            settings.Dimensions.Height = recipe.Resolution;
            settings.Style = recipe.Style;
            settings.Faction = recipe.Faction;
            PixelShipGenerator::ShipGenerationDebugInfo debugInfo;
            const PixelShipGenerator::GeneratedShip ship = generator.generate(settings, &debugInfo);

            if (debugInfo.EngineLayout != recipe.ExpectedLayout)
            {
                success = false;
                std::cerr << recipe.Name << " no longer generates the expected engine layout.\n";
                continue;
            }

            PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
            animationSettings.EngineFlicker = true;
            animationSettings.LightBlinking = false;
            animationSettings.MechanicalMicroMovement = false;
            animationSettings.HoverOffset = false;
            animationSettings.SmallDetailVariation = false;
            animationSettings.Seed = recipe.Seed ^ 0xE7037ED1A0B428DBull;
            const PixelShipGenerator::ShipIdleAnimation animation = animator.generate(ship, animationSettings);
            bool changed = false;

            for (const PixelShipGenerator::Image& frame : animation.Frames)
            {
                if (!validateAnimatedExhaustFrame(frame, ship))
                {
                    success = false;
                    std::cerr << recipe.Name << " produced invalid exhaust geometry.\n";
                    break;
                }

                changed = changed || countChangedPixelsInExhaustEnvelope(frame, ship.FinalImage, ship) > 0u;
            }

            if (!changed)
            {
                success = false;
                std::cerr << recipe.Name << " did not animate its exhaust.\n";
            }
        }
        catch (const std::exception& exception)
        {
            success = false;
            std::cerr << recipe.Name << " layout propulsion coverage failed: " << exception.what() << '\n';
        }
    }

    return success ? 0 : 1;
}
