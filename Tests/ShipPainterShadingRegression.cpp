#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>

#include "AttachmentGenerator.h"
#include "CockpitGenerator.h"
#include "DetailGenerator.h"
#include "EngineGenerator.h"
#include "HullGenerator.h"
#include "MajorFeatureGenerator.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipPainter.h"
#include "ShipPaletteGenerator.h"
#include "WeaponGenerator.h"

namespace
{
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    constexpr std::array<PixelShipGenerator::ShipStyle, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> Styles = { PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipStyle::DELTA };
    constexpr std::array<PixelShipGenerator::ShipFactionType, static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { PixelShipGenerator::ShipFactionType::FRONTIER, PixelShipGenerator::ShipFactionType::MILITARY, PixelShipGenerator::ShipFactionType::ASCENDANT, PixelShipGenerator::ShipFactionType::XENO, PixelShipGenerator::ShipFactionType::CORPORATE, PixelShipGenerator::ShipFactionType::RELIC };
    constexpr uint32_t SamplesPerConfiguration = 2u;
    constexpr uint32_t MaximumHullAttempts = 8u;

    bool generatePipeline(PixelShipGenerator::ShipGenerationContext& context)
    {
        PixelShipGenerator::HullGenerator hullGenerator;
        PixelShipGenerator::CockpitGenerator cockpitGenerator;
        PixelShipGenerator::EngineGenerator engineGenerator;
        PixelShipGenerator::MajorFeatureGenerator majorFeatureGenerator;
        PixelShipGenerator::WeaponGenerator weaponGenerator;
        PixelShipGenerator::AttachmentGenerator attachmentGenerator;
        PixelShipGenerator::DetailGenerator detailGenerator;
        PixelShipGenerator::ShipPainter shipPainter;

        context.Ship.Palette = PixelShipGenerator::ShipPaletteGenerator::generate(context.Seeds.Palette, context.FactionProfile, context.Profile);

        for (uint32_t attempt = 0u; attempt < MaximumHullAttempts; ++attempt)
        {
            context.Ship.clear();
            hullGenerator.generate(context);

            if (!hullGenerator.validate(context))
            {
                continue;
            }

            cockpitGenerator.generate(context);
            engineGenerator.generate(context);
            majorFeatureGenerator.generate(context);
            weaponGenerator.generate(context);

            if (context.Settings.AttachmentsEnabled)
            {
                attachmentGenerator.generate(context);
            }

            detailGenerator.generate(context);
            shipPainter.paint(context);
            return true;
        }

        return false;
    }

    bool imagesEqual(const PixelShipGenerator::Image& first, const PixelShipGenerator::Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool isSemanticPixel(const PixelShipGenerator::ShipGenerationContext& context, uint32_t x, uint32_t y)
    {
        const PixelShipGenerator::GeneratedShip& ship = context.Ship;
        return ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y) || context.Weapons.OccupiedMask.get(x, y);
    }

    bool hasNeighbouringSemanticPixel(const PixelShipGenerator::ShipGenerationContext& context, int32_t x, int32_t y)
    {
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                const int32_t checkX = x + offsetX;
                const int32_t checkY = y + offsetY;

                if (checkX < 0 || checkY < 0 || checkX >= static_cast<int32_t>(context.Ship.FinalImage.getWidth()) || checkY >= static_cast<int32_t>(context.Ship.FinalImage.getHeight()))
                {
                    continue;
                }

                if (isSemanticPixel(context, static_cast<uint32_t>(checkX), static_cast<uint32_t>(checkY)))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool validatePaintedPixels(const PixelShipGenerator::ShipGenerationContext& context)
    {
        const PixelShipGenerator::Image& image = context.Ship.FinalImage;

        for (uint32_t y = 0u; y < image.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < image.getWidth(); ++x)
            {
                const PixelShipGenerator::Color& color = image.getPixel(x, y);

                if (color.A != 0u && color.A != 255u)
                {
                    return false;
                }

                if (color.A == 0u || isSemanticPixel(context, x, y))
                {
                    continue;
                }

                if (!hasNeighbouringSemanticPixel(context, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool maskHasVisiblePixels(const PixelShipGenerator::PixelMask& mask, const PixelShipGenerator::Image& image)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) && image.getPixel(x, y).A == 255u)
                {
                    return true;
                }
            }
        }

        return false;
    }
}

int PixelShipGeneratorTests::runPainterShadingRegression()
{
    for (uint32_t resolution : Resolutions)
    {
        for (PixelShipGenerator::ShipStyle style : Styles)
        {
            for (PixelShipGenerator::ShipFactionType faction : Factions)
            {
                for (uint32_t sample = 0u; sample < SamplesPerConfiguration; ++sample)
                {
                    PixelShipGenerator::ShipGenerationSettings settings;
                    settings.Dimensions.Width = resolution;
                    settings.Dimensions.Height = resolution;
                    settings.Style = style;
                    settings.Faction = faction;
                    settings.Seed = 0x38000000ull + static_cast<uint64_t>(resolution) * 100000ull + static_cast<uint64_t>(static_cast<uint32_t>(style)) * 10000ull + static_cast<uint64_t>(static_cast<uint32_t>(faction)) * 1000ull + sample;
                    const PixelShipGenerator::ShipGenerationSeeds seeds = PixelShipGenerator::deriveShipGenerationSeeds(settings.Seed);
                    const PixelShipGenerator::ShipGenerationProfile& profile = PixelShipGenerator::getShipGenerationProfile(style);
                    PixelShipGenerator::ShipGenerationContext firstContext(settings, profile, seeds, nullptr);
                    PixelShipGenerator::ShipGenerationContext secondContext(settings, profile, seeds, nullptr);

                    if (!generatePipeline(firstContext) || !generatePipeline(secondContext))
                    {
                        std::cerr << "Shading pipeline generation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!imagesEqual(firstContext.Ship.FinalImage, secondContext.Ship.FinalImage))
                    {
                        std::cerr << "Shading determinism failure at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!validatePaintedPixels(firstContext))
                    {
                        std::cerr << "Shading pixel-boundary validation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    const uint32_t cockpitPixels = PixelShipGenerator::PixelMaskUtils::getMaskPixelCount(firstContext.Ship.CockpitMask);
                    const uint32_t enginePixels = PixelShipGenerator::PixelMaskUtils::getMaskPixelCount(firstContext.Ship.EngineMask);

                    if ((cockpitPixels > 0u && !maskHasVisiblePixels(firstContext.Ship.CockpitMask, firstContext.Ship.FinalImage)) || (enginePixels > 0u && !maskHasVisiblePixels(firstContext.Ship.EngineMask, firstContext.Ship.FinalImage)))
                    {
                        std::cerr << "Cockpit or engine readability validation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (PixelShipGenerator::PixelMaskUtils::getMaskPixelCount(firstContext.Weapons.OccupiedMask) > 0u && !maskHasVisiblePixels(firstContext.Weapons.OccupiedMask, firstContext.Ship.FinalImage))
                    {
                        std::cerr << "Weapon readability validation failed at resolution " << resolution << ".\n";
                        return 1;
                    }
                }
            }
        }
    }

    std::cout << "Ship painter shading regression passed.\n";
    return 0;
}
