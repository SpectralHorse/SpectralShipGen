#pragma once

#include <cstdint>
#include <vector>

#include "GenerationDomain.h"
#include "ShipGenerationRecipe.h"

namespace PixelShipGenerator
{
    ShipGenerationRecipe rerollGenerationDomains(const ShipGenerationRecipe& recipe, const std::vector<GenerationDomain>& selectedDomains, uint64_t rerollSeed);
}
