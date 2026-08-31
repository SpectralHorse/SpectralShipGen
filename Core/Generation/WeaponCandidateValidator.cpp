#include "WeaponCandidateValidator.h"

#include <array>
#include <cstdint>
#include <queue>
#include <utility>

#include "PixelMaskUtils.h"

namespace SpectralShipGen
{
    namespace WeaponGenerationInternal
    {
        bool WeaponCandidateValidator::validateCandidate(const ShipGenerationContext& context, const CandidateWeapon& candidate, WeaponCandidateValidationFailureReason* failureReason) const
        {
            if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::NONE; }

            if (PixelMaskUtils::getMaskPixelCount(candidate.RootMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.BodyMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.BarrelMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.MuzzleMask) == 0u)
            {
                if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::MISSING_SEMANTIC_GEOMETRY; }
                return false;
            }

            for (uint32_t y = 0u; y < candidate.OccupiedMask.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < candidate.OccupiedMask.getWidth(); ++x)
                {
                    if (candidate.RootMask.get(x, y) && !context.Ship.HullMask.get(x, y))
                    {
                        if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::INVALID_ROOT_SUPPORT; }
                        return false;
                    }

                    if (!candidate.OccupiedMask.get(x, y))
                    {
                        continue;
                    }

                    if (context.StructuralNegativeSpace.ReservedMask.get(x, y) || context.Ship.CockpitMask.get(x, y) || context.Ship.EngineMask.get(x, y) || context.Ship.EngineExhaustMask.get(x, y) || context.MajorFeatures.OccupiedMask.get(x, y) || context.Weapons.OccupiedMask.get(x, y))
                    {
                        if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::SEMANTIC_COLLISION; }
                        return false;
                    }
                }
            }

            if (!validateConnected(candidate))
            {
                if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::DISCONNECTED_GEOMETRY; }
                return false;
            }

            if (!validateFiringClearance(context, candidate))
            {
                if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::FIRING_CLEARANCE; }
                return false;
            }

            if (candidate.Placement.MuzzleY >= candidate.Placement.AnchorY)
            {
                if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::INVALID_MUZZLE_DIRECTION; }
                return false;
            }

            return true;
        }

        bool WeaponCandidateValidator::validateConnected(const CandidateWeapon& candidate) const
        {
            const uint32_t totalPixels = PixelMaskUtils::getMaskPixelCount(candidate.OccupiedMask);

            if (totalPixels == 0u)
            {
                return false;
            }

            std::queue<std::pair<uint32_t, uint32_t>> queue;
            PixelMask visited(candidate.OccupiedMask.getWidth(), candidate.OccupiedMask.getHeight(), false);
            bool found = false;

            for (uint32_t y = 0u; y < candidate.OccupiedMask.getHeight() && !found; ++y)
            {
                for (uint32_t x = 0u; x < candidate.OccupiedMask.getWidth(); ++x)
                {
                    if (candidate.OccupiedMask.get(x, y))
                    {
                        queue.push({ x, y });
                        visited.set(x, y, true);
                        found = true;
                        break;
                    }
                }
            }

            uint32_t visitedCount = 0u;
            constexpr std::array<std::pair<int32_t, int32_t>, 4u> Directions = { std::pair<int32_t, int32_t>{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

            while (!queue.empty())
            {
                const auto [x, y] = queue.front();
                queue.pop();
                ++visitedCount;

                for (const auto& [dx, dy] : Directions)
                {
                    const int32_t nextX = static_cast<int32_t>(x) + dx;
                    const int32_t nextY = static_cast<int32_t>(y) + dy;

                    if (nextX < 0 || nextY < 0 || nextX >= static_cast<int32_t>(candidate.OccupiedMask.getWidth()) || nextY >= static_cast<int32_t>(candidate.OccupiedMask.getHeight()))
                    {
                        continue;
                    }

                    const uint32_t px = static_cast<uint32_t>(nextX);
                    const uint32_t py = static_cast<uint32_t>(nextY);

                    if (candidate.OccupiedMask.get(px, py) && !visited.get(px, py))
                    {
                        visited.set(px, py, true);
                        queue.push({ px, py });
                    }
                }
            }

            return visitedCount == totalPixels;
        }

        bool WeaponCandidateValidator::validateFiringClearance(const ShipGenerationContext& context, const CandidateWeapon& candidate) const
        {
            const PixelMaskUtils::MaskBounds muzzleBounds = PixelMaskUtils::calculateMaskBounds(candidate.MuzzleMask);

            if (!muzzleBounds.Valid || muzzleBounds.MinY == 0u)
            {
                return false;
            }

            bool muzzleOutsideHull = false;

            for (uint32_t x = muzzleBounds.MinX; x <= muzzleBounds.MaxX; ++x)
            {
                if (!candidate.MuzzleMask.get(x, muzzleBounds.MinY))
                {
                    continue;
                }

                if (!context.Ship.HullMask.get(x, muzzleBounds.MinY))
                {
                    muzzleOutsideHull = true;
                }

                const uint32_t forwardY = muzzleBounds.MinY - 1u;

                if (context.Ship.HullMask.get(x, forwardY) || context.Ship.CockpitMask.get(x, forwardY) || context.Ship.EngineMask.get(x, forwardY) || context.MajorFeatures.OccupiedMask.get(x, forwardY) || context.Weapons.OccupiedMask.get(x, forwardY))
                {
                    return false;
                }
            }

            return muzzleOutsideHull;
        }

        bool WeaponCandidateValidator::validateSymmetricPair(const ShipGenerationContext& context, const CandidateWeapon& first, const CandidateWeapon& second, WeaponCandidateValidationFailureReason* failureReason) const
        {
            if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::NONE; }

            if (PixelMaskUtils::masksOverlap(first.OccupiedMask, second.OccupiedMask))
            {
                if (failureReason != nullptr) { *failureReason = WeaponCandidateValidationFailureReason::SEMANTIC_COLLISION; }
                return false;
            }

            return validateCandidate(context, second, failureReason);
        }
    }
}
