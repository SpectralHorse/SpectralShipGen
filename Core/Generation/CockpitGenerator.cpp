#include "CockpitGenerator.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace SpectralShipGen
{
    namespace
    {
        constexpr std::size_t sizeIndex(CockpitSizeClass sizeClass)
        {
            return static_cast<std::size_t>(sizeClass);
        }

        constexpr std::size_t shapeIndex(CockpitShapeType shapeType)
        {
            return static_cast<std::size_t>(shapeType);
        }

        uint32_t getMinimumHeightPercent(CockpitSizeClass sizeClass)
        {
            switch (sizeClass)
            {
            case CockpitSizeClass::COMPACT: return 7u;
            case CockpitSizeClass::STANDARD: return 11u;
            case CockpitSizeClass::LARGE: return 17u;
            case CockpitSizeClass::MASSIVE: return 24u;
            default: return 8u;
            }
        }

        uint32_t getMaximumHeightPercent(CockpitSizeClass sizeClass)
        {
            switch (sizeClass)
            {
            case CockpitSizeClass::COMPACT: return 12u;
            case CockpitSizeClass::STANDARD: return 18u;
            case CockpitSizeClass::LARGE: return 27u;
            case CockpitSizeClass::MASSIVE: return 38u;
            default: return 14u;
            }
        }

        uint32_t getMinimumHalfWidthDivisor(CockpitSizeClass sizeClass)
        {
            switch (sizeClass)
            {
            case CockpitSizeClass::COMPACT: return 28u;
            case CockpitSizeClass::STANDARD: return 20u;
            case CockpitSizeClass::LARGE: return 14u;
            case CockpitSizeClass::MASSIVE: return 10u;
            default: return 24u;
            }
        }

        uint32_t getMaximumHalfWidthDivisor(CockpitSizeClass sizeClass)
        {
            switch (sizeClass)
            {
            case CockpitSizeClass::COMPACT: return 18u;
            case CockpitSizeClass::STANDARD: return 12u;
            case CockpitSizeClass::LARGE: return 8u;
            case CockpitSizeClass::MASSIVE: return 5u;
            default: return 16u;
            }
        }

        bool sizeClassFeasible(const GenerationScaleTraits& traits, CockpitSizeClass sizeClass)
        {
            switch (sizeClass)
            {
            case CockpitSizeClass::COMPACT:
            case CockpitSizeClass::STANDARD:
                return true;
            case CockpitSizeClass::LARGE:
                return traits.MinimumDimension >= 32u && traits.MajorFeatureCapacity >= 18u;
            case CockpitSizeClass::MASSIVE:
                return traits.MinimumDimension >= 44u && traits.MaximumDimension >= 64u && traits.MajorFeatureCapacity >= 42u;
            default:
                return false;
            }
        }

        uint32_t clampHalfWidth(uint32_t value, uint32_t imageWidth)
        {
            const uint32_t maximum = imageWidth <= 2u ? 1u : (imageWidth - 2u) / 2u;
            return std::clamp(value, 1u, std::max(1u, maximum));
        }
    }

    void CockpitGenerator::generate(ShipGenerationContext& context) const
    {
        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
        {
            generateLegacyCockpit(context);
            return;
        }

        GeneratedShip& ship = context.Ship;
        const ExplicitShipGenerationConfiguration& settings = context.Settings;
        const ShipGenerationProfile& profile = context.Profile;
        ship.CockpitMask.clear(false);
        context.Cockpit.reset(settings.Dimensions.Width, settings.Dimensions.Height);

        const PixelMaskUtils::MaskBounds hullBounds = PixelMaskUtils::calculateMaskBounds(ship.HullMask);
        if (!hullBounds.Valid)
        {
            return;
        }

        const uint32_t hullHeight = hullBounds.MaxY - hullBounds.MinY + 1u;
        const uint32_t hullPixelCount = PixelMaskUtils::getMaskPixelCount(ship.HullMask);

        for (uint32_t attempt = 0u; attempt < MaximumCockpitGenerationAttempts; ++attempt)
        {
            if (context.DebugInfo != nullptr)
            {
                ++context.DebugInfo->CockpitPlacementAttemptCount;
            }

            const CockpitSizeClass sizeClass = selectSizeClass(context, attempt);
            const CockpitShapeType shapeType = selectShapeType(context, sizeClass);
            CandidateCockpit candidate(settings.Dimensions.Width, settings.Dimensions.Height);
            candidate.Data.SizeClass = sizeClass;
            candidate.Data.ShapeType = shapeType;

            uint32_t minimumHeight = std::max(2u, (hullHeight * getMinimumHeightPercent(sizeClass) + 99u) / 100u);
            uint32_t maximumHeight = std::max(minimumHeight, (hullHeight * getMaximumHeightPercent(sizeClass) + 99u) / 100u);
            minimumHeight = std::max(2u, GenerationMath::scaleByPercent(minimumHeight, profile.CockpitHeightPercent));
            maximumHeight = std::max(minimumHeight, GenerationMath::scaleByPercent(maximumHeight, profile.CockpitHeightPercent));

            uint32_t minimumHalfWidth = std::max(1u, settings.Dimensions.Width / getMinimumHalfWidthDivisor(sizeClass));
            uint32_t maximumHalfWidth = std::max(minimumHalfWidth, settings.Dimensions.Width / getMaximumHalfWidthDivisor(sizeClass));
            minimumHalfWidth = clampHalfWidth(GenerationMath::scaleByPercent(minimumHalfWidth, profile.CockpitWidthPercent), settings.Dimensions.Width);
            maximumHalfWidth = clampHalfWidth(std::max(minimumHalfWidth, GenerationMath::scaleByPercent(maximumHalfWidth, profile.CockpitWidthPercent)), settings.Dimensions.Width);

            if (shapeType == CockpitShapeType::ELONGATED_CANOPY)
            {
                minimumHeight = std::max(minimumHeight, maximumHeight * 4u / 5u);
                maximumHalfWidth = std::max(minimumHalfWidth, maximumHalfWidth * 4u / 5u);
            }
            else if (shapeType == CockpitShapeType::WIDE_COMMAND_DECK)
            {
                maximumHeight = std::max(minimumHeight, maximumHeight * 4u / 5u);
                minimumHalfWidth = std::max(minimumHalfWidth, maximumHalfWidth * 3u / 4u);
            }

            maximumHeight = std::min(maximumHeight, hullHeight);
            minimumHeight = std::min(minimumHeight, maximumHeight);
            const uint32_t cockpitHeight = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, minimumHeight, maximumHeight);
            const uint32_t cockpitHalfWidth = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, minimumHalfWidth, maximumHalfWidth);

            const uint32_t minimumStartY = hullBounds.MinY + ((hullHeight * profile.CockpitStartPercent.Min) / 100u);
            const uint32_t maximumProfileStartY = hullBounds.MinY + ((hullHeight * profile.CockpitStartPercent.Max) / 100u);
            if (hullBounds.MaxY + 1u < cockpitHeight)
            {
                continue;
            }
            const uint32_t latestStartY = std::min(maximumProfileStartY, hullBounds.MaxY - cockpitHeight + 1u);
            if (latestStartY < minimumStartY)
            {
                continue;
            }

            const uint32_t startY = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, minimumStartY, latestStartY);
            generateCockpitFootprint(candidate, shapeType, startY, cockpitHeight, cockpitHalfWidth);
            deriveSemanticMasks(candidate);

            const uint32_t maximumCockpitPixelCount = std::max(1u, (hullPixelCount * getMaximumCockpitHullPercent(context, sizeClass)) / 100u);
            if (!isCockpitPlacementValid(candidate, context, maximumCockpitPixelCount))
            {
                continue;
            }

            const uint32_t cost = getCockpitComplexityCost(sizeClass, shapeType);
            const bool dominant = sizeClass == CockpitSizeClass::MASSIVE || (sizeClass == CockpitSizeClass::LARGE && cost >= 8u);
            const auto spatialRegions = context.SpatialBudget.collectRegions(candidate.OccupiedMask);

            if (cost > 0u)
            {
                if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::COCKPIT_STRUCTURE, cost))
                {
                    continue;
                }

                const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, cost, dominant);
                if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::COCKPIT, 0u, 99u) >= spatialAcceptance)
                {
                    context.SpatialBudget.recordRejection(spatialRegions);
                    continue;
                }

                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::COCKPIT_STRUCTURE, cost))
                {
                    continue;
                }
                context.SpatialBudget.consume(spatialRegions, cost, dominant);
            }

            candidate.Data.ComplexityCost = cost;
            ship.CockpitMask = std::move(candidate.OccupiedMask);
            context.Cockpit = std::move(candidate.Data);

            if (context.DebugInfo != nullptr)
            {
                context.DebugInfo->CockpitPlacementSucceeded = true;
                context.DebugInfo->CockpitSize = sizeClass;
                context.DebugInfo->CockpitShape = shapeType;
                context.DebugInfo->CockpitPixelCount = PixelMaskUtils::getMaskPixelCount(ship.CockpitMask);
                context.DebugInfo->CockpitGlassPixelCount = PixelMaskUtils::getMaskPixelCount(context.Cockpit.GlassMask);
                context.DebugInfo->CockpitFramePixelCount = PixelMaskUtils::getMaskPixelCount(context.Cockpit.FrameMask);
                context.DebugInfo->CockpitBasePixelCount = PixelMaskUtils::getMaskPixelCount(context.Cockpit.BaseMask);
                context.DebugInfo->CockpitUpperSectionPixelCount = PixelMaskUtils::getMaskPixelCount(context.Cockpit.UpperSectionMask);
                context.DebugInfo->CockpitComplexityCost = cost;
            }
            return;
        }
    }


    void CockpitGenerator::generateLegacyCockpit(ShipGenerationContext& context) const
    {
        GeneratedShip& ship = context.Ship;
        const ExplicitShipGenerationConfiguration& settings = context.Settings;
        const ShipGenerationProfile& profile = context.Profile;
        ship.CockpitMask.clear(false);
        context.Cockpit.reset(settings.Dimensions.Width, settings.Dimensions.Height);

        uint32_t hullTop = ship.HullMask.getHeight();
        uint32_t hullBottom = 0u;
        for (uint32_t y = 0; y < ship.HullMask.getHeight(); ++y)
        {
            if (PixelMaskUtils::getOccupiedRowWidth(ship.HullMask, y) == 0u) { continue; }
            hullTop = std::min(hullTop, y);
            hullBottom = std::max(hullBottom, y);
        }
        if (hullTop >= ship.HullMask.getHeight()) { return; }

        const uint32_t hullHeight = hullBottom - hullTop + 1u;
        const uint32_t imageWidth = settings.Dimensions.Width;
        const uint32_t imageHeight = settings.Dimensions.Height;
        const uint32_t baseMinimumCockpitHeight = std::max(GenerationMath::scalePixelsFrom64(3u, imageHeight), imageHeight / 16u);
        const uint32_t baseMaximumCockpitHeight = std::max(baseMinimumCockpitHeight, imageHeight / 9u);
        const uint32_t baseMinimumCockpitHalfWidth = std::max(GenerationMath::scalePixelsFrom64(2u, imageWidth), imageWidth / 32u);
        const uint32_t baseMaximumCockpitHalfWidth = std::max(baseMinimumCockpitHalfWidth, imageWidth / 12u);
        const uint32_t minimumCockpitHeight = GenerationMath::scaleByPercent(baseMinimumCockpitHeight, profile.CockpitHeightPercent);
        const uint32_t maximumCockpitHeight = std::max(minimumCockpitHeight, GenerationMath::scaleByPercent(baseMaximumCockpitHeight, profile.CockpitHeightPercent));
        const uint32_t minimumCockpitHalfWidth = GenerationMath::scaleByPercent(baseMinimumCockpitHalfWidth, profile.CockpitWidthPercent);
        const uint32_t maximumCockpitHalfWidth = std::max(minimumCockpitHalfWidth, GenerationMath::scaleByPercent(baseMaximumCockpitHalfWidth, profile.CockpitWidthPercent));
        const uint32_t minimumStartY = hullTop + ((hullHeight * profile.CockpitStartPercent.Min) / 100u);
        const uint32_t maximumStartY = hullTop + ((hullHeight * profile.CockpitStartPercent.Max) / 100u);
        const uint32_t hullPixelCount = PixelMaskUtils::getMaskPixelCount(ship.HullMask);
        const uint32_t maximumCockpitPixelCount = std::max(1u, (hullPixelCount * profile.MaximumCockpitHullPercent) / 100u);

        for (uint32_t attempt = 0; attempt < 12u; ++attempt)
        {
            if (context.DebugInfo != nullptr) { ++context.DebugInfo->CockpitPlacementAttemptCount; }
            PixelMask candidate(settings.Dimensions.Width, settings.Dimensions.Height, false);
            const uint32_t cockpitHeight = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, minimumCockpitHeight, maximumCockpitHeight);
            const uint32_t cockpitHalfWidth = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, minimumCockpitHalfWidth, maximumCockpitHalfWidth);
            const uint32_t latestStartY = hullBottom >= cockpitHeight ? std::min(maximumStartY, hullBottom - cockpitHeight + 1u) : minimumStartY;
            if (latestStartY < minimumStartY) { continue; }
            const uint32_t startY = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, minimumStartY, latestStartY);
            const LegacyCockpitShape shape = static_cast<LegacyCockpitShape>(context.getGenerationRandomUInt(GenerationDomain::COCKPIT, 0u, static_cast<uint32_t>(LegacyCockpitShape::LEGACY_COCKPIT_SHAPE_END) - 1u));
            generateLegacyCockpitShape(candidate, shape, startY, cockpitHeight, cockpitHalfWidth);
            if (PixelMaskUtils::getMaskPixelCount(candidate) > maximumCockpitPixelCount) { continue; }
            if (!isLegacyCockpitPlacementValid(candidate, ship.HullMask)) { continue; }
            ship.CockpitMask = std::move(candidate);
            context.Cockpit.GlassMask = ship.CockpitMask;
            if (context.DebugInfo != nullptr)
            {
                context.DebugInfo->CockpitPlacementSucceeded = true;
                context.DebugInfo->CockpitPixelCount = PixelMaskUtils::getMaskPixelCount(ship.CockpitMask);
                context.DebugInfo->CockpitGlassPixelCount = context.DebugInfo->CockpitPixelCount;
            }
            return;
        }
    }

    void CockpitGenerator::generateLegacyCockpitShape(PixelMask& cockpitMask, LegacyCockpitShape shape, uint32_t startY, uint32_t height, uint32_t maximumHalfWidth) const
    {
        if (height == 0u || maximumHalfWidth == 0u) { return; }
        const uint32_t lastRow = height - 1u;
        for (uint32_t row = 0; row < height; ++row)
        {
            uint32_t halfWidth = 1u;
            switch (shape)
            {
            case LegacyCockpitShape::FORWARD_TAPER:
                halfWidth = lastRow == 0u ? maximumHalfWidth : 1u + (((maximumHalfWidth - 1u) * row) / lastRow);
                break;
            case LegacyCockpitShape::CANOPY:
            {
                const uint32_t middle = height / 2u;
                const uint32_t distanceFromMiddle = row > middle ? row - middle : middle - row;
                const uint32_t reduction = maximumHalfWidth > 1u ? std::min(maximumHalfWidth - 1u, distanceFromMiddle) : 0u;
                halfWidth = maximumHalfWidth - reduction;
                break;
            }
            case LegacyCockpitShape::DIAMOND:
            {
                const uint32_t middle = height / 2u;
                if (row <= middle)
                {
                    halfWidth = middle == 0u ? maximumHalfWidth : 1u + (((maximumHalfWidth - 1u) * row) / middle);
                }
                else
                {
                    const uint32_t lowerSpan = lastRow - middle;
                    halfWidth = lowerSpan == 0u ? maximumHalfWidth : 1u + (((maximumHalfWidth - 1u) * (lastRow - row)) / lowerSpan);
                }
                break;
            }
            default: return;
            }
            PixelMaskUtils::setSymmetricRowWidth(cockpitMask, startY + row, PixelMaskUtils::getSymmetricWidth(cockpitMask, halfWidth));
        }
    }

    bool CockpitGenerator::isLegacyCockpitPlacementValid(const PixelMask& cockpitMask, const PixelMask& hullMask) const
    {
        bool hasCockpitPixel = false;
        for (uint32_t y = 0; y < cockpitMask.getHeight(); ++y)
        {
            for (uint32_t x = 0; x < cockpitMask.getWidth(); ++x)
            {
                if (!cockpitMask.get(x, y)) { continue; }
                hasCockpitPixel = true;
                if (!hullMask.get(x, y)) { return false; }
            }
        }
        return hasCockpitPixel;
    }

    CockpitSizeClass CockpitGenerator::selectSizeClass(ShipGenerationContext& context, uint32_t attempt) const
    {
        if (attempt + 4u >= MaximumCockpitGenerationAttempts)
        {
            return (attempt % 2u == 0u) ? CockpitSizeClass::STANDARD : CockpitSizeClass::COMPACT;
        }

        std::array<uint32_t, static_cast<std::size_t>(CockpitSizeClass::COCKPIT_SIZE_CLASS_END)> weights = {};
        uint32_t totalWeight = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(CockpitSizeClass::COCKPIT_SIZE_CLASS_END); ++index)
        {
            const CockpitSizeClass sizeClass = static_cast<CockpitSizeClass>(index);
            if (!sizeClassFeasible(context.ScaleTraits, sizeClass)) { continue; }
            uint32_t weight = context.Profile.CockpitSizeWeights.getWeight(sizeClass);
            weight = static_cast<uint32_t>((static_cast<uint64_t>(weight) * context.FactionProfile.Cockpit.SizeWeightMultipliersPercent.getWeightPercent(sizeClass) + 50u) / 100u);
            if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::COCKPIT))
            {
                const uint32_t influence = context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::COCKPIT);
                if (sizeClass == CockpitSizeClass::COMPACT) { weight = weight * 55u / 100u; }
                else if (sizeClass == CockpitSizeClass::STANDARD) { weight = weight * 110u / 100u; }
                else { weight = static_cast<uint32_t>((static_cast<uint64_t>(weight) * influence + 50u) / 100u); }
            }
            if (sizeClass == CockpitSizeClass::LARGE)
            {
                weight = static_cast<uint32_t>((static_cast<uint64_t>(weight) * (55u + context.ScaleTraits.MajorFeatureCapacity) + 50u) / 100u);
            }
            else if (sizeClass == CockpitSizeClass::MASSIVE)
            {
                weight = static_cast<uint32_t>((static_cast<uint64_t>(weight) * (30u + context.ScaleTraits.MajorFeatureCapacity) + 50u) / 100u);
            }
            weights[index] = weight;
            totalWeight += weight;
        }

        if (totalWeight == 0u) { return CockpitSizeClass::COMPACT; }
        uint32_t roll = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, 0u, totalWeight - 1u);
        for (uint32_t index = 0u; index < weights.size(); ++index)
        {
            if (roll < weights[index]) { return static_cast<CockpitSizeClass>(index); }
            roll -= weights[index];
        }
        return CockpitSizeClass::STANDARD;
    }

    CockpitShapeType CockpitGenerator::selectShapeType(ShipGenerationContext& context, CockpitSizeClass sizeClass) const
    {
        std::array<uint32_t, static_cast<std::size_t>(CockpitShapeType::COCKPIT_SHAPE_TYPE_END)> weights = {};
        uint32_t totalWeight = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(CockpitShapeType::COCKPIT_SHAPE_TYPE_END); ++index)
        {
            const CockpitShapeType shapeType = static_cast<CockpitShapeType>(index);
            if ((shapeType == CockpitShapeType::DORSAL_BRIDGE || shapeType == CockpitShapeType::LAYERED_BRIDGE) && sizeClass == CockpitSizeClass::COMPACT) { continue; }
            if (shapeType == CockpitShapeType::LAYERED_BRIDGE && sizeClass < CockpitSizeClass::LARGE) { continue; }
            if (shapeType == CockpitShapeType::WIDE_COMMAND_DECK && context.ScaleTraits.HorizontalCapacity + 12u < context.ScaleTraits.LongitudinalCapacity) { continue; }

            uint32_t weight = context.Profile.CockpitShapeWeights.getWeight(shapeType);
            weight = static_cast<uint32_t>((static_cast<uint64_t>(weight) * context.FactionProfile.Cockpit.ShapeWeightMultipliersPercent.getWeightPercent(shapeType) + 50u) / 100u);
            if (shapeType == CockpitShapeType::ELONGATED_CANOPY && context.ScaleTraits.LongitudinalCapacity > context.ScaleTraits.HorizontalCapacity)
            {
                weight = weight * 5u / 4u;
            }
            if (shapeType == CockpitShapeType::WIDE_COMMAND_DECK && context.ScaleTraits.HorizontalCapacity > context.ScaleTraits.LongitudinalCapacity)
            {
                weight = weight * 5u / 4u;
            }
            weights[index] = weight;
            totalWeight += weight;
        }

        if (totalWeight == 0u) { return CockpitShapeType::COMPACT_CANOPY; }
        uint32_t roll = context.getGenerationRandomUInt(GenerationDomain::COCKPIT, 0u, totalWeight - 1u);
        for (uint32_t index = 0u; index < weights.size(); ++index)
        {
            if (roll < weights[index]) { return static_cast<CockpitShapeType>(index); }
            roll -= weights[index];
        }
        return CockpitShapeType::COMPACT_CANOPY;
    }

    void CockpitGenerator::generateCockpitFootprint(CandidateCockpit& candidate, CockpitShapeType shapeType, uint32_t startY, uint32_t height, uint32_t maximumHalfWidth) const
    {
        if (height == 0u || maximumHalfWidth == 0u) { return; }
        const uint32_t lastRow = height - 1u;

        for (uint32_t row = 0u; row < height; ++row)
        {
            uint32_t halfWidth = 1u;
            switch (shapeType)
            {
            case CockpitShapeType::COMPACT_CANOPY:
            case CockpitShapeType::SPLIT_CANOPY:
            {
                const uint32_t middle = height / 2u;
                if (row <= middle)
                {
                    halfWidth = middle == 0u ? maximumHalfWidth : 1u + ((maximumHalfWidth - 1u) * row) / middle;
                }
                else
                {
                    const uint32_t span = lastRow - middle;
                    const uint32_t taper = span == 0u ? 0u : ((maximumHalfWidth - 1u) * (row - middle)) / std::max(1u, span * 2u);
                    halfWidth = std::max(1u, maximumHalfWidth - taper);
                }
                break;
            }
            case CockpitShapeType::ELONGATED_CANOPY:
            {
                const uint32_t forwardSpan = std::max(1u, height * 2u / 3u);
                if (row < forwardSpan)
                {
                    halfWidth = 1u + ((maximumHalfWidth - 1u) * row) / forwardSpan;
                }
                else
                {
                    const uint32_t tailSpan = std::max(1u, lastRow - forwardSpan + 1u);
                    const uint32_t reduction = ((maximumHalfWidth - 1u) * (row - forwardSpan)) / (tailSpan * 3u);
                    halfWidth = std::max(1u, maximumHalfWidth - reduction);
                }
                break;
            }
            case CockpitShapeType::WIDE_COMMAND_DECK:
            {
                halfWidth = maximumHalfWidth;
                if (row == 0u || row == lastRow) { halfWidth = std::max(1u, maximumHalfWidth - 1u); }
                if (height >= 5u && (row == 1u || row + 2u == height)) { halfWidth = std::max(1u, maximumHalfWidth - 1u); }
                break;
            }
            case CockpitShapeType::DORSAL_BRIDGE:
            {
                if (row * 4u < height) { halfWidth = std::max(1u, maximumHalfWidth * 2u / 3u); }
                else if (row * 2u < height) { halfWidth = std::max(1u, maximumHalfWidth * 5u / 6u); }
                else { halfWidth = maximumHalfWidth; }
                break;
            }
            case CockpitShapeType::LAYERED_BRIDGE:
            {
                if (row * 3u < height) { halfWidth = std::max(1u, maximumHalfWidth * 2u / 3u); }
                else { halfWidth = maximumHalfWidth; }
                if (row == lastRow) { halfWidth = std::max(1u, maximumHalfWidth - 1u); }
                break;
            }
            default:
                return;
            }

            if (startY + row < candidate.OccupiedMask.getHeight())
            {
                PixelMaskUtils::setSymmetricRowWidth(candidate.OccupiedMask, startY + row, PixelMaskUtils::getSymmetricWidth(candidate.OccupiedMask, halfWidth));
            }
        }
    }

    void CockpitGenerator::deriveSemanticMasks(CandidateCockpit& candidate) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(candidate.OccupiedMask);
        if (!bounds.Valid) { return; }

        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!candidate.OccupiedMask.get(x, y)) { continue; }
                const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(candidate.OccupiedMask, x, y);
                if (exposure.Bottom)
                {
                    candidate.Data.BaseMask.set(x, y, true);
                }
                else if (exposure.isBoundary())
                {
                    candidate.Data.FrameMask.set(x, y, true);
                }
                else
                {
                    candidate.Data.GlassMask.set(x, y, true);
                }
            }
        }

        // Preserve readable glass on small shapes where every pixel would otherwise
        // become boundary framing.
        for (uint32_t y = bounds.MinY; y < bounds.MaxY; ++y)
        {
            uint32_t minX = candidate.OccupiedMask.getWidth();
            uint32_t maxX = 0u;
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!candidate.OccupiedMask.get(x, y)) { continue; }
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
            }
            if (minX > maxX || maxX - minX + 1u < 3u) { continue; }
            const uint32_t leftCenter = (candidate.OccupiedMask.getWidth() - 1u) / 2u;
            const uint32_t rightCenter = candidate.OccupiedMask.getWidth() / 2u;
            for (uint32_t x : { leftCenter, rightCenter })
            {
                if (!candidate.OccupiedMask.get(x, y) || candidate.Data.BaseMask.get(x, y)) { continue; }
                candidate.Data.FrameMask.set(x, y, false);
                candidate.Data.GlassMask.set(x, y, true);
            }
        }

        if (candidate.Data.ShapeType == CockpitShapeType::SPLIT_CANOPY)
        {
            const uint32_t leftCenter = (candidate.OccupiedMask.getWidth() - 1u) / 2u;
            const uint32_t rightCenter = candidate.OccupiedMask.getWidth() / 2u;
            for (uint32_t y = bounds.MinY + 1u; y < bounds.MaxY; ++y)
            {
                for (uint32_t x : { leftCenter, rightCenter })
                {
                    if (!candidate.OccupiedMask.get(x, y)) { continue; }
                    candidate.Data.GlassMask.set(x, y, false);
                    candidate.Data.FrameMask.set(x, y, true);
                }
            }
        }

        if (candidate.Data.ShapeType == CockpitShapeType::DORSAL_BRIDGE || candidate.Data.ShapeType == CockpitShapeType::LAYERED_BRIDGE)
        {
            const uint32_t upperEndY = bounds.MinY + std::max(1u, (bounds.MaxY - bounds.MinY + 1u) * 2u / 5u);
            const uint32_t centerX = candidate.OccupiedMask.getWidth() / 2u;
            const uint32_t halfSpan = std::max(1u, (bounds.MaxX - bounds.MinX + 1u) / 4u);
            for (uint32_t y = bounds.MinY; y <= std::min(bounds.MaxY, upperEndY); ++y)
            {
                for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
                {
                    const uint32_t distance = x > centerX ? x - centerX : centerX - x;
                    if (distance <= halfSpan && candidate.Data.GlassMask.get(x, y))
                    {
                        candidate.Data.UpperSectionMask.set(x, y, true);
                    }
                }
            }
        }

        // A layered bridge gets a single deliberate horizontal structural divider.
        if (candidate.Data.ShapeType == CockpitShapeType::LAYERED_BRIDGE && bounds.MaxY > bounds.MinY + 3u)
        {
            const uint32_t separatorY = bounds.MinY + (bounds.MaxY - bounds.MinY + 1u) * 2u / 5u;
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!candidate.Data.GlassMask.get(x, separatorY)) { continue; }
                candidate.Data.GlassMask.set(x, separatorY, false);
                candidate.Data.UpperSectionMask.set(x, separatorY, false);
                candidate.Data.FrameMask.set(x, separatorY, true);
            }
        }
    }

    bool CockpitGenerator::isCockpitPlacementValid(const CandidateCockpit& candidate, const ShipGenerationContext& context, uint32_t maximumCockpitPixelCount) const
    {
        const uint32_t occupiedCount = PixelMaskUtils::getMaskPixelCount(candidate.OccupiedMask);
        if (occupiedCount == 0u || occupiedCount > maximumCockpitPixelCount) { return false; }
        if (PixelMaskUtils::getMaskPixelCount(candidate.Data.GlassMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.Data.FrameMask) == 0u) { return false; }

        for (uint32_t y = 0u; y < candidate.OccupiedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < candidate.OccupiedMask.getWidth(); ++x)
            {
                const bool occupied = candidate.OccupiedMask.get(x, y);
                if (occupied && (!context.Ship.HullMask.get(x, y) || context.WingRegions.WingMask.get(x, y))) { return false; }
                if ((candidate.Data.GlassMask.get(x, y) || candidate.Data.FrameMask.get(x, y) || candidate.Data.BaseMask.get(x, y)) && !occupied) { return false; }
                if (candidate.Data.UpperSectionMask.get(x, y) && !candidate.Data.GlassMask.get(x, y)) { return false; }
                if (candidate.Data.GlassMask.get(x, y) && (candidate.Data.FrameMask.get(x, y) || candidate.Data.BaseMask.get(x, y))) { return false; }
                if (candidate.Data.FrameMask.get(x, y) && candidate.Data.BaseMask.get(x, y)) { return false; }
            }
        }
        return true;
    }

    uint32_t CockpitGenerator::getCockpitComplexityCost(CockpitSizeClass sizeClass, CockpitShapeType shapeType) const
    {
        uint32_t cost = 0u;
        if (sizeClass == CockpitSizeClass::LARGE) { cost = 6u; }
        else if (sizeClass == CockpitSizeClass::MASSIVE) { cost = 12u; }
        if (shapeType == CockpitShapeType::DORSAL_BRIDGE) { cost += 2u; }
        else if (shapeType == CockpitShapeType::LAYERED_BRIDGE) { cost += 4u; }
        else if (shapeType == CockpitShapeType::SPLIT_CANOPY && sizeClass >= CockpitSizeClass::LARGE) { cost += 1u; }
        return cost;
    }

    uint32_t CockpitGenerator::getMaximumCockpitHullPercent(const ShipGenerationContext& context, CockpitSizeClass sizeClass) const
    {
        switch (sizeClass)
        {
        case CockpitSizeClass::COMPACT: return std::min(12u, context.Profile.MaximumCockpitHullPercent);
        case CockpitSizeClass::STANDARD: return std::max(14u, context.Profile.MaximumCockpitHullPercent);
        case CockpitSizeClass::LARGE: return std::max(22u, context.Profile.MaximumCockpitHullPercent);
        case CockpitSizeClass::MASSIVE: return std::max(30u, context.Profile.MaximumCockpitHullPercent);
        default: return context.Profile.MaximumCockpitHullPercent;
        }
    }

}
