#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <SpectralShipGen/GenerationDomain.h>

namespace SpectralShipGen
{
    struct ShipGenerationSeeds
    {
        uint64_t Master = 0u;
        uint64_t Structure = 0u;
        uint64_t Palette = 0u;
        uint64_t Details = 0u;
        uint64_t Attachments = 0u;
    };

    struct ShipGenerationSeedOverrides
    {
        std::optional<uint64_t> Structure;
        std::optional<uint64_t> Palette;
        std::optional<uint64_t> Details;
        std::optional<uint64_t> Attachments;
    };

    struct GenerationDomainSeeds
    {
        std::array<uint64_t, GenerationDomainCount> Values{};

        uint64_t get(GenerationDomain domain) const { return Values[static_cast<std::size_t>(domain)]; }
        void set(GenerationDomain domain, uint64_t seed) { Values[static_cast<std::size_t>(domain)] = seed; }
    };

    struct GenerationDomainSeedOverrides
    {
        std::array<std::optional<uint64_t>, GenerationDomainCount> Values{};

        const std::optional<uint64_t>& get(GenerationDomain domain) const { return Values[static_cast<std::size_t>(domain)]; }
        std::optional<uint64_t>& get(GenerationDomain domain) { return Values[static_cast<std::size_t>(domain)]; }
        void set(GenerationDomain domain, uint64_t seed) { Values[static_cast<std::size_t>(domain)] = seed; }
        void clear(GenerationDomain domain) { Values[static_cast<std::size_t>(domain)].reset(); }
        void clearAll();
        bool hasAny() const;
    };

    bool operator==(const ShipGenerationSeeds& first, const ShipGenerationSeeds& second);
    bool operator==(const GenerationDomainSeedOverrides& first, const GenerationDomainSeedOverrides& second);
    bool operator!=(const GenerationDomainSeedOverrides& first, const GenerationDomainSeedOverrides& second);

    uint64_t mixGenerationSeed64(uint64_t value);
    ShipGenerationSeeds deriveShipGenerationSeeds(uint64_t masterSeed);
    ShipGenerationSeeds applyShipGenerationSeedOverrides(const ShipGenerationSeeds& seeds, const ShipGenerationSeedOverrides& overrides);
    uint64_t getGenerationSeedForChannel(const ShipGenerationSeeds& seeds, GenerationSeedChannel channel);
    uint64_t deriveGenerationDomainSeed(uint64_t parentSeed, GenerationDomain domain);
    GenerationDomainSeeds resolveGenerationDomainSeeds(const ShipGenerationSeeds& seeds, const GenerationDomainSeedOverrides& overrides);
    void clearGenerationDomainOverridesForChannel(GenerationDomainSeedOverrides& overrides, GenerationSeedChannel channel);
    uint64_t deriveGenerationDomainRerollSeed(uint64_t rerollSeed, GenerationDomain domain, uint64_t previousEffectiveSeed);
}
