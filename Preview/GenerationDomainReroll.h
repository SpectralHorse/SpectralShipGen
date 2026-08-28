#pragma once

#include <cstdint>
#include <vector>

#include "GenerationDomain.h"
#include "PreviewGenerationRecipe.h"

namespace PixelShipGeneratorPreview
{
    PreviewGenerationRecipe rerollGenerationDomains(const PreviewGenerationRecipe& recipe, const std::vector<PixelShipGenerator::GenerationDomain>& selectedDomains, uint64_t rerollSeed);
}
