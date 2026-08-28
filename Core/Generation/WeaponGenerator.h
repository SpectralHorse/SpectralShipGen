#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "PixelMask.h"
#include "ShipGenerationContext.h"
#include "ShipWeaponType.h"
#include "WeaponData.h"

namespace PixelShipGenerator
{
    class WeaponGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct WeaponHardpoint
        {
            uint32_t X = 0u;
            uint32_t Y = 0u;
            ShipWeaponHardpointRegion Region = ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE;
            ShipAttachmentDirection Direction = ShipAttachmentDirection::UP;
            bool PairCapable = false;
        };

        struct FactionWeaponProfile
        {
            uint32_t ChancePercent = 100u;
            int32_t SymmetryChanceOffset = 0;
            std::array<uint32_t, static_cast<std::size_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END)> WeightMultipliers = {};
            uint32_t EmissiveChance = 0u;
        };

        struct CandidateWeapon
        {
            CandidateWeapon(uint32_t width, uint32_t height) : OccupiedMask(width, height, false), RootMask(width, height, false), BodyMask(width, height, false), BarrelMask(width, height, false), MuzzleMask(width, height, false), MovableMask(width, height, false), EmissiveMask(width, height, false) {}

            PixelMask OccupiedMask;
            PixelMask RootMask;
            PixelMask BodyMask;
            PixelMask BarrelMask;
            PixelMask MuzzleMask;
            PixelMask MovableMask;
            PixelMask EmissiveMask;
            WeaponPlacement Placement;
        };

        std::vector<WeaponHardpoint> discoverHardpoints(const ShipGenerationContext& context) const;
        void addSymmetricSideHardpoints(const ShipGenerationContext& context, std::vector<WeaponHardpoint>& hardpoints, uint32_t y, uint32_t leftX, uint32_t rightX, ShipWeaponHardpointRegion region) const;
        bool isHardpointSupported(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const;
        bool hardpointMatchesMacroAsymmetryPlan(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const;
        ShipWeaponType selectWeaponType(ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, ShipWeaponHardpointRegion region, bool allowForcedType) const;
        uint32_t getStyleWeaponWeight(const ShipGenerationProfile& profile, ShipWeaponType type) const;
        bool isWeaponTypeAllowedAtHardpoint(ShipWeaponType type, ShipWeaponHardpointRegion region) const;
        bool generateCandidate(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, ShipWeaponType type, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
        bool generateSingleCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
        bool generateTwinCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
        bool generateCompactTurret(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
        bool generateRailWeapon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
        bool generateWeaponPod(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const;
        bool buildRoot(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, uint32_t halfWidth, uint32_t depth, CandidateWeapon& candidate) const;
        bool addCandidateRectangle(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t startX, int32_t startY, uint32_t width, uint32_t height) const;
        bool addCandidatePixel(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t x, int32_t y) const;
        bool validateCandidate(const ShipGenerationContext& context, const CandidateWeapon& candidate) const;
        bool validateConnected(const CandidateWeapon& candidate) const;
        bool validateFiringClearance(const ShipGenerationContext& context, const CandidateWeapon& candidate) const;
        void mirrorCandidate(const CandidateWeapon& source, CandidateWeapon& destination, uint32_t imageWidth) const;
        bool validateSymmetricPair(const ShipGenerationContext& context, const CandidateWeapon& first, const CandidateWeapon& second) const;
        void commitCandidate(ShipGenerationContext& context, CandidateWeapon& candidate, uint32_t symmetryGroup) const;
        FactionWeaponProfile getFactionWeaponProfile(ShipFactionType faction) const;
        uint32_t getGenerationChance(const ShipGenerationContext& context, const FactionWeaponProfile& factionProfile) const;
        uint32_t getMaximumWeaponGroups(const ShipGenerationContext& context) const;
        uint32_t getSymmetryChance(const ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, const WeaponHardpoint& hardpoint) const;
        uint32_t scaleWeaponPixelsFrom64(uint32_t value, uint32_t dimension, uint32_t scalePercent) const;

        static constexpr uint32_t MaximumWeaponPlacementAttempts = 18u;
    };
}
