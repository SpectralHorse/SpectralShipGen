#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

#include "LiveryData.h"
#include "ShipGenerationContext.h"

namespace PixelShipGenerator
{
    class LiveryGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        static constexpr std::size_t LiveryTypeCount = static_cast<std::size_t>(ShipLiveryType::SHIP_LIVERY_TYPE_END);

        uint32_t getTypeWeight(const ShipGenerationContext& context, ShipLiveryType type, bool secondary) const;
        ShipLiveryType selectType(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, const std::array<bool, LiveryTypeCount>& usedTypes, bool secondary) const;
        PixelMask buildMarkingMask(const ShipGenerationContext& context, ShipLiveryType type, std::mt19937_64& randomGenerator, bool& asymmetric) const;
        PixelMask buildCenterStripe(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, bool doubled) const;
        PixelMask buildWingBand(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const;
        PixelMask buildShoulderBlock(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const;
        PixelMask buildNoseBand(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const;
        PixelMask buildChevron(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const;
        PixelMask buildIdPanel(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, bool& asymmetric) const;
        PixelMask buildGeometricInsignia(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, bool& asymmetric) const;
        void sanitizeMask(const ShipGenerationContext& context, PixelMask& mask, const LiveryData& data) const;
        uint32_t getCoverageLimitPercent(const ShipGenerationContext& context, bool connected) const;
        uint32_t getMaximumSupportingPercent(const ShipGenerationContext& context) const;
        bool validateCoverage(const ShipGenerationContext& context, const PixelMask& mask, bool& materialPreservationFailure) const;
        bool addMarking(ShipGenerationContext& context, ShipLiveryType type, PixelMask mask, bool secondary, bool asymmetric) const;
        bool allowAsymmetricMarking(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const;
    };
}
