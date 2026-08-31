#pragma once

#include <cstdint>
#include <random>

#include "MaterialCompositionData.h"
#include "ShipGenerationContext.h"

namespace SpectralShipGen
{
    class MaterialCompositionGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        uint32_t getRandomUInt(std::mt19937_64& randomGenerator, uint32_t minimum, uint32_t maximum) const;
        uint32_t getZoneWeight(const ShipGenerationContext& context, ShipMaterialZoneType type) const;
        ShipMaterialZoneType selectZoneType(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, const std::array<bool, static_cast<std::size_t>(ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END)>& usedTypes) const;
        PixelMask buildZoneMask(const ShipGenerationContext& context, ShipMaterialZoneType type) const;
        PixelMask buildWingSurfaceMask(const ShipGenerationContext& context) const;
        PixelMask buildShoulderSurfaceMask(const ShipGenerationContext& context) const;
        PixelMask buildAxialBandMask(const ShipGenerationContext& context) const;
        PixelMask buildRearMechanicalMask(const ShipGenerationContext& context) const;
        PixelMask buildCockpitCollarMask(const ShipGenerationContext& context) const;
        PixelMask buildHardpointSurroundMask(const ShipGenerationContext& context) const;
        void sanitizeZoneMask(const ShipGenerationContext& context, PixelMask& mask, const MaterialCompositionData& data) const;
        bool addZone(ShipGenerationContext& context, ShipMaterialZoneType type, PixelMask mask) const;
        bool isMechanicalZone(ShipMaterialZoneType type) const;
    };
}
