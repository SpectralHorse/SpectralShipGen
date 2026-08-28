#include "EngineGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        uint32_t getPercentOfWidth(uint32_t width, uint32_t percentage)
        {
            return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(width) * percentage + 50u) / 100u));
        }

        uint32_t getLayoutMinimumWidth(EngineLayoutType layout, uint32_t unitWidth)
        {
            switch (layout)
            {
            case EngineLayoutType::CENTRAL: return unitWidth;
            case EngineLayoutType::TWIN: return unitWidth * 2u + 1u;
            case EngineLayoutType::QUAD: return unitWidth * 4u + 3u;
            case EngineLayoutType::CENTRAL_AUXILIARY: return unitWidth * 4u + 2u;
            case EngineLayoutType::WIDE_BANK: return unitWidth * 3u + 2u;
            default: return unitWidth;
            }
        }

        struct FactionEngineProfile
        {
            std::array<uint32_t, static_cast<std::size_t>(EngineLayoutType::ENGINE_LAYOUT_TYPE_END)> LayoutWeightPercent = { 100u, 100u, 100u, 100u, 100u };
            std::array<uint32_t, static_cast<std::size_t>(EngineSizeClass::ENGINE_SIZE_CLASS_END)> SizeWeightPercent = { 100u, 100u, 100u };
            uint32_t NacelleChancePercent = 100u;
            uint32_t ExternalHeightPercent = 100u;
        };

        FactionEngineProfile getFactionEngineProfile(ShipFactionType faction)
        {
            FactionEngineProfile profile;
            switch (faction)
            {
            case ShipFactionType::CORPORATE:
                profile.LayoutWeightPercent = { 78u, 132u, 112u, 108u, 118u };
                profile.SizeWeightPercent = { 55u, 170u, 70u };
                profile.NacelleChancePercent = 180u;
                profile.ExternalHeightPercent = 92u;
                break;
            case ShipFactionType::RELIC:
                profile.LayoutWeightPercent = { 148u, 82u, 55u, 138u, 68u };
                profile.SizeWeightPercent = { 15u, 55u, 330u };
                profile.NacelleChancePercent = 25u;
                profile.ExternalHeightPercent = 60u;
                break;
            default:
                break;
            }
            return profile;
        }

        uint32_t applyPercent(uint32_t value, uint32_t percent)
        {
            return static_cast<uint32_t>((static_cast<uint64_t>(value) * percent + 50u) / 100u);
        }
    }

    void EngineGenerator::generate(ShipGenerationContext& context) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationProfile& profile = context.Profile;
        const FactionEngineProfile factionEngineProfile = getFactionEngineProfile(context.Settings.Faction);
        ship.EngineMask.clear(false);
        ship.EngineExhaustMask.clear(false);
        ship.IdleAnimationMetadata.EngineComponents.clear();

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->EngineCount = 0u;
            context.DebugInfo->EngineLayout = EngineLayoutType::ENGINE_LAYOUT_TYPE_END;
            context.DebugInfo->EngineUnits.clear();
        }

        const uint32_t imageWidth = ship.HullMask.getWidth();
        const uint32_t imageHeight = ship.HullMask.getHeight();

        if (imageWidth < 3u || imageHeight < 3u)
        {
            return;
        }

        uint32_t hullBottom = 0u;
        bool foundHull = false;

        for (uint32_t y = 0u; y < imageHeight; ++y)
        {
            if (PixelMaskUtils::getOccupiedRowWidth(ship.HullMask, y) == 0u)
            {
                continue;
            }

            hullBottom = y;
            foundHull = true;
        }

        if (!foundHull || hullBottom + 2u >= imageHeight)
        {
            return;
        }

        const uint32_t rearWidth = PixelMaskUtils::getOccupiedRowWidth(ship.HullMask, hullBottom);

        if (rearWidth < 2u)
        {
            return;
        }

        const uint32_t availableRearSpace = imageHeight - hullBottom - 1u;
        const uint32_t rearStartX = (imageWidth - rearWidth) / 2u;
        std::vector<RearSupportRun> rearSupportRuns;
        collectRearSupportRuns(ship.HullMask, hullBottom, rearSupportRuns);
        const bool separatedRearArchitecture = (context.StructuralNegativeSpace.hasType(ShipStructuralNegativeSpaceType::REAR_FORK) || context.StructuralNegativeSpace.hasType(ShipStructuralNegativeSpaceType::NACELLE_CHANNEL)) && rearSupportRuns.size() >= 2u;
        EngineLayoutType layout = separatedRearArchitecture ? (rearSupportRuns.size() >= 3u ? EngineLayoutType::CENTRAL_AUXILIARY : EngineLayoutType::TWIN) : getEngineLayout(context, profile, rearWidth, imageWidth);
        const EngineSizeClass sizeClass = getEngineSizeClass(context, profile, layout, rearWidth, availableRearSpace);
        std::vector<EnginePlacement> placements;

        const bool placementsCreated = separatedRearArchitecture
            ? createSeparatedRearPlacements(rearSupportRuns, layout, sizeClass, imageWidth, placements)
            : createEnginePlacements(layout, sizeClass, rearStartX, rearWidth, imageWidth, placements);
        if (!placementsCreated || placements.empty())
        {
            return;
        }

        const uint32_t minimumDesiredExhaustLength = sizeClass == EngineSizeClass::SMALL ? 1u : GenerationMath::scalePixelsFrom64(2u, imageHeight);
        const uint32_t desiredAnimationReserve = context.ScaleTraits.LongitudinalCapacity < 20u ? 1u : GenerationMath::scalePixelsFrom64(sizeClass == EngineSizeClass::LARGE ? 3u : 2u, imageHeight);
        const uint32_t maximumReservableExhaustSpace = availableRearSpace > 1u ? availableRearSpace - 1u : 0u;
        const uint32_t propulsionPriorityReserve = std::min(maximumReservableExhaustSpace, minimumDesiredExhaustLength + desiredAnimationReserve);
        const uint32_t minimumExhaustReserve = std::max(std::max(1u, availableRearSpace / 2u), propulsionPriorityReserve);
        const uint32_t desiredExternalHeight = std::max(1u, applyPercent(getDesiredExternalHeight(sizeClass, imageHeight), factionEngineProfile.ExternalHeightPercent));
        const uint32_t maximumExternalHeight = availableRearSpace > minimumExhaustReserve ? availableRearSpace - minimumExhaustReserve : 0u;

        if (maximumExternalHeight == 0u)
        {
            return;
        }

        const uint32_t externalHeight = std::max(1u, std::min(desiredExternalHeight, maximumExternalHeight));
        const uint32_t exhaustStartY = hullBottom + externalHeight + 1u;

        if (exhaustStartY >= imageHeight)
        {
            return;
        }

        const uint32_t maximumAvailableExhaustLength = imageHeight - exhaustStartY;
        const uint32_t desiredMaximumExhaustLength = getDesiredMaximumExhaustLength(sizeClass, imageHeight);
        const uint32_t maximumExhaustLength = std::max(1u, std::min(maximumAvailableExhaustLength, desiredMaximumExhaustLength));
        const uint32_t minimumExhaustLength = std::max(1u, std::min(minimumDesiredExhaustLength, maximumExhaustLength));
        const uint32_t exhaustRange = maximumExhaustLength - minimumExhaustLength;
        uint32_t minimumBaseExhaustLength = minimumExhaustLength;
        uint32_t maximumBaseExhaustLength = minimumExhaustLength;

        if (exhaustRange >= 2u)
        {
            const uint32_t extensionReserve = std::min(desiredAnimationReserve, exhaustRange - 1u);
            minimumBaseExhaustLength = minimumExhaustLength + 1u;
            maximumBaseExhaustLength = std::max(minimumBaseExhaustLength, maximumExhaustLength - extensionReserve);
        }

        const uint32_t baseRange = maximumBaseExhaustLength - minimumBaseExhaustLength;
        const uint32_t preferredBaseMinimum = minimumBaseExhaustLength + baseRange / 3u;
        const uint32_t preferredBaseMaximum = minimumBaseExhaustLength + (baseRange * 2u + 2u) / 3u;
        const uint32_t exhaustLength = context.getGenerationRandomUInt(GenerationDomain::ENGINES, preferredBaseMinimum, std::max(preferredBaseMinimum, preferredBaseMaximum));
        const uint32_t taperMode = context.getGenerationRandomUInt(GenerationDomain::ENGINES, 0u, 2u);
        PixelMask generatedEngineMask = ship.EngineMask;
        PixelMask generatedExhaustMask = ship.EngineExhaustMask;
        generatedEngineMask.clear(false);
        generatedExhaustMask.clear(false);
        std::vector<EngineUnitDebugInfo> debugUnits;
        debugUnits.reserve(placements.size());
        std::vector<ShipEngineAnimationComponent> animationComponents;
        animationComponents.reserve(placements.size());
        std::array<bool, static_cast<std::size_t>(EngineSizeClass::ENGINE_SIZE_CLASS_END)> nacelleDecisionMade = {};
        std::array<bool, static_cast<std::size_t>(EngineSizeClass::ENGINE_SIZE_CLASS_END)> nacelleBySize = {};

        for (std::size_t placementIndex = 0u; placementIndex < placements.size(); ++placementIndex)
        {
            EnginePlacement& placement = placements[placementIndex];
            const std::size_t sizeIndex = static_cast<std::size_t>(placement.SizeClass);

            if (placement.HousingWidth >= 3u && placement.SizeClass != EngineSizeClass::SMALL && !nacelleDecisionMade[sizeIndex])
            {
                nacelleDecisionMade[sizeIndex] = true;
                uint32_t nacelleChance = static_cast<uint32_t>((static_cast<uint64_t>(profile.EngineNacelleChance) * (35u + context.ScaleTraits.SmallFeatureCapacity * 65u / 100u) + 50u) / 100u);
                nacelleChance = std::min(100u, applyPercent(nacelleChance, factionEngineProfile.NacelleChancePercent));
                nacelleBySize[sizeIndex] = context.getGenerationRandomUInt(GenerationDomain::ENGINES, 0u, 99u) < nacelleChance;
                if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedEngineNacellePresence.has_value())
                {
                    nacelleBySize[sizeIndex] = *context.CalibrationSettings->Overrides.ForcedEngineNacellePresence;
                }
            }

            const bool structuralNacelle = context.StructuralNegativeSpace.hasType(ShipStructuralNegativeSpaceType::NACELLE_CHANNEL) && placement.HousingWidth >= 2u;
            placement.Nacelle = structuralNacelle || (placement.HousingWidth >= 3u && placement.SizeClass != EngineSizeClass::SMALL && nacelleBySize[sizeIndex]);
            uint32_t rootDepth = std::min(getDesiredRootDepth(placement.SizeClass, imageHeight), hullBottom + 1u);

            while (rootDepth > 1u)
            {
                const uint32_t rootY = hullBottom - rootDepth + 1u;
                bool validRootRow = true;

                for (uint32_t x = placement.StartX; x < placement.StartX + placement.HousingWidth; ++x)
                {
                    if (!ship.HullMask.get(x, rootY) || ship.CockpitMask.get(x, rootY))
                    {
                        validRootRow = false;
                        break;
                    }
                }

                if (validRootRow)
                {
                    break;
                }

                --rootDepth;
            }

            if (!addEngineAssembly(generatedEngineMask, ship.HullMask, ship.CockpitMask, placement, hullBottom, rootDepth, externalHeight))
            {
                return;
            }

            if (!addTaperedExhaust(generatedExhaustMask, placement, exhaustStartY, exhaustLength, taperMode))
            {
                return;
            }

            EngineUnitDebugInfo unitInfo;
            unitInfo.SizeClass = placement.SizeClass;
            unitInfo.HousingStartX = placement.StartX;
            unitInfo.HousingWidth = placement.HousingWidth;
            unitInfo.NozzleStartX = placement.StartX + (placement.HousingWidth - placement.NozzleWidth) / 2u;
            unitInfo.NozzleWidth = placement.NozzleWidth;
            unitInfo.RootStartY = hullBottom - rootDepth + 1u;
            unitInfo.NozzleY = hullBottom + externalHeight;
            unitInfo.ExhaustStartY = exhaustStartY;
            unitInfo.ExhaustLength = exhaustLength;
            unitInfo.Nacelle = placement.Nacelle;
            debugUnits.push_back(unitInfo);

            ShipEngineAnimationComponent animationComponent;
            animationComponent.HousingStartX = unitInfo.HousingStartX;
            animationComponent.HousingWidth = unitInfo.HousingWidth;
            animationComponent.NozzleStartX = unitInfo.NozzleStartX;
            animationComponent.NozzleWidth = unitInfo.NozzleWidth;
            animationComponent.RootStartY = unitInfo.RootStartY;
            animationComponent.NozzleY = unitInfo.NozzleY;
            animationComponent.ExhaustStartY = unitInfo.ExhaustStartY;
            animationComponent.ExhaustLength = unitInfo.ExhaustLength;
            animationComponent.MinimumExhaustLength = minimumExhaustLength;
            animationComponent.MaximumExhaustLength = maximumExhaustLength;
            animationComponent.TaperMode = taperMode;
            animationComponent.DominantEngine = layout == EngineLayoutType::CENTRAL || (layout == EngineLayoutType::CENTRAL_AUXILIARY && placementIndex == placements.size() / 2u);
            animationComponent.Nacelle = unitInfo.Nacelle;
            animationComponents.push_back(animationComponent);
        }

        if (!validateGeneratedEngines(generatedEngineMask, generatedExhaustMask, ship.HullMask, ship.CockpitMask))
        {
            return;
        }

        ship.EngineMask = std::move(generatedEngineMask);
        ship.EngineExhaustMask = std::move(generatedExhaustMask);
        ship.IdleAnimationMetadata.EngineComponents = std::move(animationComponents);

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->EngineCount = static_cast<uint32_t>(placements.size());
            context.DebugInfo->EngineLayout = layout;
            context.DebugInfo->EngineUnits = std::move(debugUnits);
        }
    }

    EngineLayoutType EngineGenerator::getEngineLayout(ShipGenerationContext& context, const ShipGenerationProfile& profile, uint32_t rearWidth, uint32_t imageWidth) const
    {
        const uint32_t minimumUnitWidth = std::max(1u, GenerationMath::scalePixelsFrom64(2u, imageWidth));
        const bool twinAvailable = rearWidth >= getLayoutMinimumWidth(EngineLayoutType::TWIN, minimumUnitWidth);
        const bool quadAvailable = rearWidth >= getLayoutMinimumWidth(EngineLayoutType::QUAD, minimumUnitWidth);
        const bool centralAuxiliaryAvailable = rearWidth >= getLayoutMinimumWidth(EngineLayoutType::CENTRAL_AUXILIARY, minimumUnitWidth);
        const bool wideBankAvailable = rearWidth >= getLayoutMinimumWidth(EngineLayoutType::WIDE_BANK, minimumUnitWidth);
        const uint32_t horizontalCapacity = context.ScaleTraits.HorizontalCapacity;
        const uint32_t longitudinalCapacity = context.ScaleTraits.LongitudinalCapacity;
        const FactionEngineProfile factionProfile = getFactionEngineProfile(context.Settings.Faction);
        uint32_t centralWeight = applyPercent(profile.CentralEngineWeight, factionProfile.LayoutWeightPercent[static_cast<std::size_t>(EngineLayoutType::CENTRAL)]);
        uint32_t twinWeight = twinAvailable ? applyPercent(static_cast<uint32_t>((static_cast<uint64_t>(profile.TwinEngineWeight) * (65u + horizontalCapacity * 35u / 100u) + 50u) / 100u), factionProfile.LayoutWeightPercent[static_cast<std::size_t>(EngineLayoutType::TWIN)]) : 0u;
        uint32_t quadWeight = quadAvailable ? applyPercent(static_cast<uint32_t>((static_cast<uint64_t>(profile.QuadEngineWeight) * (30u + horizontalCapacity * 70u / 100u) + 50u) / 100u), factionProfile.LayoutWeightPercent[static_cast<std::size_t>(EngineLayoutType::QUAD)]) : 0u;
        const uint32_t centralAuxiliaryCapacity = (horizontalCapacity + longitudinalCapacity + 1u) / 2u;
        uint32_t centralAuxiliaryWeight = centralAuxiliaryAvailable ? applyPercent(static_cast<uint32_t>((static_cast<uint64_t>(profile.CentralAuxiliaryEngineWeight) * (40u + centralAuxiliaryCapacity * 60u / 100u) + 50u) / 100u), factionProfile.LayoutWeightPercent[static_cast<std::size_t>(EngineLayoutType::CENTRAL_AUXILIARY)]) : 0u;
        uint32_t wideBankWeight = wideBankAvailable ? applyPercent(static_cast<uint32_t>((static_cast<uint64_t>(profile.EngineBankWeight) * (25u + horizontalCapacity * 75u / 100u) + 50u) / 100u), factionProfile.LayoutWeightPercent[static_cast<std::size_t>(EngineLayoutType::WIDE_BANK)]) : 0u;

        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::ENGINES))
        {
            const uint32_t influence = context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::ENGINES);
            centralWeight = centralWeight * 115u / 100u;
            twinWeight = static_cast<uint32_t>((static_cast<uint64_t>(twinWeight) * influence + 50u) / 100u);
            quadWeight = static_cast<uint32_t>((static_cast<uint64_t>(quadWeight) * influence + 50u) / 100u);
            centralAuxiliaryWeight = static_cast<uint32_t>((static_cast<uint64_t>(centralAuxiliaryWeight) * influence + 50u) / 100u);
            wideBankWeight = static_cast<uint32_t>((static_cast<uint64_t>(wideBankWeight) * influence + 50u) / 100u);
        }
        const uint32_t totalWeight = centralWeight + twinWeight + quadWeight + centralAuxiliaryWeight + wideBankWeight;

        if (totalWeight == 0u)
        {
            return EngineLayoutType::CENTRAL;
        }

        const uint32_t roll = context.getGenerationRandomUInt(GenerationDomain::ENGINES, 0u, totalWeight - 1u);
        EngineLayoutType selected = EngineLayoutType::WIDE_BANK;
        if (roll < centralWeight) { selected = EngineLayoutType::CENTRAL; }
        else if (roll < centralWeight + twinWeight) { selected = EngineLayoutType::TWIN; }
        else if (roll < centralWeight + twinWeight + quadWeight) { selected = EngineLayoutType::QUAD; }
        else if (roll < centralWeight + twinWeight + quadWeight + centralAuxiliaryWeight) { selected = EngineLayoutType::CENTRAL_AUXILIARY; }

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedEngineLayout.has_value())
        {
            const EngineLayoutType forced = *context.CalibrationSettings->Overrides.ForcedEngineLayout;
            const bool available = forced == EngineLayoutType::CENTRAL || (forced == EngineLayoutType::TWIN && twinAvailable) || (forced == EngineLayoutType::QUAD && quadAvailable) || (forced == EngineLayoutType::CENTRAL_AUXILIARY && centralAuxiliaryAvailable) || (forced == EngineLayoutType::WIDE_BANK && wideBankAvailable);
            if (!available) { throw std::runtime_error("Forced calibration engine layout is unavailable for this hull geometry."); }
            return forced;
        }

        return selected;
    }

    EngineSizeClass EngineGenerator::getEngineSizeClass(ShipGenerationContext& context, const ShipGenerationProfile& profile, EngineLayoutType layout, uint32_t rearWidth, uint32_t availableRearSpace) const
    {
        const bool mediumAvailable = rearWidth >= 4u && availableRearSpace >= 2u;
        const bool largeGeometryAvailable = rearWidth >= 7u && availableRearSpace >= 4u;
        const bool largeLayoutAvailable = layout != EngineLayoutType::QUAD && layout != EngineLayoutType::WIDE_BANK;
        const FactionEngineProfile factionProfile = getFactionEngineProfile(context.Settings.Faction);
        uint32_t smallWeight = applyPercent(profile.SmallEngineSizeWeight, factionProfile.SizeWeightPercent[static_cast<std::size_t>(EngineSizeClass::SMALL)]);
        uint32_t mediumWeight = mediumAvailable ? applyPercent(static_cast<uint32_t>((static_cast<uint64_t>(profile.MediumEngineSizeWeight) * (60u + context.ScaleTraits.MajorFeatureCapacity * 40u / 100u) + 50u) / 100u), factionProfile.SizeWeightPercent[static_cast<std::size_t>(EngineSizeClass::MEDIUM)]) : 0u;
        uint32_t largeWeight = largeGeometryAvailable && largeLayoutAvailable ? applyPercent(static_cast<uint32_t>((static_cast<uint64_t>(profile.LargeEngineSizeWeight) * (20u + context.ScaleTraits.SmallFeatureCapacity * 80u / 100u) + 50u) / 100u), factionProfile.SizeWeightPercent[static_cast<std::size_t>(EngineSizeClass::LARGE)]) : 0u;
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::ENGINES))
        {
            const uint32_t influence = context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::ENGINES);
            smallWeight = smallWeight * 55u / 100u;
            mediumWeight = static_cast<uint32_t>((static_cast<uint64_t>(mediumWeight) * influence + 50u) / 100u);
            largeWeight = static_cast<uint32_t>((static_cast<uint64_t>(largeWeight) * influence + 50u) / 100u);
        }
        const uint32_t totalWeight = smallWeight + mediumWeight + largeWeight;

        if (totalWeight == 0u)
        {
            return EngineSizeClass::SMALL;
        }

        const uint32_t roll = context.getGenerationRandomUInt(GenerationDomain::ENGINES, 0u, totalWeight - 1u);
        EngineSizeClass selected = EngineSizeClass::LARGE;
        if (roll < smallWeight) { selected = EngineSizeClass::SMALL; }
        else if (roll < smallWeight + mediumWeight) { selected = EngineSizeClass::MEDIUM; }

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedEngineSize.has_value())
        {
            const EngineSizeClass forced = *context.CalibrationSettings->Overrides.ForcedEngineSize;
            const bool available = forced == EngineSizeClass::SMALL || (forced == EngineSizeClass::MEDIUM && mediumAvailable) || (forced == EngineSizeClass::LARGE && largeGeometryAvailable && largeLayoutAvailable);
            if (!available) { throw std::runtime_error("Forced calibration engine size is unavailable for this hull/layout geometry."); }
            return forced;
        }

        return selected;
    }

    bool EngineGenerator::createEnginePlacements(EngineLayoutType layout, EngineSizeClass sizeClass, uint32_t rearStartX, uint32_t rearWidth, uint32_t imageWidth, std::vector<EnginePlacement>& placements) const
    {
        placements.clear();
        const uint32_t minimumUnitWidth = 1u;
        const uint32_t gap = 1u;

        if (layout == EngineLayoutType::CENTRAL)
        {
            const uint32_t housingWidth = std::min(rearWidth, getHousingWidth(rearWidth, layout, sizeClass));
            const uint32_t startX = rearStartX + (rearWidth - housingWidth) / 2u;
            placements.push_back({ startX, housingWidth, getNozzleWidth(housingWidth, sizeClass), sizeClass, false });
            return true;
        }

        if (layout == EngineLayoutType::TWIN)
        {
            if (rearWidth < minimumUnitWidth * 2u + gap) { return false; }
            const uint32_t maximumWidth = (rearWidth - gap) / 2u;
            const uint32_t housingWidth = std::max(minimumUnitWidth, std::min(maximumWidth, getHousingWidth(rearWidth, layout, sizeClass)));
            const uint32_t groupWidth = housingWidth * 2u + gap;
            const uint32_t leftStart = rearStartX + (rearWidth - groupWidth) / 2u;
            const uint32_t rightStart = imageWidth - leftStart - housingWidth;
            placements.push_back({ leftStart, housingWidth, getNozzleWidth(housingWidth, sizeClass), sizeClass, false });
            placements.push_back({ rightStart, housingWidth, getNozzleWidth(housingWidth, sizeClass), sizeClass, false });
            return true;
        }

        if (layout == EngineLayoutType::QUAD)
        {
            if (rearWidth < minimumUnitWidth * 4u + gap * 3u) { return false; }
            const uint32_t maximumWidth = (rearWidth - gap * 3u) / 4u;
            const uint32_t housingWidth = std::max(minimumUnitWidth, std::min(maximumWidth, getHousingWidth(rearWidth, layout, sizeClass)));
            const uint32_t groupWidth = housingWidth * 4u + gap * 3u;
            const uint32_t leftOuterStart = rearStartX + (rearWidth - groupWidth) / 2u;
            const uint32_t leftInnerStart = leftOuterStart + housingWidth + gap;
            const uint32_t rightInnerStart = imageWidth - leftInnerStart - housingWidth;
            const uint32_t rightOuterStart = imageWidth - leftOuterStart - housingWidth;

            if (leftInnerStart + housingWidth > rightInnerStart) { return false; }

            placements.push_back({ leftOuterStart, housingWidth, getNozzleWidth(housingWidth, EngineSizeClass::SMALL), EngineSizeClass::SMALL, false });
            placements.push_back({ leftInnerStart, housingWidth, getNozzleWidth(housingWidth, EngineSizeClass::SMALL), EngineSizeClass::SMALL, false });
            placements.push_back({ rightInnerStart, housingWidth, getNozzleWidth(housingWidth, EngineSizeClass::SMALL), EngineSizeClass::SMALL, false });
            placements.push_back({ rightOuterStart, housingWidth, getNozzleWidth(housingWidth, EngineSizeClass::SMALL), EngineSizeClass::SMALL, false });
            return true;
        }

        if (layout == EngineLayoutType::CENTRAL_AUXILIARY)
        {
            if (rearWidth < 6u) { return false; }
            uint32_t centerWidth = std::min(rearWidth - 4u, getHousingWidth(rearWidth, EngineLayoutType::CENTRAL, sizeClass));
            centerWidth = std::max(2u, centerWidth);
            uint32_t auxiliaryWidth = std::max(1u, centerWidth / 2u);

            while (centerWidth + auxiliaryWidth * 2u + gap * 2u > rearWidth && auxiliaryWidth > 1u) { --auxiliaryWidth; }
            while (centerWidth + auxiliaryWidth * 2u + gap * 2u > rearWidth && centerWidth > 2u) { --centerWidth; }
            if (centerWidth + auxiliaryWidth * 2u + gap * 2u > rearWidth) { return false; }

            const uint32_t groupWidth = centerWidth + auxiliaryWidth * 2u + gap * 2u;
            const uint32_t leftAuxStart = rearStartX + (rearWidth - groupWidth) / 2u;
            const uint32_t rightAuxStart = imageWidth - leftAuxStart - auxiliaryWidth;
            const uint32_t centerStart = (imageWidth - centerWidth) / 2u;

            if (leftAuxStart + auxiliaryWidth > centerStart || centerStart + centerWidth > rightAuxStart) { return false; }

            placements.push_back({ leftAuxStart, auxiliaryWidth, getNozzleWidth(auxiliaryWidth, EngineSizeClass::SMALL), EngineSizeClass::SMALL, false });
            placements.push_back({ centerStart, centerWidth, getNozzleWidth(centerWidth, sizeClass), sizeClass, false });
            placements.push_back({ rightAuxStart, auxiliaryWidth, getNozzleWidth(auxiliaryWidth, EngineSizeClass::SMALL), EngineSizeClass::SMALL, false });
            return true;
        }

        if (layout == EngineLayoutType::WIDE_BANK)
        {
            uint32_t engineCount = rearWidth >= GenerationMath::scalePixelsFrom64(18u, imageWidth) ? 4u : 3u;

            while (engineCount > 2u && rearWidth < engineCount + (engineCount - 1u) * gap) { --engineCount; }
            if (engineCount < 3u) { return false; }

            const uint32_t maximumWidth = (rearWidth - (engineCount - 1u) * gap) / engineCount;
            const uint32_t targetWidth = getHousingWidth(rearWidth, EngineLayoutType::WIDE_BANK, sizeClass);
            const uint32_t housingWidth = std::max(1u, std::min(maximumWidth, targetWidth));
            const uint32_t groupWidth = housingWidth * engineCount + (engineCount - 1u) * gap;
            const uint32_t leftOuterStart = rearStartX + (rearWidth - groupWidth) / 2u;
            const uint32_t rightOuterStart = imageWidth - leftOuterStart - housingWidth;
            const EngineSizeClass unitSize = housingWidth >= 3u && sizeClass != EngineSizeClass::SMALL ? EngineSizeClass::MEDIUM : EngineSizeClass::SMALL;
            const uint32_t nozzleWidth = getNozzleWidth(housingWidth, unitSize);

            if (engineCount == 3u)
            {
                const uint32_t centerStart = (imageWidth - housingWidth) / 2u;
                if (leftOuterStart + housingWidth > centerStart || centerStart + housingWidth > rightOuterStart) { return false; }
                placements.push_back({ leftOuterStart, housingWidth, nozzleWidth, unitSize, false });
                placements.push_back({ centerStart, housingWidth, nozzleWidth, unitSize, false });
                placements.push_back({ rightOuterStart, housingWidth, nozzleWidth, unitSize, false });
                return true;
            }

            const uint32_t leftInnerStart = leftOuterStart + housingWidth + gap;
            const uint32_t rightInnerStart = imageWidth - leftInnerStart - housingWidth;
            if (leftInnerStart + housingWidth > rightInnerStart) { return false; }
            placements.push_back({ leftOuterStart, housingWidth, nozzleWidth, unitSize, false });
            placements.push_back({ leftInnerStart, housingWidth, nozzleWidth, unitSize, false });
            placements.push_back({ rightInnerStart, housingWidth, nozzleWidth, unitSize, false });
            placements.push_back({ rightOuterStart, housingWidth, nozzleWidth, unitSize, false });
            return true;
        }

        return false;
    }

    void EngineGenerator::collectRearSupportRuns(const PixelMask& hullMask, uint32_t y, std::vector<RearSupportRun>& runs) const
    {
        runs.clear();
        if (y >= hullMask.getHeight()) { return; }
        uint32_t x = 0u;
        while (x < hullMask.getWidth())
        {
            while (x < hullMask.getWidth() && !hullMask.get(x, y)) { ++x; }
            if (x >= hullMask.getWidth()) { break; }
            const uint32_t start = x;
            while (x < hullMask.getWidth() && hullMask.get(x, y)) { ++x; }
            runs.push_back({ start, x - start });
        }
    }

    bool EngineGenerator::createSeparatedRearPlacements(const std::vector<RearSupportRun>& runs, EngineLayoutType layout, EngineSizeClass sizeClass, uint32_t imageWidth, std::vector<EnginePlacement>& placements) const
    {
        placements.clear();
        if (runs.size() < 2u) { return false; }

        auto makePlacement = [this](const RearSupportRun& run, EngineSizeClass unitSize, EngineLayoutType unitLayout)
            {
                const uint32_t targetWidth = std::max(1u, getHousingWidth(run.Width, unitLayout, unitSize));
                const uint32_t housingWidth = std::min(run.Width, targetWidth);
                const uint32_t startX = run.StartX + (run.Width - housingWidth) / 2u;
                return EnginePlacement{ startX, housingWidth, getNozzleWidth(housingWidth, unitSize), unitSize, false };
            };

        auto makeMirroredPair = [this, imageWidth](const RearSupportRun& left, const RearSupportRun& right, EngineSizeClass unitSize, EngineLayoutType unitLayout, EnginePlacement& leftPlacement, EnginePlacement& rightPlacement)
            {
                const uint32_t availableWidth = std::min(left.Width, right.Width);
                if (availableWidth == 0u) { return false; }
                const uint32_t targetWidth = std::max(1u, getHousingWidth(availableWidth, unitLayout, unitSize));
                const uint32_t housingWidth = std::min(availableWidth, targetWidth);
                const uint32_t leftStartX = left.StartX + (left.Width - housingWidth) / 2u;
                const uint32_t rightStartX = imageWidth - leftStartX - housingWidth;
                if (rightStartX < right.StartX || rightStartX + housingWidth > right.StartX + right.Width) { return false; }
                const uint32_t nozzleWidth = getNozzleWidth(housingWidth, unitSize);
                leftPlacement = { leftStartX, housingWidth, nozzleWidth, unitSize, false };
                rightPlacement = { rightStartX, housingWidth, nozzleWidth, unitSize, false };
                return true;
            };

        if (layout == EngineLayoutType::TWIN || runs.size() == 2u)
        {
            EnginePlacement leftPlacement;
            EnginePlacement rightPlacement;
            if (!makeMirroredPair(runs.front(), runs.back(), sizeClass, EngineLayoutType::CENTRAL, leftPlacement, rightPlacement)) { return false; }
            placements.push_back(leftPlacement);
            placements.push_back(rightPlacement);
            return placements[0].StartX + placements[0].HousingWidth <= placements[1].StartX;
        }

        const RearSupportRun& left = runs.front();
        const RearSupportRun& center = runs[runs.size() / 2u];
        const RearSupportRun& right = runs.back();
        if (center.Width == 0u) { return false; }
        EnginePlacement leftPlacement;
        EnginePlacement rightPlacement;
        if (!makeMirroredPair(left, right, EngineSizeClass::SMALL, EngineLayoutType::CENTRAL, leftPlacement, rightPlacement)) { return false; }
        placements.push_back(leftPlacement);
        placements.push_back(makePlacement(center, sizeClass, EngineLayoutType::CENTRAL));
        placements.push_back(rightPlacement);
        return placements[0].StartX + placements[0].HousingWidth <= placements[1].StartX && placements[1].StartX + placements[1].HousingWidth <= placements[2].StartX && placements[2].StartX + placements[2].HousingWidth <= imageWidth;
    }

    uint32_t EngineGenerator::getHousingWidth(uint32_t rearWidth, EngineLayoutType layout, EngineSizeClass sizeClass) const
    {
        uint32_t percentage = 25u;

        switch (layout)
        {
        case EngineLayoutType::CENTRAL: percentage = sizeClass == EngineSizeClass::SMALL ? 34u : sizeClass == EngineSizeClass::MEDIUM ? 50u : 68u; break;
        case EngineLayoutType::TWIN: percentage = sizeClass == EngineSizeClass::SMALL ? 20u : sizeClass == EngineSizeClass::MEDIUM ? 28u : 35u; break;
        case EngineLayoutType::QUAD: percentage = sizeClass == EngineSizeClass::SMALL ? 10u : 14u; break;
        case EngineLayoutType::CENTRAL_AUXILIARY: percentage = sizeClass == EngineSizeClass::SMALL ? 30u : sizeClass == EngineSizeClass::MEDIUM ? 42u : 50u; break;
        case EngineLayoutType::WIDE_BANK: percentage = sizeClass == EngineSizeClass::SMALL ? 14u : 20u; break;
        default: break;
        }

        return std::min(rearWidth, getPercentOfWidth(rearWidth, percentage));
    }

    uint32_t EngineGenerator::getNozzleWidth(uint32_t housingWidth, EngineSizeClass sizeClass) const
    {
        if (housingWidth <= 2u || sizeClass == EngineSizeClass::SMALL)
        {
            return housingWidth;
        }

        const uint32_t maximumInset = (housingWidth - 1u) / 2u;
        const uint32_t desiredInset = sizeClass == EngineSizeClass::LARGE && housingWidth >= 5u ? 2u : 1u;
        const uint32_t inset = std::min(maximumInset, desiredInset);
        return std::max(1u, housingWidth - inset * 2u);
    }

    uint32_t EngineGenerator::getDesiredRootDepth(EngineSizeClass sizeClass, uint32_t imageHeight) const
    {
        const uint32_t referencePixels = sizeClass == EngineSizeClass::SMALL ? 1u : sizeClass == EngineSizeClass::MEDIUM ? 2u : 3u;
        return GenerationMath::scalePixelsFrom64(referencePixels, imageHeight);
    }

    uint32_t EngineGenerator::getDesiredExternalHeight(EngineSizeClass sizeClass, uint32_t imageHeight) const
    {
        const uint32_t referencePixels = sizeClass == EngineSizeClass::SMALL ? 2u : sizeClass == EngineSizeClass::MEDIUM ? 4u : 6u;
        return GenerationMath::scalePixelsFrom64(referencePixels, imageHeight);
    }

    uint32_t EngineGenerator::getDesiredMaximumExhaustLength(EngineSizeClass sizeClass, uint32_t imageHeight) const
    {
        const uint32_t referencePixels = sizeClass == EngineSizeClass::SMALL ? 4u : sizeClass == EngineSizeClass::MEDIUM ? 7u : 10u;
        return GenerationMath::scalePixelsFrom64(referencePixels, imageHeight);
    }

    bool EngineGenerator::addEngineAssembly(PixelMask& engineMask, const PixelMask& hullMask, const PixelMask& cockpitMask, const EnginePlacement& placement, uint32_t hullBottom, uint32_t rootDepth, uint32_t externalHeight) const
    {
        if (placement.HousingWidth == 0u || placement.NozzleWidth == 0u || placement.StartX + placement.HousingWidth > engineMask.getWidth())
        {
            return false;
        }

        for (uint32_t rootOffset = 0u; rootOffset < rootDepth; ++rootOffset)
        {
            const uint32_t y = hullBottom - rootOffset;

            for (uint32_t x = placement.StartX; x < placement.StartX + placement.HousingWidth; ++x)
            {
                if (!hullMask.get(x, y) || cockpitMask.get(x, y))
                {
                    return false;
                }
            }

            PixelMaskUtils::addMaskRectangle(engineMask, placement.StartX, y, placement.HousingWidth, 1u);
        }

        const uint32_t centerXTimesTwo = placement.StartX * 2u + placement.HousingWidth - 1u;
        const uint32_t totalInset = (placement.HousingWidth - placement.NozzleWidth) / 2u;

        for (uint32_t row = 0u; row < externalHeight; ++row)
        {
            const uint32_t y = hullBottom + 1u + row;
            uint32_t inset = 0u;

            if (totalInset > 0u)
            {
                const uint32_t holdRows = placement.Nacelle && externalHeight > 2u ? externalHeight / 2u : 0u;

                if (row < holdRows)
                {
                    inset = 0u;
                }
                else
                {
                    const uint32_t progressionRow = row - holdRows + 1u;
                    const uint32_t progressionLength = std::max(1u, externalHeight - holdRows);
                    inset = std::min(totalInset, static_cast<uint32_t>((static_cast<uint64_t>(progressionRow) * totalInset + progressionLength - 1u) / progressionLength));
                }
            }

            const uint32_t rowWidth = std::max(placement.NozzleWidth, placement.HousingWidth - inset * 2u);
            addCenteredMaskRow(engineMask, centerXTimesTwo, rowWidth, y);
        }

        return true;
    }

    bool EngineGenerator::addTaperedExhaust(PixelMask& exhaustMask, const EnginePlacement& placement, uint32_t exhaustStartY, uint32_t exhaustLength, uint32_t taperMode) const
    {
        if (placement.NozzleWidth == 0u || exhaustLength == 0u || exhaustStartY + exhaustLength > exhaustMask.getHeight())
        {
            return false;
        }

        const uint32_t nozzleStartX = placement.StartX + (placement.HousingWidth - placement.NozzleWidth) / 2u;
        const uint32_t centerXTimesTwo = nozzleStartX * 2u + placement.NozzleWidth - 1u;
        const uint32_t maximumInset = (placement.NozzleWidth - 1u) / 2u;

        for (uint32_t row = 0u; row < exhaustLength; ++row)
        {
            uint32_t effectiveRow = row;

            if (row > 0u && taperMode == 0u && effectiveRow + 1u < exhaustLength) { ++effectiveRow; }
            if (taperMode == 2u && effectiveRow > exhaustLength / 3u) { effectiveRow -= exhaustLength / 3u; }
            if (taperMode == 2u && row <= exhaustLength / 3u) { effectiveRow = 0u; }

            uint32_t inset = 0u;

            if (maximumInset > 0u && exhaustLength > 1u)
            {
                inset = std::min(maximumInset, static_cast<uint32_t>((static_cast<uint64_t>(effectiveRow) * maximumInset + exhaustLength - 2u) / (exhaustLength - 1u)));
            }

            const uint32_t rowWidth = std::max(1u, placement.NozzleWidth - inset * 2u);
            addCenteredMaskRow(exhaustMask, centerXTimesTwo, rowWidth, exhaustStartY + row);
        }

        return true;
    }

    bool EngineGenerator::validateGeneratedEngines(const PixelMask& engineMask, const PixelMask& exhaustMask, const PixelMask& hullMask, const PixelMask& cockpitMask) const
    {
        if (PixelMaskUtils::masksOverlap(engineMask, cockpitMask) || PixelMaskUtils::masksOverlap(exhaustMask, hullMask) || PixelMaskUtils::masksOverlap(exhaustMask, cockpitMask) || PixelMaskUtils::masksOverlap(engineMask, exhaustMask))
        {
            return false;
        }

        for (uint32_t y = 0u; y < exhaustMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < exhaustMask.getWidth(); ++x)
            {
                if (!exhaustMask.get(x, y))
                {
                    continue;
                }

                const bool connectedAbove = y > 0u && (exhaustMask.get(x, y - 1u) || engineMask.get(x, y - 1u));
                const bool diagonallyConnectedAbove = y > 0u && ((x > 0u && (exhaustMask.get(x - 1u, y - 1u) || engineMask.get(x - 1u, y - 1u))) || (x + 1u < exhaustMask.getWidth() && (exhaustMask.get(x + 1u, y - 1u) || engineMask.get(x + 1u, y - 1u))));

                if (!connectedAbove && !diagonallyConnectedAbove)
                {
                    return false;
                }
            }
        }

        return PixelMaskUtils::getMaskPixelCount(engineMask) > 0u && PixelMaskUtils::getMaskPixelCount(exhaustMask) > 0u;
    }

    void EngineGenerator::addCenteredMaskRow(PixelMask& mask, uint32_t centerXTimesTwo, uint32_t width, uint32_t y) const
    {
        if (width == 0u || y >= mask.getHeight())
        {
            return;
        }

        const int32_t startX = (static_cast<int32_t>(centerXTimesTwo) - static_cast<int32_t>(width - 1u)) / 2;

        if (startX < 0 || static_cast<uint32_t>(startX) + width > mask.getWidth())
        {
            return;
        }

        PixelMaskUtils::addMaskRectangle(mask, static_cast<uint32_t>(startX), y, width, 1u);
    }
}
