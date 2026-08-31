#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "MajorFeatureData.h"
#include <SpectralShipGen/PixelMask.h>
#include "ShipGenerationContext.h"

namespace SpectralShipGen
{
    class MajorFeatureGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct CandidateFeature
        {
            CandidateFeature(uint32_t width, uint32_t height) : OccupiedMask(width, height, false), RaisedMask(width, height, false), RecessedMask(width, height, false), MechanicalMask(width, height, false), EmissiveMask(width, height, false) {}

            ShipMajorFeatureType Type = ShipMajorFeatureType::CENTRAL_SPINE;
            MajorFeatureRegion Region = MajorFeatureRegion::CENTRAL_FUSELAGE;
            PixelMask OccupiedMask;
            PixelMask RaisedMask;
            PixelMask RecessedMask;
            PixelMask MechanicalMask;
            PixelMask EmissiveMask;
            bool Symmetric = true;
        };

        struct FactionMajorFeatureProfile
        {
            uint32_t ChancePercent = 100u;
            std::array<uint32_t, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> WeightMultipliers = {};
        };

        ShipMajorFeatureType selectFeatureType(ShipGenerationContext& context, const FactionMajorFeatureProfile& factionProfile, const std::array<bool, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)>& usedTypes) const;
        bool generateCandidate(ShipGenerationContext& context, ShipMajorFeatureType type, CandidateFeature& candidate) const;
        bool generateCentralSpine(ShipGenerationContext& context, CandidateFeature& candidate) const;
        bool generateArmorPlate(ShipGenerationContext& context, CandidateFeature& candidate) const;
        bool generateRecessedBay(ShipGenerationContext& context, CandidateFeature& candidate) const;
        bool generateVentBank(ShipGenerationContext& context, CandidateFeature& candidate) const;
        bool generateWingPlate(ShipGenerationContext& context, CandidateFeature& candidate) const;
        bool generateTechCore(ShipGenerationContext& context, CandidateFeature& candidate) const;
        bool validateCandidate(const ShipGenerationContext& context, const CandidateFeature& candidate) const;
        void commitCandidate(ShipGenerationContext& context, const CandidateFeature& candidate) const;
        bool addCandidatePixel(const ShipGenerationContext& context, CandidateFeature& candidate, uint32_t x, uint32_t y, PixelMask& layerMask, bool reserveOnly = false) const;
        bool addSymmetricCandidatePixel(const ShipGenerationContext& context, CandidateFeature& candidate, uint32_t x, uint32_t y, PixelMask& layerMask, bool reserveOnly = false) const;
        bool addCenteredRow(const ShipGenerationContext& context, CandidateFeature& candidate, uint32_t y, uint32_t halfWidth, PixelMask& layerMask) const;
        bool isFuselagePixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        bool isFeaturePixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        bool isWingFeaturePixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        uint32_t scaleFeaturePixels(uint32_t value, uint32_t dimension, uint32_t scalePercent) const;
        FactionMajorFeatureProfile resolveFactionProfile(const ShipFactionMajorFeatureProfile& source) const;
        uint32_t getStyleFeatureWeight(const ShipGenerationProfile& profile, ShipMajorFeatureType type) const;

        static constexpr uint32_t MaximumFeaturePlacementAttempts = 18u;
    };
}
