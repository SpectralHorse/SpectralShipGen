#include "HullGenerator.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        uint32_t getHullModifierComplexityCost(HullModifierType type)
        {
            switch (type)
            {
            case HullModifierType::BROADER_SHOULDERS: return 8u;
            case HullModifierType::SIDE_LOBES: return 10u;
            case HullModifierType::STEPPED_WING_EXTENSION: return 10u;
            case HullModifierType::NARROW_WAIST: return 6u;
            case HullModifierType::WING_CUTOUT: return 7u;
            case HullModifierType::SPLIT_NOSE: return 8u;
            default: return 0u;
            }
        }

        uint32_t applyLongitudinalOffset(uint32_t index, uint32_t lastIndex, int32_t offsetPercent)
        {
            if (offsetPercent == 0 || lastIndex == 0u)
            {
                return index;
            }

            const int64_t offset = (static_cast<int64_t>(lastIndex) * offsetPercent) / 100;
            const int64_t shifted = static_cast<int64_t>(index) + offset;
            return static_cast<uint32_t>(std::clamp<int64_t>(shifted, 0, static_cast<int64_t>(lastIndex)));
        }

        uint32_t getStructuralNegativeSpaceComplexityCost(ShipStructuralNegativeSpaceType type)
        {
            switch (type)
            {
            case ShipStructuralNegativeSpaceType::WING_CHANNEL: return 8u;
            case ShipStructuralNegativeSpaceType::REAR_FORK: return 10u;
            case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return 7u;
            case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return 9u;
            case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return 11u;
            default: return 0u;
            }
        }

        uint32_t getNegativeSpaceScaleChancePercent(const GenerationScaleTraits& traits)
        {
            if (traits.MinimumDimension <= 24u) { return 20u; }
            if (traits.MinimumDimension <= 32u) { return 55u; }
            if (traits.MinimumDimension <= 44u) { return 80u; }
            if (traits.MinimumDimension >= 96u) { return 110u; }
            return 100u;
        }

        uint32_t getFactionNegativeSpaceChancePercent(ShipFactionType faction)
        {
            switch (faction)
            {
            case ShipFactionType::FRONTIER: return 110u;
            case ShipFactionType::MILITARY: return 95u;
            case ShipFactionType::ASCENDANT: return 88u;
            case ShipFactionType::XENO: return 105u;
            case ShipFactionType::CORPORATE: return 92u;
            case ShipFactionType::RELIC: return 115u;
            default: return 100u;
            }
        }

        uint32_t getFactionNegativeSpaceWeightPercent(ShipFactionType faction, ShipStructuralNegativeSpaceType type)
        {
            switch (faction)
            {
            case ShipFactionType::FRONTIER:
                switch (type)
                {
                case ShipStructuralNegativeSpaceType::REAR_FORK: return 115u;
                case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return 135u;
                case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return 135u;
                default: return 95u;
                }
            case ShipFactionType::MILITARY:
                switch (type)
                {
                case ShipStructuralNegativeSpaceType::WING_CHANNEL: return 120u;
                case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return 115u;
                case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return 75u;
                default: return 100u;
                }
            case ShipFactionType::ASCENDANT:
                switch (type)
                {
                case ShipStructuralNegativeSpaceType::WING_CHANNEL: return 135u;
                case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return 125u;
                case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return 65u;
                case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return 80u;
                default: return 85u;
                }
            case ShipFactionType::XENO:
                switch (type)
                {
                case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return 125u;
                case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return 125u;
                case ShipStructuralNegativeSpaceType::REAR_FORK: return 110u;
                default: return 90u;
                }
            case ShipFactionType::CORPORATE:
                switch (type)
                {
                case ShipStructuralNegativeSpaceType::WING_CHANNEL: return 125u;
                case ShipStructuralNegativeSpaceType::SHOULDER_GAP: return 140u;
                case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return 115u;
                case ShipStructuralNegativeSpaceType::REAR_FORK: return 75u;
                default: return 95u;
                }
            case ShipFactionType::RELIC:
                switch (type)
                {
                case ShipStructuralNegativeSpaceType::REAR_FORK: return 135u;
                case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: return 155u;
                case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: return 115u;
                case ShipStructuralNegativeSpaceType::WING_CHANNEL: return 78u;
                default: return 95u;
                }
            default: return 100u;
            }
        }

        void reserveRemovedPixel(PixelMask& hullMask, PixelMask& reservedMask, uint32_t x, uint32_t y, bool& changed)
        {
            if (hullMask.get(x, y))
            {
                hullMask.set(x, y, false);
                reservedMask.set(x, y, true);
                changed = true;
            }
        }
    }
    void HullGenerator::generate(ShipGenerationContext& context) const
    {
        context.WingRegions.reset(context.Settings.Dimensions.Width, context.Settings.Dimensions.Height);
        generateBaseHull(context);
        captureDebugStage(context, ShipGenerationDebugStageType::BASE_HULL);
        cleanup(context.Ship.HullMask);
        captureDebugStage(context, ShipGenerationDebugStageType::CLEANED_BASE_HULL);
        applySilhouetteModifiers(context);
        const SilhouetteQualityMetrics preVoidMetrics = calculateSilhouetteQualityMetrics(context.Ship.HullMask);
        if (validate(context.Ship.HullMask) && evaluateSilhouetteProfileAcceptance(context, preVoidMetrics) == SilhouetteValidationFailureReason::NONE)
        {
            applyStructuralNegativeSpace(context);
        }
        else
        {
            context.StructuralNegativeSpace.reset(context.Settings.Dimensions.Width, context.Settings.Dimensions.Height);
            if (context.DebugInfo != nullptr)
            {
                context.DebugInfo->StructuralNegativeSpaceCount = 0u;
                context.DebugInfo->StructuralNegativeSpacePixelCount = 0u;
                context.DebugInfo->StructuralNegativeSpaceTypeCounts.fill(0u);
                context.DebugInfo->ReservedNegativeSpaceMask = PixelMask(context.Settings.Dimensions.Width, context.Settings.Dimensions.Height, false);
            }
        }
        deriveWingRegions(context);
        captureWingDebugInfo(context);
        captureDebugStage(context, ShipGenerationDebugStageType::FINAL_HULL);
    }

    void HullGenerator::generateBaseHull(ShipGenerationContext& context) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationSettings& settings = context.Settings;
        const ShipGenerationProfile& profile = context.Profile;
        const uint32_t imageWidth = settings.Dimensions.Width;
        const uint32_t imageHeight = settings.Dimensions.Height;
        const uint32_t minimumVerticalPadding = std::max(GenerationMath::scalePixelsFrom64(2u, imageHeight), GenerationMath::getPercentage(imageHeight, profile.HullVerticalPaddingPercent.Min));
        const uint32_t maximumVerticalPadding = std::max(minimumVerticalPadding, GenerationMath::getPercentage(imageHeight, profile.HullVerticalPaddingPercent.Max));
        const uint32_t topPadding = context.getGenerationRandomUInt(GenerationDomain::HULL, minimumVerticalPadding, maximumVerticalPadding);
        const uint32_t propulsionRearMargin = context.ScaleTraits.LongitudinalCapacity >= 20u && context.ScaleTraits.LongitudinalCapacity < 40u ? 1u : 0u;
        const uint32_t minimumRearPadding = std::min(imageHeight - 2u, minimumVerticalPadding + propulsionRearMargin);
        const uint32_t maximumRearPadding = std::max(minimumRearPadding, maximumVerticalPadding);
        const uint32_t rearPadding = context.getGenerationRandomUInt(GenerationDomain::HULL, minimumRearPadding, maximumRearPadding);
        const uint32_t topY = topPadding;
        const uint32_t bottomY = imageHeight - rearPadding - 1u;
        const uint32_t hullHeight = bottomY - topY + 1u;
        const uint32_t lastProfileIndex = hullHeight - 1u;
        const uint32_t minimumHorizontalPadding = std::max(GenerationMath::scalePixelsFrom64(2u, imageWidth), GenerationMath::getPercentage(imageWidth, profile.HullHorizontalPaddingPercent.Min));
        const uint32_t maximumHorizontalPadding = std::max(minimumHorizontalPadding, GenerationMath::getPercentage(imageWidth, profile.HullHorizontalPaddingPercent.Max));
        const uint32_t horizontalPadding = context.getGenerationRandomUInt(GenerationDomain::HULL, minimumHorizontalPadding, maximumHorizontalPadding);
        const uint32_t leftCenter = (imageWidth - 1u) / 2u;
        const uint32_t rightCenter = imageWidth / 2u;
        const uint32_t leftCapacity = leftCenter - horizontalPadding + 1u;
        const uint32_t rightCapacity = imageWidth - horizontalPadding - rightCenter;
        const uint32_t maximumHalfWidth = std::min(leftCapacity, rightCapacity);
        const uint32_t noseEnd = GenerationMath::getPercentage(lastProfileIndex, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.NoseEndPercent));
        const uint32_t upperFuselageEnd = GenerationMath::getPercentage(lastProfileIndex, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.UpperFuselageEndPercent));
        const uint32_t mainBodyEnd = GenerationMath::getPercentage(lastProfileIndex, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.MainBodyEndPercent));
        const uint32_t lowerBodyEnd = GenerationMath::getPercentage(lastProfileIndex, context.getGenerationRandomUInt(GenerationDomain::HULL, 72u, 80u));
        const uint32_t rearFuselageStart = GenerationMath::getPercentage(lastProfileIndex, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.RearFuselageStartPercent));
        const uint32_t noseWidth = GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.NoseWidthPercent));
        const uint32_t upperFuselageWidth = std::max(noseWidth, GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.UpperFuselageWidthPercent)));
        const uint32_t mainBodyWidth = std::max(upperFuselageWidth, GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.MainBodyWidthPercent)));
        const uint32_t rearFuselageWidth = GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.RearFuselageWidthPercent));
        const uint32_t rearWidth = GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::HULL, profile.RearWidthPercent));
        const uint32_t lowerBodyWidth = std::min(maximumHalfWidth, std::max(rearFuselageWidth, GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::HULL, 38u, 62u))));
        const WingShapeType wingShape = getWingStyle(context, profile);
        std::vector<std::pair<uint32_t, uint32_t>> fuselageAnchors;
        fuselageAnchors.emplace_back(0u, 1u);
        fuselageAnchors.emplace_back(noseEnd, noseWidth);
        fuselageAnchors.emplace_back(upperFuselageEnd, upperFuselageWidth);
        fuselageAnchors.emplace_back(mainBodyEnd, mainBodyWidth);
        fuselageAnchors.emplace_back(lowerBodyEnd, lowerBodyWidth);
        fuselageAnchors.emplace_back(rearFuselageStart, rearFuselageWidth);
        fuselageAnchors.emplace_back(lastProfileIndex, rearWidth);
        std::vector<uint32_t> fuselageProfile(hullHeight, 1u);

        for (uint32_t index = 0u; index + 1u < fuselageAnchors.size(); ++index)
        {
            fillWidthTransition(fuselageProfile, fuselageAnchors[index].first, fuselageAnchors[index + 1u].first, fuselageAnchors[index].second, fuselageAnchors[index + 1u].second);
        }

        std::vector<uint32_t> widthProfile = fuselageProfile;
        const WingProfileDefinition wingDefinition = applyWingProfile(context, wingShape, profile, fuselageProfile, widthProfile, maximumHalfWidth);

        for (uint32_t row = 0u; row < hullHeight; ++row)
        {
            context.WingRegions.FuselageHalfWidths[topY + row] = fuselageProfile[row];
        }

        context.WingRegions.Shape = wingDefinition.Valid ? wingDefinition.Shape : WingShapeType::NONE;

        if (wingDefinition.Valid)
        {
            context.WingRegions.StartY = topY + wingDefinition.StartIndex;
            context.WingRegions.PeakY = topY + wingDefinition.PeakIndex;
            context.WingRegions.EndY = topY + wingDefinition.EndIndex;
        }

        rasterizeSymmetricHull(ship.HullMask, widthProfile, topY);
    }

    HullGenerator::WingProfileDefinition HullGenerator::applyWingProfile(ShipGenerationContext& context, WingShapeType wingShape, const ShipGenerationProfile& profile, const std::vector<uint32_t>& fuselageProfile, std::vector<uint32_t>& widthProfile, uint32_t maximumHalfWidth) const
    {
        WingProfileDefinition definition;
        definition.Shape = wingShape;

        if (wingShape == WingShapeType::NONE || widthProfile.size() < 3u || maximumHalfWidth == 0u)
        {
            return definition;
        }

        const uint32_t lastIndex = static_cast<uint32_t>(widthProfile.size() - 1u);
        const uint32_t rootRows = std::max(1u, GenerationMath::scaleByPercent(GenerationMath::scaleVerticalPixelsFrom64(2u, context.Settings.Dimensions), profile.WingRootLengthPercent));
        uint32_t desiredWingWidth = 0u;
        uint32_t rootExtensionDivisor = 2u;
        uint32_t taperPercent = 55u;
        uint32_t steppingRows = 1u;

        if (wingShape == WingShapeType::SMALL)
        {
            definition.StartIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 56u, 62u));
            definition.PeakIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 66u, 72u));
            definition.TaperIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 74u, 79u));
            definition.EndIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 81u, 85u));
            const uint32_t minimumIncrease = std::max(GenerationMath::scaleHorizontalPixelsFrom64(2u, context.Settings.Dimensions), GenerationMath::getPercentage(maximumHalfWidth, profile.SmallWingIncreasePercent.Min));
            const uint32_t maximumIncrease = std::max(minimumIncrease, GenerationMath::getPercentage(maximumHalfWidth, profile.SmallWingIncreasePercent.Max));
            desiredWingWidth = std::min(maximumHalfWidth, fuselageProfile[definition.PeakIndex] + context.getGenerationRandomUInt(GenerationDomain::WINGS, minimumIncrease, maximumIncrease));
            taperPercent = profile.SmallWingTaperPercent;
        }
        else if (wingShape == WingShapeType::SWEPT)
        {
            definition.StartIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 53u, 59u));
            definition.PeakIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 66u, 73u));
            definition.TaperIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 75u, 81u));
            definition.EndIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 84u, 88u));
            const uint32_t minimumWingWidth = std::min(maximumHalfWidth, fuselageProfile[definition.PeakIndex] + std::max(GenerationMath::scaleHorizontalPixelsFrom64(3u, context.Settings.Dimensions), maximumHalfWidth / 7u));
            desiredWingWidth = std::max(minimumWingWidth, GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::WINGS, profile.SweptWingWidthPercent)));
            rootExtensionDivisor = 3u;
            taperPercent = profile.SweptWingTaperPercent;
        }
        else
        {
            definition.StartIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 54u, 60u));
            definition.PeakIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 63u, 69u));
            definition.TaperIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 74u, 80u));
            definition.EndIndex = GenerationMath::getPercentage(lastIndex, context.getGenerationRandomUInt(GenerationDomain::WINGS, 84u, 88u));
            const uint32_t minimumWingWidth = std::min(maximumHalfWidth, fuselageProfile[definition.PeakIndex] + std::max(GenerationMath::scaleHorizontalPixelsFrom64(4u, context.Settings.Dimensions), maximumHalfWidth / 6u));
            desiredWingWidth = std::max(minimumWingWidth, GenerationMath::getHalfWidthFromPercentage(maximumHalfWidth, context.getGenerationRandomUInt(GenerationDomain::WINGS, profile.BroadWingWidthPercent)));
            rootExtensionDivisor = 2u;
            taperPercent = profile.BroadWingTaperPercent;
            steppingRows = std::max(1u, GenerationMath::scaleVerticalPixelsFrom64(2u, context.Settings.Dimensions));
        }

        definition.StartIndex = applyLongitudinalOffset(definition.StartIndex, lastIndex, profile.WingLongitudinalOffsetPercent);
        definition.PeakIndex = applyLongitudinalOffset(definition.PeakIndex, lastIndex, profile.WingLongitudinalOffsetPercent);
        definition.TaperIndex = applyLongitudinalOffset(definition.TaperIndex, lastIndex, profile.WingLongitudinalOffsetPercent);
        definition.EndIndex = applyLongitudinalOffset(definition.EndIndex, lastIndex, profile.WingLongitudinalOffsetPercent);

        definition.RootEndIndex = std::min(lastIndex, definition.StartIndex + rootRows);
        definition.PeakIndex = std::max(definition.RootEndIndex, definition.PeakIndex);
        definition.TaperIndex = std::max(definition.PeakIndex, definition.TaperIndex);
        definition.EndIndex = std::max(definition.TaperIndex, definition.EndIndex);
        definition.EndIndex = std::min(lastIndex, definition.EndIndex);

        if (definition.EndIndex <= definition.StartIndex || desiredWingWidth <= fuselageProfile[definition.PeakIndex])
        {
            return definition;
        }

        const uint32_t peakExtension = desiredWingWidth - fuselageProfile[definition.PeakIndex];
        const uint32_t baseRootExtension = std::max(1u, peakExtension / rootExtensionDivisor);
        const uint32_t rootExtension = std::min(peakExtension, std::max(1u, GenerationMath::scaleByPercent(baseRootExtension, profile.WingRootWidthPercent)));
        const uint32_t rootWidth = std::min(maximumHalfWidth, fuselageProfile[definition.RootEndIndex] + rootExtension);
        const uint32_t taperReduction = std::max(1u, (peakExtension * taperPercent) / 100u);
        const uint32_t taperWidth = std::max(fuselageProfile[definition.TaperIndex], desiredWingWidth > taperReduction ? desiredWingWidth - taperReduction : fuselageProfile[definition.TaperIndex]);
        fillWidthTransition(widthProfile, definition.StartIndex, definition.RootEndIndex, fuselageProfile[definition.StartIndex], rootWidth);
        fillWidthTransition(widthProfile, definition.RootEndIndex, definition.PeakIndex, rootWidth, desiredWingWidth);
        fillWidthTransition(widthProfile, definition.PeakIndex, definition.TaperIndex, desiredWingWidth, taperWidth);
        fillWidthTransition(widthProfile, definition.TaperIndex, definition.EndIndex, taperWidth, fuselageProfile[definition.EndIndex]);

        for (uint32_t index = definition.StartIndex; index <= definition.EndIndex; ++index)
        {
            widthProfile[index] = std::max(widthProfile[index], fuselageProfile[index]);
        }

        if (steppingRows > 1u)
        {
            applyWingProfileStepping(widthProfile, definition.StartIndex, definition.EndIndex, steppingRows);
        }

        definition.Valid = true;
        return definition;
    }

    void HullGenerator::applyWingProfileStepping(std::vector<uint32_t>& widthProfile, uint32_t startIndex, uint32_t endIndex, uint32_t rowStep) const
    {
        if (rowStep <= 1u || startIndex >= widthProfile.size() || endIndex >= widthProfile.size() || startIndex > endIndex)
        {
            return;
        }

        for (uint32_t groupStart = startIndex; groupStart <= endIndex; groupStart += rowStep)
        {
            const uint32_t groupEnd = std::min(endIndex, groupStart + rowStep - 1u);
            uint32_t groupWidth = 0u;

            for (uint32_t index = groupStart; index <= groupEnd; ++index)
            {
                groupWidth = std::max(groupWidth, widthProfile[index]);
            }

            for (uint32_t index = groupStart; index <= groupEnd; ++index)
            {
                widthProfile[index] = groupWidth;
            }
        }
    }

    void HullGenerator::deriveWingRegions(ShipGenerationContext& context) const
    {
        WingRegionData& regions = context.WingRegions;
        regions.WingMask.clear(false);
        regions.WingRootMask.clear(false);
        regions.OuterWingMask.clear(false);
        regions.MaximumSpan = 0u;
        regions.MaximumExtension = 0u;
        regions.RootThickness = 0u;

        if (regions.Shape == WingShapeType::NONE || regions.StartY >= context.Ship.HullMask.getHeight() || regions.EndY >= context.Ship.HullMask.getHeight() || regions.StartY > regions.EndY)
        {
            return;
        }

        const PixelMask& hullMask = context.Ship.HullMask;
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        uint32_t firstWingY = hullMask.getHeight();
        uint32_t lastWingY = 0u;

        for (uint32_t y = regions.StartY; y <= regions.EndY; ++y)
        {
            const uint32_t fuselageHalfWidth = y < regions.FuselageHalfWidths.size() ? regions.FuselageHalfWidths[y] : 0u;

            if (fuselageHalfWidth == 0u)
            {
                continue;
            }

            const uint32_t fuselageLeft = leftCenter - (fuselageHalfWidth - 1u);
            const uint32_t fuselageRight = rightCenter + (fuselageHalfWidth - 1u);
            uint32_t rowExtension = 0u;

            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (!hullMask.get(x, y))
                {
                    continue;
                }

                if (x < fuselageLeft)
                {
                    rowExtension = std::max(rowExtension, fuselageLeft - x);
                }
                else if (x > fuselageRight)
                {
                    rowExtension = std::max(rowExtension, x - fuselageRight);
                }
            }

            if (rowExtension == 0u)
            {
                continue;
            }

            firstWingY = std::min(firstWingY, y);
            lastWingY = std::max(lastWingY, y);
            regions.MaximumSpan = std::max(regions.MaximumSpan, PixelMaskUtils::getOccupiedRowWidth(hullMask, y));
            regions.MaximumExtension = std::max(regions.MaximumExtension, rowExtension);
            const uint32_t rootBand = std::min(rowExtension, std::max(1u, std::min(GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()), (rowExtension + 1u) / 2u)));
            regions.RootThickness = std::max(regions.RootThickness, rootBand);

            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (!hullMask.get(x, y) || (x >= fuselageLeft && x <= fuselageRight))
                {
                    continue;
                }

                const uint32_t outwardDistance = x < fuselageLeft ? fuselageLeft - x : x - fuselageRight;
                regions.WingMask.set(x, y, true);

                if (outwardDistance <= rootBand)
                {
                    regions.WingRootMask.set(x, y, true);
                }

                if (rowExtension >= 2u && outwardDistance * 100u >= rowExtension * 65u)
                {
                    regions.OuterWingMask.set(x, y, true);
                }
            }
        }

        if (firstWingY < hullMask.getHeight())
        {
            regions.StartY = firstWingY;
            regions.EndY = lastWingY;
        }
        else
        {
            regions.Shape = WingShapeType::NONE;
        }
    }

    void HullGenerator::captureWingDebugInfo(ShipGenerationContext& context) const
    {
        if (context.DebugInfo == nullptr)
        {
            return;
        }

        const WingRegionData& regions = context.WingRegions;
        context.DebugInfo->WingShape = regions.Shape;
        context.DebugInfo->WingStartY = regions.StartY;
        context.DebugInfo->WingEndY = regions.EndY;
        context.DebugInfo->WingMaximumSpan = regions.MaximumSpan;
        context.DebugInfo->WingMaximumExtension = regions.MaximumExtension;
        context.DebugInfo->WingRootThickness = regions.RootThickness;
        context.DebugInfo->WingPixelCount = PixelMaskUtils::getMaskPixelCount(regions.WingMask);
        context.DebugInfo->WingRootPixelCount = PixelMaskUtils::getMaskPixelCount(regions.WingRootMask);
        context.DebugInfo->OuterWingPixelCount = PixelMaskUtils::getMaskPixelCount(regions.OuterWingMask);
    }

    bool HullGenerator::validateWingRegions(const ShipGenerationContext& context) const
    {
        const WingRegionData& regions = context.WingRegions;

        if (regions.Shape == WingShapeType::NONE)
        {
            return true;
        }

        const PixelMask& hullMask = context.Ship.HullMask;
        const uint32_t wingPixelCount = PixelMaskUtils::getMaskPixelCount(regions.WingMask);

        if (wingPixelCount == 0u || PixelMaskUtils::getMaskPixelCount(regions.WingRootMask) == 0u)
        {
            return false;
        }

        uint32_t wingRowCount = 0u;
        uint32_t rootContactRowCount = 0u;

        for (uint32_t y = regions.StartY; y <= regions.EndY && y < hullMask.getHeight(); ++y)
        {
            bool hasWingPixel = false;
            bool hasRootContact = false;

            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (!regions.WingMask.get(x, y))
                {
                    continue;
                }

                hasWingPixel = true;

                if (!hullMask.get(x, y))
                {
                    return false;
                }

                const uint32_t mirrorX = hullMask.getWidth() - 1u - x;

                if (!regions.WingMask.get(mirrorX, y))
                {
                    return false;
                }

                if (regions.WingRootMask.get(x, y))
                {
                    const int32_t inwardX = x < hullMask.getWidth() / 2u ? static_cast<int32_t>(x) + 1 : static_cast<int32_t>(x) - 1;
                    hasRootContact = hasRootContact || PixelMaskUtils::isMaskPixel(hullMask, inwardX, static_cast<int32_t>(y));
                }
            }

            if (hasWingPixel)
            {
                ++wingRowCount;
            }

            if (hasRootContact)
            {
                ++rootContactRowCount;
            }
        }

        const uint32_t largeWingThreshold = GenerationMath::scalePixelsFrom64(4u, hullMask.getWidth());
        const uint32_t minimumConnectedRows = std::max(1u, GenerationMath::scalePixelsFrom64(3u, hullMask.getHeight()));

        if (regions.MaximumExtension >= largeWingThreshold && (wingRowCount < minimumConnectedRows || rootContactRowCount < minimumConnectedRows))
        {
            return false;
        }

        return true;
    }

    void HullGenerator::cleanup(PixelMask& hullMask) const
    {
        if (hullMask.getHeight() < 3u)
        {
            return;
        }

        std::vector<uint32_t> rowWidths(hullMask.getHeight(), 0u);

        for (uint32_t y = 0u; y < hullMask.getHeight(); ++y)
        {
            rowWidths[y] = PixelMaskUtils::getOccupiedRowWidth(hullMask, y);
        }

        for (uint32_t y = 1u; y + 1u < hullMask.getHeight(); ++y)
        {
            const uint32_t previousWidth = rowWidths[y - 1u];
            const uint32_t currentWidth = rowWidths[y];
            const uint32_t nextWidth = rowWidths[y + 1u];

            if (previousWidth == 0u || currentWidth == 0u || nextWidth == 0u)
            {
                continue;
            }

            const uint32_t neighbourDifference = GenerationMath::getDifference(previousWidth, nextWidth);
            const uint32_t previousDifference = GenerationMath::getDifference(currentWidth, previousWidth);
            const uint32_t nextDifference = GenerationMath::getDifference(currentWidth, nextWidth);
            const uint32_t neighbourTolerance = GenerationMath::scalePixelsFrom64(2u, hullMask.getWidth());
            const uint32_t isolatedDiscontinuityThreshold = GenerationMath::scalePixelsFrom64(4u, hullMask.getWidth());

            if (neighbourDifference <= neighbourTolerance && previousDifference >= isolatedDiscontinuityThreshold && nextDifference >= isolatedDiscontinuityThreshold)
            {
                const uint32_t correctedWidth = (previousWidth + nextWidth) / 2u;
                PixelMaskUtils::setSymmetricRowWidth(hullMask, y, correctedWidth);
            }
        }
    }

    void HullGenerator::applySilhouetteModifiers(ShipGenerationContext& context) const
    {
        const ShipGenerationProfile& profile = context.Profile;
        constexpr std::size_t ModifierCount = static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END);
        std::array<bool, ModifierCount> selectedModifiers = {};

        if (profile.MaximumHullModifiers != 0u && profile.HullModifierChance != 0u)
        {
            const uint32_t modifierChancePercent = 50u + context.ScaleTraits.MajorFeatureCapacity / 2u;
            uint32_t scaledModifierChance = static_cast<uint32_t>((static_cast<uint64_t>(profile.HullModifierChance) * modifierChancePercent + 50u) / 100u);
            if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::SILHOUETTE))
            {
                scaledModifierChance = std::min(100u, static_cast<uint32_t>((static_cast<uint64_t>(scaledModifierChance) * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::SILHOUETTE) + 50u) / 100u));
            }

            if (context.getGenerationRandomUInt(GenerationDomain::HULL, 0u, 99u) < scaledModifierChance)
            {
                const uint32_t scaleMaximumModifierCount = 1u + context.ScaleTraits.MajorFeatureCapacity / 35u;
                const uint32_t maximumModifierCount = std::min({ profile.MaximumHullModifiers, static_cast<uint32_t>(ModifierCount), scaleMaximumModifierCount });
                const uint32_t targetModifierCount = context.getGenerationRandomUInt(GenerationDomain::HULL, 1u, maximumModifierCount);

                for (uint32_t index = 0u; index < targetModifierCount; ++index)
                {
                    const HullModifierType type = getHullModifierType(context, profile, selectedModifiers);
                    if (type == HullModifierType::HULL_MODIFIER_TYPE_END) { break; }
                    selectedModifiers[static_cast<std::size_t>(type)] = true;
                }
            }
        }

        for (uint32_t index = 0u; index <= static_cast<uint32_t>(HullModifierType::STEPPED_WING_EXTENSION); ++index)
        {
            if (selectedModifiers[index])
            {
                tryApplyHullModifier(context, static_cast<HullModifierType>(index));
            }
        }

        applySilhouetteGuidance(context, selectedModifiers);
        captureDebugStage(context, ShipGenerationDebugStageType::AFTER_ADDITIVE_MODIFIERS);

        for (uint32_t index = static_cast<uint32_t>(HullModifierType::NARROW_WAIST); index < static_cast<uint32_t>(HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            if (selectedModifiers[index])
            {
                tryApplyHullModifier(context, static_cast<HullModifierType>(index));
            }
        }

        captureDebugStage(context, ShipGenerationDebugStageType::AFTER_SUBTRACTIVE_MODIFIERS);
    }

    void HullGenerator::applySilhouetteGuidance(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)>& selectedModifiers) const
    {
        // Profiles may deliberately opt out of additive silhouette guidance
        // when their authored grammar already relies on a clean macro-form.
        if (!context.Profile.SilhouetteGuidanceEnabled)
        {
            return;
        }

        const SilhouetteQualityMetrics metrics = calculateSilhouetteQualityMetrics(context.Ship.HullMask);
        const uint32_t targetArticulation = getScaleAdjustedArticulationTarget(context);
        const bool underusesLateralCanvas = metrics.NormalizedWidthPercent < context.Profile.MinimumSilhouetteWidthPercent;
        const bool selectedSubtractiveArticulation = selectedModifiers[static_cast<std::size_t>(HullModifierType::NARROW_WAIST)] || selectedModifiers[static_cast<std::size_t>(HullModifierType::WING_CUTOUT)] || selectedModifiers[static_cast<std::size_t>(HullModifierType::SPLIT_NOSE)];
        const bool genericFormRisk = metrics.BoundingFillPercent + 8u >= context.Profile.SilhouetteConvexFillTriggerPercent || metrics.LongestStableWidthRunPercent > context.Profile.SilhouetteMaximumStableRunPercent;
        const bool weakArticulation = context.Profile.SilhouetteWeakArticulationGuidanceEnabled && !selectedSubtractiveArticulation && metrics.ArticulationCount <= targetArticulation && metrics.InteriorContractionPercent < 8u && metrics.ShoulderProminencePercent < 8u && genericFormRisk;

        if (!underusesLateralCanvas && !weakArticulation)
        {
            return;
        }

        std::array<HullModifierType, 3u> priorities = { HullModifierType::BROADER_SHOULDERS, HullModifierType::SIDE_LOBES, HullModifierType::STEPPED_WING_EXTENSION };
        const auto guidanceWeight = [&](HullModifierType type)
        {
            switch (type)
            {
            case HullModifierType::BROADER_SHOULDERS: return context.Profile.SilhouetteGuidanceWeights.BroaderShoulders;
            case HullModifierType::SIDE_LOBES: return context.Profile.SilhouetteGuidanceWeights.SideLobes;
            case HullModifierType::STEPPED_WING_EXTENSION: return context.Profile.SilhouetteGuidanceWeights.SteppedWingExtension;
            default: return 0u;
            }
        };
        std::stable_sort(priorities.begin(), priorities.end(), [&](HullModifierType a, HullModifierType b) { return guidanceWeight(a) > guidanceWeight(b); });

        // Faction only nudges the manner of articulation. The resolved profile remains
        // the primary structural language and no profile x faction matrix is used.
        if (context.Settings.Faction == ShipFactionType::FRONTIER || context.Settings.Faction == ShipFactionType::XENO || context.Settings.Faction == ShipFactionType::RELIC)
        {
            std::swap(priorities[1u], priorities[2u]);
        }

        for (HullModifierType type : priorities)
        {
            const std::size_t index = static_cast<std::size_t>(type);
            if (selectedModifiers[index] || getHullModifierWeight(context.Profile, type) == 0u || !context.ComplexityBudget.canAfford(GenerationComplexityCategory::SILHOUETTE, getHullModifierComplexityCost(type)))
            {
                continue;
            }

            if (tryApplyHullModifier(context, type))
            {
                if (context.DebugInfo != nullptr) { ++context.DebugInfo->SilhouetteGuidanceAppliedCount; }
                return;
            }
        }
    }

    void HullGenerator::applyStructuralNegativeSpace(ShipGenerationContext& context) const
    {
        context.StructuralNegativeSpace.reset(context.Settings.Dimensions.Width, context.Settings.Dimensions.Height);
        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->StructuralNegativeSpaceCount = 0u;
            context.DebugInfo->StructuralNegativeSpaceAttemptCount = 0u;
            context.DebugInfo->StructuralNegativeSpaceSuccessCount = 0u;
            context.DebugInfo->StructuralNegativeSpacePixelCount = 0u;
            context.DebugInfo->StructuralNegativeSpaceTypeCounts.fill(0u);
            context.DebugInfo->ReservedNegativeSpaceMask = PixelMask(context.Settings.Dimensions.Width, context.Settings.Dimensions.Height, false);
        }

        const ShipGenerationProfile& profile = context.Profile;
        if (profile.StructuralNegativeSpaceChance == 0u || profile.MaximumStructuralNegativeSpaceStructures == 0u)
        {
            return;
        }

        uint32_t chance = static_cast<uint32_t>((static_cast<uint64_t>(profile.StructuralNegativeSpaceChance) * profile.StructuralNegativeSpaceScalePercent + 50u) / 100u);
        chance = static_cast<uint32_t>((static_cast<uint64_t>(chance) * getNegativeSpaceScaleChancePercent(context.ScaleTraits) + 50u) / 100u);
        chance = static_cast<uint32_t>((static_cast<uint64_t>(chance) * getFactionNegativeSpaceChancePercent(context.Settings.Faction) + 50u) / 100u);
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::NEGATIVE_SPACE))
        {
            chance = static_cast<uint32_t>((static_cast<uint64_t>(chance) * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::NEGATIVE_SPACE) + 50u) / 100u);
            if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::NEGATIVE_SPACE)) { chance = std::max(chance, 72u); }
        }
        chance = std::min(100u, chance);
        if (context.getGenerationRandomUInt(GenerationDomain::HULL, 0u, 99u) >= chance)
        {
            return;
        }

        constexpr std::size_t TypeCount = static_cast<std::size_t>(ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END);
        std::array<bool, TypeCount> selectedTypes = {};
        const uint32_t scaleMaximum = context.ScaleTraits.MinimumDimension >= 64u ? 2u : 1u;
        const uint32_t maximumCount = std::min({ profile.MaximumStructuralNegativeSpaceStructures, static_cast<uint32_t>(TypeCount), scaleMaximum });
        const uint32_t targetCount = maximumCount <= 1u ? 1u : context.getGenerationRandomUInt(GenerationDomain::HULL, 1u, maximumCount);

        uint32_t successfulCount = 0u;
        for (uint32_t attempt = 0u; attempt < static_cast<uint32_t>(TypeCount) && successfulCount < targetCount; ++attempt)
        {
            const ShipStructuralNegativeSpaceType type = selectStructuralNegativeSpaceType(context, selectedTypes);
            if (type == ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END) { break; }
            selectedTypes[static_cast<std::size_t>(type)] = true;
            if (context.DebugInfo != nullptr) { ++context.DebugInfo->StructuralNegativeSpaceAttemptCount; }
            if (tryApplyStructuralNegativeSpace(context, type))
            {
                ++successfulCount;
                if (context.DebugInfo != nullptr) { ++context.DebugInfo->StructuralNegativeSpaceSuccessCount; }
            }
        }

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->StructuralNegativeSpaceCount = static_cast<uint32_t>(context.StructuralNegativeSpace.Placements.size());
            context.DebugInfo->StructuralNegativeSpacePixelCount = PixelMaskUtils::getMaskPixelCount(context.StructuralNegativeSpace.ReservedMask);
            context.DebugInfo->ReservedNegativeSpaceMask = context.StructuralNegativeSpace.ReservedMask;
            for (const StructuralNegativeSpacePlacement& placement : context.StructuralNegativeSpace.Placements)
            {
                ++context.DebugInfo->StructuralNegativeSpaceTypeCounts[static_cast<std::size_t>(placement.Type)];
            }
        }
    }

    ShipStructuralNegativeSpaceType HullGenerator::selectStructuralNegativeSpaceType(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END)>& selectedTypes) const
    {
        uint64_t totalWeight = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END); ++index)
        {
            if (selectedTypes[index]) { continue; }
            const ShipStructuralNegativeSpaceType type = static_cast<ShipStructuralNegativeSpaceType>(index);
            const uint32_t cost = getStructuralNegativeSpaceComplexityCost(type);
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::SILHOUETTE, cost)) { continue; }
            totalWeight += getStructuralNegativeSpaceWeight(context, type);
        }
        if (totalWeight == 0u) { return ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END; }

        uint64_t roll = context.getGenerationRandomUInt64(GenerationDomain::HULL, 0u, totalWeight - 1u);
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END); ++index)
        {
            if (selectedTypes[index]) { continue; }
            const ShipStructuralNegativeSpaceType type = static_cast<ShipStructuralNegativeSpaceType>(index);
            const uint32_t cost = getStructuralNegativeSpaceComplexityCost(type);
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::SILHOUETTE, cost)) { continue; }
            const uint64_t weight = getStructuralNegativeSpaceWeight(context, type);
            if (roll < weight) { return type; }
            roll -= weight;
        }
        return ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END;
    }

    uint32_t HullGenerator::getStructuralNegativeSpaceWeight(const ShipGenerationContext& context, ShipStructuralNegativeSpaceType type) const
    {
        if (context.ScaleTraits.MinimumDimension <= 24u && type != ShipStructuralNegativeSpaceType::WING_CHANNEL && type != ShipStructuralNegativeSpaceType::REAR_FORK)
        {
            return 0u;
        }
        if (context.ScaleTraits.MinimumDimension < 44u && (type == ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY || type == ShipStructuralNegativeSpaceType::NACELLE_CHANNEL))
        {
            return 0u;
        }
        if (type == ShipStructuralNegativeSpaceType::WING_CHANNEL && context.WingRegions.Shape == WingShapeType::NONE)
        {
            return 0u;
        }

        uint32_t weight = context.Profile.StructuralNegativeSpaceWeights.getWeight(type);
        weight = static_cast<uint32_t>((static_cast<uint64_t>(weight) * getFactionNegativeSpaceWeightPercent(context.Settings.Faction, type) + 50u) / 100u);
        return weight;
    }

    bool HullGenerator::tryApplyStructuralNegativeSpace(ShipGenerationContext& context, ShipStructuralNegativeSpaceType type) const
    {
        PixelMask candidate = context.Ship.HullMask;
        PixelMask removed(candidate.getWidth(), candidate.getHeight(), false);
        bool changed = false;
        switch (type)
        {
        case ShipStructuralNegativeSpaceType::WING_CHANNEL: changed = applyWingChannel(context, candidate, removed); break;
        case ShipStructuralNegativeSpaceType::REAR_FORK: changed = applyRearFork(context, candidate, removed); break;
        case ShipStructuralNegativeSpaceType::SHOULDER_GAP: changed = applyShoulderGap(context, candidate, removed); break;
        case ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY: changed = applyOpenFrameBay(context, candidate, removed); break;
        case ShipStructuralNegativeSpaceType::NACELLE_CHANNEL: changed = applyNacelleChannel(context, candidate, removed); break;
        default: return false;
        }

        if (!changed || PixelMaskUtils::getMaskPixelCount(removed) == 0u || !isHullConnected(candidate))
        {
            return false;
        }

        const SilhouetteQualityMetrics candidateMetrics = calculateSilhouetteQualityMetrics(candidate);
        const uint32_t maximumAllowedRowWidthDelta = std::max(GenerationMath::scalePixelsFrom64(9u, context.Settings.Dimensions.Width), context.Settings.Dimensions.Width / 3u);
        if (candidateMetrics.CanvasFillPercent < 5u || candidateMetrics.CanvasFillPercent > 55u ||
            candidateMetrics.NormalizedHeightPercent < 50u || candidateMetrics.NormalizedWidthPercent < 20u || candidateMetrics.WidthVariationPercent < 15u ||
            candidateMetrics.MaximumRowWidthDelta > maximumAllowedRowWidthDelta || candidateMetrics.NearMaximumRowPercent >= 92u)
        {
            return false;
        }
        if (evaluateSilhouetteProfileAcceptance(context, candidateMetrics) != SilhouetteValidationFailureReason::NONE)
        {
            return false;
        }

        const uint32_t cost = getStructuralNegativeSpaceComplexityCost(type);
        if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::SILHOUETTE, cost)) { return false; }

        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(removed);
        StructuralNegativeSpacePlacement placement;
        placement.Type = type;
        placement.MinX = bounds.MinX;
        placement.MaxX = bounds.MaxX;
        placement.MinY = bounds.MinY;
        placement.MaxY = bounds.MaxY;
        placement.PixelCount = PixelMaskUtils::getMaskPixelCount(removed);
        context.Ship.HullMask = std::move(candidate);
        PixelMaskUtils::mergeMask(context.StructuralNegativeSpace.ReservedMask, removed);
        context.StructuralNegativeSpace.Placements.push_back(placement);
        return true;
    }

    bool HullGenerator::applyWingChannel(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const
    {
        if (context.WingRegions.Shape == WingShapeType::NONE || context.WingRegions.EndY <= context.WingRegions.StartY + 3u) { return false; }
        const uint32_t span = context.WingRegions.EndY - context.WingRegions.StartY + 1u;
        const uint32_t verticalBorder = std::max({ 1u, GenerationMath::scalePixelsFrom64(2u, hullMask.getHeight()), span / 5u });
        if (span <= verticalBorder * 2u + 1u) { return false; }
        const uint32_t startY = context.WingRegions.StartY + verticalBorder;
        const uint32_t endY = context.WingRegions.EndY - verticalBorder;
        const uint32_t gapWidth = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 96u ? 2u : 1u, hullMask.getWidth()));
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        const uint32_t braceY = startY + (endY - startY) / 2u;
        bool changed = false;
        uint32_t usefulRows = 0u;

        for (uint32_t y = startY; y <= endY; ++y)
        {
            if (endY > startY + 2u && y == braceY) { continue; }
            const uint32_t halfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;
            if (halfWidth == 0u) { continue; }
            const RowBounds row = calculateRowBounds(hullMask, y);
            if (!row.Valid) { continue; }
            const uint32_t fuselageLeft = leftCenter - (halfWidth - 1u);
            const uint32_t fuselageRight = rightCenter + (halfWidth - 1u);
            if (fuselageLeft <= row.MinX || row.MaxX <= fuselageRight) { continue; }
            const uint32_t leftExtension = fuselageLeft - row.MinX;
            const uint32_t rightExtension = row.MaxX - fuselageRight;
            const uint32_t extension = std::min(leftExtension, rightExtension);
            if (extension < gapWidth + 3u) { continue; }

            const uint32_t inwardClearance = std::max(1u, extension / 3u);
            const uint32_t leftGapEnd = fuselageLeft - inwardClearance;
            if (leftGapEnd < row.MinX + gapWidth) { continue; }
            const uint32_t leftGapStart = leftGapEnd - gapWidth + 1u;
            const uint32_t rightGapStart = hullMask.getWidth() - 1u - leftGapEnd;
            const uint32_t rightGapEnd = hullMask.getWidth() - 1u - leftGapStart;
            if (leftGapStart <= row.MinX || leftGapEnd >= fuselageLeft || rightGapStart <= fuselageRight || rightGapEnd >= row.MaxX) { continue; }

            bool valid = hullMask.get(leftGapStart - 1u, y) && hullMask.get(leftGapEnd + 1u, y) && hullMask.get(rightGapStart - 1u, y) && hullMask.get(rightGapEnd + 1u, y);
            for (uint32_t x = leftGapStart; valid && x <= leftGapEnd; ++x)
            {
                valid = hullMask.get(x, y) && hullMask.get(hullMask.getWidth() - 1u - x, y);
            }
            if (!valid) { continue; }

            for (uint32_t x = leftGapStart; x <= leftGapEnd; ++x)
            {
                reserveRemovedPixel(hullMask, reservedMask, x, y, changed);
                reserveRemovedPixel(hullMask, reservedMask, hullMask.getWidth() - 1u - x, y, changed);
            }
            ++usefulRows;
        }

        return changed && usefulRows >= std::max(1u, GenerationMath::scalePixelsFrom64(2u, hullMask.getHeight()));
    }

    bool HullGenerator::applyRearFork(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);
        if (!bounds.Valid || bounds.MaxY <= bounds.MinY + 5u) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startPercent = context.getGenerationRandomUInt(GenerationDomain::HULL, context.Profile.RearForkStartPercent);
        const uint32_t startY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, startPercent);
        const uint32_t gapHalfWidth = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 64u ? 2u : 1u, hullMask.getWidth()));
        const uint32_t gapWidth = hullMask.getWidth() % 2u == 0u ? gapHalfWidth * 2u : gapHalfWidth * 2u - 1u;
        const uint32_t minimumProngWidth = std::max(2u, GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()));
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        const uint32_t gapStartX = leftCenter - (gapHalfWidth - 1u);
        const uint32_t gapEndX = rightCenter + (gapHalfWidth - 1u);

        for (uint32_t y = startY; y <= bounds.MaxY; ++y)
        {
            const RowBounds row = calculateRowBounds(hullMask, y);
            if (!row.Valid || row.MinX + minimumProngWidth > gapStartX || gapEndX + minimumProngWidth > row.MaxX) { return false; }
            for (uint32_t x = gapStartX; x <= gapEndX; ++x) { if (!hullMask.get(x, y)) { return false; } }
        }

        bool changed = false;
        for (uint32_t y = startY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = gapStartX; x <= gapEndX; ++x) { reserveRemovedPixel(hullMask, reservedMask, x, y, changed); }
        }
        return changed && gapWidth > 0u;
    }

    bool HullGenerator::applyShoulderGap(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);
        if (!bounds.Valid) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, context.getGenerationRandomUInt(GenerationDomain::HULL, 30u, 46u));
        const uint32_t halfHeight = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 64u ? 3u : 2u, hullMask.getHeight()));
        const uint32_t startY = centerY > halfHeight ? centerY - halfHeight : bounds.MinY;
        const uint32_t endY = std::min(bounds.MaxY, centerY + halfHeight);
        const uint32_t gapWidth = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 64u ? 2u : 1u, hullMask.getWidth()));
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        bool changed = false;
        uint32_t cutRows = 0u;

        for (uint32_t y = startY + 1u; y < endY; ++y)
        {
            const RowBounds row = calculateRowBounds(hullMask, y);
            if (!row.Valid) { continue; }
            const uint32_t leftHalfSpan = leftCenter >= row.MinX ? leftCenter - row.MinX + 1u : 0u;
            if (leftHalfSpan < gapWidth + 4u) { continue; }
            const uint32_t offset = std::max(2u, leftHalfSpan * 58u / 100u);
            if (offset <= gapWidth || leftCenter < offset) { continue; }
            const uint32_t leftGapEnd = leftCenter - offset;
            if (leftGapEnd < row.MinX + 1u) { continue; }
            const uint32_t leftGapStart = leftGapEnd >= gapWidth - 1u ? leftGapEnd - (gapWidth - 1u) : 0u;
            const uint32_t rightGapStart = hullMask.getWidth() - 1u - leftGapEnd;
            const uint32_t rightGapEnd = hullMask.getWidth() - 1u - leftGapStart;
            if (leftGapStart <= row.MinX || rightGapEnd >= row.MaxX) { continue; }
            bool valid = hullMask.get(leftGapStart - 1u, y) && hullMask.get(leftGapEnd + 1u, y) && hullMask.get(rightGapStart - 1u, y) && hullMask.get(rightGapEnd + 1u, y);
            for (uint32_t x = leftGapStart; valid && x <= leftGapEnd; ++x) { valid = hullMask.get(x, y) && hullMask.get(hullMask.getWidth() - 1u - x, y); }
            if (!valid) { continue; }
            for (uint32_t x = leftGapStart; x <= leftGapEnd; ++x)
            {
                reserveRemovedPixel(hullMask, reservedMask, x, y, changed);
                reserveRemovedPixel(hullMask, reservedMask, hullMask.getWidth() - 1u - x, y, changed);
            }
            ++cutRows;
        }
        return changed && cutRows >= 1u;
    }

    bool HullGenerator::applyOpenFrameBay(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);
        if (!bounds.Valid || context.ScaleTraits.MinimumDimension < 44u) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, context.getGenerationRandomUInt(GenerationDomain::HULL, 48u, 66u));
        const uint32_t frame = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 96u ? 2u : 1u, hullMask.getWidth()));
        const uint32_t innerHalfWidth = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 64u ? 3u : 2u, hullMask.getWidth()));
        const uint32_t innerHalfHeight = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 64u ? 4u : 2u, hullMask.getHeight()));
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        if (leftCenter < innerHalfWidth + frame || centerY < innerHalfHeight + frame) { return false; }
        const uint32_t innerMinX = leftCenter - (innerHalfWidth - 1u);
        const uint32_t innerMaxX = rightCenter + (innerHalfWidth - 1u);
        const uint32_t innerMinY = centerY - innerHalfHeight;
        const uint32_t innerMaxY = std::min(bounds.MaxY - frame, centerY + innerHalfHeight);
        if (innerMaxY <= innerMinY || innerMinY <= bounds.MinY + frame) { return false; }
        const uint32_t outerMinX = innerMinX - frame;
        const uint32_t outerMaxX = innerMaxX + frame;
        const uint32_t outerMinY = innerMinY - frame;
        const uint32_t outerMaxY = innerMaxY + frame;
        if (outerMaxX >= hullMask.getWidth() || outerMaxY > bounds.MaxY) { return false; }

        for (uint32_t y = outerMinY; y <= outerMaxY; ++y)
        {
            for (uint32_t x = outerMinX; x <= outerMaxX; ++x) { if (!hullMask.get(x, y)) { return false; } }
        }
        bool changed = false;
        for (uint32_t y = innerMinY; y <= innerMaxY; ++y)
        {
            for (uint32_t x = innerMinX; x <= innerMaxX; ++x) { reserveRemovedPixel(hullMask, reservedMask, x, y, changed); }
        }
        return changed;
    }

    bool HullGenerator::applyNacelleChannel(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);
        if (!bounds.Valid || context.ScaleTraits.MinimumDimension < 44u) { return false; }
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, context.getGenerationRandomUInt(GenerationDomain::HULL, 64u, 74u));
        const uint32_t gapWidth = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.MinimumDimension >= 96u ? 2u : 1u, hullMask.getWidth()));
        const uint32_t minimumBoomWidth = std::max(2u, GenerationMath::scalePixelsFrom64(2u, hullMask.getWidth()));
        const uint32_t minimumCenterWidth = std::max(3u, GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()));
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        const RowBounds reference = calculateRowBounds(hullMask, startY);
        if (!reference.Valid) { return false; }
        const uint32_t halfSpan = leftCenter >= reference.MinX ? leftCenter - reference.MinX + 1u : 0u;
        if (halfSpan < minimumBoomWidth + gapWidth + minimumCenterWidth / 2u) { return false; }
        const uint32_t gapOffset = std::max(minimumCenterWidth / 2u + gapWidth, halfSpan * 55u / 100u);
        if (leftCenter < gapOffset) { return false; }
        const uint32_t leftGapEnd = leftCenter - gapOffset;
        const uint32_t leftGapStart = leftGapEnd >= gapWidth - 1u ? leftGapEnd - (gapWidth - 1u) : 0u;
        const uint32_t rightGapStart = hullMask.getWidth() - 1u - leftGapEnd;
        const uint32_t rightGapEnd = hullMask.getWidth() - 1u - leftGapStart;

        for (uint32_t y = startY; y <= bounds.MaxY; ++y)
        {
            const RowBounds row = calculateRowBounds(hullMask, y);
            if (!row.Valid || leftGapStart <= row.MinX || rightGapEnd >= row.MaxX) { return false; }
            if (leftGapStart - row.MinX < minimumBoomWidth || row.MaxX - rightGapEnd < minimumBoomWidth || rightGapStart - leftGapEnd - 1u < minimumCenterWidth) { return false; }
            for (uint32_t x = leftGapStart; x <= leftGapEnd; ++x)
            {
                if (!hullMask.get(x, y) || !hullMask.get(hullMask.getWidth() - 1u - x, y)) { return false; }
            }
        }
        bool changed = false;
        for (uint32_t y = startY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = leftGapStart; x <= leftGapEnd; ++x)
            {
                reserveRemovedPixel(hullMask, reservedMask, x, y, changed);
                reserveRemovedPixel(hullMask, reservedMask, hullMask.getWidth() - 1u - x, y, changed);
            }
        }
        return changed;
    }

    void HullGenerator::captureDebugStage(const ShipGenerationContext& context, ShipGenerationDebugStageType type) const
    {
        if (context.DebugInfo == nullptr)
        {
            return;
        }

        context.DebugInfo->HullStages.emplace_back(type, context.Ship.HullMask);
    }

    HullModifierType HullGenerator::getHullModifierType(ShipGenerationContext& context, const ShipGenerationProfile& profile, const std::array<bool, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)>& selectedModifiers) const
    {
        uint64_t totalWeight = 0u;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            if (selectedModifiers[index])
            {
                continue;
            }

            const HullModifierType type = static_cast<HullModifierType>(index);
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::SILHOUETTE, getHullModifierComplexityCost(type)))
            {
                continue;
            }

            totalWeight += getHullModifierWeight(profile, type);
        }

        if (totalWeight == 0u)
        {
            return HullModifierType::HULL_MODIFIER_TYPE_END;
        }

        uint64_t roll = context.getGenerationRandomUInt(GenerationDomain::HULL, 0u, static_cast<uint32_t>(totalWeight - 1u));

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedHullModifier.has_value())
        {
            const HullModifierType forced = *context.CalibrationSettings->Overrides.ForcedHullModifier;
            const std::size_t forcedIndex = static_cast<std::size_t>(forced);
            if (forced != HullModifierType::HULL_MODIFIER_TYPE_END && forcedIndex < selectedModifiers.size() && !selectedModifiers[forcedIndex] && context.ComplexityBudget.canAfford(GenerationComplexityCategory::SILHOUETTE, getHullModifierComplexityCost(forced)))
            {
                return forced;
            }
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(HullModifierType::HULL_MODIFIER_TYPE_END); ++index)
        {
            if (selectedModifiers[index])
            {
                continue;
            }

            const HullModifierType type = static_cast<HullModifierType>(index);
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::SILHOUETTE, getHullModifierComplexityCost(type)))
            {
                continue;
            }

            const uint64_t weight = getHullModifierWeight(profile, type);

            if (roll < weight)
            {
                return type;
            }

            roll -= weight;
        }

        return HullModifierType::HULL_MODIFIER_TYPE_END;
    }

    uint32_t HullGenerator::getHullModifierWeight(const ShipGenerationProfile& profile, HullModifierType type) const
    {
        switch (type)
        {
        case HullModifierType::BROADER_SHOULDERS: return profile.BroaderShouldersModifierWeight;
        case HullModifierType::SIDE_LOBES: return profile.SideLobesModifierWeight;
        case HullModifierType::STEPPED_WING_EXTENSION: return profile.SteppedWingModifierWeight;
        case HullModifierType::NARROW_WAIST: return profile.NarrowWaistModifierWeight;
        case HullModifierType::WING_CUTOUT: return profile.WingCutoutModifierWeight;
        case HullModifierType::SPLIT_NOSE: return profile.SplitNoseModifierWeight;
        default: return 0u;
        }
    }

    bool HullGenerator::tryApplyHullModifier(ShipGenerationContext& context, HullModifierType type) const
    {
        if (context.DebugInfo != nullptr && type != HullModifierType::HULL_MODIFIER_TYPE_END)
        {
            ++context.DebugInfo->HullModifierAttemptCounts[static_cast<std::size_t>(type)];
        }

        PixelMask candidate = context.Ship.HullMask;
        bool changed = false;

        switch (type)
        {
        case HullModifierType::BROADER_SHOULDERS: changed = applyBroaderShoulders(context, candidate); break;
        case HullModifierType::SIDE_LOBES: changed = applySideLobes(context, candidate); break;
        case HullModifierType::STEPPED_WING_EXTENSION: changed = applySteppedWingExtension(context, candidate); break;
        case HullModifierType::NARROW_WAIST: changed = applyNarrowWaist(context, candidate); break;
        case HullModifierType::WING_CUTOUT: changed = applyWingCutout(context, candidate); break;
        case HullModifierType::SPLIT_NOSE: changed = applySplitNose(context, candidate); break;
        default: return false;
        }

        if (!changed || !validate(candidate))
        {
            if (context.DebugInfo != nullptr && type != HullModifierType::HULL_MODIFIER_TYPE_END)
            {
                ++context.DebugInfo->HullModifierRejectionCounts[static_cast<std::size_t>(type)];
            }

            return false;
        }

        const uint32_t complexityCost = getHullModifierComplexityCost(type);
        if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::SILHOUETTE, complexityCost))
        {
            return false;
        }

        context.Ship.HullMask = std::move(candidate);

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->AppliedHullModifiers.push_back(type);
        }

        return true;
    }

    bool HullGenerator::applyBroaderShoulders(ShipGenerationContext& context, PixelMask& hullMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerPercent = context.getGenerationRandomUInt(GenerationDomain::HULL, 28u, 40u);
        const uint32_t centerY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, centerPercent);
        const uint32_t halfSpan = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(3u, hullMask.getHeight()), GenerationMath::scalePixelsFrom64(6u, hullMask.getHeight()));
        const uint32_t maximumExtension = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(2u, hullMask.getWidth()), GenerationMath::scalePixelsFrom64(4u, hullMask.getWidth()));
        const uint32_t startY = centerY > halfSpan ? centerY - halfSpan : bounds.MinY;
        const uint32_t endY = std::min(bounds.MaxY, centerY + halfSpan);
        bool changed = false;

        for (uint32_t y = std::max(bounds.MinY, startY); y <= endY; ++y)
        {
            const uint32_t distance = y > centerY ? y - centerY : centerY - y;
            const uint32_t extension = halfSpan == 0u ? maximumExtension : 1u + ((maximumExtension - 1u) * (halfSpan - std::min(halfSpan, distance))) / halfSpan;
            changed = extendRowEdges(hullMask, y, extension) || changed;
        }

        return changed;
    }

    bool HullGenerator::applySideLobes(ShipGenerationContext& context, PixelMask& hullMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerPercent = context.getGenerationRandomUInt(GenerationDomain::HULL, 52u, 70u);
        const uint32_t centerY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, centerPercent);
        const uint32_t halfSpan = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(3u, hullMask.getHeight()), GenerationMath::scalePixelsFrom64(6u, hullMask.getHeight()));
        const uint32_t maximumExtension = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()), GenerationMath::scalePixelsFrom64(6u, hullMask.getWidth()));
        const uint32_t startY = centerY > halfSpan ? centerY - halfSpan : bounds.MinY;
        const uint32_t endY = std::min(bounds.MaxY, centerY + halfSpan);
        bool changed = false;

        for (uint32_t y = std::max(bounds.MinY, startY); y <= endY; ++y)
        {
            const uint32_t distance = y > centerY ? y - centerY : centerY - y;
            const uint32_t reduction = distance / 2u;
            const uint32_t extension = maximumExtension > reduction ? maximumExtension - reduction : 1u;
            changed = extendRowEdges(hullMask, y, extension) || changed;
        }

        return changed;
    }

    bool HullGenerator::applySteppedWingExtension(ShipGenerationContext& context, PixelMask& hullMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startPercent = context.getGenerationRandomUInt(GenerationDomain::HULL, 56u, 68u);
        const uint32_t startY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, startPercent);
        const uint32_t minimumSpan = GenerationMath::scalePixelsFrom64(7u, hullMask.getHeight());
        const uint32_t maximumSpan = std::max(minimumSpan, GenerationMath::scalePixelsFrom64(12u, hullMask.getHeight()));
        const uint32_t span = context.getGenerationRandomUInt(GenerationDomain::HULL, minimumSpan, maximumSpan);
        const uint32_t endY = std::min(bounds.MaxY, startY + span - 1u);
        const uint32_t innerExtension = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(1u, hullMask.getWidth()), GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()));
        const uint32_t outerExtension = innerExtension + context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(2u, hullMask.getWidth()), GenerationMath::scalePixelsFrom64(4u, hullMask.getWidth()));
        const uint32_t actualSpan = endY - startY + 1u;
        bool changed = false;

        for (uint32_t y = startY; y <= endY; ++y)
        {
            const uint32_t localRow = y - startY;
            const bool middleStep = localRow * 3u >= actualSpan && localRow * 3u < actualSpan * 2u;
            const uint32_t extension = middleStep ? outerExtension : innerExtension;
            changed = extendRowEdges(hullMask, y, extension) || changed;
        }

        return changed;
    }

    bool HullGenerator::applyNarrowWaist(ShipGenerationContext& context, PixelMask& hullMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerPercent = context.getGenerationRandomUInt(GenerationDomain::HULL, 40u, 58u);
        const uint32_t centerY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, centerPercent);
        const uint32_t halfSpan = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(3u, hullMask.getHeight()), GenerationMath::scalePixelsFrom64(6u, hullMask.getHeight()));
        const uint32_t maximumReduction = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(1u, hullMask.getWidth()), GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()));
        const uint32_t minimumRemainingWidth = std::max(GenerationMath::scalePixelsFrom64(8u, hullMask.getWidth()), hullMask.getWidth() / 8u);
        const uint32_t startY = centerY > halfSpan ? centerY - halfSpan : bounds.MinY;
        const uint32_t endY = std::min(bounds.MaxY, centerY + halfSpan);
        bool changed = false;

        for (uint32_t y = std::max(bounds.MinY, startY); y <= endY; ++y)
        {
            const uint32_t distance = y > centerY ? y - centerY : centerY - y;
            const uint32_t reduction = halfSpan == 0u ? maximumReduction : 1u + ((maximumReduction - 1u) * (halfSpan - std::min(halfSpan, distance))) / halfSpan;
            changed = removeRowEdgePixels(hullMask, y, reduction, minimumRemainingWidth) || changed;
        }

        return changed;
    }

    bool HullGenerator::applyWingCutout(ShipGenerationContext& context, PixelMask& hullMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerPercent = context.getGenerationRandomUInt(GenerationDomain::HULL, 62u, 80u);
        const uint32_t centerY = bounds.MinY + GenerationMath::getPercentage(hullHeight - 1u, centerPercent);
        const uint32_t halfSpan = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(2u, hullMask.getHeight()), GenerationMath::scalePixelsFrom64(4u, hullMask.getHeight()));
        const uint32_t maximumDepth = context.getGenerationRandomUInt(GenerationDomain::HULL, GenerationMath::scalePixelsFrom64(1u, hullMask.getWidth()), GenerationMath::scalePixelsFrom64(3u, hullMask.getWidth()));
        const uint32_t minimumRemainingWidth = std::max(GenerationMath::scalePixelsFrom64(10u, hullMask.getWidth()), hullMask.getWidth() / 6u);
        const uint32_t startY = centerY > halfSpan ? centerY - halfSpan : bounds.MinY;
        const uint32_t endY = std::min(bounds.MaxY, centerY + halfSpan);
        bool changed = false;

        for (uint32_t y = std::max(bounds.MinY, startY); y <= endY; ++y)
        {
            const uint32_t distance = y > centerY ? y - centerY : centerY - y;
            const uint32_t depth = maximumDepth > distance ? maximumDepth - distance : 1u;
            changed = removeRowEdgePixels(hullMask, y, depth, minimumRemainingWidth) || changed;
        }

        return changed;
    }

    bool HullGenerator::applySplitNose(ShipGenerationContext& context, PixelMask& hullMask) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(hullMask);

        if (!bounds.Valid || bounds.MaxY <= bounds.MinY + 3u)
        {
            return false;
        }

        const uint32_t maximumAvailableDepth = std::max(2u, (bounds.MaxY - bounds.MinY + 1u) / 5u);
        const uint32_t minimumDepth = std::min(maximumAvailableDepth, GenerationMath::scalePixelsFrom64(3u, hullMask.getHeight()));
        const uint32_t desiredMaximumDepth = std::max(minimumDepth, GenerationMath::scalePixelsFrom64(7u, hullMask.getHeight()));
        const uint32_t maximumDepth = std::min(maximumAvailableDepth, desiredMaximumDepth);
        const uint32_t depth = context.getGenerationRandomUInt(GenerationDomain::HULL, minimumDepth, maximumDepth);
        const uint32_t maximumGapHalfWidth = std::max(1u, GenerationMath::scalePixelsFrom64(1u, hullMask.getWidth()));
        const uint32_t gapHalfWidth = context.getGenerationRandomUInt(GenerationDomain::HULL, 1u, maximumGapHalfWidth);
        const uint32_t gapWidth = hullMask.getWidth() % 2u == 0u ? gapHalfWidth * 2u : gapHalfWidth * 2u - 1u;
        const uint32_t minimumProngWidth = GenerationMath::scalePixelsFrom64(2u, hullMask.getWidth());
        const uint32_t requiredRowWidth = minimumProngWidth * 2u + gapWidth;
        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        const uint32_t gapStartX = leftCenter - (gapHalfWidth - 1u);
        const uint32_t gapEndX = rightCenter + (gapHalfWidth - 1u);
        const uint32_t bridgeY = bounds.MinY + depth;
        bool changed = false;

        for (uint32_t y = bounds.MinY; y < bridgeY; ++y)
        {
            const uint32_t currentWidth = PixelMaskUtils::getOccupiedRowWidth(hullMask, y);

            if (currentWidth < requiredRowWidth)
            {
                const uint32_t extension = (requiredRowWidth - currentWidth + 1u) / 2u;
                extendRowEdges(hullMask, y, extension);
            }

            for (uint32_t x = gapStartX; x <= gapEndX; ++x)
            {
                if (hullMask.get(x, y))
                {
                    hullMask.set(x, y, false);
                    changed = true;
                }
            }
        }

        if (bridgeY <= bounds.MaxY)
        {
            const uint32_t bridgeWidth = PixelMaskUtils::getOccupiedRowWidth(hullMask, bridgeY);

            if (bridgeWidth < requiredRowWidth)
            {
                const uint32_t extension = (requiredRowWidth - bridgeWidth + 1u) / 2u;
                changed = extendRowEdges(hullMask, bridgeY, extension) || changed;
            }
        }

        return changed;
    }

    bool HullGenerator::validate(const ShipGenerationContext& context) const
    {
        const SilhouetteQualityMetrics metrics = calculateSilhouetteQualityMetrics(context.Ship.HullMask);
        const SilhouetteValidationFailureReason reason = evaluateSilhouetteValidation(context, metrics);

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->SilhouetteMetrics = metrics;
            context.DebugInfo->LastSilhouetteValidationFailure = reason;
            if (reason != SilhouetteValidationFailureReason::NONE)
            {
                ++context.DebugInfo->SilhouetteValidationFailureCounts[static_cast<std::size_t>(reason)];
            }
        }

        return reason == SilhouetteValidationFailureReason::NONE;
    }

    bool HullGenerator::validate(const PixelMask& hullMask) const
    {
        const HullMetrics metrics = calculateHullMetrics(hullMask);

        if (metrics.PixelCount == 0u)
        {
            return false;
        }

        if (!isHullConnected(hullMask))
        {
            return false;
        }

        const uint64_t canvasArea = static_cast<uint64_t>(hullMask.getWidth()) * static_cast<uint64_t>(hullMask.getHeight());
        const uint64_t minimumHullArea = (canvasArea * 7u) / 100u;
        const uint64_t maximumHullArea = (canvasArea * 55u) / 100u;

        if (metrics.PixelCount < minimumHullArea || metrics.PixelCount > maximumHullArea)
        {
            return false;
        }

        const uint32_t minimumAcceptableHeight = hullMask.getHeight() / 2u;

        if (metrics.OccupiedHeight < minimumAcceptableHeight)
        {
            return false;
        }

        const uint32_t minimumAcceptableWidth = std::max(GenerationMath::scalePixelsFrom64(8u, hullMask.getWidth()), hullMask.getWidth() / 5u);

        if (metrics.MaximumOccupiedWidth < minimumAcceptableWidth)
        {
            return false;
        }

        if (metrics.MaximumOccupiedWidth * 4u < metrics.OccupiedHeight)
        {
            return false;
        }

        const uint32_t widthVariation = metrics.MaximumOccupiedWidth - metrics.MinimumOccupiedWidth;
        const uint32_t minimumWidthVariation = std::max(GenerationMath::scalePixelsFrom64(4u, hullMask.getWidth()), metrics.MaximumOccupiedWidth / 4u);

        if (widthVariation < minimumWidthVariation)
        {
            return false;
        }

        const uint32_t nearMaximumTolerance = GenerationMath::scalePixelsFrom64(2u, hullMask.getWidth());
        const uint32_t nearMaximumThreshold = metrics.MaximumOccupiedWidth > nearMaximumTolerance ? metrics.MaximumOccupiedWidth - nearMaximumTolerance : metrics.MaximumOccupiedWidth;
        uint32_t nearMaximumRowCount = 0u;

        for (uint32_t y = 0u; y < hullMask.getHeight(); ++y)
        {
            const uint32_t rowWidth = PixelMaskUtils::getOccupiedRowWidth(hullMask, y);

            if (rowWidth > nearMaximumThreshold)
            {
                ++nearMaximumRowCount;
            }
        }

        if (nearMaximumRowCount * 100u >= metrics.OccupiedHeight * 82u)
        {
            return false;
        }

        const uint32_t maximumAllowedRowWidthDelta = std::max(GenerationMath::scalePixelsFrom64(6u, hullMask.getWidth()), hullMask.getWidth() / 4u);

        if (metrics.MaximumRowWidthDelta > maximumAllowedRowWidthDelta)
        {
            return false;
        }

        return true;
    }

    SilhouetteValidationFailureReason HullGenerator::evaluateSilhouetteValidation(const ShipGenerationContext& context, const SilhouetteQualityMetrics& metrics) const
    {
        if (!validate(context.Ship.HullMask))
        {
            const bool tinyBroadSilhouetteException = context.Profile.AllowTinyBroadSilhouetteLegacyValidationException && context.ScaleTraits.MinimumDimension <= 24u &&
                metrics.PixelCount != 0u && isHullConnected(context.Ship.HullMask) && metrics.CanvasFillPercent >= 7u && metrics.CanvasFillPercent <= 68u &&
                metrics.NormalizedHeightPercent >= 50u && metrics.NormalizedWidthPercent >= 50u && metrics.WidthVariationPercent >= 20u &&
                metrics.MaximumRowWidthDelta <= std::max(GenerationMath::scalePixelsFrom64(8u, context.Settings.Dimensions.Width), context.Settings.Dimensions.Width / 3u) && metrics.NearMaximumRowPercent < 90u;
            const bool intentionalNegativeSpaceException = !context.StructuralNegativeSpace.empty() && metrics.PixelCount != 0u &&
                isHullConnected(context.Ship.HullMask) && metrics.CanvasFillPercent >= 5u && metrics.CanvasFillPercent <= 55u &&
                metrics.NormalizedHeightPercent >= 50u && metrics.NormalizedWidthPercent >= 20u && metrics.WidthVariationPercent >= 15u &&
                metrics.MaximumRowWidthDelta <= std::max(GenerationMath::scalePixelsFrom64(9u, context.Settings.Dimensions.Width), context.Settings.Dimensions.Width / 3u) &&
                metrics.NearMaximumRowPercent < 92u;
            if (!tinyBroadSilhouetteException && !intentionalNegativeSpaceException)
            {
                return SilhouetteValidationFailureReason::LEGACY_GEOMETRY_VALIDATION;
            }
        }
        if (!validateWingRegions(context))
        {
            return SilhouetteValidationFailureReason::INVALID_WING_REGIONS;
        }

        return evaluateSilhouetteProfileAcceptance(context, metrics);
    }

    SilhouetteValidationFailureReason HullGenerator::evaluateSilhouetteProfileAcceptance(const ShipGenerationContext& context, const SilhouetteQualityMetrics& metrics) const
    {
        // Some authored profiles deliberately opt out of Task-56 acceptance
        // pressure while still recording the same diagnostics metrics.
        if (!context.Profile.SilhouetteProfileValidationEnabled)
        {
            return SilhouetteValidationFailureReason::NONE;
        }

        uint32_t minimumWidthPercent = context.Profile.MinimumSilhouetteWidthPercent;
        uint32_t minimumHeightPercent = context.Profile.MinimumSilhouetteHeightPercent;
        if (context.ScaleTraits.MinimumDimension <= 24u)
        {
            minimumWidthPercent = minimumWidthPercent > 8u ? minimumWidthPercent - 8u : 0u;
        }
        else if (context.ScaleTraits.MinimumDimension <= 32u)
        {
            minimumWidthPercent = minimumWidthPercent > 4u ? minimumWidthPercent - 4u : 0u;
        }
        else if (context.ScaleTraits.MinimumDimension <= 44u)
        {
            minimumWidthPercent = minimumWidthPercent > 2u ? minimumWidthPercent - 2u : 0u;
        }
        if (context.ScaleTraits.MinimumDimension <= 24u && context.Profile.TinySilhouetteExtraWidthRelaxationPercent != 0u)
        {
            const uint32_t relaxation = context.Profile.TinySilhouetteExtraWidthRelaxationPercent;
            minimumWidthPercent = minimumWidthPercent > relaxation ? minimumWidthPercent - relaxation : 0u;
        }

        if (context.Settings.Dimensions.Height * 4u > context.Settings.Dimensions.Width * 5u)
        {
            minimumWidthPercent = minimumWidthPercent > 5u ? minimumWidthPercent - 5u : 0u;
        }
        if (context.Settings.Dimensions.Width * 4u > context.Settings.Dimensions.Height * 5u)
        {
            minimumHeightPercent = minimumHeightPercent > 5u ? minimumHeightPercent - 5u : 0u;
        }

        if (metrics.NormalizedWidthPercent < minimumWidthPercent)
        {
            return SilhouetteValidationFailureReason::LOW_LATERAL_UTILIZATION;
        }
        if (metrics.NormalizedHeightPercent < minimumHeightPercent)
        {
            return SilhouetteValidationFailureReason::LOW_LONGITUDINAL_UTILIZATION;
        }

        const uint32_t targetArticulation = getScaleAdjustedArticulationTarget(context);
        const uint32_t strongWingExtension = std::max(1u, context.Settings.Dimensions.Width / 16u);
        const bool strongLateralStructure = context.WingRegions.Shape != WingShapeType::NONE && context.WingRegions.MaximumExtension >= strongWingExtension;
        const bool cleanAxialTaper = context.Profile.CleanAxialTaperArticulationExemption && metrics.NoseTaperPercent >= 45u && metrics.RearTaperPercent >= 18u;
        const bool validWingWedge = context.Profile.WingWedgeArticulationExemption && context.WingRegions.Shape != WingShapeType::NONE && metrics.NormalizedWidthPercent >= minimumWidthPercent;
        const bool meaningfulInteriorShape = metrics.InteriorContractionPercent >= 8u || metrics.ShoulderProminencePercent >= 8u;

        if (metrics.BoundingFillPercent >= context.Profile.SilhouetteConvexFillTriggerPercent && metrics.LongestStableWidthRunPercent > context.Profile.SilhouetteMaximumStableRunPercent && metrics.NearMaximumRowPercent >= 40u && metrics.ArticulationCount <= targetArticulation && !meaningfulInteriorShape && !validWingWedge)
        {
            return SilhouetteValidationFailureReason::EXCESSIVE_SOLID_MASS;
        }

        const bool genericFormRisk = metrics.BoundingFillPercent + 8u >= context.Profile.SilhouetteConvexFillTriggerPercent || metrics.LongestStableWidthRunPercent > context.Profile.SilhouetteMaximumStableRunPercent || metrics.NearMaximumRowPercent >= 32u;
        if (metrics.ArticulationCount < targetArticulation && !meaningfulInteriorShape && !strongLateralStructure && !cleanAxialTaper && !validWingWedge && genericFormRisk)
        {
            return SilhouetteValidationFailureReason::LOW_ARTICULATION;
        }

        return SilhouetteValidationFailureReason::NONE;
    }

    uint32_t HullGenerator::getScaleAdjustedArticulationTarget(const ShipGenerationContext& context) const
    {
        uint32_t target = context.Profile.SilhouetteArticulationTarget;
        const uint32_t minimumDimension = context.ScaleTraits.MinimumDimension;
        if (minimumDimension <= 24u)
        {
            target = target > 2u ? target - 2u : 1u;
        }
        else if (minimumDimension <= 44u)
        {
            target = target > 1u ? target - 1u : 1u;
        }
        else if (minimumDimension >= 96u && context.Profile.SilhouetteArticulationTarget >= 3u)
        {
            ++target;
        }
        return std::max(1u, target);
    }

    bool HullGenerator::isHullConnected(const PixelMask& hullMask) const
    {
        const uint32_t width = hullMask.getWidth();
        const uint32_t height = hullMask.getHeight();
        const uint32_t totalPixels = PixelMaskUtils::getMaskPixelCount(hullMask);

        if (totalPixels == 0u)
        {
            return false;
        }

        uint32_t startX = 0u;
        uint32_t startY = 0u;
        bool foundStart = false;

        for (uint32_t y = 0u; y < height && !foundStart; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (!hullMask.get(x, y))
                {
                    continue;
                }

                startX = x;
                startY = y;
                foundStart = true;
                break;
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
                const std::size_t visitedIndex = static_cast<std::size_t>(pixelY) * width + pixelX;

                if (visited[visitedIndex] != 0u || !hullMask.get(pixelX, pixelY))
                {
                    continue;
                }

                visited[visitedIndex] = 1u;
                pending.emplace_back(pixelX, pixelY);
            }
        }

        return visitedCount == totalPixels;
    }

    HullGenerator::RowBounds HullGenerator::calculateRowBounds(const PixelMask& hullMask, uint32_t y) const
    {
        RowBounds bounds;

        if (y >= hullMask.getHeight())
        {
            return bounds;
        }

        for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
        {
            if (!hullMask.get(x, y))
            {
                continue;
            }

            if (!bounds.Valid)
            {
                bounds.MinX = x;
                bounds.MaxX = x;
                bounds.Valid = true;
                continue;
            }

            bounds.MaxX = x;
        }

        return bounds;
    }

    bool HullGenerator::extendRowEdges(PixelMask& hullMask, uint32_t y, uint32_t amount) const
    {
        if (amount == 0u)
        {
            return false;
        }

        const RowBounds bounds = calculateRowBounds(hullMask, y);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t minimumX = bounds.MinX > amount ? bounds.MinX - amount : 0u;
        const uint32_t maximumX = std::min(hullMask.getWidth() - 1u, bounds.MaxX + amount);
        bool changed = false;

        for (uint32_t x = minimumX; x < bounds.MinX; ++x)
        {
            hullMask.set(x, y, true);
            changed = true;
        }

        for (uint32_t x = bounds.MaxX + 1u; x <= maximumX; ++x)
        {
            hullMask.set(x, y, true);
            changed = true;
        }

        return changed;
    }

    bool HullGenerator::removeRowEdgePixels(PixelMask& hullMask, uint32_t y, uint32_t amount, uint32_t minimumRemainingWidth) const
    {
        if (amount == 0u)
        {
            return false;
        }

        const RowBounds bounds = calculateRowBounds(hullMask, y);

        if (!bounds.Valid)
        {
            return false;
        }

        const uint32_t currentWidth = PixelMaskUtils::getOccupiedRowWidth(hullMask, y);

        if (currentWidth <= minimumRemainingWidth || amount * 2u > currentWidth - minimumRemainingWidth)
        {
            return false;
        }

        for (uint32_t offset = 0u; offset < amount; ++offset)
        {
            hullMask.set(bounds.MinX + offset, y, false);
            hullMask.set(bounds.MaxX - offset, y, false);
        }

        return true;
    }

    WingShapeType HullGenerator::getWingStyle(ShipGenerationContext& context, const ShipGenerationProfile& profile) const
    {
        const uint32_t horizontalCapacity = context.ScaleTraits.HorizontalCapacity;
        const uint32_t longitudinalCapacity = context.ScaleTraits.LongitudinalCapacity;
        uint32_t noWingWeight = profile.NoWingWeight + (100u - horizontalCapacity) / 5u;
        uint32_t smallWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(profile.SmallWingWeight) * (70u + horizontalCapacity * 30u / 100u) + 50u) / 100u);
        uint32_t sweptWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(profile.SweptWingWeight) * (50u + horizontalCapacity / 2u) + 50u) / 100u);
        uint32_t broadWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(profile.BroadWingWeight) * (35u + horizontalCapacity * 65u / 100u) + 50u) / 100u);

        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::WINGS))
        {
            const uint32_t influence = context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::WINGS);
            noWingWeight = std::max(1u, noWingWeight * 55u / 100u);
            smallWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(smallWingWeight) * influence + 50u) / 100u);
            sweptWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(sweptWingWeight) * influence + 50u) / 100u);
            broadWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(broadWingWeight) * influence + 50u) / 100u);
        }

        if (horizontalCapacity > longitudinalCapacity + 15u)
        {
            broadWingWeight = static_cast<uint32_t>((static_cast<uint64_t>(broadWingWeight) * 115u + 50u) / 100u);
        }

        const uint32_t totalWeight = noWingWeight + smallWingWeight + sweptWingWeight + broadWingWeight;

        if (totalWeight == 0u)
        {
            return WingShapeType::NONE;
        }

        const uint32_t roll = context.getGenerationRandomUInt(GenerationDomain::WINGS, 0u, totalWeight - 1u);

        if (roll < noWingWeight)
        {
            return WingShapeType::NONE;
        }

        if (roll < noWingWeight + smallWingWeight)
        {
            return WingShapeType::SMALL;
        }

        if (roll < noWingWeight + smallWingWeight + sweptWingWeight)
        {
            return WingShapeType::SWEPT;
        }

        return WingShapeType::BROAD;
    }

    HullGenerator::HullMetrics HullGenerator::calculateHullMetrics(const PixelMask& hullMask) const
    {
        HullMetrics metrics;

        if (hullMask.empty())
        {
            return metrics;
        }

        metrics.MinimumOccupiedWidth = std::numeric_limits<uint32_t>::max();
        metrics.MinimumOccupiedHeight = std::numeric_limits<uint32_t>::max();
        std::vector<uint32_t> columnHeights(hullMask.getWidth(), 0u);
        uint32_t minimumY = hullMask.getHeight();
        uint32_t maximumY = 0u;
        uint32_t previousOccupiedRowWidth = 0u;
        bool hasPreviousOccupiedRow = false;

        for (uint32_t y = 0u; y < hullMask.getHeight(); ++y)
        {
            uint32_t rowWidth = 0u;

            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (!hullMask.get(x, y))
                {
                    continue;
                }

                ++metrics.PixelCount;
                ++rowWidth;
                ++columnHeights[x];
                minimumY = std::min(minimumY, y);
                maximumY = std::max(maximumY, y);
            }

            if (rowWidth == 0u)
            {
                continue;
            }

            metrics.MinimumOccupiedWidth = std::min(metrics.MinimumOccupiedWidth, rowWidth);
            metrics.MaximumOccupiedWidth = std::max(metrics.MaximumOccupiedWidth, rowWidth);

            if (hasPreviousOccupiedRow)
            {
                metrics.MaximumRowWidthDelta = std::max(metrics.MaximumRowWidthDelta, GenerationMath::getDifference(previousOccupiedRowWidth, rowWidth));
            }

            previousOccupiedRowWidth = rowWidth;
            hasPreviousOccupiedRow = true;
        }

        if (metrics.PixelCount == 0u)
        {
            metrics.MinimumOccupiedWidth = 0u;
            metrics.MinimumOccupiedHeight = 0u;
            return metrics;
        }

        metrics.OccupiedHeight = maximumY - minimumY + 1u;

        for (uint32_t columnHeight : columnHeights)
        {
            if (columnHeight == 0u)
            {
                continue;
            }

            metrics.MinimumOccupiedHeight = std::min(metrics.MinimumOccupiedHeight, columnHeight);
            metrics.MaximumOccupiedHeight = std::max(metrics.MaximumOccupiedHeight, columnHeight);
        }

        return metrics;
    }

    void HullGenerator::fillWidthTransition(std::vector<uint32_t>& profile, uint32_t startIndex, uint32_t endIndex, uint32_t startWidth, uint32_t endWidth) const
    {
        if (startIndex >= profile.size() || endIndex >= profile.size() || startIndex > endIndex)
        {
            return;
        }

        if (startIndex == endIndex)
        {
            profile[startIndex] = endWidth;
            return;
        }

        const uint32_t span = endIndex - startIndex;

        for (uint32_t index = startIndex; index <= endIndex; ++index)
        {
            const uint32_t step = index - startIndex;

            if (endWidth >= startWidth)
            {
                const uint32_t difference = endWidth - startWidth;
                profile[index] = startWidth + ((difference * step + span / 2u) / span);
            }
            else
            {
                const uint32_t difference = startWidth - endWidth;
                profile[index] = startWidth - ((difference * step + span / 2u) / span);
            }
        }
    }

    void HullGenerator::rasterizeSymmetricHull(PixelMask& mask, const std::vector<uint32_t>& profile, uint32_t topY) const
    {
        const uint32_t leftCenter = (mask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = mask.getWidth() / 2u;

        for (uint32_t row = 0u; row < profile.size(); ++row)
        {
            const uint32_t y = topY + row;
            const uint32_t halfWidth = profile[row];
            const uint32_t leftX = leftCenter - (halfWidth - 1u);
            const uint32_t rightX = rightCenter + (halfWidth - 1u);

            for (uint32_t x = leftX; x <= rightX; ++x)
            {
                mask.set(x, y);
            }
        }
    }
}
