#pragma once

#include <cstdint>

#include "ShipAttachment.h"
#include "ShipFactionType.h"

namespace PixelShipGenerator
{
    struct ShipAttachmentWeights
    {
        uint32_t WeaponMount = 100u;
        uint32_t SensorArray = 100u;
        uint32_t AuxiliaryPod = 100u;
        uint32_t Radiator = 100u;
        uint32_t ArmorFin = 100u;
        uint32_t TechnologyNode = 100u;

        uint32_t getWeight(ShipAttachmentType type) const;
    };

    struct ShipFactionAttachmentProfile
    {
        ShipAttachmentWeights WeightMultipliersPercent;
        uint32_t AttachmentChancePercent = 100u;
        int32_t SymmetryChanceOffset = 0;
    };

    const ShipFactionAttachmentProfile& getShipFactionAttachmentProfile(ShipFactionType faction);
}