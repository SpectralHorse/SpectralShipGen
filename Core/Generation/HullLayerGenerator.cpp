#include "HullLayerGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "GenerationMath.h"
#include "MacroAsymmetryPlanner.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        constexpr uint32_t MaximumHullLayerPlacementAttempts = 8u;

        uint32_t getFactionLayerChancePercent(ShipFactionType faction)
        {
            switch (faction)
            {
            case ShipFactionType::FRONTIER: return 108u;
            case ShipFactionType::MILITARY: return 105u;
            case ShipFactionType::ASCENDANT: return 88u;
            case ShipFactionType::XENO: return 100u;
            case ShipFactionType::CORPORATE: return 106u;
            case ShipFactionType::RELIC: return 124u;
            default: return 100u;
            }
        }

        uint32_t getOverlapPixelCount(const PixelMask& first, const PixelMask& second)
        {
            const uint32_t width = std::min(first.getWidth(), second.getWidth());
            const uint32_t height = std::min(first.getHeight(), second.getHeight());
            uint32_t count = 0u;
            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (first.get(x, y) && second.get(x, y)) { ++count; }
                }
            }
            return count;
        }
    }

    void HullLayerGenerator::generate(ShipGenerationContext& context) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        context.HullLayers.reset(width, height);
        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->HullLayerMask = context.HullLayers.OccupiedMask;
            context.DebugInfo->HullLayerUpperMask = context.HullLayers.UpperMask;
        }

        if (width == 0u || height == 0u || !context.ComplexityBudget.canAfford(GenerationComplexityCategory::HULL_LAYER, 10u))
        {
            return;
        }

        const uint32_t scaleChance = 22u + (context.ScaleTraits.MajorFeatureCapacity * 78u + 50u) / 100u;
        uint32_t generationChance = static_cast<uint32_t>((static_cast<uint64_t>(context.Profile.HullLayerChance) * scaleChance + 50u) / 100u);
        generationChance = static_cast<uint32_t>((static_cast<uint64_t>(generationChance) * getFactionLayerChancePercent(context.Settings.Faction) + 50u) / 100u);
        if (context.VisualHierarchy.InfluenceEnabled)
        {
            if (context.VisualHierarchy.targets(ShipVisualAnchorType::HULL_LAYERS))
            {
                generationChance = generationChance * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::HULL_LAYERS) / 100u;
                if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::HULL_LAYERS)) { generationChance = std::max(generationChance, 86u); }
            }
            else if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::CENTRAL_CORE))
            {
                generationChance = generationChance * 62u / 100u;
            }
            else
            {
                generationChance = generationChance * context.VisualHierarchy.getCompetingFeaturePercent(ShipVisualAnchorType::HULL_LAYERS) / 100u;
            }
        }
        generationChance = std::min(context.VisualHierarchy.InfluenceEnabled ? 96u : 92u, generationChance);

        if (!context.MacroAsymmetry.targets(MacroAsymmetryCategory::HULL_LAYER) && context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, 99u) >= generationChance)
        {
            return;
        }

        uint32_t maximumLayers = 1u;
        if (context.ScaleTraits.MajorFeatureCapacity >= 42u) { maximumLayers = 2u; }
        if (context.ScaleTraits.MajorFeatureCapacity >= 82u) { maximumLayers = 3u; }
        maximumLayers = std::min(maximumLayers, context.Profile.MaximumHullLayers);
        if (context.Settings.Faction == ShipFactionType::RELIC) { maximumLayers = std::min(maximumLayers, 2u); }
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.isPrimary(ShipVisualAnchorType::HULL_LAYERS) && context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM)
        {
            maximumLayers = std::min(3u, maximumLayers + 1u);
        }

        const uint32_t targetLayerCount = context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 1u, maximumLayers);
        std::array<bool, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)> usedTypes = {};

        for (uint32_t layerIndex = 0u; layerIndex < targetLayerCount; ++layerIndex)
        {
            bool placed = false;
            for (uint32_t attempt = 0u; attempt < MaximumHullLayerPlacementAttempts; ++attempt)
            {
                if (attempt == MaximumHullLayerPlacementAttempts / 2u && context.MacroAsymmetry.targets(MacroAsymmetryCategory::HULL_LAYER))
                {
                    MacroAsymmetryPlanner::reject(context);
                }

                const bool plannedAsymmetry = context.MacroAsymmetry.targets(MacroAsymmetryCategory::HULL_LAYER);
                const ShipHullLayerType type = plannedAsymmetry ? getPlannedAsymmetricLayerType(context) : selectLayerType(context, usedTypes);
                if (type == ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END) { break; }

                CandidateLayer candidate(width, height);
                candidate.Type = type;
                candidate.Order = getLayerOrder(type);

                if (!generateCandidate(context, type, candidate))
                {
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->HullLayerPlacementRejectionCount; }
                    continue;
                }
                if (plannedAsymmetry) { restrictCandidateToMacroSide(context, candidate); }

                if (!validateCandidate(context, candidate))
                {
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->HullLayerPlacementRejectionCount; }
                    continue;
                }

                const uint32_t cost = getLayerComplexityCost(type);
                const bool dominant = plannedAsymmetry || isDominantLayer(type);
                if (plannedAsymmetry && !MacroAsymmetryPlanner::canAcceptCandidate(context, candidate.Mask, cost))
                {
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->HullLayerPlacementRejectionCount; }
                    continue;
                }
                const auto spatialRegions = context.SpatialBudget.collectRegions(candidate.Mask);
                const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, cost, dominant);
                if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, 99u) >= spatialAcceptance)
                {
                    context.SpatialBudget.recordRejection(spatialRegions);
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->HullLayerPlacementRejectionCount; }
                    continue;
                }

                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::HULL_LAYER, cost))
                {
                    break;
                }

                if (plannedAsymmetry) { MacroAsymmetryPlanner::fulfill(context, candidate.Mask, cost); }
                context.SpatialBudget.consume(spatialRegions, cost, dominant);
                commitCandidate(context, candidate);
                usedTypes[static_cast<std::size_t>(type)] = true;
                placed = true;
                break;
            }

            if (!placed) { break; }
        }

        if (context.MacroAsymmetry.targets(MacroAsymmetryCategory::HULL_LAYER)) { MacroAsymmetryPlanner::reject(context); }

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->HullLayerCount = static_cast<uint32_t>(context.HullLayers.Placements.size());
            context.DebugInfo->HullLayerPixelCount = PixelMaskUtils::getMaskPixelCount(context.HullLayers.OccupiedMask);
            context.DebugInfo->HullLayerLowerPixelCount = PixelMaskUtils::getMaskPixelCount(context.HullLayers.LowerMask);
            context.DebugInfo->HullLayerUpperPixelCount = PixelMaskUtils::getMaskPixelCount(context.HullLayers.UpperMask);
            context.DebugInfo->HullLayerMask = context.HullLayers.OccupiedMask;
            context.DebugInfo->HullLayerUpperMask = context.HullLayers.UpperMask;
        }
    }

    ShipHullLayerType HullLayerGenerator::selectLayerType(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)>& usedTypes) const
    {
        std::array<uint64_t, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)> weights = {};
        uint64_t totalWeight = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END); ++index)
        {
            const ShipHullLayerType type = static_cast<ShipHullLayerType>(index);
            if (usedTypes[index]) { continue; }
            if (type == ShipHullLayerType::WING_ARMOR && !context.WingRegions.hasWings()) { continue; }
            if (type == ShipHullLayerType::SHOULDER_ARMOR && !context.WingRegions.hasWings() && context.ScaleTraits.HorizontalCapacity < 35u) { continue; }
            const uint32_t cost = getLayerComplexityCost(type);
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::HULL_LAYER, cost)) { continue; }
            const uint64_t weight = getLayerWeight(context.Profile, context.Settings.Faction, type);
            weights[index] = weight;
            totalWeight += weight;
        }

        if (totalWeight == 0u) { return ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END; }
        uint64_t roll = context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, static_cast<uint32_t>(std::min<uint64_t>(totalWeight - 1u, 0xFFFFFFFFull)));
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END); ++index)
        {
            if (roll < weights[index]) { return static_cast<ShipHullLayerType>(index); }
            roll -= weights[index];
        }
        return ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END;
    }

    bool HullLayerGenerator::generateCandidate(ShipGenerationContext& context, ShipHullLayerType type, CandidateLayer& candidate) const
    {
        switch (type)
        {
        case ShipHullLayerType::CENTRAL_DORSAL_PLATE: return generateCentralDorsalPlate(context, candidate);
        case ShipHullLayerType::FORWARD_ARMOR: return generateForwardArmor(context, candidate);
        case ShipHullLayerType::WING_ARMOR: return generateWingArmor(context, candidate);
        case ShipHullLayerType::SHOULDER_ARMOR: return generateShoulderArmor(context, candidate);
        case ShipHullLayerType::REAR_ENGINE_COVER: return generateRearEngineCover(context, candidate);
        default: return false;
        }
    }

    bool HullLayerGenerator::generateCentralDorsalPlate(ShipGenerationContext& context, CandidateLayer& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const auto cockpitBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.CockpitMask);
        const uint32_t earliest = std::max(bounds.MinY + hullHeight * 30u / 100u, cockpitBounds.Valid ? std::min(bounds.MaxY, cockpitBounds.MaxY + 1u) : bounds.MinY);
        const uint32_t maximumLength = std::max(3u, hullHeight * 38u / 100u);
        if (earliest + 2u > bounds.MaxY) { return false; }
        const uint32_t length = context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, std::min(3u, bounds.MaxY - earliest + 1u), std::min(maximumLength, bounds.MaxY - earliest + 1u));
        const uint32_t startY = earliest;
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        const uint32_t maxHalf = std::max(1u, std::min(4u, GenerationMath::scaleHorizontalPixelsFrom64(3u, context.Settings.Dimensions)));
        const uint32_t halfWidth = context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 1u, maxHalf);

        for (uint32_t row = 0u; row < length; ++row)
        {
            const uint32_t y = startY + row;
            uint32_t rowHalf = halfWidth;
            if (rowHalf > 1u && (row == 0u || row + 1u == length)) { --rowHalf; }
            const uint32_t left = leftCenter >= rowHalf - 1u ? leftCenter - (rowHalf - 1u) : 0u;
            const uint32_t right = std::min(width - 1u, rightCenter + (rowHalf - 1u));
            for (uint32_t x = left; x <= right; ++x)
            {
                if (!addPixel(context, candidate, x, y)) { return false; }
            }
        }
        return true;
    }

    bool HullLayerGenerator::generateForwardArmor(ShipGenerationContext& context, CandidateLayer& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + hullHeight * 22u / 100u;
        const uint32_t endY = std::min(bounds.MaxY, bounds.MinY + hullHeight * 45u / 100u);
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;

        for (uint32_t y = startY; y <= endY; ++y)
        {
            if (y >= context.WingRegions.FuselageHalfWidths.size()) { continue; }
            const uint32_t halfWidth = context.WingRegions.FuselageHalfWidths[y];
            if (halfWidth < 3u) { continue; }
            const uint32_t band = std::max(1u, halfWidth * 45u / 100u);
            const uint32_t innerDistance = std::max(1u, halfWidth / 4u);
            for (uint32_t distance = innerDistance; distance < std::min(halfWidth, innerDistance + band); ++distance)
            {
                const uint32_t leftX = leftCenter >= distance ? leftCenter - distance : 0u;
                const uint32_t rightX = std::min(width - 1u, rightCenter + distance);
                if (isLayerPixelAvailable(context, leftX, y)) { candidate.Mask.set(leftX, y, true); }
                if (isLayerPixelAvailable(context, rightX, y)) { candidate.Mask.set(rightX, y, true); }
            }
        }
        return PixelMaskUtils::getMaskPixelCount(candidate.Mask) >= 4u;
    }

    bool HullLayerGenerator::generateWingArmor(ShipGenerationContext& context, CandidateLayer& candidate) const
    {
        if (!context.WingRegions.hasWings()) { return false; }
        const uint32_t startY = context.WingRegions.StartY;
        const uint32_t endY = context.WingRegions.EndY;
        if (startY >= context.Ship.HullMask.getHeight() || endY < startY) { return false; }
        const uint32_t bandStart = startY + (endY - startY) / 5u;
        const uint32_t bandEnd = endY - (endY - startY) / 5u;
        const uint32_t maximumDepth = context.ScaleTraits.ShadingComplexity >= 60u ? 2u : 1u;
        for (uint32_t y = bandStart; y <= bandEnd; ++y)
        {
            for (uint32_t x = 0u; x < context.Ship.HullMask.getWidth(); ++x)
            {
                if (!context.WingRegions.WingMask.get(x, y) || context.WingRegions.WingRootMask.get(x, y)) { continue; }
                if (PixelMaskUtils::getMaskDepth(context.WingRegions.WingMask, x, y, maximumDepth) == 0u) { continue; }
                if (isLayerPixelAvailable(context, x, y)) { candidate.Mask.set(x, y, true); }
            }
        }
        return PixelMaskUtils::getMaskPixelCount(candidate.Mask) >= 6u;
    }

    bool HullLayerGenerator::generateShoulderArmor(ShipGenerationContext& context, CandidateLayer& candidate) const
    {
        if (context.WingRegions.hasWings() && PixelMaskUtils::getMaskPixelCount(context.WingRegions.WingRootMask) >= 6u)
        {
            const auto rootBounds = PixelMaskUtils::calculateMaskBounds(context.WingRegions.WingRootMask);
            for (uint32_t y = rootBounds.MinY; y <= rootBounds.MaxY; ++y)
            {
                for (uint32_t x = rootBounds.MinX; x <= rootBounds.MaxX; ++x)
                {
                    if (!context.WingRegions.WingRootMask.get(x, y)) { continue; }
                    if (PixelMaskUtils::getMaskDepth(context.WingRegions.WingRootMask, x, y, 1u) == 0u && context.ScaleTraits.MajorFeatureCapacity < 55u) { continue; }
                    if (isLayerPixelAvailable(context, x, y)) { candidate.Mask.set(x, y, true); }
                }
            }
            return PixelMaskUtils::getMaskPixelCount(candidate.Mask) >= 5u;
        }

        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + hullHeight * 42u / 100u;
        const uint32_t endY = std::min(bounds.MaxY, bounds.MinY + hullHeight * 62u / 100u);
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        for (uint32_t y = startY; y <= endY; ++y)
        {
            if (y >= context.WingRegions.FuselageHalfWidths.size()) { continue; }
            const uint32_t halfWidth = context.WingRegions.FuselageHalfWidths[y];
            if (halfWidth < 3u) { continue; }
            const uint32_t outerStart = std::max(1u, halfWidth * 55u / 100u);
            for (uint32_t distance = outerStart; distance < halfWidth; ++distance)
            {
                const uint32_t leftX = leftCenter >= distance ? leftCenter - distance : 0u;
                const uint32_t rightX = std::min(width - 1u, rightCenter + distance);
                if (isLayerPixelAvailable(context, leftX, y)) { candidate.Mask.set(leftX, y, true); }
                if (isLayerPixelAvailable(context, rightX, y)) { candidate.Mask.set(rightX, y, true); }
            }
        }
        return PixelMaskUtils::getMaskPixelCount(candidate.Mask) >= 4u;
    }

    bool HullLayerGenerator::generateRearEngineCover(ShipGenerationContext& context, CandidateLayer& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + hullHeight * 68u / 100u;
        const uint32_t endY = std::min(bounds.MaxY, bounds.MinY + hullHeight * 90u / 100u);
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        for (uint32_t y = startY; y <= endY; ++y)
        {
            if (y >= context.WingRegions.FuselageHalfWidths.size()) { continue; }
            const uint32_t halfWidth = context.WingRegions.FuselageHalfWidths[y];
            if (halfWidth < 2u) { continue; }
            const uint32_t outerStart = std::max(1u, halfWidth / 2u);
            for (uint32_t distance = outerStart; distance < halfWidth; ++distance)
            {
                const uint32_t leftX = leftCenter >= distance ? leftCenter - distance : 0u;
                const uint32_t rightX = std::min(width - 1u, rightCenter + distance);
                if (isLayerPixelAvailable(context, leftX, y)) { candidate.Mask.set(leftX, y, true); }
                if (isLayerPixelAvailable(context, rightX, y)) { candidate.Mask.set(rightX, y, true); }
            }
        }
        return PixelMaskUtils::getMaskPixelCount(candidate.Mask) >= 4u;
    }

    ShipHullLayerType HullLayerGenerator::getPlannedAsymmetricLayerType(const ShipGenerationContext& context) const
    {
        switch (context.MacroAsymmetry.TargetRegion)
        {
        case GenerationSpatialRegion::LEFT_OUTER_WING:
        case GenerationSpatialRegion::RIGHT_OUTER_WING:
            return context.WingRegions.hasWings() ? ShipHullLayerType::WING_ARMOR : ShipHullLayerType::FORWARD_ARMOR;
        case GenerationSpatialRegion::LEFT_WING_ROOT:
        case GenerationSpatialRegion::RIGHT_WING_ROOT:
            return context.WingRegions.hasWings() ? ShipHullLayerType::SHOULDER_ARMOR : ShipHullLayerType::FORWARD_ARMOR;
        default:
            return ShipHullLayerType::FORWARD_ARMOR;
        }
    }

    void HullLayerGenerator::restrictCandidateToMacroSide(const ShipGenerationContext& context, CandidateLayer& candidate) const
    {
        const uint32_t width = candidate.Mask.getWidth();
        for (uint32_t y = 0u; y < candidate.Mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (candidate.Mask.get(x, y) && !context.MacroAsymmetry.isDominantX(x, width))
                {
                    candidate.Mask.set(x, y, false);
                }
            }
        }
    }

    bool HullLayerGenerator::validateCandidate(const ShipGenerationContext& context, const CandidateLayer& candidate) const
    {
        const uint32_t candidatePixels = PixelMaskUtils::getMaskPixelCount(candidate.Mask);
        const uint32_t minimumDimension = std::min(candidate.Mask.getWidth(), candidate.Mask.getHeight());
        const uint32_t minimumPixels = std::max(3u, GenerationMath::scalePixelsFrom64(5u, minimumDimension));
        if (candidatePixels < minimumPixels) { return false; }

        const uint32_t hullPixels = PixelMaskUtils::getMaskPixelCount(context.Ship.HullMask);
        const uint32_t existingPixels = PixelMaskUtils::getMaskPixelCount(context.HullLayers.OccupiedMask);
        if (hullPixels == 0u || static_cast<uint64_t>(existingPixels + candidatePixels) * 100u > static_cast<uint64_t>(hullPixels) * 38u) { return false; }

        const uint32_t overlapPixels = getOverlapPixelCount(candidate.Mask, context.HullLayers.OccupiedMask);
        const uint32_t overlapLimit = candidate.Order == 0u ? 18u : 34u;
        if (candidatePixels > 0u && overlapPixels * 100u > candidatePixels * overlapLimit) { return false; }

        for (uint32_t y = 0u; y < candidate.Mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < candidate.Mask.getWidth(); ++x)
            {
                if (!candidate.Mask.get(x, y)) { continue; }
                if (!isLayerPixelAvailable(context, x, y)) { return false; }
            }
        }
        return true;
    }

    void HullLayerGenerator::commitCandidate(ShipGenerationContext& context, const CandidateLayer& candidate) const
    {
        HullLayerPlacement placement;
        placement.Type = candidate.Type;
        placement.Order = candidate.Order;
        placement.Mask = candidate.Mask;
        const auto bounds = PixelMaskUtils::calculateMaskBounds(candidate.Mask);
        placement.MinX = bounds.MinX;
        placement.MaxX = bounds.MaxX;
        placement.MinY = bounds.MinY;
        placement.MaxY = bounds.MaxY;
        context.HullLayers.Placements.push_back(placement);
        PixelMaskUtils::mergeMask(context.HullLayers.OccupiedMask, candidate.Mask);
        PixelMaskUtils::mergeMask(candidate.Order == 0u ? context.HullLayers.LowerMask : context.HullLayers.UpperMask, candidate.Mask);

        for (uint32_t y = 0u; y < candidate.Mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < candidate.Mask.getWidth(); ++x)
            {
                if (candidate.Mask.get(x, y) && PixelMaskUtils::getDirectionalMaskExposure(candidate.Mask, x, y).isBoundary())
                {
                    context.HullLayers.BoundaryMask.set(x, y, true);
                }
            }
        }

        if (context.DebugInfo != nullptr)
        {
            ++context.DebugInfo->HullLayerTypeCounts[static_cast<std::size_t>(candidate.Type)];
            if (candidate.Order == 0u) { ++context.DebugInfo->HullLayerLowerCount; }
            else { ++context.DebugInfo->HullLayerUpperCount; }
        }
    }

    bool HullLayerGenerator::isLayerPixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        if (!context.Ship.HullMask.isInBounds(x, y) || !context.Ship.HullMask.get(x, y)) { return false; }
        if (context.Ship.CockpitMask.get(x, y) || context.Ship.EngineMask.get(x, y) || context.Ship.EngineExhaustMask.get(x, y)) { return false; }
        return true;
    }

    bool HullLayerGenerator::addPixel(const ShipGenerationContext& context, CandidateLayer& candidate, uint32_t x, uint32_t y) const
    {
        if (!isLayerPixelAvailable(context, x, y)) { return false; }
        candidate.Mask.set(x, y, true);
        return true;
    }

    bool HullLayerGenerator::addSymmetricPixel(const ShipGenerationContext& context, CandidateLayer& candidate, uint32_t x, uint32_t y) const
    {
        if (x >= context.Ship.HullMask.getWidth()) { return false; }
        const uint32_t mirroredX = context.Ship.HullMask.getWidth() - 1u - x;
        if (!isLayerPixelAvailable(context, x, y) || !isLayerPixelAvailable(context, mirroredX, y)) { return false; }
        candidate.Mask.set(x, y, true);
        candidate.Mask.set(mirroredX, y, true);
        return true;
    }

    uint32_t HullLayerGenerator::getLayerComplexityCost(ShipHullLayerType type) const
    {
        switch (type)
        {
        case ShipHullLayerType::CENTRAL_DORSAL_PLATE: return 12u;
        case ShipHullLayerType::FORWARD_ARMOR: return 11u;
        case ShipHullLayerType::WING_ARMOR: return 14u;
        case ShipHullLayerType::SHOULDER_ARMOR: return 13u;
        case ShipHullLayerType::REAR_ENGINE_COVER: return 10u;
        default: return 0u;
        }
    }

    bool HullLayerGenerator::isDominantLayer(ShipHullLayerType type) const
    {
        return type == ShipHullLayerType::WING_ARMOR || type == ShipHullLayerType::SHOULDER_ARMOR || type == ShipHullLayerType::CENTRAL_DORSAL_PLATE;
    }

    uint32_t HullLayerGenerator::getLayerOrder(ShipHullLayerType type) const
    {
        switch (type)
        {
        case ShipHullLayerType::WING_ARMOR:
        case ShipHullLayerType::FORWARD_ARMOR:
        case ShipHullLayerType::REAR_ENGINE_COVER:
            return 0u;
        case ShipHullLayerType::CENTRAL_DORSAL_PLATE:
        case ShipHullLayerType::SHOULDER_ARMOR:
            return 1u;
        default:
            return 0u;
        }
    }

    uint32_t HullLayerGenerator::getLayerWeight(const ShipGenerationProfile& profile, ShipFactionType faction, ShipHullLayerType type) const
    {
        std::array<uint32_t, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)> weights = {
            profile.HullLayerWeights.CentralDorsalPlate,
            profile.HullLayerWeights.ForwardArmor,
            profile.HullLayerWeights.WingArmor,
            profile.HullLayerWeights.ShoulderArmor,
            profile.HullLayerWeights.RearEngineCover
        };

        switch (faction)
        {
        case ShipFactionType::FRONTIER:
            weights[static_cast<std::size_t>(ShipHullLayerType::FORWARD_ARMOR)] += 20u;
            weights[static_cast<std::size_t>(ShipHullLayerType::WING_ARMOR)] += 15u;
            weights[static_cast<std::size_t>(ShipHullLayerType::REAR_ENGINE_COVER)] += 15u;
            break;
        case ShipFactionType::MILITARY:
            weights[static_cast<std::size_t>(ShipHullLayerType::FORWARD_ARMOR)] += 20u;
            weights[static_cast<std::size_t>(ShipHullLayerType::SHOULDER_ARMOR)] += 15u;
            break;
        case ShipFactionType::ASCENDANT:
            weights[static_cast<std::size_t>(ShipHullLayerType::CENTRAL_DORSAL_PLATE)] += 35u;
            weights[static_cast<std::size_t>(ShipHullLayerType::FORWARD_ARMOR)] += 10u;
            weights[static_cast<std::size_t>(ShipHullLayerType::REAR_ENGINE_COVER)] = std::max(1u, weights[static_cast<std::size_t>(ShipHullLayerType::REAR_ENGINE_COVER)] / 2u);
            break;
        case ShipFactionType::XENO:
            weights[static_cast<std::size_t>(ShipHullLayerType::CENTRAL_DORSAL_PLATE)] += 15u;
            weights[static_cast<std::size_t>(ShipHullLayerType::WING_ARMOR)] += 20u;
            break;
        case ShipFactionType::CORPORATE:
            weights[static_cast<std::size_t>(ShipHullLayerType::CENTRAL_DORSAL_PLATE)] += 18u;
            weights[static_cast<std::size_t>(ShipHullLayerType::FORWARD_ARMOR)] += 22u;
            weights[static_cast<std::size_t>(ShipHullLayerType::SHOULDER_ARMOR)] += 18u;
            weights[static_cast<std::size_t>(ShipHullLayerType::REAR_ENGINE_COVER)] += 8u;
            break;
        case ShipFactionType::RELIC:
            weights[static_cast<std::size_t>(ShipHullLayerType::CENTRAL_DORSAL_PLATE)] += 55u;
            weights[static_cast<std::size_t>(ShipHullLayerType::SHOULDER_ARMOR)] += 30u;
            weights[static_cast<std::size_t>(ShipHullLayerType::REAR_ENGINE_COVER)] += 25u;
            weights[static_cast<std::size_t>(ShipHullLayerType::WING_ARMOR)] = std::max(1u, weights[static_cast<std::size_t>(ShipHullLayerType::WING_ARMOR)] * 3u / 4u);
            break;
        default: break;
        }
        return weights[static_cast<std::size_t>(type)];
    }
}
