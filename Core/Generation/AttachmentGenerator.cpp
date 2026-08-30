#include "AttachmentGenerator.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "GenerationMath.h"
#include "MacroAsymmetryPlanner.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        uint32_t getAttachmentComplexityCost(ShipAttachmentType type)
        {
            switch (type)
            {
            case ShipAttachmentType::WEAPON_MOUNT: return 8u;
            case ShipAttachmentType::SENSOR_ARRAY: return 6u;
            case ShipAttachmentType::AUXILIARY_POD: return 10u;
            case ShipAttachmentType::RADIATOR: return 8u;
            case ShipAttachmentType::ARMOR_FIN: return 7u;
            case ShipAttachmentType::TECHNOLOGY_NODE: return 7u;
            default: return 0u;
            }
        }


        GenerationSpatialRegion getAttachmentSpatialRegion(ShipAttachmentRegion region, uint32_t x, uint32_t width)
        {
            const bool left = x < width / 2u;
            switch (region)
            {
            case ShipAttachmentRegion::FRONT: return GenerationSpatialRegion::NOSE;
            case ShipAttachmentRegion::FRONT_SIDE: return GenerationSpatialRegion::FRONT_FUSELAGE;
            case ShipAttachmentRegion::MIDDLE_SIDE: return GenerationSpatialRegion::MID_FUSELAGE;
            case ShipAttachmentRegion::WING_OUTER_SIDE: return left ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING;
            case ShipAttachmentRegion::REAR_SIDE:
            case ShipAttachmentRegion::REAR: return GenerationSpatialRegion::REAR_FUSELAGE;
            default: return GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
            }
        }

        bool isDominantAttachment(ShipAttachmentType type)
        {
            return type == ShipAttachmentType::AUXILIARY_POD;
        }
        uint32_t getPairedAttachmentComplexityCost(ShipAttachmentType type)
        {
            const uint32_t singleCost = getAttachmentComplexityCost(type);
            return singleCost + (singleCost + 1u) / 2u;
        }
    }
    void AttachmentGenerator::generate(ShipGenerationContext& context) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationConfiguration& settings = context.Settings;
        const ShipGenerationProfile& styleProfile = context.Profile;
        ship.AttachmentMask.clear(false);
        ship.AttachmentPlacements.clear();

        const ResolvedAttachmentProfile profile = resolveAttachmentProfile(styleProfile, context.FactionProfile.Attachments);
        const bool macroRequested = context.MacroAsymmetry.targets(MacroAsymmetryCategory::ATTACHMENT);

        if (profile.MaximumAttachmentGroups == 0u || !context.ComplexityBudget.canAfford(GenerationComplexityCategory::ATTACHMENT, 6u))
        {
            if (macroRequested) { MacroAsymmetryPlanner::reject(context); }
            return;
        }

        const uint32_t attachmentChance = static_cast<uint32_t>((static_cast<uint64_t>(profile.AttachmentChance) * (55u + context.ScaleTraits.AttachmentComplexity * 45u / 100u) + 50u) / 100u);

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedAttachmentPresence.has_value())
        {
            const bool normallyGenerated = attachmentChance > 0u && context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 99u) < attachmentChance;
            (void)normallyGenerated;
            if (!*context.CalibrationSettings->Overrides.ForcedAttachmentPresence) { return; }
        }
        else if (!macroRequested && (attachmentChance == 0u || context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 99u) >= attachmentChance))
        {
            return;
        }

        const std::vector<AttachmentAnchor> anchors = discoverAttachmentAnchors(context);

        if (anchors.empty())
        {
            if (macroRequested) { MacroAsymmetryPlanner::reject(context); }
            return;
        }

        const uint32_t scaleMaximumAttachmentGroups = 1u + context.ScaleTraits.AttachmentComplexity / 45u;
        const uint32_t maximumAttachmentGroups = std::min(profile.MaximumAttachmentGroups, scaleMaximumAttachmentGroups);
        const uint32_t groupCount = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, maximumAttachmentGroups);
        uint32_t nextSymmetryGroup = 1u;
        bool forcedTypePending = context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedAttachmentType.has_value();

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->AttachmentRequestedGroupCount = groupCount;
        }

        for (uint32_t group = 0u; group < groupCount; ++group)
        {
            for (uint32_t attempt = 0u; attempt < MaximumAttachmentPlacementAttempts; ++attempt)
            {
                if (context.DebugInfo != nullptr)
                {
                    ++context.DebugInfo->AttachmentPlacementAttemptCount;
                }

                if (attempt == MaximumAttachmentPlacementAttempts / 2u && context.MacroAsymmetry.targets(MacroAsymmetryCategory::ATTACHMENT))
                {
                    MacroAsymmetryPlanner::reject(context);
                }

                const bool plannedAsymmetry = context.MacroAsymmetry.targets(MacroAsymmetryCategory::ATTACHMENT);
                const uint32_t startIndex = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, static_cast<uint32_t>(anchors.size() - 1u));
                const AttachmentAnchor* anchorPtr = nullptr;
                for (uint32_t offset = 0u; offset < anchors.size(); ++offset)
                {
                    const AttachmentAnchor& possible = anchors[(startIndex + offset) % anchors.size()];
                    if (!plannedAsymmetry || anchorMatchesMacroAsymmetryPlan(context, possible))
                    {
                        anchorPtr = &possible;
                        break;
                    }
                }
                if (anchorPtr == nullptr) { continue; }
                const AttachmentAnchor& anchor = *anchorPtr;
                const ShipAttachmentType type = plannedAsymmetry ? getMacroAsymmetryAttachmentType(profile, anchor.Region) : getAttachmentType(context, profile, anchor.Region, forcedTypePending);

                if (type == ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END)
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->AttachmentPlacementFailureCount;
                    }

                    continue;
                }

                PixelMask candidateMask(ship.AttachmentMask.getWidth(), ship.AttachmentMask.getHeight(), false);
                ShipAttachmentPlacement placement;

                if (!generateAttachmentCandidate(context, ship, anchor, type, profile.AttachmentSizePercent, candidateMask, placement))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->AttachmentPlacementFailureCount;
                    }

                    continue;
                }

                if (!validateAttachmentCandidate(context, candidateMask, anchor))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->AttachmentPlacementFailureCount;
                    }

                    continue;
                }

                AttachmentAnchor mirroredAnchor = anchor;
                mirroredAnchor.X = ship.AttachmentMask.getWidth() - 1u - anchor.X;
                mirroredAnchor.Direction = getMirroredAttachmentDirection(anchor.Direction);

                const bool mirroredAnchorIsDifferent = mirroredAnchor.X != anchor.X || mirroredAnchor.Direction != anchor.Direction;
                const bool wantsSymmetricPair = !plannedAsymmetry && mirroredAnchorIsDifferent && context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 99u) < getAttachmentSymmetryChance(profile, type);
                bool symmetricPairGenerated = false;

                if (wantsSymmetricPair)
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->AttachmentSymmetricPairAttemptCount;
                    }

                    if (isAttachmentAnchorValid(context, mirroredAnchor))
                    {
                        PixelMask mirroredCandidateMask(ship.AttachmentMask.getWidth(), ship.AttachmentMask.getHeight(), false);
                        mirrorAttachmentCandidate(candidateMask, mirroredCandidateMask);

                        if (validateAttachmentCandidate(context, mirroredCandidateMask, mirroredAnchor) && !PixelMaskUtils::masksOverlap(candidateMask, mirroredCandidateMask))
                        {
                            auto spatialRegions = context.SpatialBudget.makeRegionSet(getAttachmentSpatialRegion(anchor.Region, anchor.X, ship.AttachmentMask.getWidth()));
                            context.SpatialBudget.addRegion(spatialRegions, context.SpatialBudget.getMirroredRegion(getAttachmentSpatialRegion(anchor.Region, anchor.X, ship.AttachmentMask.getWidth())));
                            const uint32_t localCost = getAttachmentComplexityCost(type);
                            const bool dominant = isDominantAttachment(type);
                            const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, localCost, dominant);
                            if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 99u) >= spatialAcceptance)
                            {
                                context.SpatialBudget.recordRejection(spatialRegions);
                                if (context.DebugInfo != nullptr) { ++context.DebugInfo->AttachmentPlacementFailureCount; }
                                continue;
                            }

                            if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::ATTACHMENT, getPairedAttachmentComplexityCost(type)))
                            {
                                break;
                            }

                            context.SpatialBudget.consume(spatialRegions, localCost, dominant);
                            placement.SymmetryGroup = nextSymmetryGroup;

                            ShipAttachmentPlacement mirroredPlacement = mirrorAttachmentPlacement(placement, ship.AttachmentMask.getWidth());
                            mirroredPlacement.SymmetryGroup = nextSymmetryGroup;

                            PixelMaskUtils::mergeMask(ship.AttachmentMask, candidateMask);
                            PixelMaskUtils::mergeMask(ship.AttachmentMask, mirroredCandidateMask);

                            ship.AttachmentPlacements.push_back(placement);
                            ship.AttachmentPlacements.push_back(mirroredPlacement);

                            if (context.DebugInfo != nullptr)
                            {
                                ++context.DebugInfo->AttachmentPlacedGroupCount;
                            }

                            ++nextSymmetryGroup;
                            forcedTypePending = false;
                            symmetricPairGenerated = true;
                        }
                    }

                    if (!symmetricPairGenerated && context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->AttachmentSymmetricPairFailureCount;
                    }
                }

                if (symmetricPairGenerated)
                {
                    break;
                }

                const uint32_t localCost = getAttachmentComplexityCost(type);
                if (plannedAsymmetry && !MacroAsymmetryPlanner::canAcceptCandidate(context, candidateMask, localCost))
                {
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->AttachmentPlacementFailureCount; }
                    continue;
                }
                const auto spatialRegions = context.SpatialBudget.makeRegionSet(getAttachmentSpatialRegion(anchor.Region, anchor.X, ship.AttachmentMask.getWidth()));
                const bool dominant = plannedAsymmetry || isDominantAttachment(type);
                const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, localCost, dominant);
                if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 99u) >= spatialAcceptance)
                {
                    context.SpatialBudget.recordRejection(spatialRegions);
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->AttachmentPlacementFailureCount; }
                    continue;
                }

                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::ATTACHMENT, localCost))
                {
                    break;
                }

                if (plannedAsymmetry) { MacroAsymmetryPlanner::fulfill(context, candidateMask, localCost); }
                context.SpatialBudget.consume(spatialRegions, localCost, dominant);
                PixelMaskUtils::mergeMask(ship.AttachmentMask, candidateMask);
                ship.AttachmentPlacements.push_back(placement);
                forcedTypePending = false;

                if (context.DebugInfo != nullptr)
                {
                    ++context.DebugInfo->AttachmentPlacedGroupCount;
                }

                break;
            }
        }

        if (context.MacroAsymmetry.targets(MacroAsymmetryCategory::ATTACHMENT)) { MacroAsymmetryPlanner::reject(context); }
    }

    std::vector<AttachmentGenerator::AttachmentAnchor> AttachmentGenerator::discoverAttachmentAnchors(const ShipGenerationContext& context) const
    {
        const GeneratedShip& ship = context.Ship;
        std::vector<AttachmentAnchor> anchors;
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(ship.HullMask);

        if (!bounds.Valid)
        {
            return anchors;
        }

        const uint32_t imageWidth = ship.HullMask.getWidth();
        const uint32_t hullHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t imageCenter = imageWidth / 2u;
        uint32_t maximumRowWidth = 0u;

        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            maximumRowWidth = std::max(maximumRowWidth, PixelMaskUtils::getOccupiedRowWidth(ship.HullMask, y));
        }

        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            const uint32_t rowWidth = PixelMaskUtils::getOccupiedRowWidth(ship.HullMask, y);

            for (uint32_t x = 0u; x < imageWidth; ++x)
            {
                if (!ship.HullMask.get(x, y))
                {
                    continue;
                }

                if (ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y))
                {
                    continue;
                }

                const uint32_t verticalPercent = hullHeight > 1u ? ((y - bounds.MinY) * 100u) / (hullHeight - 1u) : 0u;

                if (verticalPercent <= 12u && !PixelMaskUtils::isMaskPixel(ship.HullMask, static_cast<int32_t>(x), static_cast<int32_t>(y) - 1))
                {
                    AttachmentAnchor anchor = { x, y, ShipAttachmentRegion::FRONT, ShipAttachmentDirection::UP };

                    if (isAttachmentAnchorValid(context, anchor))
                    {
                        anchors.push_back(anchor);
                    }
                }

                if (verticalPercent >= 90u && !PixelMaskUtils::isMaskPixel(ship.HullMask, static_cast<int32_t>(x), static_cast<int32_t>(y) + 1))
                {
                    AttachmentAnchor anchor = { x, y, ShipAttachmentRegion::REAR, ShipAttachmentDirection::DOWN };

                    if (isAttachmentAnchorValid(context, anchor))
                    {
                        anchors.push_back(anchor);
                    }
                }

                if (x < imageCenter && !PixelMaskUtils::isMaskPixel(ship.HullMask, static_cast<int32_t>(x) - 1, static_cast<int32_t>(y)))
                {
                    AttachmentAnchor anchor = { x, y, getSideAttachmentRegion(context, x, y, bounds.MinY, hullHeight, rowWidth, maximumRowWidth), ShipAttachmentDirection::LEFT };

                    if (isAttachmentAnchorValid(context, anchor))
                    {
                        anchors.push_back(anchor);
                    }
                }

                if (x >= imageCenter && !PixelMaskUtils::isMaskPixel(ship.HullMask, static_cast<int32_t>(x) + 1, static_cast<int32_t>(y)))
                {
                    AttachmentAnchor anchor = { x, y, getSideAttachmentRegion(context, x, y, bounds.MinY, hullHeight, rowWidth, maximumRowWidth), ShipAttachmentDirection::RIGHT };

                    if (isAttachmentAnchorValid(context, anchor))
                    {
                        anchors.push_back(anchor);
                    }
                }
            }
        }

        return anchors;
    }

    ShipAttachmentRegion AttachmentGenerator::getSideAttachmentRegion(const ShipGenerationContext& context, uint32_t x, uint32_t y, uint32_t hullTop, uint32_t hullHeight, uint32_t rowWidth, uint32_t maximumRowWidth) const
    {
        const uint32_t relativeY = y - hullTop;
        const uint32_t verticalPercent = hullHeight > 1u ? (relativeY * 100u) / (hullHeight - 1u) : 0u;

        if (context.WingRegions.hasWings() && context.WingRegions.OuterWingMask.get(x, y))
        {
            return ShipAttachmentRegion::WING_OUTER_SIDE;
        }

        const bool legacyOuterWingRegion = !context.WingRegions.hasWings() && maximumRowWidth > 0u && rowWidth * 100u >= maximumRowWidth * 80u;

        if (verticalPercent < 30u)
        {
            return ShipAttachmentRegion::FRONT_SIDE;
        }

        if (verticalPercent >= 45u && verticalPercent <= 82u && legacyOuterWingRegion)
        {
            return ShipAttachmentRegion::WING_OUTER_SIDE;
        }

        if (verticalPercent < 66u)
        {
            return ShipAttachmentRegion::MIDDLE_SIDE;
        }

        return ShipAttachmentRegion::REAR_SIDE;
    }

    bool AttachmentGenerator::isAttachmentAnchorValid(const ShipGenerationContext& context, const AttachmentAnchor& anchor) const
    {
        const GeneratedShip& ship = context.Ship;
        if (!ship.HullMask.isInBounds(anchor.X, anchor.Y))
        {
            return false;
        }

        if (!ship.HullMask.get(anchor.X, anchor.Y))
        {
            return false;
        }

        if (ship.CockpitMask.get(anchor.X, anchor.Y))
        {
            return false;
        }

        if (ship.EngineMask.get(anchor.X, anchor.Y))
        {
            return false;
        }

        if (context.MajorFeatures.OccupiedMask.get(anchor.X, anchor.Y))
        {
            return false;
        }

        if (context.Weapons.OccupiedMask.get(anchor.X, anchor.Y))
        {
            return false;
        }

        const auto [offsetX, offsetY] = getAttachmentDirectionOffset(anchor.Direction);
        const int32_t attachmentX = static_cast<int32_t>(anchor.X) + offsetX;
        const int32_t attachmentY = static_cast<int32_t>(anchor.Y) + offsetY;

        if (attachmentX < 0 || attachmentY < 0)
        {
            return false;
        }

        if (attachmentX >= static_cast<int32_t>(ship.HullMask.getWidth()) || attachmentY >= static_cast<int32_t>(ship.HullMask.getHeight()))
        {
            return false;
        }

        if (ship.HullMask.get(static_cast<uint32_t>(attachmentX), static_cast<uint32_t>(attachmentY)))
        {
            return false;
        }

        if (ship.EngineMask.get(static_cast<uint32_t>(attachmentX), static_cast<uint32_t>(attachmentY)))
        {
            return false;
        }

        if (ship.EngineExhaustMask.get(static_cast<uint32_t>(attachmentX), static_cast<uint32_t>(attachmentY)))
        {
            return false;
        }

        if (context.Weapons.OccupiedMask.get(static_cast<uint32_t>(attachmentX), static_cast<uint32_t>(attachmentY)))
        {
            return false;
        }

        return true;
    }

    bool AttachmentGenerator::anchorMatchesMacroAsymmetryPlan(const ShipGenerationContext& context, const AttachmentAnchor& anchor) const
    {
        if (!context.MacroAsymmetry.targets(MacroAsymmetryCategory::ATTACHMENT)) { return true; }
        if (!context.MacroAsymmetry.isDominantX(anchor.X, context.Ship.HullMask.getWidth())) { return false; }
        const GenerationSpatialRegion region = getAttachmentSpatialRegion(anchor.Region, anchor.X, context.Ship.HullMask.getWidth());
        const GenerationSpatialRegion target = context.MacroAsymmetry.TargetRegion;
        if (target == GenerationSpatialRegion::LEFT_OUTER_WING || target == GenerationSpatialRegion::RIGHT_OUTER_WING) { return region == target; }
        if (target == GenerationSpatialRegion::LEFT_WING_ROOT || target == GenerationSpatialRegion::RIGHT_WING_ROOT)
        {
            return region == target || region == (context.MacroAsymmetry.isLeftSide() ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING);
        }
        return region == target || (target == GenerationSpatialRegion::MID_FUSELAGE && region == GenerationSpatialRegion::FRONT_FUSELAGE);
    }

    ShipAttachmentType AttachmentGenerator::getMacroAsymmetryAttachmentType(const ResolvedAttachmentProfile& profile, ShipAttachmentRegion region) const
    {
        constexpr std::array<ShipAttachmentType, 4u> Preferred = { ShipAttachmentType::AUXILIARY_POD, ShipAttachmentType::SENSOR_ARRAY, ShipAttachmentType::TECHNOLOGY_NODE, ShipAttachmentType::WEAPON_MOUNT };
        ShipAttachmentType best = ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END;
        uint64_t bestWeight = 0u;
        for (const ShipAttachmentType type : Preferred)
        {
            if (!isAttachmentTypeAllowedInRegion(type, region)) { continue; }
            const uint64_t weight = profile.TypeWeights[static_cast<std::size_t>(type)];
            if (weight > bestWeight) { bestWeight = weight; best = type; }
        }
        return best;
    }

    ShipAttachmentType AttachmentGenerator::getAttachmentType(ShipGenerationContext& context, const ResolvedAttachmentProfile& profile, ShipAttachmentRegion region, bool allowForcedType) const
    {
        uint64_t totalWeight = 0u;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            const ShipAttachmentType type = static_cast<ShipAttachmentType>(index);

            if (!isAttachmentTypeAllowedInRegion(type, region))
            {
                continue;
            }

            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::ATTACHMENT, getAttachmentComplexityCost(type)))
            {
                continue;
            }

            totalWeight += profile.TypeWeights[index];
        }

        if (totalWeight == 0u)
        {
            return ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END;
        }

        uint64_t roll = context.getGenerationRandomUInt64(GenerationDomain::ATTACHMENTS, 0u, totalWeight - 1u);

        if (allowForcedType && context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedAttachmentType.has_value())
        {
            const ShipAttachmentType forced = *context.CalibrationSettings->Overrides.ForcedAttachmentType;
            const std::size_t forcedIndex = static_cast<std::size_t>(forced);
            if (forced != ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END && forcedIndex < profile.TypeWeights.size() && profile.TypeWeights[forcedIndex] > 0u && isAttachmentTypeAllowedInRegion(forced, region))
            {
                return forced;
            }
        }

        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            const ShipAttachmentType type = static_cast<ShipAttachmentType>(index);

            if (!isAttachmentTypeAllowedInRegion(type, region))
            {
                continue;
            }

            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::ATTACHMENT, getAttachmentComplexityCost(type)))
            {
                continue;
            }

            const uint64_t weight = profile.TypeWeights[index];

            if (roll < weight)
            {
                return type;
            }

            roll -= weight;
        }

        return ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END;
    }

    bool AttachmentGenerator::generateAttachmentCandidate(ShipGenerationContext& context, const GeneratedShip& ship, const AttachmentAnchor& anchor, ShipAttachmentType type, uint32_t sizePercent, PixelMask& candidateMask, ShipAttachmentPlacement& placement) const
    {
        candidateMask.clear(false);

        bool generated = false;

        switch (type)
        {
        case ShipAttachmentType::WEAPON_MOUNT: generated = generateWeaponMountAttachment(context, anchor, sizePercent, candidateMask); break;
        case ShipAttachmentType::SENSOR_ARRAY: generated = generateSensorArrayAttachment(context, anchor, sizePercent, candidateMask); break;
        case ShipAttachmentType::AUXILIARY_POD: generated = generateAuxiliaryPodAttachment(context, anchor, sizePercent, candidateMask); break;
        case ShipAttachmentType::RADIATOR: generated = generateRadiatorAttachment(context, anchor, sizePercent, candidateMask); break;
        case ShipAttachmentType::ARMOR_FIN: generated = generateArmorFinAttachment(context, anchor, sizePercent, candidateMask); break;
        case ShipAttachmentType::TECHNOLOGY_NODE: generated = generateTechnologyNodeAttachment(context, anchor, sizePercent, candidateMask); break;
        default: return false;
        }

        if (!generated)
        {
            return false;
        }

        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(candidateMask);
        if (!bounds.Valid)
        {
            return false;
        }

        placement.Type = type;
        placement.Region = anchor.Region;
        placement.Direction = anchor.Direction;
        placement.AnchorX = anchor.X;
        placement.AnchorY = anchor.Y;
        placement.MinimumX = bounds.MinX;
        placement.MaximumX = bounds.MaxX;
        placement.MinimumY = bounds.MinY;
        placement.MaximumY = bounds.MaxY;
        placement.SymmetryGroup = 0u;

        return true;
    }

    bool AttachmentGenerator::validateAttachmentCandidate(const ShipGenerationContext& context, const PixelMask& candidateMask, const AttachmentAnchor& anchor) const
    {
        const GeneratedShip& ship = context.Ship;
        if (!isAttachmentAnchorValid(context, anchor))
        {
            return false;
        }

        const auto [offsetX, offsetY] = getAttachmentDirectionOffset(anchor.Direction);
        const uint32_t connectedX = static_cast<uint32_t>(static_cast<int32_t>(anchor.X) + offsetX);
        const uint32_t connectedY = static_cast<uint32_t>(static_cast<int32_t>(anchor.Y) + offsetY);

        if (!candidateMask.get(connectedX, connectedY))
        {
            return false;
        }

        bool containsPixel = false;

        for (uint32_t y = 0u; y < candidateMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < candidateMask.getWidth(); ++x)
            {
                if (!candidateMask.get(x, y))
                {
                    continue;
                }

                containsPixel = true;

                if (ship.HullMask.get(x, y))
                {
                    return false;
                }

                if (context.StructuralNegativeSpace.ReservedMask.get(x, y))
                {
                    return false;
                }

                if (ship.CockpitMask.get(x, y))
                {
                    return false;
                }

                if (ship.EngineMask.get(x, y))
                {
                    return false;
                }

                if (ship.EngineExhaustMask.get(x, y))
                {
                    return false;
                }

                if (ship.AttachmentMask.get(x, y))
                {
                    return false;
                }

                if (context.Weapons.OccupiedMask.get(x, y))
                {
                    return false;
                }

                if (PixelMaskUtils::hasNeighbouringMaskPixel(context.Weapons.OccupiedMask, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    return false;
                }

                if (PixelMaskUtils::hasNeighbouringMaskPixel(ship.AttachmentMask, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    return false;
                }
            }
        }

        return containsPixel;
    }

    void AttachmentGenerator::mirrorAttachmentCandidate(const PixelMask& sourceMask, PixelMask& destinationMask) const
    {
        destinationMask.clear(false);

        for (uint32_t y = 0u; y < sourceMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < sourceMask.getWidth(); ++x)
            {
                if (!sourceMask.get(x, y))
                {
                    continue;
                }

                destinationMask.set(sourceMask.getWidth() - 1u - x, y, true);
            }
        }
    }

    ShipAttachmentPlacement AttachmentGenerator::mirrorAttachmentPlacement(const ShipAttachmentPlacement& placement, uint32_t imageWidth) const
    {
        ShipAttachmentPlacement mirrored = placement;

        mirrored.AnchorX = imageWidth - 1u - placement.AnchorX;
        mirrored.Direction = getMirroredAttachmentDirection(placement.Direction);
        mirrored.MinimumX = imageWidth - 1u - placement.MaximumX;
        mirrored.MaximumX = imageWidth - 1u - placement.MinimumX;

        return mirrored;
    }

    std::pair<int32_t, int32_t> AttachmentGenerator::getAttachmentDirectionOffset(ShipAttachmentDirection direction) const
    {
        switch (direction)
        {
        case ShipAttachmentDirection::UP: return { 0, -1 };
        case ShipAttachmentDirection::DOWN: return { 0, 1 };
        case ShipAttachmentDirection::LEFT: return { -1, 0 };
        case ShipAttachmentDirection::RIGHT: return { 1, 0 };
        default: return { 0, 0 };
        }
    }

    ShipAttachmentDirection AttachmentGenerator::getMirroredAttachmentDirection(ShipAttachmentDirection direction) const
    {
        if (direction == ShipAttachmentDirection::LEFT)
        {
            return ShipAttachmentDirection::RIGHT;
        }

        if (direction == ShipAttachmentDirection::RIGHT)
        {
            return ShipAttachmentDirection::LEFT;
        }

        return direction;
    }

    bool AttachmentGenerator::isAttachmentTypeAllowedInRegion(ShipAttachmentType type, ShipAttachmentRegion region) const
    {
        switch (type)
        {
        case ShipAttachmentType::WEAPON_MOUNT: return region == ShipAttachmentRegion::FRONT || region == ShipAttachmentRegion::FRONT_SIDE || region == ShipAttachmentRegion::WING_OUTER_SIDE;
        case ShipAttachmentType::SENSOR_ARRAY: return region == ShipAttachmentRegion::FRONT || region == ShipAttachmentRegion::FRONT_SIDE || region == ShipAttachmentRegion::MIDDLE_SIDE || region == ShipAttachmentRegion::REAR_SIDE;
        case ShipAttachmentType::AUXILIARY_POD: return region == ShipAttachmentRegion::MIDDLE_SIDE || region == ShipAttachmentRegion::WING_OUTER_SIDE || region == ShipAttachmentRegion::REAR_SIDE;
        case ShipAttachmentType::RADIATOR: return region == ShipAttachmentRegion::MIDDLE_SIDE || region == ShipAttachmentRegion::WING_OUTER_SIDE || region == ShipAttachmentRegion::REAR_SIDE;
        case ShipAttachmentType::ARMOR_FIN: return region == ShipAttachmentRegion::FRONT_SIDE || region == ShipAttachmentRegion::MIDDLE_SIDE || region == ShipAttachmentRegion::WING_OUTER_SIDE || region == ShipAttachmentRegion::REAR_SIDE;
        case ShipAttachmentType::TECHNOLOGY_NODE: return true;
        default: return false;
        }
    }

    uint32_t AttachmentGenerator::getAttachmentSymmetryChance(const ResolvedAttachmentProfile& profile, ShipAttachmentType type) const
    {
        int32_t typeOffset = 0;

        switch (type)
        {
        case ShipAttachmentType::WEAPON_MOUNT: typeOffset = 10; break;
        case ShipAttachmentType::SENSOR_ARRAY: typeOffset = -30; break;
        case ShipAttachmentType::AUXILIARY_POD: typeOffset = 10; break;
        case ShipAttachmentType::RADIATOR: typeOffset = 15; break;
        case ShipAttachmentType::ARMOR_FIN: typeOffset = 15; break;
        case ShipAttachmentType::TECHNOLOGY_NODE: typeOffset = -15; break;
        default: break;
        }

        return static_cast<uint32_t>(std::clamp(static_cast<int32_t>(profile.SymmetricAttachmentChance) + typeOffset, 0, 100));
    }

    bool AttachmentGenerator::generateWeaponMountAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const
    {
        const bool horizontalDirection = anchor.Direction == ShipAttachmentDirection::LEFT || anchor.Direction == ShipAttachmentDirection::RIGHT;
        const uint32_t outwardDimension = horizontalDirection ? candidateMask.getWidth() : candidateMask.getHeight();
        const uint32_t tangentDimension = horizontalDirection ? candidateMask.getHeight() : candidateMask.getWidth();
        const uint32_t minimumBodyLength = scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent);
        const uint32_t maximumBodyLength = std::max(minimumBodyLength, scaleAttachmentPixelsFrom64(4u, outwardDimension, sizePercent));
        const uint32_t bodyLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumBodyLength, maximumBodyLength);
        const uint32_t minimumBodyThickness = scaleAttachmentPixelsFrom64(2u, tangentDimension, sizePercent);
        const uint32_t maximumBodyThickness = std::max(minimumBodyThickness, scaleAttachmentPixelsFrom64(4u, tangentDimension, sizePercent));
        const uint32_t bodyThickness = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumBodyThickness, maximumBodyThickness);
        const uint32_t minimumBarrelLength = scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent);
        const uint32_t maximumBarrelLength = std::max(minimumBarrelLength, scaleAttachmentPixelsFrom64(5u, outwardDimension, sizePercent));
        const uint32_t barrelLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumBarrelLength, maximumBarrelLength);
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 2u);
        const int32_t bodyStart = -static_cast<int32_t>(bodyThickness / 2u);

        if (!addAttachmentLocalRectangle(candidateMask, anchor, 1u, bodyLength, bodyStart, bodyThickness))
        {
            return false;
        }

        if (variant == 0u)
        {
            const uint32_t barrelThickness = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, std::max(1u, GenerationMath::scalePixelsFrom64(2u, tangentDimension)));
            const int32_t barrelStart = -static_cast<int32_t>(barrelThickness / 2u);

            return addAttachmentLocalRectangle(candidateMask, anchor, bodyLength + 1u, barrelLength, barrelStart, barrelThickness);
        }

        if (variant == 1u)
        {
            const uint32_t tipThickness = std::max(1u, bodyThickness > 2u ? bodyThickness - 2u : 1u);
            const int32_t tipStart = -static_cast<int32_t>(tipThickness / 2u);

            return addAttachmentLocalRectangle(candidateMask, anchor, bodyLength + 1u, std::max(1u, barrelLength / 2u), tipStart, tipThickness);
        }

        if (bodyThickness < 3u)
        {
            return addAttachmentLocalRectangle(candidateMask, anchor, bodyLength + 1u, barrelLength, 0, 1u);
        }

        if (!addAttachmentLocalRectangle(candidateMask, anchor, bodyLength + 1u, barrelLength, -1, 1u))
        {
            return false;
        }

        return addAttachmentLocalRectangle(candidateMask, anchor, bodyLength + 1u, barrelLength, 1, 1u);
    }

    bool AttachmentGenerator::generateSensorArrayAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const
    {
        const bool horizontalDirection = anchor.Direction == ShipAttachmentDirection::LEFT || anchor.Direction == ShipAttachmentDirection::RIGHT;
        const uint32_t outwardDimension = horizontalDirection ? candidateMask.getWidth() : candidateMask.getHeight();
        const uint32_t minimumStemLength = scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent);
        const uint32_t maximumStemLength = std::max(minimumStemLength, scaleAttachmentPixelsFrom64(4u, outwardDimension, sizePercent));
        const uint32_t stemLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumStemLength, maximumStemLength);
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 2u);

        if (!addAttachmentLocalRectangle(candidateMask, anchor, 1u, stemLength, 0, 1u))
        {
            return false;
        }

        if (variant == 0u)
        {
            return addAttachmentLocalRectangle(candidateMask, anchor, stemLength + 1u, 1u, -1, 3u);
        }

        if (variant == 1u)
        {
            if (!addAttachmentLocalRectangle(candidateMask, anchor, stemLength + 1u, 2u, -1, 3u))
            {
                return false;
            }

            return true;
        }

        if (!addAttachmentLocalRectangle(candidateMask, anchor, stemLength + 1u, 2u, -1, 1u))
        {
            return false;
        }

        return addAttachmentLocalRectangle(candidateMask, anchor, stemLength + 1u, 2u, 1, 1u);
    }

    bool AttachmentGenerator::generateAuxiliaryPodAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const
    {
        const bool horizontalDirection = anchor.Direction == ShipAttachmentDirection::LEFT || anchor.Direction == ShipAttachmentDirection::RIGHT;
        const uint32_t outwardDimension = horizontalDirection ? candidateMask.getWidth() : candidateMask.getHeight();
        const uint32_t tangentDimension = horizontalDirection ? candidateMask.getHeight() : candidateMask.getWidth();
        const uint32_t maximumConnectorLength = std::max(1u, scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent));
        const uint32_t connectorLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, maximumConnectorLength);
        const uint32_t minimumPodLength = scaleAttachmentPixelsFrom64(3u, outwardDimension, sizePercent);
        const uint32_t maximumPodLength = std::max(minimumPodLength, scaleAttachmentPixelsFrom64(5u, outwardDimension, sizePercent));
        const uint32_t podLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumPodLength, maximumPodLength);
        const uint32_t minimumPodThickness = scaleAttachmentPixelsFrom64(3u, tangentDimension, sizePercent);
        const uint32_t maximumPodThickness = std::max(minimumPodThickness, scaleAttachmentPixelsFrom64(5u, tangentDimension, sizePercent));
        const uint32_t podThickness = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumPodThickness, maximumPodThickness);
        const int32_t podStart = -static_cast<int32_t>(podThickness / 2u);

        if (!addAttachmentLocalRectangle(candidateMask, anchor, 1u, connectorLength, 0, 1u))
        {
            return false;
        }

        if (!addAttachmentLocalRectangle(candidateMask, anchor, connectorLength + 1u, podLength, podStart, podThickness))
        {
            return false;
        }

        if (podThickness >= 3u && context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 99u) < 60u)
        {
            setAttachmentLocalPixel(candidateMask, anchor, connectorLength + podLength, podStart, false);
            setAttachmentLocalPixel(candidateMask, anchor, connectorLength + podLength, podStart + static_cast<int32_t>(podThickness) - 1, false);
        }

        return true;
    }

    bool AttachmentGenerator::generateRadiatorAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const
    {
        const bool horizontalDirection = anchor.Direction == ShipAttachmentDirection::LEFT || anchor.Direction == ShipAttachmentDirection::RIGHT;
        const uint32_t outwardDimension = horizontalDirection ? candidateMask.getWidth() : candidateMask.getHeight();
        const uint32_t tangentDimension = horizontalDirection ? candidateMask.getHeight() : candidateMask.getWidth();
        const uint32_t maximumConnectorLength = std::max(1u, scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent));
        const uint32_t connectorLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, maximumConnectorLength);
        const uint32_t maximumPanelDepth = std::max(1u, scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent));
        const uint32_t panelDepth = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, maximumPanelDepth);
        const uint32_t minimumPanelLength = scaleAttachmentPixelsFrom64(4u, tangentDimension, sizePercent);
        const uint32_t maximumPanelLength = std::max(minimumPanelLength, scaleAttachmentPixelsFrom64(8u, tangentDimension, sizePercent));
        const uint32_t panelLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumPanelLength, maximumPanelLength);
        const int32_t panelStart = -static_cast<int32_t>(panelLength / 2u);

        if (!addAttachmentLocalRectangle(candidateMask, anchor, 1u, connectorLength, 0, 1u))
        {
            return false;
        }

        return addAttachmentLocalRectangle(candidateMask, anchor, connectorLength + 1u, panelDepth, panelStart, panelLength);
    }

    bool AttachmentGenerator::generateArmorFinAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const
    {
        const bool horizontalDirection = anchor.Direction == ShipAttachmentDirection::LEFT || anchor.Direction == ShipAttachmentDirection::RIGHT;
        const uint32_t outwardDimension = horizontalDirection ? candidateMask.getWidth() : candidateMask.getHeight();
        const uint32_t tangentDimension = horizontalDirection ? candidateMask.getHeight() : candidateMask.getWidth();
        const uint32_t minimumLength = scaleAttachmentPixelsFrom64(3u, outwardDimension, sizePercent);
        const uint32_t maximumLength = std::max(minimumLength, scaleAttachmentPixelsFrom64(7u, outwardDimension, sizePercent));
        const uint32_t length = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumLength, maximumLength);
        const uint32_t minimumThickness = scaleAttachmentPixelsFrom64(3u, tangentDimension, sizePercent);
        const uint32_t maximumThicknessLimit = std::max(minimumThickness, scaleAttachmentPixelsFrom64(6u, tangentDimension, sizePercent));
        const uint32_t maximumThickness = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, minimumThickness, maximumThicknessLimit);

        for (uint32_t outward = 1u; outward <= length; ++outward)
        {
            const uint32_t reduction = length > 1u ? ((outward - 1u) * (maximumThickness - 1u)) / (length - 1u) : 0u;
            const uint32_t thickness = std::max(1u, maximumThickness - reduction);
            const int32_t tangentStart = -static_cast<int32_t>(thickness / 2u);

            if (!addAttachmentLocalRectangle(candidateMask, anchor, outward, 1u, tangentStart, thickness))
            {
                return false;
            }
        }

        return true;
    }

    bool AttachmentGenerator::generateTechnologyNodeAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const
    {
        const bool horizontalDirection = anchor.Direction == ShipAttachmentDirection::LEFT || anchor.Direction == ShipAttachmentDirection::RIGHT;
        const uint32_t outwardDimension = horizontalDirection ? candidateMask.getWidth() : candidateMask.getHeight();
        const uint32_t tangentDimension = horizontalDirection ? candidateMask.getHeight() : candidateMask.getWidth();
        const uint32_t maximumConnectorLength = std::max(1u, scaleAttachmentPixelsFrom64(2u, outwardDimension, sizePercent));
        const uint32_t connectorLength = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, maximumConnectorLength);
        const uint32_t maximumRadius = std::max(1u, scaleAttachmentPixelsFrom64(2u, tangentDimension, sizePercent));
        const uint32_t radius = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 1u, maximumRadius);
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 1u);

        if (!addAttachmentLocalRectangle(candidateMask, anchor, 1u, connectorLength, 0, 1u))
        {
            return false;
        }

        if (variant == 0u)
        {
            for (int32_t offset = -static_cast<int32_t>(radius); offset <= static_cast<int32_t>(radius); ++offset)
            {
                const uint32_t absoluteOffset = static_cast<uint32_t>(offset < 0 ? -offset : offset);
                const uint32_t halfSpan = radius - absoluteOffset;
                const uint32_t tangentSize = halfSpan * 2u + 1u;
                const int32_t tangentStart = -static_cast<int32_t>(halfSpan);
                const uint32_t outwardDistance = connectorLength + 1u + static_cast<uint32_t>(offset + static_cast<int32_t>(radius));

                if (!addAttachmentLocalRectangle(candidateMask, anchor, outwardDistance, 1u, tangentStart, tangentSize))
                {
                    return false;
                }
            }

            return true;
        }

        const uint32_t centerDistance = connectorLength + radius + 1u;

        if (!addAttachmentLocalRectangle(candidateMask, anchor, centerDistance - radius, radius * 2u + 1u, 0, 1u))
        {
            return false;
        }

        return addAttachmentLocalRectangle(candidateMask, anchor, centerDistance, 1u, -static_cast<int32_t>(radius), radius * 2u + 1u);
    }

    bool AttachmentGenerator::addAttachmentLocalRectangle(PixelMask& mask, const AttachmentAnchor& anchor, uint32_t outwardStart, uint32_t outwardLength, int32_t tangentStart, uint32_t tangentSize) const
    {
        if (outwardLength == 0u || tangentSize == 0u)
        {
            return false;
        }

        for (uint32_t outward = 0u; outward < outwardLength; ++outward)
        {
            for (uint32_t tangent = 0u; tangent < tangentSize; ++tangent)
            {
                const auto [outwardX, outwardY] = getAttachmentDirectionOffset(anchor.Direction);
                const auto [tangentX, tangentY] = getAttachmentTangentOffset(anchor.Direction);
                const int32_t tangentOffset = tangentStart + static_cast<int32_t>(tangent);
                const int32_t x = static_cast<int32_t>(anchor.X) + outwardX * static_cast<int32_t>(outwardStart + outward) + tangentX * tangentOffset;
                const int32_t y = static_cast<int32_t>(anchor.Y) + outwardY * static_cast<int32_t>(outwardStart + outward) + tangentY * tangentOffset;

                if (x < 0 || y < 0)
                {
                    return false;
                }

                if (x >= static_cast<int32_t>(mask.getWidth()) || y >= static_cast<int32_t>(mask.getHeight()))
                {
                    return false;
                }
            }
        }

        for (uint32_t outward = 0u; outward < outwardLength; ++outward)
        {
            for (uint32_t tangent = 0u; tangent < tangentSize; ++tangent)
            {
                const int32_t tangentOffset = tangentStart + static_cast<int32_t>(tangent);

                setAttachmentLocalPixel(mask, anchor, outwardStart + outward, tangentOffset, true);
            }
        }

        return true;
    }

    bool AttachmentGenerator::setAttachmentLocalPixel(PixelMask& mask, const AttachmentAnchor& anchor, uint32_t outwardDistance, int32_t tangentOffset, bool value) const
    {
        const auto [outwardX, outwardY] = getAttachmentDirectionOffset(anchor.Direction);
        const auto [tangentX, tangentY] = getAttachmentTangentOffset(anchor.Direction);

        const int32_t x = static_cast<int32_t>(anchor.X) + outwardX * static_cast<int32_t>(outwardDistance) + tangentX * tangentOffset;
        const int32_t y = static_cast<int32_t>(anchor.Y) + outwardY * static_cast<int32_t>(outwardDistance) + tangentY * tangentOffset;

        if (x < 0 || y < 0)
        {
            return false;
        }

        if (x >= static_cast<int32_t>(mask.getWidth()) || y >= static_cast<int32_t>(mask.getHeight()))
        {
            return false;
        }

        mask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), value);
        return true;
    }

    std::pair<int32_t, int32_t> AttachmentGenerator::getAttachmentTangentOffset(ShipAttachmentDirection direction) const
    {
        if (direction == ShipAttachmentDirection::LEFT || direction == ShipAttachmentDirection::RIGHT)
        {
            return { 0, 1 };
        }

        return { 1, 0 };
    }

    AttachmentGenerator::ResolvedAttachmentProfile AttachmentGenerator::resolveAttachmentProfile(const ShipGenerationProfile& styleProfile, const ShipFactionAttachmentProfile& factionProfile) const
    {
        ResolvedAttachmentProfile result;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            const ShipAttachmentType type = static_cast<ShipAttachmentType>(index);
            const uint64_t styleWeight = styleProfile.AttachmentWeights.getWeight(type);
            const uint64_t factionMultiplier = factionProfile.WeightMultipliersPercent.getWeight(type);

            result.TypeWeights[index] = styleWeight * factionMultiplier;
        }

        result.AttachmentChance = std::min(100u, static_cast<uint32_t>((static_cast<uint64_t>(styleProfile.AttachmentChance) * factionProfile.AttachmentChancePercent + 50u) / 100u));
        result.MaximumAttachmentGroups = styleProfile.MaximumAttachmentGroups;
        result.SymmetricAttachmentChance = static_cast<uint32_t>(std::clamp(static_cast<int32_t>(styleProfile.SymmetricAttachmentChance) + factionProfile.SymmetryChanceOffset, 0, 100));
        result.AttachmentSizePercent = styleProfile.AttachmentSizePercent;

        return result;
    }

    uint32_t AttachmentGenerator::scaleAttachmentPixelsFrom64(uint32_t value, uint32_t dimension, uint32_t sizePercent) const
    {
        const uint32_t resolutionScaledValue = GenerationMath::scalePixelsFrom64(value, dimension);
        return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(resolutionScaledValue) * sizePercent + 50u) / 100u));
    }
}
