#include "MajorFeatureGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        uint32_t getMajorFeatureComplexityCost(ShipMajorFeatureType type)
        {
            switch (type)
            {
            case ShipMajorFeatureType::CENTRAL_SPINE: return 16u;
            case ShipMajorFeatureType::ARMOR_PLATE: return 15u;
            case ShipMajorFeatureType::RECESSED_BAY: return 12u;
            case ShipMajorFeatureType::VENT_BANK: return 10u;
            case ShipMajorFeatureType::WING_PLATE: return 14u;
            case ShipMajorFeatureType::TECH_CORE: return 16u;
            default: return 0u;
            }
        }
    }
    void MajorFeatureGenerator::generate(ShipGenerationContext& context) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        context.MajorFeatures.reset(width, height);
        context.Ship.IdleAnimationMetadata.MajorFeatureMechanicalMask.clear(false);
        context.Ship.IdleAnimationMetadata.MajorFeatureEmissiveMask.clear(false);
        context.Ship.IdleAnimationMetadata.MajorFeatureComponents.clear();

        if (width == 0u || height == 0u)
        {
            return;
        }

        const PixelMaskUtils::MaskBounds hullBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);

        if (!hullBounds.Valid)
        {
            return;
        }

        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::MAJOR_FEATURE, 10u))
        {
            return;
        }

        const FactionMajorFeatureProfile factionProfile = resolveFactionProfile(context.FactionProfile.MajorFeatures);
        uint32_t generationChance = static_cast<uint32_t>((static_cast<uint64_t>(context.Profile.MajorFeatureChance) * factionProfile.ChancePercent + 50u) / 100u);
        const uint32_t scaleChancePercent = std::min(100u, 35u + (context.ScaleTraits.MajorFeatureCapacity * 65u + 20u) / 40u);
        generationChance = static_cast<uint32_t>((static_cast<uint64_t>(generationChance) * scaleChancePercent + 50u) / 100u);
        if (context.VisualHierarchy.InfluenceEnabled)
        {
            if (context.VisualHierarchy.targets(ShipVisualAnchorType::MAJOR_FEATURE))
            {
                generationChance = static_cast<uint32_t>((static_cast<uint64_t>(generationChance) * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::MAJOR_FEATURE) + 50u) / 100u);
                if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::MAJOR_FEATURE)) { generationChance = std::max(generationChance, 86u); }
            }
            else
            {
                generationChance = generationChance * context.VisualHierarchy.getCompetingFeaturePercent(ShipVisualAnchorType::MAJOR_FEATURE) / 100u;
            }
        }
        generationChance = std::min(100u, generationChance);

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedMajorFeaturePresence.has_value())
        {
            const bool normallyGenerated = generationChance > 0u && context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 0u, 99u) < generationChance;
            (void)normallyGenerated;
            if (!*context.CalibrationSettings->Overrides.ForcedMajorFeaturePresence) { return; }
        }
        else if (generationChance == 0u || context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 0u, 99u) >= generationChance)
        {
            return;
        }

        const uint32_t scaleMaximumFeatureCount = std::min(3u, 1u + context.ScaleTraits.MajorFeatureCapacity / 30u);
        const uint32_t maximumFeatureCount = std::min(context.Profile.MaximumMajorFeatures, scaleMaximumFeatureCount);

        if (maximumFeatureCount == 0u)
        {
            return;
        }

        const uint32_t targetFeatureCount = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 1u, maximumFeatureCount);
        std::array<bool, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> usedTypes = {};

        for (uint32_t featureIndex = 0u; featureIndex < targetFeatureCount; ++featureIndex)
        {
            bool placed = false;

            for (uint32_t attempt = 0u; attempt < MaximumFeaturePlacementAttempts; ++attempt)
            {
                const ShipMajorFeatureType type = selectFeatureType(context, factionProfile, usedTypes);

                if (type == ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)
                {
                    break;
                }

                if (context.DebugInfo != nullptr)
                {
                    ++context.DebugInfo->MajorFeaturePlacementAttemptCount;
                }

                CandidateFeature candidate(width, height);
                candidate.Type = type;

                if (!generateCandidate(context, type, candidate) || !validateCandidate(context, candidate))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->MajorFeaturePlacementRejectionCount;
                    }

                    continue;
                }

                const uint32_t complexityCost = getMajorFeatureComplexityCost(type);
                const auto spatialRegions = context.SpatialBudget.collectRegions(candidate.OccupiedMask);
                const bool dominant = complexityCost >= 12u;
                const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, complexityCost, dominant);
                if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 0u, 99u) >= spatialAcceptance)
                {
                    context.SpatialBudget.recordRejection(spatialRegions);
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->MajorFeaturePlacementRejectionCount;
                    }
                    continue;
                }

                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::MAJOR_FEATURE, complexityCost))
                {
                    break;
                }

                context.SpatialBudget.consume(spatialRegions, complexityCost, dominant);
                commitCandidate(context, candidate);
                usedTypes[static_cast<std::size_t>(type)] = true;
                placed = true;
                break;
            }

            if (!placed)
            {
                break;
            }
        }

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->MajorFeatureCount = static_cast<uint32_t>(context.MajorFeatures.Placements.size());
            context.DebugInfo->MajorFeaturePixelCount = PixelMaskUtils::getMaskPixelCount(context.MajorFeatures.OccupiedMask);
            context.DebugInfo->MajorFeatureRaisedPixelCount = PixelMaskUtils::getMaskPixelCount(context.MajorFeatures.RaisedMask);
            context.DebugInfo->MajorFeatureRecessedPixelCount = PixelMaskUtils::getMaskPixelCount(context.MajorFeatures.RecessedMask);
            context.DebugInfo->MajorFeatureMechanicalPixelCount = PixelMaskUtils::getMaskPixelCount(context.MajorFeatures.MechanicalMask);
            context.DebugInfo->MajorFeatureEmissivePixelCount = PixelMaskUtils::getMaskPixelCount(context.MajorFeatures.EmissiveMask);
        }
    }

    ShipMajorFeatureType MajorFeatureGenerator::selectFeatureType(ShipGenerationContext& context, const FactionMajorFeatureProfile& factionProfile, const std::array<bool, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)>& usedTypes) const
    {
        std::array<uint64_t, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> weights = {};
        uint64_t totalWeight = 0u;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END); ++index)
        {
            const ShipMajorFeatureType type = static_cast<ShipMajorFeatureType>(index);

            if (usedTypes[index])
            {
                continue;
            }

            if (type == ShipMajorFeatureType::WING_PLATE && (!context.WingRegions.hasWings() || context.WingRegions.MaximumExtension < 2u))
            {
                continue;
            }

            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::MAJOR_FEATURE, getMajorFeatureComplexityCost(type)))
            {
                continue;
            }

            const uint64_t styleWeight = getStyleFeatureWeight(context.Profile, type);
            const uint64_t factionMultiplier = factionProfile.WeightMultipliers[index];
            weights[index] = (styleWeight * factionMultiplier + 50u) / 100u;
            totalWeight += weights[index];
        }

        if (totalWeight == 0u)
        {
            return ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END;
        }

        uint64_t roll = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 0u, static_cast<uint32_t>(std::min<uint64_t>(totalWeight - 1u, UINT32_MAX)));

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedMajorFeatureType.has_value())
        {
            const ShipMajorFeatureType forced = *context.CalibrationSettings->Overrides.ForcedMajorFeatureType;
            const std::size_t forcedIndex = static_cast<std::size_t>(forced);
            if (forced != ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END && forcedIndex < usedTypes.size() && !usedTypes[forcedIndex] && weights[forcedIndex] > 0u)
            {
                return forced;
            }
        }

        for (uint32_t index = 0u; index < weights.size(); ++index)
        {
            if (roll < weights[index])
            {
                return static_cast<ShipMajorFeatureType>(index);
            }

            roll -= weights[index];
        }

        return ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END;
    }

    bool MajorFeatureGenerator::generateCandidate(ShipGenerationContext& context, ShipMajorFeatureType type, CandidateFeature& candidate) const
    {
        switch (type)
        {
        case ShipMajorFeatureType::CENTRAL_SPINE: return generateCentralSpine(context, candidate);
        case ShipMajorFeatureType::ARMOR_PLATE: return generateArmorPlate(context, candidate);
        case ShipMajorFeatureType::RECESSED_BAY: return generateRecessedBay(context, candidate);
        case ShipMajorFeatureType::VENT_BANK: return generateVentBank(context, candidate);
        case ShipMajorFeatureType::WING_PLATE: return generateWingPlate(context, candidate);
        case ShipMajorFeatureType::TECH_CORE: return generateTechCore(context, candidate);
        default: return false;
        }
    }

    bool MajorFeatureGenerator::generateCentralSpine(ShipGenerationContext& context, CandidateFeature& candidate) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t scalePercent = context.Profile.MajorFeatureScalePercent;
        const uint32_t minimumLength = std::max(3u, scaleFeaturePixels(7u, hullHeight, scalePercent));
        const uint32_t maximumLength = std::max(minimumLength, scaleFeaturePixels(14u, hullHeight, scalePercent));
        const uint32_t length = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, minimumLength, std::min(maximumLength, std::max(minimumLength, hullHeight / 2u)));
        const uint32_t earliestStart = bounds.MinY + hullHeight * 30u / 100u;
        const uint32_t latestStart = bounds.MinY + hullHeight * 58u / 100u;

        if (earliestStart >= bounds.MaxY || length >= hullHeight)
        {
            return false;
        }

        const uint32_t startY = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, earliestStart, std::min(latestStart, bounds.MaxY - length + 1u));
        const uint32_t maximumHalfWidth = std::max(1u, std::min(scaleFeaturePixels(2u, context.Ship.HullMask.getWidth(), scalePercent), 3u));
        const uint32_t halfWidth = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 1u, maximumHalfWidth);
        candidate.Region = MajorFeatureRegion::CENTRAL_FUSELAGE;
        candidate.Symmetric = true;

        for (uint32_t offsetY = 0u; offsetY < length; ++offsetY)
        {
            uint32_t rowHalfWidth = halfWidth;

            if (halfWidth > 1u && (offsetY == 0u || offsetY + 1u == length))
            {
                --rowHalfWidth;
            }

            if (!addCenteredRow(context, candidate, startY + offsetY, rowHalfWidth, candidate.RaisedMask))
            {
                return false;
            }
        }

        return true;
    }

    bool MajorFeatureGenerator::generateArmorPlate(ShipGenerationContext& context, CandidateFeature& candidate) const
    {
        const uint32_t height = context.Ship.HullMask.getHeight();
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t scalePercent = context.Profile.MajorFeatureScalePercent;

        if (context.WingRegions.hasWings() && PixelMaskUtils::getMaskPixelCount(context.WingRegions.WingRootMask) >= 8u && context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 0u, 99u) < 55u)
        {
            const PixelMaskUtils::MaskBounds rootBounds = PixelMaskUtils::calculateMaskBounds(context.WingRegions.WingRootMask);
            const uint32_t availableHeight = rootBounds.MaxY - rootBounds.MinY + 1u;
            const uint32_t plateHeight = std::min(availableHeight, std::max(2u, scaleFeaturePixels(4u, height, scalePercent)));
            const uint32_t startY = rootBounds.MinY + (availableHeight - plateHeight) / 2u;
            candidate.Region = MajorFeatureRegion::WING_ROOT;
            candidate.Symmetric = true;

            for (uint32_t y = startY; y < startY + plateHeight; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (!context.WingRegions.WingRootMask.get(x, y))
                    {
                        continue;
                    }

                    if ((y == startY || y + 1u == startY + plateHeight) && (x == rootBounds.MinX || x == rootBounds.MaxX))
                    {
                        continue;
                    }

                    if (!addCandidatePixel(context, candidate, x, y, candidate.RaisedMask))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        const PixelMaskUtils::MaskBounds hullBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        const uint32_t hullHeight = hullBounds.MaxY - hullBounds.MinY + 1u;
        const uint32_t plateHeight = std::max(2u, scaleFeaturePixels(4u, height, scalePercent));

        if (plateHeight >= hullHeight)
        {
            return false;
        }

        const uint32_t startY = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, hullBounds.MinY + hullHeight * 38u / 100u, std::min(hullBounds.MinY + hullHeight * 64u / 100u, hullBounds.MaxY - plateHeight + 1u));
        const uint32_t leftCenter = (width - 1u) / 2u;
        candidate.Region = MajorFeatureRegion::CENTRAL_FUSELAGE;
        candidate.Symmetric = true;

        for (uint32_t offsetY = 0u; offsetY < plateHeight; ++offsetY)
        {
            const uint32_t y = startY + offsetY;
            const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;

            if (fuselageHalfWidth < 3u)
            {
                return false;
            }

            const uint32_t fuselageLeft = leftCenter - (fuselageHalfWidth - 1u);
            const uint32_t plateWidth = std::min(std::max(2u, scaleFeaturePixels(4u, width, scalePercent)), std::max(2u, fuselageHalfWidth - 1u));

            for (uint32_t offsetX = 0u; offsetX < plateWidth; ++offsetX)
            {
                if ((offsetY == 0u || offsetY + 1u == plateHeight) && offsetX == 0u)
                {
                    continue;
                }

                if (!addSymmetricCandidatePixel(context, candidate, fuselageLeft + offsetX, y, candidate.RaisedMask))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool MajorFeatureGenerator::generateRecessedBay(ShipGenerationContext& context, CandidateFeature& candidate) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        const uint32_t height = context.Ship.HullMask.getHeight();
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t scalePercent = context.Profile.MajorFeatureScalePercent;
        const uint32_t bayHeight = std::max(3u, scaleFeaturePixels(5u, height, scalePercent));

        if (bayHeight >= hullHeight)
        {
            return false;
        }

        const uint32_t startY = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, bounds.MinY + hullHeight * 50u / 100u, std::min(bounds.MinY + hullHeight * 70u / 100u, bounds.MaxY - bayHeight + 1u));
        uint32_t minimumFuselageHalfWidth = width;

        for (uint32_t y = startY; y < startY + bayHeight; ++y)
        {
            const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;
            minimumFuselageHalfWidth = std::min(minimumFuselageHalfWidth, fuselageHalfWidth);
        }

        if (minimumFuselageHalfWidth < 2u)
        {
            return false;
        }

        const uint32_t maximumHalfWidth = std::min(minimumFuselageHalfWidth - 1u, std::max(1u, scaleFeaturePixels(5u, width, scalePercent)));
        const uint32_t halfWidth = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, std::min(2u, maximumHalfWidth), maximumHalfWidth);
        candidate.Region = MajorFeatureRegion::REAR_FUSELAGE;
        candidate.Symmetric = true;

        for (uint32_t offsetY = 0u; offsetY < bayHeight; ++offsetY)
        {
            uint32_t rowHalfWidth = halfWidth;

            if (halfWidth > 1u && (offsetY == 0u || offsetY + 1u == bayHeight))
            {
                --rowHalfWidth;
            }

            if (!addCenteredRow(context, candidate, startY + offsetY, rowHalfWidth, candidate.RecessedMask))
            {
                return false;
            }
        }

        if (bayHeight >= 4u && halfWidth >= 2u)
        {
            const uint32_t slotY = startY + bayHeight / 2u;
            const uint32_t centerLeft = (width - 1u) / 2u;
            const uint32_t centerRight = width / 2u;
            candidate.MechanicalMask.set(centerLeft, slotY, true);
            candidate.MechanicalMask.set(centerRight, slotY, true);
        }

        return true;
    }

    bool MajorFeatureGenerator::generateVentBank(ShipGenerationContext& context, CandidateFeature& candidate) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        const uint32_t scalePercent = context.Profile.MajorFeatureScalePercent;

        if (context.WingRegions.hasWings() && PixelMaskUtils::getMaskPixelCount(context.WingRegions.WingRootMask) >= 10u && context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, 0u, 99u) < 45u)
        {
            const PixelMaskUtils::MaskBounds rootBounds = PixelMaskUtils::calculateMaskBounds(context.WingRegions.WingRootMask);
            const uint32_t availableHeight = rootBounds.MaxY - rootBounds.MinY + 1u;
            const uint32_t bankHeight = std::min(availableHeight, std::max(3u, scaleFeaturePixels(5u, height, scalePercent)));
            const uint32_t startY = rootBounds.MinY + (availableHeight - bankHeight) / 2u;
            candidate.Region = MajorFeatureRegion::WING_ROOT;
            candidate.Symmetric = true;

            for (uint32_t y = startY; y < startY + bankHeight; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (!context.WingRegions.WingRootMask.get(x, y))
                    {
                        continue;
                    }

                    PixelMask& layer = (y - startY) % 2u == 0u ? candidate.MechanicalMask : candidate.RecessedMask;

                    if (!addCandidatePixel(context, candidate, x, y, layer))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t bankHeight = std::max(3u, scaleFeaturePixels(5u, height, scalePercent));

        if (bankHeight >= hullHeight)
        {
            return false;
        }

        const uint32_t startY = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, bounds.MinY + hullHeight * 58u / 100u, std::min(bounds.MinY + hullHeight * 76u / 100u, bounds.MaxY - bankHeight + 1u));
        const uint32_t leftCenter = (width - 1u) / 2u;
        candidate.Region = MajorFeatureRegion::REAR_FUSELAGE;
        candidate.Symmetric = true;

        for (uint32_t offsetY = 0u; offsetY < bankHeight; ++offsetY)
        {
            const uint32_t y = startY + offsetY;
            const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;

            if (fuselageHalfWidth < 3u)
            {
                return false;
            }

            const uint32_t fuselageLeft = leftCenter - (fuselageHalfWidth - 1u);
            const uint32_t bankWidth = std::min(std::max(2u, scaleFeaturePixels(4u, width, scalePercent)), std::max(2u, fuselageHalfWidth - 1u));

            for (uint32_t offsetX = 0u; offsetX < bankWidth; ++offsetX)
            {
                PixelMask& layer = offsetY % 2u == 0u ? candidate.MechanicalMask : candidate.RecessedMask;

                if (!addSymmetricCandidatePixel(context, candidate, fuselageLeft + offsetX, y, layer))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool MajorFeatureGenerator::generateWingPlate(ShipGenerationContext& context, CandidateFeature& candidate) const
    {
        if (!context.WingRegions.hasWings() || context.WingRegions.MaximumExtension < 2u)
        {
            return false;
        }

        const PixelMask& wingMask = context.WingRegions.WingMask;
        const uint32_t width = wingMask.getWidth();
        const uint32_t bandHalfHeight = std::max(1u, scaleFeaturePixels(2u, wingMask.getHeight(), context.Profile.MajorFeatureScalePercent));
        const uint32_t startY = context.WingRegions.PeakY > bandHalfHeight ? std::max(context.WingRegions.StartY, context.WingRegions.PeakY - bandHalfHeight) : context.WingRegions.StartY;
        const uint32_t endY = std::min(context.WingRegions.EndY, context.WingRegions.PeakY + bandHalfHeight);
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        candidate.Region = MajorFeatureRegion::WING;
        candidate.Symmetric = true;

        for (uint32_t y = startY; y <= endY; ++y)
        {
            const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;

            if (fuselageHalfWidth == 0u)
            {
                continue;
            }

            const uint32_t fuselageLeft = leftCenter - (fuselageHalfWidth - 1u);
            const uint32_t fuselageRight = rightCenter + (fuselageHalfWidth - 1u);
            uint32_t rowExtension = 0u;

            for (uint32_t x = 0u; x < width; ++x)
            {
                if (!wingMask.get(x, y))
                {
                    continue;
                }

                if (x < fuselageLeft) { rowExtension = std::max(rowExtension, fuselageLeft - x); }
                if (x > fuselageRight) { rowExtension = std::max(rowExtension, x - fuselageRight); }
            }

            if (rowExtension < 2u)
            {
                continue;
            }

            const uint32_t innerDistance = std::max(1u, rowExtension / 4u);
            const uint32_t outerDistance = rowExtension > 2u ? rowExtension - 1u : rowExtension;

            for (uint32_t distance = innerDistance; distance <= outerDistance; ++distance)
            {
                const uint32_t leftX = fuselageLeft - distance;
                const uint32_t rightX = fuselageRight + distance;

                if (!wingMask.get(leftX, y) || !wingMask.get(rightX, y))
                {
                    continue;
                }

                if (!addCandidatePixel(context, candidate, leftX, y, candidate.RaisedMask) || !addCandidatePixel(context, candidate, rightX, y, candidate.RaisedMask))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool MajorFeatureGenerator::generateTechCore(ShipGenerationContext& context, CandidateFeature& candidate) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t scalePercent = context.Profile.MajorFeatureScalePercent;
        const uint32_t radiusY = std::max(1u, std::min(scaleFeaturePixels(3u, height, scalePercent), std::max(1u, hullHeight / 6u)));
        const uint32_t radiusX = std::max(1u, std::min(scaleFeaturePixels(3u, width, scalePercent), 4u));
        const uint32_t minimumCenterY = bounds.MinY + hullHeight * 40u / 100u;
        const uint32_t maximumCenterY = bounds.MinY + hullHeight * 64u / 100u;

        if (minimumCenterY < radiusY || maximumCenterY + radiusY > bounds.MaxY)
        {
            return false;
        }

        const uint32_t centerY = context.getGenerationRandomUInt(GenerationDomain::MAJOR_FEATURES, minimumCenterY, maximumCenterY);
        candidate.Region = MajorFeatureRegion::CENTRAL_FUSELAGE;
        candidate.Symmetric = true;

        for (int32_t offsetY = -static_cast<int32_t>(radiusY); offsetY <= static_cast<int32_t>(radiusY); ++offsetY)
        {
            const uint32_t absoluteY = static_cast<uint32_t>(offsetY < 0 ? -offsetY : offsetY);
            const uint32_t rowHalfWidth = std::max(1u, radiusX - (absoluteY * radiusX) / (radiusY + 1u));
            const uint32_t y = static_cast<uint32_t>(static_cast<int32_t>(centerY) + offsetY);

            if (!addCenteredRow(context, candidate, y, rowHalfWidth, candidate.RecessedMask))
            {
                return false;
            }
        }

        const uint32_t innerHalfWidth = std::max(1u, radiusX / 2u);
        const uint32_t innerRadiusY = std::max(1u, radiusY / 2u);

        for (int32_t offsetY = -static_cast<int32_t>(innerRadiusY); offsetY <= static_cast<int32_t>(innerRadiusY); ++offsetY)
        {
            const uint32_t y = static_cast<uint32_t>(static_cast<int32_t>(centerY) + offsetY);

            if (!addCenteredRow(context, candidate, y, innerHalfWidth, candidate.EmissiveMask))
            {
                return false;
            }
        }

        return true;
    }

    bool MajorFeatureGenerator::validateCandidate(const ShipGenerationContext& context, const CandidateFeature& candidate) const
    {
        const uint32_t occupiedPixels = PixelMaskUtils::getMaskPixelCount(candidate.OccupiedMask);
        const uint32_t hullPixels = PixelMaskUtils::getMaskPixelCount(context.Ship.HullMask);
        const uint32_t minimumDimension = std::min(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight());
        const uint32_t minimumPixels = std::max(3u, GenerationMath::scalePixelsFrom64(6u, minimumDimension));

        if (occupiedPixels < minimumPixels || hullPixels == 0u)
        {
            return false;
        }

        const uint32_t existingPixels = PixelMaskUtils::getMaskPixelCount(context.MajorFeatures.OccupiedMask);

        if ((static_cast<uint64_t>(existingPixels + occupiedPixels) * 100u) > static_cast<uint64_t>(hullPixels) * 28u)
        {
            return false;
        }

        const uint32_t visiblePixels = PixelMaskUtils::getMaskPixelCount(candidate.RaisedMask) + PixelMaskUtils::getMaskPixelCount(candidate.RecessedMask) + PixelMaskUtils::getMaskPixelCount(candidate.MechanicalMask) + PixelMaskUtils::getMaskPixelCount(candidate.EmissiveMask);

        if (visiblePixels < std::max(2u, occupiedPixels / 3u))
        {
            return false;
        }

        for (uint32_t y = 0u; y < candidate.OccupiedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < candidate.OccupiedMask.getWidth(); ++x)
            {
                if (!candidate.OccupiedMask.get(x, y))
                {
                    continue;
                }

                if (!isFeaturePixelAvailable(context, x, y))
                {
                    return false;
                }

                if (occupiedPixels > 3u && PixelMaskUtils::getMaskNeighbourCount(candidate.OccupiedMask, static_cast<int32_t>(x), static_cast<int32_t>(y)) == 0u)
                {
                    return false;
                }
            }
        }

        return true;
    }

    void MajorFeatureGenerator::commitCandidate(ShipGenerationContext& context, const CandidateFeature& candidate) const
    {
        PixelMaskUtils::mergeMask(context.MajorFeatures.OccupiedMask, candidate.OccupiedMask);
        PixelMaskUtils::mergeMask(context.MajorFeatures.RaisedMask, candidate.RaisedMask);
        PixelMaskUtils::mergeMask(context.MajorFeatures.RecessedMask, candidate.RecessedMask);
        PixelMaskUtils::mergeMask(context.MajorFeatures.MechanicalMask, candidate.MechanicalMask);
        PixelMaskUtils::mergeMask(context.MajorFeatures.EmissiveMask, candidate.EmissiveMask);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(candidate.OccupiedMask);
        MajorFeaturePlacement placement;
        placement.Type = candidate.Type;
        placement.Region = candidate.Region;
        placement.MinX = bounds.MinX;
        placement.MaxX = bounds.MaxX;
        placement.MinY = bounds.MinY;
        placement.MaxY = bounds.MaxY;
        placement.Symmetric = candidate.Symmetric;
        context.MajorFeatures.Placements.push_back(placement);

        if (candidate.Type == ShipMajorFeatureType::VENT_BANK || candidate.Type == ShipMajorFeatureType::TECH_CORE)
        {
            PixelMaskUtils::mergeMask(context.Ship.IdleAnimationMetadata.MajorFeatureMechanicalMask, candidate.MechanicalMask);
            PixelMaskUtils::mergeMask(context.Ship.IdleAnimationMetadata.MajorFeatureEmissiveMask, candidate.EmissiveMask);

            ShipMajorFeatureAnimationComponent animationComponent;
            animationComponent.Type = candidate.Type;
            animationComponent.MinimumX = bounds.MinX;
            animationComponent.MaximumX = bounds.MaxX;
            animationComponent.MinimumY = bounds.MinY;
            animationComponent.MaximumY = bounds.MaxY;
            animationComponent.Symmetric = candidate.Symmetric;
            context.Ship.IdleAnimationMetadata.MajorFeatureComponents.push_back(animationComponent);
        }

        if (context.DebugInfo != nullptr)
        {
            ++context.DebugInfo->MajorFeatureTypeCounts[static_cast<std::size_t>(candidate.Type)];
        }
    }

    bool MajorFeatureGenerator::addCandidatePixel(const ShipGenerationContext& context, CandidateFeature& candidate, uint32_t x, uint32_t y, PixelMask& layerMask, bool reserveOnly) const
    {
        if (!isFeaturePixelAvailable(context, x, y))
        {
            return false;
        }

        candidate.OccupiedMask.set(x, y, true);

        if (!reserveOnly)
        {
            layerMask.set(x, y, true);
        }

        return true;
    }

    bool MajorFeatureGenerator::addSymmetricCandidatePixel(const ShipGenerationContext& context, CandidateFeature& candidate, uint32_t x, uint32_t y, PixelMask& layerMask, bool reserveOnly) const
    {
        if (x >= context.Ship.HullMask.getWidth())
        {
            return false;
        }

        const uint32_t mirroredX = context.Ship.HullMask.getWidth() - 1u - x;

        if (!isFeaturePixelAvailable(context, x, y) || !isFeaturePixelAvailable(context, mirroredX, y))
        {
            return false;
        }

        candidate.OccupiedMask.set(x, y, true);
        candidate.OccupiedMask.set(mirroredX, y, true);

        if (!reserveOnly)
        {
            layerMask.set(x, y, true);
            layerMask.set(mirroredX, y, true);
        }

        return true;
    }

    bool MajorFeatureGenerator::addCenteredRow(const ShipGenerationContext& context, CandidateFeature& candidate, uint32_t y, uint32_t halfWidth, PixelMask& layerMask) const
    {
        if (halfWidth == 0u || y >= context.Ship.HullMask.getHeight())
        {
            return false;
        }

        const uint32_t leftCenter = (context.Ship.HullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = context.Ship.HullMask.getWidth() / 2u;

        if (halfWidth > leftCenter + 1u)
        {
            return false;
        }

        const uint32_t leftX = leftCenter - (halfWidth - 1u);
        const uint32_t rightX = rightCenter + (halfWidth - 1u);

        for (uint32_t x = leftX; x <= rightX; ++x)
        {
            if (!isFuselagePixel(context, x, y) || !addCandidatePixel(context, candidate, x, y, layerMask))
            {
                return false;
            }
        }

        return true;
    }

    bool MajorFeatureGenerator::isFuselagePixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        if (!context.Ship.HullMask.isInBounds(x, y) || !context.Ship.HullMask.get(x, y))
        {
            return false;
        }

        if (y >= context.WingRegions.FuselageHalfWidths.size() || context.WingRegions.FuselageHalfWidths[y] == 0u)
        {
            return true;
        }

        const uint32_t halfWidth = context.WingRegions.FuselageHalfWidths[y];
        const uint32_t leftCenter = (context.Ship.HullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = context.Ship.HullMask.getWidth() / 2u;
        const uint32_t leftX = leftCenter - (halfWidth - 1u);
        const uint32_t rightX = rightCenter + (halfWidth - 1u);
        return x >= leftX && x <= rightX;
    }

    bool MajorFeatureGenerator::isFeaturePixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        if (!context.Ship.HullMask.isInBounds(x, y) || !context.Ship.HullMask.get(x, y))
        {
            return false;
        }

        if (context.Ship.CockpitMask.get(x, y) || context.Ship.EngineMask.get(x, y) || context.Ship.EngineExhaustMask.get(x, y))
        {
            return false;
        }

        if (context.MajorFeatures.OccupiedMask.get(x, y))
        {
            return false;
        }

        if (PixelMaskUtils::hasNeighbouringMaskPixel(context.MajorFeatures.OccupiedMask, static_cast<int32_t>(x), static_cast<int32_t>(y)))
        {
            return false;
        }

        return true;
    }

    bool MajorFeatureGenerator::isWingFeaturePixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        return context.WingRegions.WingMask.isInBounds(x, y) && context.WingRegions.WingMask.get(x, y) && isFeaturePixelAvailable(context, x, y);
    }

    uint32_t MajorFeatureGenerator::scaleFeaturePixels(uint32_t value, uint32_t dimension, uint32_t scalePercent) const
    {
        const uint32_t scaled = GenerationMath::scalePixelsFrom64(value, dimension);
        return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(scaled) * scalePercent + 50u) / 100u));
    }

    MajorFeatureGenerator::FactionMajorFeatureProfile MajorFeatureGenerator::resolveFactionProfile(const ShipFactionMajorFeatureProfile& source) const
    {
        FactionMajorFeatureProfile profile;
        profile.ChancePercent = source.ChancePercent;
        for (std::size_t index = 0u; index < profile.WeightMultipliers.size(); ++index)
        {
            profile.WeightMultipliers[index] = source.WeightMultipliersPercent.getWeightPercent(static_cast<ShipMajorFeatureType>(index));
        }
        return profile;
    }

    uint32_t MajorFeatureGenerator::getStyleFeatureWeight(const ShipGenerationProfile& profile, ShipMajorFeatureType type) const
    {
        return profile.MajorFeatureWeights.getWeight(type);
    }
}
