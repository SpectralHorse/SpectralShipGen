#pragma once

#include <cstdint>

#include "ShipGenerationContext.h"
#include "WeaponGenerationInternal.h"

namespace PixelShipGenerator
{
    namespace WeaponGenerationInternal
    {
        class WeaponCandidateBuilder
        {
        public:
            bool generateCandidate(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, ShipWeaponType type, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
            void mirrorCandidate(const CandidateWeapon& source, CandidateWeapon& destination, uint32_t imageWidth) const;

        private:
            bool generateSingleCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
            bool generateTwinCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
            bool generateCompactTurret(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
            bool generateRailWeapon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
            bool generateWeaponPod(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
            bool buildRoot(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, uint32_t halfWidth, uint32_t depth, CandidateWeapon& candidate) const;
            bool addCandidateRectangle(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t startX, int32_t startY, uint32_t width, uint32_t height) const;
            bool addCandidatePixel(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t x, int32_t y) const;
            uint32_t getAssemblyScalePercent(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const;
            uint32_t scaleWeaponPixelsFrom64(uint32_t value, uint32_t dimension, uint32_t scalePercent) const;
        };
    }
}
