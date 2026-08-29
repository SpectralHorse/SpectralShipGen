#include "RegressionSuites.h"

#include <algorithm>
#include <array>
#include <cmath>
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

            if (firstAnimation.FrameWidth != recipe.Resolution || firstAnimation.FrameHeight != recipe.Resolution || firstAnimation.Frames.size() != firstAnimation.Sampling.ActualFrameCount || firstAnimation.Frames.size() < settings.MinimumFrameCount || firstAnimation.Frames.size() > settings.MaximumFrameCount)
            {
                success = false;
                std::cerr << recipe.Name << " returned unexpected animation dimensions or adaptive frame count.\n";
                continue;
            }

            if (firstAnimation.DurationMilliseconds != settings.AnimationDurationMilliseconds || std::abs(firstAnimation.FrameDurationMilliseconds * static_cast<double>(firstAnimation.Frames.size()) - static_cast<double>(settings.AnimationDurationMilliseconds)) > 0.0001)
            {
                success = false;
                std::cerr << recipe.Name << " did not preserve semantic duration / derived frame duration.\n";
            }

            if (firstAnimation.NormalizedSampleTimes.size() != firstAnimation.Frames.size() || firstAnimation.NormalizedSampleTimes.empty() || firstAnimation.NormalizedSampleTimes.front() != 0.0 || firstAnimation.NormalizedSampleTimes.back() >= 1.0)
            {
                success = false;
                std::cerr << recipe.Name << " did not sample the normalized [0,1) timeline correctly.\n";
            }
            else
            {
                for (std::size_t frameIndex = 0u; frameIndex < firstAnimation.NormalizedSampleTimes.size(); ++frameIndex)
                {
                    const double expectedTime = static_cast<double>(frameIndex) / static_cast<double>(firstAnimation.Frames.size());
                    if (std::abs(firstAnimation.NormalizedSampleTimes[frameIndex] - expectedTime) > 0.000000000001)
                    {
                        success = false;
                        std::cerr << recipe.Name << " does not sample frameIndex / ActualFrameCount exactly.\n";
                        break;
                    }
                }
            }

            if (firstAnimation.Frames.empty() || !imagesEqual(firstAnimation.Frames.front(), ship.FinalImage) || !imagesEqual(animator.evaluateFrameAtNormalizedTime(ship, 0.0, settings), ship.FinalImage))
            {
                success = false;
                std::cerr << recipe.Name << " frame 0 does not exactly equal the static generated image.\n";
            }

            if (!imagesEqual(animator.evaluateFrameAtNormalizedTime(ship, 1.0, settings), ship.FinalImage))
            {
                success = false;
                std::cerr << recipe.Name << " looping t=1 evaluation does not wrap to the static t=0 state.\n";
            }

            if (firstAnimation.Frames.size() != secondAnimation.Frames.size() || firstAnimation.Sampling.ActualFrameCount != secondAnimation.Sampling.ActualFrameCount || firstAnimation.Sampling.ActiveAnimatedComponentCount != secondAnimation.Sampling.ActiveAnimatedComponentCount || firstAnimation.Sampling.IndependentPhaseGroupCount != secondAnimation.Sampling.IndependentPhaseGroupCount || firstAnimation.Sampling.MaximumMechanicalTravelPixels != secondAnimation.Sampling.MaximumMechanicalTravelPixels || firstAnimation.Sampling.MaximumExhaustTravelPixels != secondAnimation.Sampling.MaximumExhaustTravelPixels)
            {
                success = false;
                std::cerr << recipe.Name << " is not deterministic in adaptive sampling diagnostics.\n";
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
            const std::size_t expectedSpritesheetPixelCount = static_cast<std::size_t>(recipe.Resolution) * firstAnimation.Sampling.ActualFrameCount * recipe.Resolution;
            if (spritesheet.getPixels().size() != expectedSpritesheetPixelCount || !validateSpritesheetOrder(firstAnimation, spritesheet))
            {
                success = false;
                std::cerr << recipe.Name << " spritesheet has incorrect actual-count dimensions or frame ordering.\n";
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
                    if (getAnimatedExhaustLength(frame, ship, component) != component.ExhaustLength) { exhaustLengthChanged = true; }
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
            for (const PixelShipGenerator::Image& frame : engineMechanicalAnimation.Frames) { changedEnginePixels += countChangedPixelsInMask(frame, ship.FinalImage, ship.EngineMask); }
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

    // Preserve the existing optional-component coverage fixture. This is intentionally not replaced by Task-67-specific output hashes.
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

    // Equivalent normalized positions must be frame-density independent.
    try
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = 0x6A09E667F3BCC909ull;
        settings.Dimensions.Width = 96u;
        settings.Dimensions.Height = 64u;
        settings.Style = PixelShipGenerator::ShipStyle::INDUSTRIAL;
        settings.Faction = PixelShipGenerator::ShipFactionType::XENO;
        const PixelShipGenerator::GeneratedShip ship = generator.generate(settings);
        constexpr std::array<uint32_t, 3u> FrameCounts = { 10u, 20u, 60u };
        constexpr std::array<uint32_t, 2u> Numerators = { 3u, 6u };
        constexpr std::array<uint32_t, 2u> Denominator = { 10u, 10u };
        std::array<PixelShipGenerator::ShipIdleAnimation, 3u> animations;

        for (std::size_t densityIndex = 0u; densityIndex < FrameCounts.size(); ++densityIndex)
        {
            PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
            animationSettings.AnimationDurationMilliseconds = 1500u;
            animationSettings.FrameCount = FrameCounts[densityIndex];
            animationSettings.SamplingMode = PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT;
            animationSettings.Seed = 0xBB67AE8584CAA73Bull;
            animations[densityIndex] = animator.generate(ship, animationSettings);
            if (animations[densityIndex].Frames.size() != FrameCounts[densityIndex] || animations[densityIndex].NormalizedSampleTimes.back() >= 1.0)
            {
                success = false;
                std::cerr << "normalized_density_" << FrameCounts[densityIndex] << " did not preserve exact test density or [0,1) sampling.\n";
            }
        }

        std::size_t uniqueSixtyFrameStates = 0u;
        for (std::size_t frameIndex = 0u; frameIndex < animations[2].Frames.size(); ++frameIndex)
        {
            bool seenEarlier = false;
            for (std::size_t earlierIndex = 0u; earlierIndex < frameIndex; ++earlierIndex)
            {
                if (imagesEqual(animations[2].Frames[frameIndex], animations[2].Frames[earlierIndex]))
                {
                    seenEarlier = true;
                    break;
                }
            }
            if (!seenEarlier) { ++uniqueSixtyFrameStates; }
        }
        if (uniqueSixtyFrameStates <= 10u)
        {
            success = false;
            std::cerr << "60-frame normalized sampling still collapses to approximately ten raster states.\n";
        }

        for (std::size_t sampleIndex = 0u; sampleIndex < Numerators.size(); ++sampleIndex)
        {
            const double normalizedTime = static_cast<double>(Numerators[sampleIndex]) / static_cast<double>(Denominator[sampleIndex]);
            const PixelShipGenerator::Image direct = animator.evaluateFrameAtNormalizedTime(ship, normalizedTime, [&]() { PixelShipGenerator::ShipIdleAnimationSettings s; s.Seed = 0xBB67AE8584CAA73Bull; return s; }());
            const uint32_t index10 = static_cast<uint32_t>(normalizedTime * 10.0 + 0.5);
            const uint32_t index20 = static_cast<uint32_t>(normalizedTime * 20.0 + 0.5);
            const uint32_t index60 = static_cast<uint32_t>(normalizedTime * 60.0 + 0.5);
            if (!imagesEqual(animations[0].Frames[index10], animations[1].Frames[index20]) || !imagesEqual(animations[0].Frames[index10], animations[2].Frames[index60]) || !imagesEqual(animations[0].Frames[index10], direct))
            {
                success = false;
                std::cerr << "normalized-time equivalence failed at t=" << normalizedTime << ".\n";
            }
        }
    }
    catch (const std::exception& exception)
    {
        success = false;
        std::cerr << "normalized-time equivalence coverage failed: " << exception.what() << '\n';
    }

    // Adaptive sampling must react to actual animated complexity rather than canvas resolution alone.
    try
    {
        PixelShipGenerator::ShipGenerationSettings complexSettings;
        complexSettings.Seed = 0xA4093822299F31D0ull;
        complexSettings.Dimensions.Width = 96u;
        complexSettings.Dimensions.Height = 96u;
        complexSettings.Style = PixelShipGenerator::ShipStyle::INDUSTRIAL;
        complexSettings.Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
        const PixelShipGenerator::GeneratedShip complexShip = generator.generate(complexSettings);
        PixelShipGenerator::ShipIdleAnimationSettings adaptiveSettings;
        adaptiveSettings.Seed = 0x082EFA98EC4E6C89ull;
        const PixelShipGenerator::ShipIdleAnimation complexAnimation = animator.generate(complexShip, adaptiveSettings);

        PixelShipGenerator::ShipGenerationSettings largeSimpleSettings;
        largeSimpleSettings.Seed = 0x452821E638D01377ull;
        largeSimpleSettings.Dimensions.Width = 160u;
        largeSimpleSettings.Dimensions.Height = 160u;
        largeSimpleSettings.Style = PixelShipGenerator::ShipStyle::SLEEK;
        largeSimpleSettings.Faction = PixelShipGenerator::ShipFactionType::CORPORATE;
        const PixelShipGenerator::GeneratedShip largeSimpleShip = generator.generate(largeSimpleSettings);
        const PixelShipGenerator::ShipIdleAnimation naturallySimpleLargeAnimation = animator.generate(largeSimpleShip, adaptiveSettings);
        if (complexAnimation.Sampling.ActiveAnimatedComponentCount <= naturallySimpleLargeAnimation.Sampling.ActiveAnimatedComponentCount || complexAnimation.Sampling.ActualFrameCount <= naturallySimpleLargeAnimation.Sampling.ActualFrameCount)
        {
            success = false;
            std::cerr << "complex smaller ship did not receive denser adaptive sampling than naturally simpler larger ship.\n";
        }

        PixelShipGenerator::ShipIdleAnimationSettings noEffects = adaptiveSettings;
        noEffects.EngineFlicker = false;
        noEffects.LightBlinking = false;
        noEffects.MechanicalMicroMovement = false;
        noEffects.HoverOffset = false;
        noEffects.SmallDetailVariation = false;
        const PixelShipGenerator::ShipIdleAnimation disabledLargeAnimation = animator.generate(largeSimpleShip, noEffects);

        if (disabledLargeAnimation.Sampling.ActiveAnimatedComponentCount != 0u || disabledLargeAnimation.Sampling.ActualFrameCount != noEffects.MinimumFrameCount)
        {
            success = false;
            std::cerr << "large ship with no active animation was forced to extra frames solely by resolution.\n";
        }

        PixelShipGenerator::ShipIdleAnimationSettings legacyCountHint = noEffects;
        legacyCountHint.FrameCount = 60u;
        const PixelShipGenerator::ShipIdleAnimation adaptiveIgnoresLegacyCount = animator.generate(largeSimpleShip, legacyCountHint);
        if (adaptiveIgnoresLegacyCount.Sampling.ActualFrameCount != legacyCountHint.MinimumFrameCount)
        {
            success = false;
            std::cerr << "adaptive sampling still treats legacy FrameCount as a mandatory output count.\n";
        }
    }
    catch (const std::exception& exception)
    {
        success = false;
        std::cerr << "adaptive complexity coverage failed: " << exception.what() << '\n';
    }

    // Rectangular output must use the actual adaptive frame count without changing native frame dimensions.
    try
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = 0x243F6A8885A308D3ull;
        settings.Dimensions.Width = 96u;
        settings.Dimensions.Height = 44u;
        settings.Style = PixelShipGenerator::ShipStyle::SPEARHEAD;
        settings.Faction = PixelShipGenerator::ShipFactionType::RELIC;
        const PixelShipGenerator::GeneratedShip ship = generator.generate(settings);
        PixelShipGenerator::ShipIdleAnimationSettings animationSettings;
        animationSettings.Seed = 0x13198A2E03707344ull;
        const PixelShipGenerator::ShipIdleAnimation animation = animator.generate(ship, animationSettings);
        const PixelShipGenerator::Image sheet = PixelShipGenerator::createHorizontalSpritesheet(animation);
        if (animation.FrameWidth != 96u || animation.FrameHeight != 44u || animation.Frames.empty() || !imagesEqual(animation.Frames.front(), ship.FinalImage) || sheet.getPixels().size() != static_cast<std::size_t>(96u) * animation.Frames.size() * 44u)
        {
            success = false;
            std::cerr << "rectangular adaptive animation/spritesheet dimensions are invalid.\n";
        }
    }
    catch (const std::exception& exception)
    {
        success = false;
        std::cerr << "rectangular adaptive animation coverage failed: " << exception.what() << '\n';
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
