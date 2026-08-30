
#pragma once

#include <cstddef>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "GeneratedShip.h"
#include "PixelMask.h"
#include "ShipGenerationContext.h"

namespace PixelShipGenerator
{
    class AttachmentGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct AttachmentAnchor
        {
            uint32_t X = 0u;
            uint32_t Y = 0u;
            ShipAttachmentRegion Region = ShipAttachmentRegion::MIDDLE_SIDE;
            ShipAttachmentDirection Direction = ShipAttachmentDirection::LEFT;
        };

        struct ResolvedAttachmentProfile
        {
            std::array<uint64_t, static_cast<std::size_t>(ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END)> TypeWeights = {};
            uint32_t AttachmentChance = 0u;
            uint32_t MaximumAttachmentGroups = 0u;
            uint32_t SymmetricAttachmentChance = 0u;
            uint32_t AttachmentSizePercent = 100u;
        };

        std::vector<AttachmentAnchor> discoverAttachmentAnchors(const ShipGenerationContext& context) const;
        ShipAttachmentRegion getSideAttachmentRegion(const ShipGenerationContext& context, uint32_t x, uint32_t y, uint32_t hullTop, uint32_t hullHeight, uint32_t rowWidth, uint32_t maximumRowWidth) const;
        bool isAttachmentAnchorValid(const ShipGenerationContext& context, const AttachmentAnchor& anchor) const;
        bool anchorMatchesMacroAsymmetryPlan(const ShipGenerationContext& context, const AttachmentAnchor& anchor) const;
        ShipAttachmentType getMacroAsymmetryAttachmentType(const ResolvedAttachmentProfile& profile, ShipAttachmentRegion region) const;
        ShipAttachmentType getAttachmentType(ShipGenerationContext& context, const ResolvedAttachmentProfile& profile, ShipAttachmentRegion region, bool allowForcedType) const;
        bool generateAttachmentCandidate(ShipGenerationContext& context, const GeneratedShip& ship, const AttachmentAnchor& anchor, ShipAttachmentType type, uint32_t sizePercent, PixelMask& candidateMask, ShipAttachmentPlacement& placement) const;
        bool validateAttachmentCandidate(const ShipGenerationContext& context, const PixelMask& candidateMask, const AttachmentAnchor& anchor) const;
        void mirrorAttachmentCandidate(const PixelMask& sourceMask, PixelMask& destinationMask) const;
        ShipAttachmentPlacement mirrorAttachmentPlacement(const ShipAttachmentPlacement& placement, uint32_t imageWidth) const;
        std::pair<int32_t, int32_t> getAttachmentDirectionOffset(ShipAttachmentDirection direction) const;
        ShipAttachmentDirection getMirroredAttachmentDirection(ShipAttachmentDirection direction) const;
        bool isAttachmentTypeAllowedInRegion(ShipAttachmentType type, ShipAttachmentRegion region) const;
        uint32_t getAttachmentSymmetryChance(const ResolvedAttachmentProfile& profile, ShipAttachmentType type) const;
        bool generateWeaponMountAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const;
        bool generateSensorArrayAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const;
        bool generateAuxiliaryPodAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const;
        bool generateRadiatorAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const;
        bool generateArmorFinAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const;
        bool generateTechnologyNodeAttachment(ShipGenerationContext& context, const AttachmentAnchor& anchor, uint32_t sizePercent, PixelMask& candidateMask) const;
        bool addAttachmentLocalRectangle(PixelMask& mask, const AttachmentAnchor& anchor, uint32_t outwardStart, uint32_t outwardLength, int32_t tangentStart, uint32_t tangentSize) const;
        bool setAttachmentLocalPixel(PixelMask& mask, const AttachmentAnchor& anchor, uint32_t outwardDistance, int32_t tangentOffset, bool value = true) const;
        std::pair<int32_t, int32_t> getAttachmentTangentOffset(ShipAttachmentDirection direction) const;
        ResolvedAttachmentProfile resolveAttachmentProfile(const ShipGenerationProfile& styleProfile, const ShipFactionAttachmentProfile& factionProfile) const;
        uint32_t scaleAttachmentPixelsFrom64(uint32_t value, uint32_t dimension, uint32_t sizePercent) const;

        static constexpr uint32_t MaximumAttachmentPlacementAttempts = 24u;
    };
}
