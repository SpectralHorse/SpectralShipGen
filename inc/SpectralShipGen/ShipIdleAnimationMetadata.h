#pragma once

#include <cstdint>
#include <vector>

#include <SpectralShipGen/PixelMask.h>
#include <SpectralShipGen/ShipAttachment.h>
#include <SpectralShipGen/ShipMajorFeatureType.h>
#include <SpectralShipGen/ShipWeaponType.h>

namespace SpectralShipGen
{
    struct ShipEngineAnimationComponent
    {
        uint32_t HousingStartX = 0u;
        uint32_t HousingWidth = 0u;
        uint32_t NozzleStartX = 0u;
        uint32_t NozzleWidth = 0u;
        uint32_t RootStartY = 0u;
        uint32_t NozzleY = 0u;
        uint32_t ExhaustStartY = 0u;
        uint32_t ExhaustLength = 0u;
        uint32_t MinimumExhaustLength = 0u;
        uint32_t MaximumExhaustLength = 0u;
        uint32_t TaperMode = 0u;
        bool DominantEngine = false;
        bool Nacelle = false;
    };

    struct ShipWeaponAnimationComponent
    {
        ShipWeaponType Type = ShipWeaponType::SINGLE_CANNON;
        ShipWeaponHardpointRegion Region = ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE;
        ShipAttachmentDirection Direction = ShipAttachmentDirection::UP;
        uint32_t AnchorX = 0u;
        uint32_t AnchorY = 0u;
        uint32_t MinimumX = 0u;
        uint32_t MaximumX = 0u;
        uint32_t MinimumY = 0u;
        uint32_t MaximumY = 0u;
        uint32_t SymmetryGroup = 0u;
        bool MovableBarrel = false;
        bool Emissive = false;
    };

    struct ShipMajorFeatureAnimationComponent
    {
        ShipMajorFeatureType Type = ShipMajorFeatureType::CENTRAL_SPINE;
        uint32_t MinimumX = 0u;
        uint32_t MaximumX = 0u;
        uint32_t MinimumY = 0u;
        uint32_t MaximumY = 0u;
        bool Symmetric = true;
    };

    struct ShipIdleAnimationMetadata
    {
        void reset(uint32_t width, uint32_t height)
        {
            WeaponOccupiedMask.reset(width, height, false);
            WeaponMovableMask.reset(width, height, false);
            WeaponMuzzleMask.reset(width, height, false);
            WeaponEmissiveMask.reset(width, height, false);
            MajorFeatureMechanicalMask.reset(width, height, false);
            MajorFeatureEmissiveMask.reset(width, height, false);
            EngineComponents.clear();
            WeaponComponents.clear();
            MajorFeatureComponents.clear();
        }

        void clear()
        {
            WeaponOccupiedMask.clear(false);
            WeaponMovableMask.clear(false);
            WeaponMuzzleMask.clear(false);
            WeaponEmissiveMask.clear(false);
            MajorFeatureMechanicalMask.clear(false);
            MajorFeatureEmissiveMask.clear(false);
            EngineComponents.clear();
            WeaponComponents.clear();
            MajorFeatureComponents.clear();
        }

        PixelMask WeaponOccupiedMask;
        PixelMask WeaponMovableMask;
        PixelMask WeaponMuzzleMask;
        PixelMask WeaponEmissiveMask;
        PixelMask MajorFeatureMechanicalMask;
        PixelMask MajorFeatureEmissiveMask;
        std::vector<ShipEngineAnimationComponent> EngineComponents;
        std::vector<ShipWeaponAnimationComponent> WeaponComponents;
        std::vector<ShipMajorFeatureAnimationComponent> MajorFeatureComponents;
    };
}
