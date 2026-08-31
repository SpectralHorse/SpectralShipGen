#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <PixelShipGenerator/PixelMask.h>
#include <PixelShipGenerator/ShipWeaponType.h>
#include "WeaponData.h"

namespace PixelShipGenerator
{
    namespace WeaponGenerationInternal
    {
        struct WeaponHardpoint
        {
            uint32_t X = 0u;
            uint32_t Y = 0u;
            ShipWeaponHardpointRegion Region = ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE;
            ShipAttachmentDirection Direction = ShipAttachmentDirection::UP;
            bool PairCapable = false;
            uint32_t FeasibilityPercent = 100u;
        };

        struct FactionWeaponProfile
        {
            uint32_t ChancePercent = 100u;
            int32_t SymmetryChanceOffset = 0;
            std::array<uint32_t, static_cast<std::size_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END)> WeightMultipliers = {};
            uint32_t EmissiveChance = 0u;
        };

        enum class WeaponCandidateValidationFailureReason : uint32_t
        {
            NONE = 0u,
            MISSING_SEMANTIC_GEOMETRY,
            INVALID_ROOT_SUPPORT,
            SEMANTIC_COLLISION,
            DISCONNECTED_GEOMETRY,
            FIRING_CLEARANCE,
            INVALID_MUZZLE_DIRECTION
        };

        struct CandidateWeapon
        {
            CandidateWeapon(uint32_t width, uint32_t height)
                : OccupiedMask(width, height, false),
                  RootMask(width, height, false),
                  BodyMask(width, height, false),
                  BarrelMask(width, height, false),
                  MuzzleMask(width, height, false),
                  MovableMask(width, height, false),
                  EmissiveMask(width, height, false)
            {
            }

            PixelMask OccupiedMask;
            PixelMask RootMask;
            PixelMask BodyMask;
            PixelMask BarrelMask;
            PixelMask MuzzleMask;
            PixelMask MovableMask;
            PixelMask EmissiveMask;
            WeaponPlacement Placement;
        };
    }
}
