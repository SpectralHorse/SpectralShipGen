#pragma once

#include <cstdint>
#include <vector>

#include <PixelShipGenerator/GenerationSpatialBudget.h>
#include "ShipGenerationContext.h"
#include "WeaponGenerationInternal.h"

namespace PixelShipGenerator
{
    namespace WeaponGenerationInternal
    {
        class WeaponHardpointPlanner
        {
        public:
            std::vector<WeaponHardpoint> discoverHardpoints(const ShipGenerationContext& context) const;
            const WeaponHardpoint* selectHardpoint(ShipGenerationContext& context, const std::vector<WeaponHardpoint>& hardpoints, bool plannedAsymmetry) const;
            bool hardpointMatchesMacroAsymmetryPlan(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const;
            GenerationSpatialRegion getSpatialRegion(ShipWeaponHardpointRegion region, uint32_t x, uint32_t width) const;

        private:
            void addSymmetricSideHardpoints(const ShipGenerationContext& context, std::vector<WeaponHardpoint>& hardpoints, uint32_t y, uint32_t leftX, uint32_t rightX, ShipWeaponHardpointRegion region) const;
            bool isHardpointSupported(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const;
            uint32_t getHardpointFeasibilityPercent(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const;
        };
    }
}
