#pragma once

#include "ShipGenerationContext.h"
#include "WeaponGenerationInternal.h"

namespace PixelShipGenerator
{
    namespace WeaponGenerationInternal
    {
        class WeaponCandidateValidator
        {
        public:
            bool validateCandidate(const ShipGenerationContext& context, const CandidateWeapon& candidate, WeaponCandidateValidationFailureReason* failureReason = nullptr) const;
            bool validateSymmetricPair(const ShipGenerationContext& context, const CandidateWeapon& first, const CandidateWeapon& second, WeaponCandidateValidationFailureReason* failureReason = nullptr) const;

        private:
            bool validateConnected(const CandidateWeapon& candidate) const;
            bool validateFiringClearance(const ShipGenerationContext& context, const CandidateWeapon& candidate) const;
        };
    }
}
