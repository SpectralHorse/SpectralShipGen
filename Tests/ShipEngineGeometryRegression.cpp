#include "RegressionSuites.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "ShipFactionType.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{
    bool masksOverlap(const PixelShipGenerator::PixelMask& first, const PixelShipGenerator::PixelMask& second)
    {
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) && second.get(x, y)) { return true; }
            }
        }

        return false;
    }

    bool hasConnectedExhaust(const PixelShipGenerator::GeneratedShip& ship)
    {
        for (uint32_t y = 0u; y < ship.EngineExhaustMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.EngineExhaustMask.getWidth(); ++x)
            {
                if (!ship.EngineExhaustMask.get(x, y)) { continue; }
                if (y == 0u) { return false; }

                bool connected = ship.EngineExhaustMask.get(x, y - 1u) || ship.EngineMask.get(x, y - 1u);
                if (x > 0u) { connected = connected || ship.EngineExhaustMask.get(x - 1u, y - 1u) || ship.EngineMask.get(x - 1u, y - 1u); }
                if (x + 1u < ship.EngineExhaustMask.getWidth()) { connected = connected || ship.EngineExhaustMask.get(x + 1u, y - 1u) || ship.EngineMask.get(x + 1u, y - 1u); }
                if (!connected) { return false; }
            }
        }

        return true;
    }

    uint32_t getExhaustRowWidth(const PixelShipGenerator::PixelMask& mask, const PixelShipGenerator::EngineUnitDebugInfo& unit, uint32_t y)
    {
        uint32_t count = 0u;
        const uint32_t minimumX = unit.NozzleStartX;
        const uint32_t maximumX = std::min(mask.getWidth(), unit.NozzleStartX + unit.NozzleWidth);

        for (uint32_t x = minimumX; x < maximumX; ++x)
        {
            if (mask.get(x, y)) { ++count; }
        }

        return count;
    }


    void verifyMirroredEnginePair(const PixelShipGenerator::EngineUnitDebugInfo& first, const PixelShipGenerator::EngineUnitDebugInfo& second, uint32_t imageWidth)
    {
        if (first.SizeClass != second.SizeClass || first.HousingWidth != second.HousingWidth || first.NozzleWidth != second.NozzleWidth || first.RootStartY != second.RootStartY || first.NozzleY != second.NozzleY || first.ExhaustStartY != second.ExhaustStartY || first.ExhaustLength != second.ExhaustLength || first.Nacelle != second.Nacelle) { throw std::runtime_error("Mirrored engine units do not share matching geometry."); }
        if (first.HousingStartX + second.HousingStartX + first.HousingWidth != imageWidth) { throw std::runtime_error("Mirrored engine housings are not structurally symmetric."); }
        if (first.NozzleStartX + second.NozzleStartX + first.NozzleWidth != imageWidth) { throw std::runtime_error("Mirrored engine nozzles are not structurally symmetric."); }
    }

    void verifyPairedEngineSymmetry(const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo, uint32_t imageWidth)
    {
        switch (debugInfo.EngineLayout)
        {
        case PixelShipGenerator::EngineLayoutType::TWIN:
            if (debugInfo.EngineUnits.size() != 2u) { throw std::runtime_error("TWIN layout engine count mismatch."); }
            verifyMirroredEnginePair(debugInfo.EngineUnits[0], debugInfo.EngineUnits[1], imageWidth);
            break;
        case PixelShipGenerator::EngineLayoutType::QUAD:
            if (debugInfo.EngineUnits.size() != 4u) { throw std::runtime_error("QUAD layout engine count mismatch."); }
            verifyMirroredEnginePair(debugInfo.EngineUnits[0], debugInfo.EngineUnits[3], imageWidth);
            verifyMirroredEnginePair(debugInfo.EngineUnits[1], debugInfo.EngineUnits[2], imageWidth);
            break;
        case PixelShipGenerator::EngineLayoutType::CENTRAL_AUXILIARY:
            if (debugInfo.EngineUnits.size() != 3u) { throw std::runtime_error("CENTRAL_AUXILIARY layout engine count mismatch."); }
            verifyMirroredEnginePair(debugInfo.EngineUnits[0], debugInfo.EngineUnits[2], imageWidth);
            break;
        case PixelShipGenerator::EngineLayoutType::WIDE_BANK:
            if (debugInfo.EngineUnits.size() == 3u) { verifyMirroredEnginePair(debugInfo.EngineUnits[0], debugInfo.EngineUnits[2], imageWidth); }
            else if (debugInfo.EngineUnits.size() == 4u) { verifyMirroredEnginePair(debugInfo.EngineUnits[0], debugInfo.EngineUnits[3], imageWidth); verifyMirroredEnginePair(debugInfo.EngineUnits[1], debugInfo.EngineUnits[2], imageWidth); }
            else { throw std::runtime_error("WIDE_BANK layout engine count mismatch."); }
            break;
        default:
            break;
        }
    }

    void verifyEngineUnit(const PixelShipGenerator::GeneratedShip& ship, const PixelShipGenerator::EngineUnitDebugInfo& unit)
    {
        if (unit.HousingWidth == 0u || unit.NozzleWidth == 0u || unit.NozzleWidth > unit.HousingWidth || unit.ExhaustLength == 0u) { throw std::runtime_error("Invalid engine unit dimensions."); }
        if (unit.HousingStartX + unit.HousingWidth > ship.EngineMask.getWidth()) { throw std::runtime_error("Engine housing exceeds image bounds."); }
        if (unit.NozzleStartX + unit.NozzleWidth > ship.EngineMask.getWidth()) { throw std::runtime_error("Engine nozzle exceeds image bounds."); }
        if (unit.RootStartY >= ship.HullMask.getHeight() || unit.NozzleY >= ship.EngineMask.getHeight()) { throw std::runtime_error("Engine root/nozzle Y coordinate exceeds image bounds."); }
        if (unit.ExhaustStartY != unit.NozzleY + 1u) { throw std::runtime_error("Exhaust does not begin immediately behind its nozzle."); }
        if (unit.ExhaustStartY + unit.ExhaustLength > ship.EngineExhaustMask.getHeight()) { throw std::runtime_error("Engine exhaust exceeds image bounds."); }

        for (uint32_t x = unit.HousingStartX; x < unit.HousingStartX + unit.HousingWidth; ++x)
        {
            if (!ship.HullMask.get(x, unit.RootStartY) || ship.CockpitMask.get(x, unit.RootStartY)) { throw std::runtime_error("Engine root is not validly embedded in the rear hull."); }
        }

        for (uint32_t x = unit.NozzleStartX; x < unit.NozzleStartX + unit.NozzleWidth; ++x)
        {
            if (!ship.EngineMask.get(x, unit.NozzleY)) { throw std::runtime_error("Debug nozzle geometry does not match EngineMask."); }
        }

        uint32_t previousWidth = unit.NozzleWidth;
        uint32_t finalWidth = unit.NozzleWidth;

        for (uint32_t row = 0u; row < unit.ExhaustLength; ++row)
        {
            const uint32_t rowWidth = getExhaustRowWidth(ship.EngineExhaustMask, unit, unit.ExhaustStartY + row);
            if (row == 0u && rowWidth != unit.NozzleWidth) { throw std::runtime_error("Exhaust does not begin at full nozzle width."); }
            if (rowWidth == 0u || rowWidth > previousWidth) { throw std::runtime_error("Exhaust taper is not coherently non-increasing."); }
            previousWidth = rowWidth;
            finalWidth = rowWidth;
        }

        if (unit.NozzleWidth >= 3u && unit.ExhaustLength >= 2u && finalWidth >= unit.NozzleWidth) { throw std::runtime_error("Wide exhaust did not taper."); }
    }
}

