
#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "GeneratedShip.h"
#include "PixelMask.h"
#include "ShipGenerationContext.h"
#include "ShipSurfaceDetailProfile.h"

namespace PixelShipGenerator
{
    class DetailGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        void planDetailMotifs(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile, std::mt19937_64& randomGenerator) const;
        void generatePlannedMotifs(ShipGenerationContext& context, std::mt19937_64& randomGenerator) const;
        ShipDetailMotifType selectDetailMotif(const ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile, std::mt19937_64& randomGenerator, ShipDetailMotifType excluded) const;
        GenerationSpatialRegion getPreferredMotifRegion(const ShipGenerationContext& context, ShipDetailMotifType type, bool secondary, std::mt19937_64& randomGenerator) const;
        bool tryGenerateMotifOccurrence(ShipGenerationContext& context, ShipDetailMotifType type, GenerationSpatialRegion preferredRegion, bool secondary, std::mt19937_64& randomGenerator) const;
        bool trySelectMotifAnchor(ShipGenerationContext& context, ShipDetailMotifType type, GenerationSpatialRegion preferredRegion, std::mt19937_64& randomGenerator, uint32_t& x, uint32_t& y) const;
        bool buildMotifPixels(ShipGenerationContext& context, ShipDetailMotifType type, uint32_t x, uint32_t y, bool mirrored, std::mt19937_64& randomGenerator, std::vector<std::pair<uint32_t, uint32_t>>& pixels) const;
        bool areMotifPixelsMaterialCoherent(const ShipGenerationContext& context, const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const;
        bool isPreferredMotifMaterial(const ShipGenerationContext& context, ShipDetailMotifType type, uint32_t x, uint32_t y) const;
        uint32_t getMotifComplexityCost(ShipDetailMotifType type) const;
        uint32_t getFreeformDetailPercent(const ShipGenerationContext& context) const;

        void generateAccentDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const;
        void generateMechanicalDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const;
        void generateLightDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const;
        void generateSupplementalSurfaceDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const;
        bool generatePanelSeamDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const;
        bool generateGeometricMarkingDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const;
        bool generateMechanicalExposureDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const;
        bool generateRepeatingMotifDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const;
        bool generateIdentificationMarkingDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const;
        bool generateLuminousChannelDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const;
        SupplementalSurfaceDetailType getSupplementalSurfaceDetailType(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const;
        bool tryAddSymmetricDetailRectangle(ShipGenerationContext& context, PixelMask& targetMask, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, uint32_t spatialCost = 2u, bool allowHullLayerBoundary = false) const;
        bool tryAddDetailRectangle(ShipGenerationContext& context, PixelMask& targetMask, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, uint32_t spatialCost = 2u, bool allowHullLayerBoundary = false) const;
        bool isDetailRectangleAvailable(const ShipGenerationContext& context, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, bool allowHullLayerBoundary = false) const;
        ResolvedSurfaceDetailProfile resolveSurfaceDetailProfile(const ShipGenerationConfiguration& settings, const ShipGenerationProfile& styleProfile, const ShipFactionSurfaceDetailProfile& factionProfile) const;
        uint32_t composeSurfaceDetailPercent(uint32_t baseValue, uint32_t multiplierPercent) const;
        uint32_t composeSurfaceDetailChance(uint32_t baseChance, uint32_t multiplierPercent) const;
        bool isSurfaceDetailPixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y, bool allowHullLayerBoundary = false) const;
        bool areSurfaceDetailPixelsAvailable(const ShipGenerationContext& context, const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const;
        void commitSurfaceDetailPixels(PixelMask& mask, const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const;
        bool addSurfaceDetailCandidatePixel(const ShipGenerationContext& context, std::vector<std::pair<uint32_t, uint32_t>>& pixels, int32_t x, int32_t y, bool mirrored) const;
        bool trySelectDetailAnchorFromMask(ShipGenerationContext& context, const PixelMask& mask, uint32_t& x, uint32_t& y) const;

        static constexpr uint32_t MaximumDetailPlacementAttempts = 16u;
    };
}
