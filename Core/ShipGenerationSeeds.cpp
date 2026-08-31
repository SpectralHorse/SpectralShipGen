#include <SpectralShipGen/ShipGenerationSeeds.h>

#include <stdexcept>

namespace SpectralShipGen
{
    namespace
    {
        constexpr uint64_t StructureSeedSalt = 0x243F6A8885A308D3ull;
        constexpr uint64_t PaletteSeedSalt = 0x13198A2E03707344ull;
        constexpr uint64_t DetailsSeedSalt = 0xA4093822299F31D0ull;
        constexpr uint64_t AttachmentsSeedSalt = 0x082EFA98EC4E6C89ull;

        uint64_t getGenerationDomainSalt(GenerationDomain domain)
        {
            switch (domain)
            {
            case GenerationDomain::HULL: return 0xC13FA9A902A6328Full;
            case GenerationDomain::WINGS: return 0x91E10DA5C79E7B1Dull;
            case GenerationDomain::COCKPIT: return 0xD192E819D6EF5218ull;
            case GenerationDomain::ENGINES: return 0xA24BAED4963EE407ull;
            case GenerationDomain::HULL_LAYERS: return 0x9FB21C651E98DF25ull;
            case GenerationDomain::MAJOR_FEATURES: return 0xB7E151628AED2A6Bull;
            case GenerationDomain::MACRO_ASYMMETRY: return 0x8AED2A6ABF715880ull;
            case GenerationDomain::WEAPONS: return 0x94D049BB133111EBull;
            case GenerationDomain::ATTACHMENTS: return 0x369DEA0F31A53F85ull;
            case GenerationDomain::PALETTE: return 0xDB4F0B9175AE2165ull;
            case GenerationDomain::DETAILS: return 0xBBE0563303A4615Full;
            default: throw std::invalid_argument("Unknown GenerationDomain value.");
            }
        }
    }

    void GenerationDomainSeedOverrides::clearAll()
    {
        for (std::optional<uint64_t>& value : Values) { value.reset(); }
    }

    bool GenerationDomainSeedOverrides::hasAny() const
    {
        for (const std::optional<uint64_t>& value : Values)
        {
            if (value.has_value()) { return true; }
        }
        return false;
    }

    bool operator==(const ShipGenerationSeeds& first, const ShipGenerationSeeds& second)
    {
        return first.Master == second.Master && first.Structure == second.Structure && first.Palette == second.Palette && first.Details == second.Details && first.Attachments == second.Attachments;
    }

    bool operator==(const GenerationDomainSeedOverrides& first, const GenerationDomainSeedOverrides& second)
    {
        return first.Values == second.Values;
    }

    bool operator!=(const GenerationDomainSeedOverrides& first, const GenerationDomainSeedOverrides& second)
    {
        return !(first == second);
    }

    uint64_t mixGenerationSeed64(uint64_t value)
    {
        value += 0x9E3779B97F4A7C15ull;
        value = (value ^ (value >> 30u)) * 0xBF58476D1CE4E5B9ull;
        value = (value ^ (value >> 27u)) * 0x94D049BB133111EBull;

        return value ^ (value >> 31u);
    }

    ShipGenerationSeeds deriveShipGenerationSeeds(uint64_t masterSeed)
    {
        ShipGenerationSeeds seeds;

        seeds.Master = masterSeed;
        seeds.Structure = mixGenerationSeed64(masterSeed ^ StructureSeedSalt);
        seeds.Palette = mixGenerationSeed64(masterSeed ^ PaletteSeedSalt);
        seeds.Details = mixGenerationSeed64(masterSeed ^ DetailsSeedSalt);
        seeds.Attachments = mixGenerationSeed64(masterSeed ^ AttachmentsSeedSalt);

        return seeds;
    }

    ShipGenerationSeeds applyShipGenerationSeedOverrides(const ShipGenerationSeeds& seeds, const ShipGenerationSeedOverrides& overrides)
    {
        ShipGenerationSeeds result = seeds;

        if (overrides.Structure.has_value()) { result.Structure = *overrides.Structure; }
        if (overrides.Palette.has_value()) { result.Palette = *overrides.Palette; }
        if (overrides.Details.has_value()) { result.Details = *overrides.Details; }
        if (overrides.Attachments.has_value()) { result.Attachments = *overrides.Attachments; }

        return result;
    }

    uint64_t getGenerationSeedForChannel(const ShipGenerationSeeds& seeds, GenerationSeedChannel channel)
    {
        switch (channel)
        {
        case GenerationSeedChannel::STRUCTURE: return seeds.Structure;
        case GenerationSeedChannel::PALETTE: return seeds.Palette;
        case GenerationSeedChannel::DETAILS: return seeds.Details;
        case GenerationSeedChannel::ATTACHMENTS: return seeds.Attachments;
        default: throw std::invalid_argument("Unknown GenerationSeedChannel value.");
        }
    }

    uint64_t deriveGenerationDomainSeed(uint64_t parentSeed, GenerationDomain domain)
    {
        return mixGenerationSeed64(parentSeed ^ getGenerationDomainSalt(domain));
    }

    GenerationDomainSeeds resolveGenerationDomainSeeds(const ShipGenerationSeeds& seeds, const GenerationDomainSeedOverrides& overrides)
    {
        GenerationDomainSeeds result;
        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            const uint64_t parentSeed = getGenerationSeedForChannel(seeds, getGenerationDomainParentChannel(domain));
            uint64_t resolved = deriveGenerationDomainSeed(parentSeed, domain);
            if (overrides.Values[index].has_value()) { resolved = *overrides.Values[index]; }
            result.Values[index] = resolved;
        }
        return result;
    }

    void clearGenerationDomainOverridesForChannel(GenerationDomainSeedOverrides& overrides, GenerationSeedChannel channel)
    {
        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            if (getGenerationDomainParentChannel(domain) == channel) { overrides.Values[index].reset(); }
        }
    }

    uint64_t deriveGenerationDomainRerollSeed(uint64_t rerollSeed, GenerationDomain domain, uint64_t previousEffectiveSeed)
    {
        const uint64_t domainSalt = getGenerationDomainSalt(domain);
        return mixGenerationSeed64(rerollSeed ^ domainSalt ^ mixGenerationSeed64(previousEffectiveSeed + 0xD1B54A32D192ED03ull));
    }
}
