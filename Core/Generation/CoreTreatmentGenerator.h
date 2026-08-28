#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "CoreTreatmentData.h"
#include "ShipGenerationContext.h"

namespace PixelShipGenerator
{
    class CoreTreatmentGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct Candidate
        {
            Candidate(uint32_t width, uint32_t height)
                : Raised(width, height, false), Recessed(width, height, false), Secondary(width, height, false), Luminous(width, height, false) {}

            ShipCoreTreatmentType Type = ShipCoreTreatmentType::CENTRAL_SPINE;
            PixelMask Raised;
            PixelMask Recessed;
            PixelMask Secondary;
            PixelMask Luminous;
        };

        void deriveCoreRegion(ShipGenerationContext& context) const;
        ShipCoreTreatmentType selectTreatmentType(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)>& used) const;
        bool generateCandidate(ShipGenerationContext& context, ShipCoreTreatmentType type, Candidate& candidate) const;
        bool generateCentralSpine(ShipGenerationContext& context, Candidate& candidate) const;
        bool generateCockpitSurround(ShipGenerationContext& context, Candidate& candidate) const;
        bool generateRaisedCorePlate(ShipGenerationContext& context, Candidate& candidate) const;
        bool generateLateralRecesses(ShipGenerationContext& context, Candidate& candidate) const;
        bool generateLongitudinalArmorBand(ShipGenerationContext& context, Candidate& candidate) const;
        bool generateCoreChannel(ShipGenerationContext& context, Candidate& candidate) const;
        bool validateCandidate(const ShipGenerationContext& context, const Candidate& candidate) const;
        void commitCandidate(ShipGenerationContext& context, const Candidate& candidate, uint32_t cost, bool dominant) const;
        bool addSymmetricPixel(const ShipGenerationContext& context, PixelMask& mask, int32_t x, int32_t y) const;
        bool addPixel(const ShipGenerationContext& context, PixelMask& mask, int32_t x, int32_t y) const;
        uint32_t getTreatmentCost(ShipCoreTreatmentType type) const;
        bool isDominantTreatment(ShipCoreTreatmentType type) const;
        uint32_t getTreatmentWeight(ShipStyle style, ShipFactionType faction, ShipCoreTreatmentType type) const;
        void rebuildBoundaryMask(CoreTreatmentData& data) const;
    };
}
