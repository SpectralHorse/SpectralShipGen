#include "WeaponCandidateBuilder.h"

#include <algorithm>
#include <cstdint>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace WeaponGenerationInternal
    {
        bool WeaponCandidateBuilder::generateCandidate(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, ShipWeaponType type, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
        {
            candidate.Placement.Type = type;
            candidate.Placement.Region = hardpoint.Region;
            candidate.Placement.Direction = hardpoint.Direction;
            candidate.Placement.AnchorX = hardpoint.X;
            candidate.Placement.AnchorY = hardpoint.Y;

            bool generated = false;

            switch (type)
            {
            case ShipWeaponType::SINGLE_CANNON: generated = generateSingleCannon(context, hardpoint, factionProfile, candidate); break;
            case ShipWeaponType::TWIN_CANNON: generated = generateTwinCannon(context, hardpoint, factionProfile, candidate); break;
            case ShipWeaponType::COMPACT_TURRET: generated = generateCompactTurret(context, hardpoint, factionProfile, candidate); break;
            case ShipWeaponType::RAIL_WEAPON: generated = generateRailWeapon(context, hardpoint, factionProfile, candidate); break;
            case ShipWeaponType::WEAPON_POD: generated = generateWeaponPod(context, hardpoint, factionProfile, candidate); break;
            default: return false;
            }

            if (!generated)
            {
                return false;
            }

            const PixelMaskUtils::MaskBounds bodyBounds = PixelMaskUtils::calculateMaskBounds(candidate.BodyMask);
            const PixelMaskUtils::MaskBounds barrelBounds = PixelMaskUtils::calculateMaskBounds(candidate.BarrelMask);
            const PixelMaskUtils::MaskBounds muzzleBounds = PixelMaskUtils::calculateMaskBounds(candidate.MuzzleMask);

            if (!bodyBounds.Valid || !barrelBounds.Valid || !muzzleBounds.Valid)
            {
                return false;
            }

            candidate.Placement.BodyMinX = bodyBounds.MinX;
            candidate.Placement.BodyMaxX = bodyBounds.MaxX;
            candidate.Placement.BodyMinY = bodyBounds.MinY;
            candidate.Placement.BodyMaxY = bodyBounds.MaxY;
            candidate.Placement.BarrelMinX = barrelBounds.MinX;
            candidate.Placement.BarrelMaxX = barrelBounds.MaxX;
            candidate.Placement.BarrelMinY = barrelBounds.MinY;
            candidate.Placement.BarrelMaxY = barrelBounds.MaxY;
            const uint32_t muzzleCenterX = (muzzleBounds.MinX + muzzleBounds.MaxX) / 2u;
            candidate.Placement.MuzzleX = muzzleBounds.MinX;
            candidate.Placement.MuzzleY = muzzleBounds.MinY;
            uint32_t bestMuzzleDistance = candidate.OccupiedMask.getWidth();

            for (uint32_t x = muzzleBounds.MinX; x <= muzzleBounds.MaxX; ++x)
            {
                if (!candidate.MuzzleMask.get(x, muzzleBounds.MinY))
                {
                    continue;
                }

                const uint32_t distance = x > muzzleCenterX ? x - muzzleCenterX : muzzleCenterX - x;

                if (distance < bestMuzzleDistance)
                {
                    bestMuzzleDistance = distance;
                    candidate.Placement.MuzzleX = x;
                }
            }
            candidate.Placement.MovableBarrel = PixelMaskUtils::getMaskPixelCount(candidate.MovableMask) > 0u;
            candidate.Placement.Emissive = PixelMaskUtils::getMaskPixelCount(candidate.EmissiveMask) > 0u;
            return true;
        }

        bool WeaponCandidateBuilder::generateSingleCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
        {
            const uint32_t scalePercent = getAssemblyScalePercent(context, hardpoint);
            const uint32_t width = candidate.OccupiedMask.getWidth();
            const uint32_t height = candidate.OccupiedMask.getHeight();
            const uint32_t bodyWidth = std::max(1u, scaleWeaponPixelsFrom64(4u, width, scalePercent));
            const uint32_t bodyLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));
            const uint32_t barrelLength = std::max(1u, scaleWeaponPixelsFrom64(5u, height, scalePercent));
            const uint32_t barrelWidth = std::min(bodyWidth, std::max(1u, scaleWeaponPixelsFrom64(1u, width, scalePercent)));
            const uint32_t totalLength = bodyLength + barrelLength;

            if (hardpoint.Y + 1u < totalLength || !buildRoot(context, hardpoint, bodyWidth > 2u ? 1u : 0u, std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
            {
                return false;
            }

            const int32_t bodyX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(bodyWidth / 2u);
            const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
            const int32_t barrelX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(barrelWidth / 2u);
            const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);

            if (!addCandidateRectangle(candidate, candidate.BodyMask, bodyX, bodyY, bodyWidth, bodyLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, barrelX, barrelY, barrelWidth, barrelLength))
            {
                return false;
            }

            const uint32_t muzzleWidth = std::min(bodyWidth, std::max(barrelWidth, scaleWeaponPixelsFrom64(2u, width, scalePercent)));
            const int32_t muzzleX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(muzzleWidth / 2u);

            if (!addCandidateRectangle(candidate, candidate.MuzzleMask, muzzleX, barrelY, muzzleWidth, 1u))
            {
                return false;
            }

            for (uint32_t y = static_cast<uint32_t>(barrelY); y < static_cast<uint32_t>(bodyY); ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (candidate.BarrelMask.get(x, y)) { candidate.MovableMask.set(x, y, true); }
                }
            }

            const bool emissive = context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance;
            if (emissive) { candidate.EmissiveMask.set(candidate.Placement.AnchorX, static_cast<uint32_t>(barrelY), true); }
            return true;
        }

        bool WeaponCandidateBuilder::generateTwinCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
        {
            const uint32_t scalePercent = getAssemblyScalePercent(context, hardpoint);
            const uint32_t width = candidate.OccupiedMask.getWidth();
            const uint32_t height = candidate.OccupiedMask.getHeight();
            const uint32_t bodyWidth = std::max(3u, scaleWeaponPixelsFrom64(6u, width, scalePercent));
            const uint32_t bodyLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));
            const uint32_t barrelLength = std::max(2u, scaleWeaponPixelsFrom64(5u, height, scalePercent));
            const uint32_t barrelWidth = std::max(1u, scaleWeaponPixelsFrom64(1u, width, scalePercent));
            const uint32_t separation = std::max(1u, bodyWidth / 4u);

            if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, std::max(1u, bodyWidth / 3u), std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
            {
                return false;
            }

            const int32_t bodyX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(bodyWidth / 2u);
            const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
            const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);
            const int32_t leftBarrelX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(separation) - static_cast<int32_t>(barrelWidth);
            const int32_t rightBarrelX = static_cast<int32_t>(hardpoint.X) + static_cast<int32_t>(separation);

            if (!addCandidateRectangle(candidate, candidate.BodyMask, bodyX, bodyY, bodyWidth, bodyLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, leftBarrelX, barrelY, barrelWidth, barrelLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, rightBarrelX, barrelY, barrelWidth, barrelLength))
            {
                return false;
            }

            if (!addCandidateRectangle(candidate, candidate.MuzzleMask, leftBarrelX, barrelY, barrelWidth, 1u) || !addCandidateRectangle(candidate, candidate.MuzzleMask, rightBarrelX, barrelY, barrelWidth, 1u))
            {
                return false;
            }

            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

            if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance)
            {
                PixelMaskUtils::mergeMask(candidate.EmissiveMask, candidate.MuzzleMask);
            }

            return true;
        }

        bool WeaponCandidateBuilder::generateCompactTurret(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
        {
            const uint32_t scalePercent = getAssemblyScalePercent(context, hardpoint);
            const uint32_t width = candidate.OccupiedMask.getWidth();
            const uint32_t height = candidate.OccupiedMask.getHeight();
            const uint32_t bodyWidth = std::max(3u, scaleWeaponPixelsFrom64(6u, width, scalePercent));
            const uint32_t bodyLength = std::max(2u, scaleWeaponPixelsFrom64(4u, height, scalePercent));
            const uint32_t barrelLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));

            if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, std::max(1u, bodyWidth / 3u), std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
            {
                return false;
            }

            const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;

            for (uint32_t row = 0u; row < bodyLength; ++row)
            {
                uint32_t rowWidth = bodyWidth;
                if (row == 0u || row + 1u == bodyLength) { rowWidth = std::max(1u, bodyWidth - 2u); }
                const int32_t rowX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(rowWidth / 2u);
                if (!addCandidateRectangle(candidate, candidate.BodyMask, rowX, bodyY + static_cast<int32_t>(row), rowWidth, 1u)) { return false; }
            }

            const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);
            if (!addCandidateRectangle(candidate, candidate.BarrelMask, static_cast<int32_t>(hardpoint.X), barrelY, 1u, barrelLength) || !addCandidatePixel(candidate, candidate.MuzzleMask, static_cast<int32_t>(hardpoint.X), barrelY))
            {
                return false;
            }

            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

            if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance)
            {
                candidate.EmissiveMask.set(hardpoint.X, static_cast<uint32_t>(barrelY), true);
            }

            return true;
        }

        bool WeaponCandidateBuilder::generateRailWeapon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
        {
            const uint32_t scalePercent = getAssemblyScalePercent(context, hardpoint);
            const uint32_t width = candidate.OccupiedMask.getWidth();
            const uint32_t height = candidate.OccupiedMask.getHeight();
            const uint32_t bodyWidth = std::max(2u, scaleWeaponPixelsFrom64(3u, width, scalePercent));
            const uint32_t bodyLength = std::max(2u, scaleWeaponPixelsFrom64(4u, height, scalePercent));
            const uint32_t barrelLength = std::max(3u, scaleWeaponPixelsFrom64(8u, height, scalePercent));

            if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, bodyWidth > 2u ? 1u : 0u, std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
            {
                return false;
            }

            const int32_t bodyX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(bodyWidth / 2u);
            const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
            const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);

            if (!addCandidateRectangle(candidate, candidate.BodyMask, bodyX, bodyY, bodyWidth, bodyLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, static_cast<int32_t>(hardpoint.X), barrelY, 1u, barrelLength) || !addCandidatePixel(candidate, candidate.MuzzleMask, static_cast<int32_t>(hardpoint.X), barrelY))
            {
                return false;
            }

            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

            if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < std::max(35u, factionProfile.EmissiveChance))
            {
                for (int32_t y = bodyY; y >= std::max<int32_t>(barrelY, bodyY - static_cast<int32_t>(std::max(1u, bodyLength / 2u))); --y)
                {
                    if (y >= 0) { candidate.EmissiveMask.set(hardpoint.X, static_cast<uint32_t>(y), true); }
                }
            }

            return true;
        }

        bool WeaponCandidateBuilder::generateWeaponPod(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
        {
            const uint32_t scalePercent = getAssemblyScalePercent(context, hardpoint);
            const uint32_t width = candidate.OccupiedMask.getWidth();
            const uint32_t height = candidate.OccupiedMask.getHeight();
            const uint32_t bodyWidth = std::max(4u, scaleWeaponPixelsFrom64(8u, width, scalePercent));
            const uint32_t bodyLength = std::max(2u, scaleWeaponPixelsFrom64(5u, height, scalePercent));
            const uint32_t barrelLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));
            const uint32_t barrelOffset = std::max(1u, bodyWidth / 4u);

            if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, std::max(1u, bodyWidth / 3u), std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
            {
                return false;
            }

            const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
            const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);
            const int32_t leftBarrelX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(barrelOffset);
            const int32_t rightBarrelX = static_cast<int32_t>(hardpoint.X) + static_cast<int32_t>(barrelOffset);

            for (uint32_t row = 0u; row < bodyLength; ++row)
            {
                const uint32_t rowWidth = row == 0u ? std::max(2u, bodyWidth - 2u) : bodyWidth;
                const int32_t rowX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(rowWidth / 2u);
                if (!addCandidateRectangle(candidate, candidate.BodyMask, rowX, bodyY + static_cast<int32_t>(row), rowWidth, 1u)) { return false; }
            }

            if (!addCandidateRectangle(candidate, candidate.BarrelMask, leftBarrelX, barrelY, 1u, barrelLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, rightBarrelX, barrelY, 1u, barrelLength) || !addCandidatePixel(candidate, candidate.MuzzleMask, leftBarrelX, barrelY) || !addCandidatePixel(candidate, candidate.MuzzleMask, rightBarrelX, barrelY))
            {
                return false;
            }

            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
            PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

            if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance)
            {
                PixelMaskUtils::mergeMask(candidate.EmissiveMask, candidate.MuzzleMask);
            }

            return true;
        }

        bool WeaponCandidateBuilder::buildRoot(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, uint32_t halfWidth, uint32_t depth, CandidateWeapon& candidate) const
        {
            const int32_t startX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(halfWidth);
            uint32_t supportCount = 0u;
            uint32_t possibleCount = 0u;

            for (uint32_t offsetY = 0u; offsetY < depth; ++offsetY)
            {
                const uint32_t y = hardpoint.Y + offsetY;

                if (y >= context.Ship.HullMask.getHeight())
                {
                    break;
                }

                for (uint32_t offsetX = 0u; offsetX < halfWidth * 2u + 1u; ++offsetX)
                {
                    const int32_t x = startX + static_cast<int32_t>(offsetX);
                    ++possibleCount;

                    if (x < 0 || x >= static_cast<int32_t>(context.Ship.HullMask.getWidth()))
                    {
                        continue;
                    }

                    const uint32_t pixelX = static_cast<uint32_t>(x);

                    if (!context.Ship.HullMask.get(pixelX, y) || context.Ship.CockpitMask.get(pixelX, y) || context.Ship.EngineMask.get(pixelX, y) || context.MajorFeatures.OccupiedMask.get(pixelX, y))
                    {
                        continue;
                    }

                    candidate.RootMask.set(pixelX, y, true);
                    candidate.OccupiedMask.set(pixelX, y, true);
                    ++supportCount;
                }
            }

            const uint32_t minimumSupport = std::max(1u, (possibleCount + 1u) / 2u);
            return supportCount >= minimumSupport;
        }

        bool WeaponCandidateBuilder::addCandidateRectangle(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t startX, int32_t startY, uint32_t width, uint32_t height) const
        {
            if (width == 0u || height == 0u || startX < 0 || startY < 0)
            {
                return false;
            }

            if (startX + static_cast<int32_t>(width) > static_cast<int32_t>(candidate.OccupiedMask.getWidth()) || startY + static_cast<int32_t>(height) > static_cast<int32_t>(candidate.OccupiedMask.getHeight()))
            {
                return false;
            }

            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    semanticMask.set(static_cast<uint32_t>(startX) + x, static_cast<uint32_t>(startY) + y, true);
                    candidate.OccupiedMask.set(static_cast<uint32_t>(startX) + x, static_cast<uint32_t>(startY) + y, true);
                }
            }

            return true;
        }

        bool WeaponCandidateBuilder::addCandidatePixel(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t x, int32_t y) const
        {
            if (x < 0 || y < 0 || x >= static_cast<int32_t>(candidate.OccupiedMask.getWidth()) || y >= static_cast<int32_t>(candidate.OccupiedMask.getHeight()))
            {
                return false;
            }

            semanticMask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), true);
            candidate.OccupiedMask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), true);
            return true;
        }

        void WeaponCandidateBuilder::mirrorCandidate(const CandidateWeapon& source, CandidateWeapon& destination, uint32_t imageWidth) const
        {
            auto mirrorMask = [imageWidth](const PixelMask& input, PixelMask& output)
                {
                    for (uint32_t y = 0u; y < input.getHeight(); ++y)
                    {
                        for (uint32_t x = 0u; x < input.getWidth(); ++x)
                        {
                            if (input.get(x, y)) { output.set(imageWidth - 1u - x, y, true); }
                        }
                    }
                };

            mirrorMask(source.OccupiedMask, destination.OccupiedMask);
            mirrorMask(source.RootMask, destination.RootMask);
            mirrorMask(source.BodyMask, destination.BodyMask);
            mirrorMask(source.BarrelMask, destination.BarrelMask);
            mirrorMask(source.MuzzleMask, destination.MuzzleMask);
            mirrorMask(source.MovableMask, destination.MovableMask);
            mirrorMask(source.EmissiveMask, destination.EmissiveMask);
            destination.Placement = source.Placement;
            destination.Placement.AnchorX = imageWidth - 1u - source.Placement.AnchorX;
            destination.Placement.BodyMinX = imageWidth - 1u - source.Placement.BodyMaxX;
            destination.Placement.BodyMaxX = imageWidth - 1u - source.Placement.BodyMinX;
            destination.Placement.BarrelMinX = imageWidth - 1u - source.Placement.BarrelMaxX;
            destination.Placement.BarrelMaxX = imageWidth - 1u - source.Placement.BarrelMinX;
            destination.Placement.MuzzleX = imageWidth - 1u - source.Placement.MuzzleX;
        }

        uint32_t WeaponCandidateBuilder::getAssemblyScalePercent(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const
        {
            const uint32_t basePercent = context.Profile.LargeWeaponScalePercent;
            const uint32_t capacity = context.ScaleTraits.MajorFeatureCapacity;
            if (capacity <= 40u) { return basePercent; }

            const uint32_t capacityGrowth = std::min(25u, (capacity - 40u) * 25u / 60u);
            const uint32_t localFactor = hardpoint.FeasibilityPercent <= 70u
                ? 0u
                : std::min(100u, (hardpoint.FeasibilityPercent - 70u) * 2u);
            uint32_t growthPercent = capacityGrowth * localFactor / 100u;

            if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.isPrimary(ShipVisualAnchorType::WEAPONS) && localFactor >= 50u)
            {
                growthPercent += 8u;
            }

            const uint32_t scaled = static_cast<uint32_t>((static_cast<uint64_t>(basePercent) * (100u + growthPercent) + 50u) / 100u);
            return std::min(180u, scaled);
        }

        uint32_t WeaponCandidateBuilder::scaleWeaponPixelsFrom64(uint32_t value, uint32_t dimension, uint32_t scalePercent) const
        {
            const uint32_t scaled = GenerationMath::scalePixelsFrom64(value, dimension);
            return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(scaled) * scalePercent + 50u) / 100u));
        }
    }
}
