#pragma once

#include <cstdint>
#include <vector>

#include <SpectralShipGen/PixelMask.h>
#include <SpectralShipGen/ShipAttachment.h>
#include <SpectralShipGen/ShipWeaponType.h>

namespace SpectralShipGen
{
    struct WeaponPlacement
    {
        ShipWeaponType Type = ShipWeaponType::SINGLE_CANNON;
        ShipWeaponHardpointRegion Region = ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE;
        ShipAttachmentDirection Direction = ShipAttachmentDirection::UP;
        uint32_t AnchorX = 0u;
        uint32_t AnchorY = 0u;
        uint32_t BodyMinX = 0u;
        uint32_t BodyMaxX = 0u;
        uint32_t BodyMinY = 0u;
        uint32_t BodyMaxY = 0u;
        uint32_t BarrelMinX = 0u;
        uint32_t BarrelMaxX = 0u;
        uint32_t BarrelMinY = 0u;
        uint32_t BarrelMaxY = 0u;
        uint32_t MuzzleX = 0u;
        uint32_t MuzzleY = 0u;
        uint32_t SymmetryGroup = 0u;
        bool MovableBarrel = false;
        bool Emissive = false;
    };

    struct WeaponData
    {
        void reset(uint32_t width, uint32_t height)
        {
            OccupiedMask = PixelMask(width, height, false);
            RootMask = PixelMask(width, height, false);
            BodyMask = PixelMask(width, height, false);
            BarrelMask = PixelMask(width, height, false);
            MuzzleMask = PixelMask(width, height, false);
            MovableMask = PixelMask(width, height, false);
            EmissiveMask = PixelMask(width, height, false);
            Placements.clear();
        }

        bool empty() const
        {
            return Placements.empty();
        }

        PixelMask OccupiedMask;
        PixelMask RootMask;
        PixelMask BodyMask;
        PixelMask BarrelMask;
        PixelMask MuzzleMask;
        PixelMask MovableMask;
        PixelMask EmissiveMask;
        std::vector<WeaponPlacement> Placements;
    };
}
