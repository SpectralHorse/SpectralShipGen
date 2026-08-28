#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "PixelMask.h"
#include "ShipGenerationContext.h"
#include "ShipGenerationProfile.h"
#include "SilhouetteQualityMetrics.h"

namespace PixelShipGenerator
{
    class HullGenerator
    {
    public:
        void generate(ShipGenerationContext& context) const;
        bool validate(const PixelMask& hullMask) const;
        bool validate(const ShipGenerationContext& context) const;

    private:
        struct HullMetrics
        {
            uint32_t PixelCount = 0u;
            uint32_t OccupiedHeight = 0u;
            uint32_t MinimumOccupiedWidth = 0u;
            uint32_t MaximumOccupiedWidth = 0u;
            uint32_t MinimumOccupiedHeight = 0u;
            uint32_t MaximumOccupiedHeight = 0u;
            uint32_t MaximumRowWidthDelta = 0u;
        };

        struct RowBounds
        {
            uint32_t MinX = 0u;
            uint32_t MaxX = 0u;
            bool Valid = false;
        };

        struct WingProfileDefinition
        {
            WingShapeType Shape = WingShapeType::NONE;
            uint32_t StartIndex = 0u;
            uint32_t RootEndIndex = 0u;
            uint32_t PeakIndex = 0u;
            uint32_t TaperIndex = 0u;
            uint32_t EndIndex = 0u;
            bool Valid = false;
        };

        void generateBaseHull(ShipGenerationContext& context) const;
        void cleanup(PixelMask& hullMask) const;
        void applySilhouetteModifiers(ShipGenerationContext& context) const;
        void applySilhouetteGuidance(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)>& selectedModifiers) const;
        void applyStructuralNegativeSpace(ShipGenerationContext& context) const;
        ShipStructuralNegativeSpaceType selectStructuralNegativeSpaceType(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END)>& selectedTypes) const;
        uint32_t getStructuralNegativeSpaceWeight(const ShipGenerationContext& context, ShipStructuralNegativeSpaceType type) const;
        bool tryApplyStructuralNegativeSpace(ShipGenerationContext& context, ShipStructuralNegativeSpaceType type) const;
        bool applyWingChannel(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const;
        bool applyRearFork(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const;
        bool applyShoulderGap(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const;
        bool applyOpenFrameBay(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const;
        bool applyNacelleChannel(ShipGenerationContext& context, PixelMask& hullMask, PixelMask& reservedMask) const;
        WingProfileDefinition applyWingProfile(ShipGenerationContext& context, WingShapeType wingShape, const ShipGenerationProfile& profile, const std::vector<uint32_t>& fuselageProfile, std::vector<uint32_t>& widthProfile, uint32_t maximumHalfWidth) const;
        void applyWingProfileStepping(std::vector<uint32_t>& widthProfile, uint32_t startIndex, uint32_t endIndex, uint32_t rowStep) const;
        void deriveWingRegions(ShipGenerationContext& context) const;
        void captureWingDebugInfo(ShipGenerationContext& context) const;
        bool validateWingRegions(const ShipGenerationContext& context) const;
        void captureDebugStage(const ShipGenerationContext& context, ShipGenerationDebugStageType type) const;
        HullModifierType getHullModifierType(ShipGenerationContext& context, const ShipGenerationProfile& profile, const std::array<bool, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)>& selectedModifiers) const;
        uint32_t getHullModifierWeight(const ShipGenerationProfile& profile, HullModifierType type) const;
        bool tryApplyHullModifier(ShipGenerationContext& context, HullModifierType type) const;
        bool applyBroaderShoulders(ShipGenerationContext& context, PixelMask& hullMask) const;
        bool applySideLobes(ShipGenerationContext& context, PixelMask& hullMask) const;
        bool applySteppedWingExtension(ShipGenerationContext& context, PixelMask& hullMask) const;
        bool applyNarrowWaist(ShipGenerationContext& context, PixelMask& hullMask) const;
        bool applyWingCutout(ShipGenerationContext& context, PixelMask& hullMask) const;
        bool applySplitNose(ShipGenerationContext& context, PixelMask& hullMask) const;
        bool isHullConnected(const PixelMask& hullMask) const;
        RowBounds calculateRowBounds(const PixelMask& hullMask, uint32_t y) const;
        bool extendRowEdges(PixelMask& hullMask, uint32_t y, uint32_t amount) const;
        bool removeRowEdgePixels(PixelMask& hullMask, uint32_t y, uint32_t amount, uint32_t minimumRemainingWidth) const;
        WingShapeType getWingStyle(ShipGenerationContext& context, const ShipGenerationProfile& profile) const;
        HullMetrics calculateHullMetrics(const PixelMask& hullMask) const;
        SilhouetteValidationFailureReason evaluateSilhouetteValidation(const ShipGenerationContext& context, const SilhouetteQualityMetrics& metrics) const;
        SilhouetteValidationFailureReason evaluateSilhouetteProfileAcceptance(const ShipGenerationContext& context, const SilhouetteQualityMetrics& metrics) const;
        uint32_t getScaleAdjustedArticulationTarget(const ShipGenerationContext& context) const;
        void fillWidthTransition(std::vector<uint32_t>& profile, uint32_t startIndex, uint32_t endIndex, uint32_t startWidth, uint32_t endWidth) const;
        void rasterizeSymmetricHull(PixelMask& mask, const std::vector<uint32_t>& profile, uint32_t topY) const;
    };
}
