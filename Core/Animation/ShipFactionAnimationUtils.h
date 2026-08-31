#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

#include <SpectralShipGen/ShipFactionProfile.h>

namespace SpectralShipGen
{
namespace FactionAnimationInternal
{
    inline uint32_t applyValueScale(uint32_t value, const ShipFactionValueScale& scale)
    {
        if (scale.Denominator == 0u) { return value; }
        const uint64_t scaled = static_cast<uint64_t>(value) * static_cast<uint64_t>(scale.Numerator) / static_cast<uint64_t>(scale.Denominator);
        return static_cast<uint32_t>(std::min<uint64_t>(scaled, std::numeric_limits<uint32_t>::max()));
    }

    inline uint32_t applySignedOffset(uint32_t value, int32_t offset)
    {
        const int64_t adjusted = static_cast<int64_t>(value) + static_cast<int64_t>(offset);
        if (adjusted <= 0) { return 0u; }
        return static_cast<uint32_t>(std::min<int64_t>(adjusted, std::numeric_limits<uint32_t>::max()));
    }

    inline bool applyBooleanOverride(bool value, ShipFactionAnimationBooleanOverride overrideValue)
    {
        switch (overrideValue)
        {
        case ShipFactionAnimationBooleanOverride::ENABLE: return true;
        case ShipFactionAnimationBooleanOverride::DISABLE: return false;
        case ShipFactionAnimationBooleanOverride::INHERIT:
        default: return value;
        }
    }

    inline uint32_t applyOptionalMaximum(uint32_t value, uint32_t maximum)
    {
        return maximum == 0u ? value : std::min(value, maximum);
    }
}
}
