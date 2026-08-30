#include "ShipSurfaceDetailProfile.h"

#include "ShipFactionProfile.h"

namespace PixelShipGenerator
{
    uint32_t ShipDetailMotifWeights::getWeight(ShipDetailMotifType type) const
    {
        switch (type)
        {
        case ShipDetailMotifType::PAIRED_VENTS: return PairedVents;
        case ShipDetailMotifType::TRIPLE_VENT_BANK: return TripleVentBank;
        case ShipDetailMotifType::PAIRED_LIGHTS: return PairedLights;
        case ShipDetailMotifType::THREE_NODE_LIGHTS: return ThreeNodeLights;
        case ShipDetailMotifType::PARALLEL_SEAMS: return ParallelSeams;
        case ShipDetailMotifType::REPEATED_DASHES: return RepeatedDashes;
        case ShipDetailMotifType::RECESSED_SLOT: return RecessedSlot;
        default: return 0u;
        }
    }

    uint32_t SupplementalSurfaceDetailWeights::getWeight(SupplementalSurfaceDetailType type) const
    {
        switch (type)
        {
        case SupplementalSurfaceDetailType::PANEL_SEAM: return PanelSeam;
        case SupplementalSurfaceDetailType::GEOMETRIC_MARKING: return GeometricMarking;
        case SupplementalSurfaceDetailType::MECHANICAL_EXPOSURE: return MechanicalExposure;
        case SupplementalSurfaceDetailType::REPEATING_MOTIF: return RepeatingMotif;
        case SupplementalSurfaceDetailType::IDENTIFICATION_MARKING: return IdentificationMarking;
        case SupplementalSurfaceDetailType::LUMINOUS_CHANNEL: return LuminousChannel;
        default: return 0u;
        }
    }

    const ShipFactionSurfaceDetailProfile& getShipFactionSurfaceDetailProfile(ShipFactionType faction)
    {
        if (faction >= ShipFactionType::SHIP_FACTION_TYPE_END)
        {
            return getShipFactionProfile(ShipFactionType::FRONTIER).SurfaceDetails;
        }
        return getShipFactionProfile(faction).SurfaceDetails;
    }
}
