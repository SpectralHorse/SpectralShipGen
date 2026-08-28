#pragma once

#include <cstdint>

#include "ShipFactionType.h"
#include "ShipDetailMotifType.h"

namespace PixelShipGenerator
{
    enum class SupplementalSurfaceDetailType : uint32_t
    {
        PANEL_SEAM = 0u,
        GEOMETRIC_MARKING,
        MECHANICAL_EXPOSURE,
        REPEATING_MOTIF,
        IDENTIFICATION_MARKING,
        LUMINOUS_CHANNEL,

        SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END
    };

    struct SupplementalSurfaceDetailWeights
    {
        uint32_t PanelSeam = 100u;
        uint32_t GeometricMarking = 100u;
        uint32_t MechanicalExposure = 100u;
        uint32_t RepeatingMotif = 100u;
        // These Task-55 languages are opt-in through faction profiles so
        // the four original factions preserve their previous distributions.
        uint32_t IdentificationMarking = 0u;
        uint32_t LuminousChannel = 0u;

        uint32_t getWeight(SupplementalSurfaceDetailType type) const;
    };


    struct ShipDetailMotifWeights
    {
        uint32_t PairedVents = 100u;
        uint32_t TripleVentBank = 100u;
        uint32_t PairedLights = 100u;
        uint32_t ThreeNodeLights = 100u;
        uint32_t ParallelSeams = 100u;
        uint32_t RepeatedDashes = 100u;
        uint32_t RecessedSlot = 100u;

        uint32_t getWeight(ShipDetailMotifType type) const;
    };

    struct ShipFactionSurfaceDetailProfile
    {
        uint32_t DetailDensityPercent = 100u;
        uint32_t MechanicalPatternCountPercent = 100u;
        uint32_t LightPatternCountPercent = 100u;
        uint32_t AccentPanelWeightPercent = 100u;
        uint32_t AccentStripeWeightPercent = 100u;
        uint32_t AccentArmorWeightPercent = 100u;
        uint32_t HorizontalVentChancePercent = 100u;
        SupplementalSurfaceDetailWeights SupplementalWeightMultipliersPercent;
        ShipDetailMotifWeights MotifWeightMultipliersPercent;
        uint32_t MotifRepeatPercent = 100u;
        int32_t AsymmetricDetailChanceOffset = 0;
    };

    struct ResolvedSurfaceDetailProfile
    {
        uint32_t DetailDensityPercent = 100u;
        uint32_t MechanicalPatternCountPercent = 100u;
        uint32_t LightPatternCountPercent = 100u;
        uint32_t AccentPanelWeight = 100u;
        uint32_t AccentStripeWeight = 100u;
        uint32_t AccentArmorWeight = 100u;
        uint32_t HorizontalVentChance = 50u;
        SupplementalSurfaceDetailWeights SupplementalWeights;
        ShipDetailMotifWeights MotifWeights;
        uint32_t MotifRepeatPercent = 100u;
        uint32_t AsymmetricDetailChance = 10u;
    };

    const ShipFactionSurfaceDetailProfile& getShipFactionSurfaceDetailProfile(ShipFactionType faction);
}