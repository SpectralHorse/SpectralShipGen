#include <PixelShipGenerator/GenerationDomainReroll.h>

#include <array>
#include <cstddef>

#include <PixelShipGenerator/ShipGenerationSeeds.h>

namespace PixelShipGenerator
{
    ShipGenerationRecipe rerollGenerationDomains(const ShipGenerationRecipe& recipe, const std::vector<GenerationDomain>& selectedDomains, uint64_t rerollSeed)
    {
        if (selectedDomains.empty()) { return recipe; }

        std::array<bool, GenerationDomainCount> selected{};
        for (const GenerationDomain domain : selectedDomains)
        {
            const std::size_t index = static_cast<std::size_t>(domain);
            if (index < selected.size()) { selected[index] = true; }
        }

        ShipGenerationRecipe result = recipe;
        result.RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        const GenerationDomainSeeds effectiveSeeds = resolveGenerationDomainSeeds(result.Seeds, result.DomainSeedOverrides, GenerationRandomStreamMode::DOMAIN_SUBSTREAMS);

        for (std::size_t index = 0u; index < selected.size(); ++index)
        {
            if (!selected[index]) { continue; }
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            result.DomainSeedOverrides.set(domain, deriveGenerationDomainRerollSeed(rerollSeed, domain, effectiveSeeds.Values[index]));
        }

        return result;
    }
}
