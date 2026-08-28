#pragma once

#include <cstddef>
#include <cstdint>

namespace PixelShipGenerator
{
    enum class GenerationSeedChannel : uint32_t
    {
        STRUCTURE = 0u,
        PALETTE,
        DETAILS,
        ATTACHMENTS,
        GENERATION_SEED_CHANNEL_END
    };

    enum class GenerationDomain : uint32_t
    {
        HULL = 0u,
        WINGS,
        COCKPIT,
        ENGINES,
        HULL_LAYERS,
        MAJOR_FEATURES,
        MACRO_ASYMMETRY,
        WEAPONS,
        ATTACHMENTS,
        PALETTE,
        DETAILS,
        GENERATION_DOMAIN_END
    };

    inline constexpr std::size_t GenerationDomainCount = static_cast<std::size_t>(GenerationDomain::GENERATION_DOMAIN_END);

    GenerationSeedChannel getGenerationDomainParentChannel(GenerationDomain domain);
    const char* getGenerationDomainName(GenerationDomain domain);
    const char* getGenerationDomainDependencyDescription(GenerationDomain domain);
}
