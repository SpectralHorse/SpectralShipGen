#pragma once

#include <cstdint>

#include "Color.h"
#include "GeneratedShip.h"
#include "PixelMask.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include "ShipPalette.h"

namespace PixelShipGenerator
{
    class ShipPainter
    {
    public:
        void paint(ShipGenerationContext& context) const;
        void paintIdleWeaponLayer(Image& image, const GeneratedShip& ship, const PixelMask& occupiedMask, const PixelMask& movableMask, const PixelMask& muzzleMask, const PixelMask& emissiveMask, const PixelMask& affectedMask) const;

    private:
        void paintHull(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette, const ShipGenerationProfile& profile) const;
        void paintMaterialComposition(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintCoreTreatment(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintHullLayers(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintMajorFeatures(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintWeapons(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintAttachments(GeneratedShip& ship, const ShipPalette& palette, const GenerationScaleTraits& scaleTraits) const;
        void paintLivery(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintCockpit(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette, const GenerationScaleTraits& scaleTraits) const;
        void paintEngines(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette, const GenerationScaleTraits& scaleTraits) const;
        void paintDetails(GeneratedShip& ship, const ShipPalette& palette) const;
        void paintComponentDepthReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintSemanticContactShadow(const ShipGenerationContext& context, GeneratedShip& ship, const PixelMask& elevatedMask, bool strongShadow) const;
        void paintWeaponMountReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintAttachmentMountReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintCockpitReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintEngineMountReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintWingRootReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const;
        void paintLowerRightContactShadow(GeneratedShip& ship, const PixelMask& elevatedMask, const PixelMask& supportMask, const Color& shadowColor, uint32_t shadowDistance) const;

        bool isShipStructurePixel(const GeneratedShip& ship, int32_t x, int32_t y) const;
        bool hasNeighbouringShipStructurePixel(const GeneratedShip& ship, int32_t x, int32_t y) const;
        uint32_t getShipStructureNeighbourCount(const GeneratedShip& ship, int32_t x, int32_t y) const;
        bool isWingInnerBoundaryPixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        bool isFuselageWingBoundaryPixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        bool isCentralRidgePixel(const ShipGenerationContext& context, uint32_t x, uint32_t y, uint32_t bandWidth) const;

        Color getDirectionalStructureColor(const GeneratedShip& ship, uint32_t x, uint32_t y, const Color& deepShadow, const Color& shadow, const Color& base, const Color& highlight, const Color& edgeHighlight) const;
        Color getDirectionalMaskColor(const PixelMask& mask, uint32_t x, uint32_t y, const Color& deepShadow, const Color& shadow, const Color& base, const Color& highlight, const Color& edgeHighlight) const;
        Color getRaisedMaskColor(const PixelMask& mask, uint32_t x, uint32_t y, const Color& interiorColor, const ShipPalette& palette, bool strongBevel) const;
        Color getRecessedMaskColor(const PixelMask& mask, uint32_t x, uint32_t y, const Color& interiorColor, const ShipPalette& palette, bool reflectedEdge) const;

        const MajorFeaturePlacement* getMajorFeaturePlacementForPixel(const MajorFeatureData& features, uint32_t x, uint32_t y) const;
        const ShipAttachmentPlacement* getAttachmentPlacementForPixel(const GeneratedShip& ship, uint32_t x, uint32_t y) const;
        uint32_t getAttachmentOutwardDistance(const ShipAttachmentPlacement& placement, uint32_t x, uint32_t y) const;
        uint32_t getAttachmentMaximumOutwardDistance(const ShipAttachmentPlacement& placement) const;
        int32_t getAttachmentTangentCoordinate(const ShipAttachmentPlacement& placement, uint32_t x, uint32_t y) const;

        bool isSecondaryHullTonePixel(const GeneratedShip& ship, uint32_t x, uint32_t y, const ShipGenerationProfile& profile) const;
        uint64_t getDeterministicPaintHash(uint64_t seed, uint32_t x, uint32_t y, uint64_t salt) const;
        uint32_t getColorLuma(const Color& color) const;
        bool isDepthSupportHullPixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const;
        Color getDepthShadowColor(const Color& currentColor, const ShipPalette& palette, bool strongShadow) const;

        Color getHullPixelColor(const ShipGenerationContext& context, const GeneratedShip& ship, uint32_t x, uint32_t y, const ShipPalette& palette, const ShipGenerationProfile& profile) const;
        Color getAttachmentPixelColor(const GeneratedShip& ship, const ShipAttachmentPlacement& placement, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity) const;
        Color getLegacyCockpitPixelColor(const PixelMask& cockpitMask, const PixelMaskUtils::MaskBounds& bounds, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity) const;
        Color getCockpitGlassPixelColor(const CockpitData& cockpit, const PixelMaskUtils::MaskBounds& bounds, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity) const;
        Color getEnginePixelColor(const GeneratedShip& ship, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity, const ShipFactionFinishProfile& finishProfile) const;
        Color getMechanicalPixelColor(const PixelMask& mechanicalMask, uint32_t x, uint32_t y, const ShipPalette& palette) const;
        Color getAccentPixelColor(const PixelMask& accentMask, uint32_t x, uint32_t y, const ShipPalette& palette) const;
        Color getLightPixelColor(const PixelMask& lightMask, uint32_t x, uint32_t y, const ShipPalette& palette) const;
    };
}
