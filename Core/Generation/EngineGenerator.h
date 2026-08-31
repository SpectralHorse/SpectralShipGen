#pragma once

#include <cstdint>
#include <vector>

#include "ShipGenerationContext.h"
#include <PixelShipGenerator/ShipGenerationDebugInfo.h>
#include <PixelShipGenerator/ShipGenerationProfile.h>

namespace PixelShipGenerator
{
    class EngineGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;

    private:
        struct EnginePlacement
        {
            uint32_t StartX = 0u;
            uint32_t HousingWidth = 0u;
            uint32_t NozzleWidth = 0u;
            EngineSizeClass SizeClass = EngineSizeClass::SMALL;
            bool Nacelle = false;
        };

        struct RearSupportRun
        {
            uint32_t StartX = 0u;
            uint32_t Width = 0u;
        };

        EngineLayoutType getEngineLayout(ShipGenerationContext& context, const ShipGenerationProfile& profile, uint32_t rearWidth, uint32_t imageWidth) const;
        EngineSizeClass getEngineSizeClass(ShipGenerationContext& context, const ShipGenerationProfile& profile, EngineLayoutType layout, uint32_t rearWidth, uint32_t availableRearSpace) const;
        bool createEnginePlacements(EngineLayoutType layout, EngineSizeClass sizeClass, uint32_t rearStartX, uint32_t rearWidth, uint32_t imageWidth, std::vector<EnginePlacement>& placements) const;
        void collectRearSupportRuns(const PixelMask& hullMask, uint32_t y, std::vector<RearSupportRun>& runs) const;
        bool createSeparatedRearPlacements(const std::vector<RearSupportRun>& runs, EngineLayoutType layout, EngineSizeClass sizeClass, uint32_t imageWidth, std::vector<EnginePlacement>& placements) const;
        uint32_t getHousingWidth(uint32_t rearWidth, EngineLayoutType layout, EngineSizeClass sizeClass) const;
        uint32_t getNozzleWidth(uint32_t housingWidth, EngineSizeClass sizeClass) const;
        uint32_t getDesiredRootDepth(EngineSizeClass sizeClass, uint32_t imageHeight) const;
        uint32_t getDesiredExternalHeight(EngineSizeClass sizeClass, uint32_t imageHeight) const;
        uint32_t getDesiredMaximumExhaustLength(EngineSizeClass sizeClass, uint32_t imageHeight) const;
        bool addEngineAssembly(PixelMask& engineMask, const PixelMask& hullMask, const PixelMask& cockpitMask, const EnginePlacement& placement, uint32_t hullBottom, uint32_t rootDepth, uint32_t externalHeight) const;
        bool addTaperedExhaust(PixelMask& exhaustMask, const EnginePlacement& placement, uint32_t exhaustStartY, uint32_t exhaustLength, uint32_t taperMode) const;
        bool validateGeneratedEngines(const PixelMask& engineMask, const PixelMask& exhaustMask, const PixelMask& hullMask, const PixelMask& cockpitMask) const;
        void addCenteredMaskRow(PixelMask& mask, uint32_t centerXTimesTwo, uint32_t width, uint32_t y) const;
    };
}
