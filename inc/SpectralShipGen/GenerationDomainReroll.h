#pragma once

#include <cstdint>
#include <vector>

#include <SpectralShipGen/GenerationDomain.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>

namespace SpectralShipGen
{
    ShipGenerationRecipe rerollGenerationDomains(const ShipGenerationRecipe& recipe, const std::vector<GenerationDomain>& selectedDomains, uint64_t rerollSeed);
}
