#include "WeaponHardpointPlanner.h"

#include <algorithm>
#include <cstdint>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace SpectralShipGen
{
    namespace WeaponGenerationInternal
    {
        std::vector<WeaponHardpoint> WeaponHardpointPlanner::discoverHardpoints(const ShipGenerationContext& context) const
        {
            std::vector<WeaponHardpoint> hardpoints;
            const PixelMaskUtils::MaskBounds hullBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);

            if (!hullBounds.Valid)
            {
                return hardpoints;
            }

            const uint32_t width = context.Ship.HullMask.getWidth();
            const uint32_t hullHeight = hullBounds.MaxY - hullBounds.MinY + 1u;
            const uint32_t leftCenter = (width - 1u) / 2u;
            const uint32_t rightCenter = width / 2u;
            const uint32_t rowStep = std::max(1u, GenerationMath::scalePixelsFrom64(3u, context.Ship.HullMask.getHeight()));
            uint32_t maximumRowWidth = 0u;

            for (uint32_t y = hullBounds.MinY; y <= hullBounds.MaxY; ++y)
            {
                maximumRowWidth = std::max(maximumRowWidth, PixelMaskUtils::getOccupiedRowWidth(context.Ship.HullMask, y));
            }

            for (uint32_t y = hullBounds.MinY; y <= hullBounds.MaxY; y += rowStep)
            {
                const uint32_t verticalPercent = hullHeight > 1u ? ((y - hullBounds.MinY) * 100u) / (hullHeight - 1u) : 0u;
                const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;

                if (verticalPercent >= 12u && verticalPercent <= 32u)
                {
                    WeaponHardpoint central = { rightCenter, y, ShipWeaponHardpointRegion::CENTRAL_NOSE, ShipAttachmentDirection::UP, false };

                    if (context.Ship.HullMask.get(central.X, central.Y) && isHardpointSupported(context, central))
                    {
                        central.FeasibilityPercent = getHardpointFeasibilityPercent(context, central);
                        hardpoints.push_back(central);
                    }
                }

                if (verticalPercent >= 28u && verticalPercent <= 52u)
                {
                    WeaponHardpoint central = { rightCenter, y, ShipWeaponHardpointRegion::CENTRAL_BODY, ShipAttachmentDirection::UP, false };

                    if (context.Ship.HullMask.get(central.X, central.Y) && isHardpointSupported(context, central))
                    {
                        central.FeasibilityPercent = getHardpointFeasibilityPercent(context, central);
                        hardpoints.push_back(central);
                    }
                }

                if (fuselageHalfWidth >= 2u && verticalPercent >= 20u && verticalPercent <= 48u)
                {
                    const uint32_t leftX = leftCenter - (fuselageHalfWidth - 1u);
                    const uint32_t rightX = rightCenter + (fuselageHalfWidth - 1u);
                    addSymmetricSideHardpoints(context, hardpoints, y, leftX, rightX, ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE);
                }

                if (context.WingRegions.hasWings())
                {
                    uint32_t leftRootX = width;
                    uint32_t leftOuterX = width;
                    bool rootFound = false;
                    bool outerFound = false;

                    for (uint32_t x = 0u; x <= leftCenter; ++x)
                    {
                        if (context.WingRegions.WingRootMask.get(x, y))
                        {
                            leftRootX = std::min(leftRootX, x);
                            rootFound = true;
                        }

                        if (context.WingRegions.OuterWingMask.get(x, y))
                        {
                            leftOuterX = std::min(leftOuterX, x);
                            outerFound = true;
                        }
                    }

                    if (rootFound)
                    {
                        addSymmetricSideHardpoints(context, hardpoints, y, leftRootX, width - 1u - leftRootX, ShipWeaponHardpointRegion::WING_ROOT);
                    }

                    if (outerFound && verticalPercent <= 70u)
                    {
                        addSymmetricSideHardpoints(context, hardpoints, y, leftOuterX, width - 1u - leftOuterX, ShipWeaponHardpointRegion::OUTER_WING);
                    }
                }

                const uint32_t rowWidth = PixelMaskUtils::getOccupiedRowWidth(context.Ship.HullMask, y);

                if (verticalPercent >= 30u && verticalPercent <= 56u && maximumRowWidth > 0u && rowWidth * 100u >= maximumRowWidth * 72u)
                {
                    uint32_t leftEdge = width;
                    uint32_t rightEdge = 0u;
                    bool rowFound = false;

                    for (uint32_t x = 0u; x < width; ++x)
                    {
                        if (!context.Ship.HullMask.get(x, y))
                        {
                            continue;
                        }

                        leftEdge = std::min(leftEdge, x);
                        rightEdge = std::max(rightEdge, x);
                        rowFound = true;
                    }

                    if (rowFound && leftEdge < rightEdge)
                    {
                        addSymmetricSideHardpoints(context, hardpoints, y, leftEdge, rightEdge, ShipWeaponHardpointRegion::FORWARD_SHOULDER);
                    }
                }
            }

            return hardpoints;
        }

        void WeaponHardpointPlanner::addSymmetricSideHardpoints(const ShipGenerationContext& context, std::vector<WeaponHardpoint>& hardpoints, uint32_t y, uint32_t leftX, uint32_t rightX, ShipWeaponHardpointRegion region) const
        {
            if (leftX >= context.Ship.HullMask.getWidth() || rightX >= context.Ship.HullMask.getWidth() || leftX >= rightX)
            {
                return;
            }

            WeaponHardpoint left = { leftX, y, region, ShipAttachmentDirection::UP, true };
            WeaponHardpoint right = { rightX, y, region, ShipAttachmentDirection::UP, true };

            if (isHardpointSupported(context, left))
            {
                left.FeasibilityPercent = getHardpointFeasibilityPercent(context, left);
                hardpoints.push_back(left);
            }

            if (isHardpointSupported(context, right))
            {
                right.FeasibilityPercent = getHardpointFeasibilityPercent(context, right);
                hardpoints.push_back(right);
            }
        }

        bool WeaponHardpointPlanner::isHardpointSupported(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const
        {
            if (!context.Ship.HullMask.isInBounds(hardpoint.X, hardpoint.Y) || !context.Ship.HullMask.get(hardpoint.X, hardpoint.Y))
            {
                return false;
            }

            if (hardpoint.Y < 2u || context.Ship.CockpitMask.get(hardpoint.X, hardpoint.Y) || context.Ship.EngineMask.get(hardpoint.X, hardpoint.Y) || context.MajorFeatures.OccupiedMask.get(hardpoint.X, hardpoint.Y))
            {
                return false;
            }

            uint32_t supportCount = 0u;

            for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
            {
                for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const int32_t x = static_cast<int32_t>(hardpoint.X) + offsetX;
                    const int32_t y = static_cast<int32_t>(hardpoint.Y) + offsetY;

                    if (PixelMaskUtils::isMaskPixel(context.Ship.HullMask, x, y) && !PixelMaskUtils::isMaskPixel(context.Ship.CockpitMask, x, y) && !PixelMaskUtils::isMaskPixel(context.MajorFeatures.OccupiedMask, x, y))
                    {
                        ++supportCount;
                    }
                }
            }

            const uint32_t minimumSupport = context.ScaleTraits.SmallFeatureCapacity == 0u ? 3u : 4u;
            return supportCount >= minimumSupport;
        }

        uint32_t WeaponHardpointPlanner::getHardpointFeasibilityPercent(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const
        {
            const uint32_t width = context.Ship.HullMask.getWidth();
            const uint32_t height = context.Ship.HullMask.getHeight();
            if (width == 0u || height == 0u) { return 0u; }

            const uint32_t scalePercent = context.Profile.LargeWeaponScalePercent;
            const uint32_t envelopeWidth = std::max(3u, GenerationMath::scalePixelsFrom64(9u, width) * scalePercent / 100u);
            const uint32_t envelopeDepth = std::max(4u, GenerationMath::scalePixelsFrom64(12u, height) * scalePercent / 100u);
            const int32_t startX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(envelopeWidth / 2u);
            const int32_t startY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(envelopeDepth) + 1;

            uint32_t sampleCount = 0u;
            uint32_t blockedCount = 0u;
            for (uint32_t oy = 0u; oy < envelopeDepth; ++oy)
            {
                const int32_t y = startY + static_cast<int32_t>(oy);
                if (y < 0 || y >= static_cast<int32_t>(height)) { blockedCount += envelopeWidth; sampleCount += envelopeWidth; continue; }
                for (uint32_t ox = 0u; ox < envelopeWidth; ++ox)
                {
                    const int32_t x = startX + static_cast<int32_t>(ox);
                    ++sampleCount;
                    if (x < 0 || x >= static_cast<int32_t>(width)) { ++blockedCount; continue; }
                    const uint32_t px = static_cast<uint32_t>(x);
                    const uint32_t py = static_cast<uint32_t>(y);
                    if (context.StructuralNegativeSpace.ReservedMask.get(px, py) ||
                        context.Ship.CockpitMask.get(px, py) || context.Ship.EngineMask.get(px, py) ||
                        context.Ship.EngineExhaustMask.get(px, py) || context.MajorFeatures.OccupiedMask.get(px, py) ||
                        context.Weapons.OccupiedMask.get(px, py))
                    {
                        ++blockedCount;
                    }
                }
            }

            if (sampleCount == 0u) { return 0u; }
            const uint32_t clearPercent = 100u - std::min(100u, blockedCount * 100u / sampleCount);
            // 55..125 keeps weak hardpoints selectable while strongly preferring clear local envelopes.
            return std::clamp(55u + clearPercent * 70u / 100u, 55u, 125u);
        }

        bool WeaponHardpointPlanner::hardpointMatchesMacroAsymmetryPlan(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const
        {
            if (!context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON)) { return true; }
            if (!context.MacroAsymmetry.isDominantX(hardpoint.X, context.Ship.HullMask.getWidth())) { return false; }
            const GenerationSpatialRegion region = getSpatialRegion(hardpoint.Region, hardpoint.X, context.Ship.HullMask.getWidth());
            const GenerationSpatialRegion target = context.MacroAsymmetry.TargetRegion;
            if (target == GenerationSpatialRegion::LEFT_OUTER_WING || target == GenerationSpatialRegion::RIGHT_OUTER_WING || target == GenerationSpatialRegion::LEFT_WING_ROOT || target == GenerationSpatialRegion::RIGHT_WING_ROOT)
            {
                return region == target;
            }
            return region == target || (target == GenerationSpatialRegion::MID_FUSELAGE && region == GenerationSpatialRegion::FRONT_FUSELAGE);
        }

        const WeaponHardpoint* WeaponHardpointPlanner::selectHardpoint(ShipGenerationContext& context, const std::vector<WeaponHardpoint>& hardpoints, bool plannedAsymmetry) const
        {
            uint64_t totalHardpointWeight = 0u;
            for (const WeaponHardpoint& possible : hardpoints)
            {
                if (plannedAsymmetry && !hardpointMatchesMacroAsymmetryPlan(context, possible)) { continue; }
                uint64_t hardpointWeight = context.Profile.LargeWeaponHardpointWeights.getWeight(possible.Region);
                hardpointWeight = hardpointWeight * possible.FeasibilityPercent / 100u;
                if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::WEAPONS))
                {
                    const GenerationSpatialRegion region = getSpatialRegion(possible.Region, possible.X, context.Ship.HullMask.getWidth());
                    const GenerationSpatialRegion target = context.VisualHierarchy.TargetRegion;
                    if (region == target || region == context.SpatialBudget.getMirroredRegion(target))
                    {
                        hardpointWeight = hardpointWeight * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::WEAPONS) / 100u;
                    }
                }
                totalHardpointWeight += hardpointWeight;
            }

            if (totalHardpointWeight == 0u) { return nullptr; }

            uint64_t hardpointRoll = context.getGenerationRandomUInt64(GenerationDomain::WEAPONS, 0u, totalHardpointWeight - 1u);
            for (const WeaponHardpoint& possible : hardpoints)
            {
                if (plannedAsymmetry && !hardpointMatchesMacroAsymmetryPlan(context, possible)) { continue; }
                uint64_t weight = context.Profile.LargeWeaponHardpointWeights.getWeight(possible.Region);
                weight = weight * possible.FeasibilityPercent / 100u;
                if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::WEAPONS))
                {
                    const GenerationSpatialRegion region = getSpatialRegion(possible.Region, possible.X, context.Ship.HullMask.getWidth());
                    const GenerationSpatialRegion target = context.VisualHierarchy.TargetRegion;
                    if (region == target || region == context.SpatialBudget.getMirroredRegion(target))
                    {
                        weight = weight * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::WEAPONS) / 100u;
                    }
                }
                if (hardpointRoll < weight) { return &possible; }
                hardpointRoll -= weight;
            }
            return nullptr;
        }

        GenerationSpatialRegion WeaponHardpointPlanner::getSpatialRegion(ShipWeaponHardpointRegion region, uint32_t x, uint32_t width) const
        {
            const bool left = x < width / 2u;
            switch (region)
            {
            case ShipWeaponHardpointRegion::CENTRAL_NOSE: return GenerationSpatialRegion::NOSE;
            case ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE:
            case ShipWeaponHardpointRegion::FORWARD_SHOULDER: return GenerationSpatialRegion::FRONT_FUSELAGE;
            case ShipWeaponHardpointRegion::WING_ROOT: return left ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::RIGHT_WING_ROOT;
            case ShipWeaponHardpointRegion::OUTER_WING: return left ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING;
            case ShipWeaponHardpointRegion::CENTRAL_BODY: return GenerationSpatialRegion::MID_FUSELAGE;
            default: return GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
            }
        }

    }
}