int PixelShipGeneratorTests::runEngineGeometryRegression()
{
    const uint32_t resolutions[] = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    PixelShipGenerator::ShipGenerator generator;

    for (uint32_t resolution : resolutions)
    {
        uint32_t maximumHousingWidth = 0u;
        uint32_t generatedEngineShipCount = 0u;

        for (uint32_t styleIndex = 0u; styleIndex < static_cast<uint32_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END); ++styleIndex)
        {
            for (uint32_t sample = 0u; sample < 24u; ++sample)
            {
                PixelShipGenerator::ShipGenerationSettings settings;
                settings.Seed = 0xA24BAED4963EE407ull ^ (static_cast<uint64_t>(resolution) << 32u) ^ (static_cast<uint64_t>(styleIndex) << 24u) ^ sample;
                settings.Dimensions.Width = resolution;
                settings.Dimensions.Height = resolution;
                settings.Style = static_cast<PixelShipGenerator::ShipStyle>(styleIndex);
                settings.Faction = static_cast<PixelShipGenerator::ShipFactionType>(styleIndex % static_cast<uint32_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END));
                PixelShipGenerator::ShipGenerationDebugInfo debugInfo;
                const PixelShipGenerator::GeneratedShip ship = generator.generate(settings, &debugInfo);

                if (masksOverlap(ship.EngineMask, ship.CockpitMask)) { throw std::runtime_error("Engine overlaps cockpit."); }
                if (masksOverlap(ship.EngineExhaustMask, ship.HullMask)) { throw std::runtime_error("Exhaust overlaps hull."); }
                if (masksOverlap(ship.EngineExhaustMask, ship.CockpitMask)) { throw std::runtime_error("Exhaust overlaps cockpit."); }
                if (masksOverlap(ship.EngineExhaustMask, ship.EngineMask)) { throw std::runtime_error("Exhaust overlaps engine geometry."); }
                if (!hasConnectedExhaust(ship)) { throw std::runtime_error("Detached exhaust pixel detected."); }
                if (debugInfo.EngineCount != debugInfo.EngineUnits.size()) { throw std::runtime_error("Engine debug unit count mismatch."); }
                verifyPairedEngineSymmetry(debugInfo, resolution);

                if (debugInfo.EngineCount > 0u)
                {
                    ++generatedEngineShipCount;
                }

                for (const PixelShipGenerator::EngineUnitDebugInfo& unit : debugInfo.EngineUnits)
                {
                    verifyEngineUnit(ship, unit);
                    if (debugInfo.EngineLayout == PixelShipGenerator::EngineLayoutType::QUAD && unit.SizeClass != PixelShipGenerator::EngineSizeClass::SMALL) { throw std::runtime_error("QUAD engine unit exceeded SMALL size class."); }
                    if (debugInfo.EngineLayout == PixelShipGenerator::EngineLayoutType::WIDE_BANK && unit.SizeClass == PixelShipGenerator::EngineSizeClass::LARGE) { throw std::runtime_error("WIDE_BANK engine unit exceeded constrained size class."); }
                    maximumHousingWidth = std::max(maximumHousingWidth, unit.HousingWidth);
                }
            }
        }

        if (generatedEngineShipCount == 0u) { throw std::runtime_error("No engines generated for a supported resolution."); }
        if (resolution >= 32u && maximumHousingWidth < 3u) { throw std::runtime_error("Engine housings did not reach meaningful multi-pixel width."); }
    }

    std::cout << "Ship engine geometry regression passed.\n";
    return 0;
}
