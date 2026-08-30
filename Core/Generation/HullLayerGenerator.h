#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "HullLayerData.h"
#include "ShipGenerationContext.h"

namespace PixelShipGenerator
{
    class HullLayerGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct CandidateLayer
        {
            CandidateLayer(uint32_t width, uint32_t height) : Mask(width, height, false) {}

            ShipHullLayerType Type = ShipHullLayerType::CENTRAL_DORSAL_PLATE;
            uint32_t Order = 0u;
            PixelMask Mask;
        };

        ShipHullLayerType selectLayerType(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END)>& usedTypes) const;
        bool generateCandidate(ShipGenerationContext& context, ShipHullLayerType type, CandidateLayer& candidate) const;
        bool generateCentralDorsalPlate(ShipGenerationContext& context, CandidateLayer& candidate) const;
        bool generateForwardArmor(ShipGenerationContext& context, CandidateLayer& candidate) const;
        bool generateWingArmor(ShipGenerationContext& context, CandidateLayer& candidate) const;
        bool generateShoulderArmor(ShipGenerationContext& context, CandidateLayer& candidate) const;
        bool generateRearEngineCover(ShipGenerationContext& context, CandidateLayer& candidate) const;
        bool validateCandidate(const ShipGenerationContext& context, const CandidateLayer& candidate) const;
        ShipHullLayerType getPlannedAsymmetricLayerType(const ShipGenerationContext& context) const;
        void restrictCandidateToMacroSide(const ShipGenerationContext& context, CandidateLayer& candidate) const;
        void commitCandidate(ShipGenerationContext& context, const CandidateLayer& candidate) const;
        bool isLayerPixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        bool addPixel(const ShipGenerationContext& context, CandidateLayer& candidate, uint32_t x, uint32_t y) const;
        bool addSymmetricPixel(const ShipGenerationContext& context, CandidateLayer& candidate, uint32_t x, uint32_t y) const;
        uint32_t getLayerComplexityCost(ShipHullLayerType type) const;
        bool isDominantLayer(ShipHullLayerType type) const;
        uint32_t getLayerOrder(ShipHullLayerType type) const;
        uint32_t getLayerWeight(const ShipGenerationProfile& profile, const ShipFactionHullLayerProfile& factionProfile, ShipHullLayerType type) const;
    };
}
