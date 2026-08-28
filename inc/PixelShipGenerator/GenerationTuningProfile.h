#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "ShipAttachment.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationProfile.h"
#include "ShipMajorFeatureType.h"
#include "ShipWeaponType.h"

namespace PixelShipGenerator
{
    enum class GenerationWeightGroup : uint32_t
    {
        ENGINE_LAYOUT = 0u,
        ENGINE_SIZE,
        HULL_MODIFIER,
        MAJOR_FEATURE_TYPE,
        ATTACHMENT_TYPE,
        LARGE_WEAPON_TYPE,
        ENGINE_NACELLE_PRESENCE,
        MAJOR_FEATURE_PRESENCE,
        ATTACHMENT_PRESENCE,
        LARGE_WEAPON_PRESENCE,
        GENERATION_WEIGHT_GROUP_END
    };

    enum class GenerationWeightGroupKind : uint32_t
    {
        RELATIVE_WEIGHTS = 0u,
        BINARY_PROBABILITY
    };

    struct GenerationTuningStyleProfile
    {
        std::array<uint32_t, static_cast<std::size_t>(EngineLayoutType::ENGINE_LAYOUT_TYPE_END)> EngineLayoutWeights = {};
        std::array<uint32_t, static_cast<std::size_t>(EngineSizeClass::ENGINE_SIZE_CLASS_END)> EngineSizeWeights = {};
        std::array<uint32_t, static_cast<std::size_t>(HullModifierType::HULL_MODIFIER_TYPE_END)> HullModifierWeights = {};
        std::array<uint32_t, static_cast<std::size_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> MajorFeatureWeights = {};
        std::array<uint32_t, static_cast<std::size_t>(ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END)> AttachmentWeights = {};
        std::array<uint32_t, static_cast<std::size_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END)> LargeWeaponWeights = {};
        uint32_t EngineNacelleChance = 0u;
        uint32_t MajorFeatureChance = 0u;
        uint32_t AttachmentChance = 0u;
        uint32_t LargeWeaponChance = 0u;
    };

    struct GenerationTuningProfile
    {
        std::array<GenerationTuningStyleProfile, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> Styles = {};
    };

    struct GenerationCalibrationOverrides
    {
        std::optional<EngineLayoutType> ForcedEngineLayout;
        std::optional<EngineSizeClass> ForcedEngineSize;
        std::optional<HullModifierType> ForcedHullModifier;
        std::optional<ShipMajorFeatureType> ForcedMajorFeatureType;
        std::optional<ShipAttachmentType> ForcedAttachmentType;
        std::optional<ShipWeaponType> ForcedLargeWeaponType;
        std::optional<bool> ForcedEngineNacellePresence;
        std::optional<bool> ForcedMajorFeaturePresence;
        std::optional<bool> ForcedAttachmentPresence;
        std::optional<bool> ForcedLargeWeaponPresence;
    };

    struct GenerationCalibrationSettings
    {
        const GenerationTuningProfile* TuningProfile = nullptr;
        GenerationCalibrationOverrides Overrides;
        std::optional<GenerationWeightGroup> IsolatedGroup;
        uint64_t IsolationSalt = 0u;
    };

    GenerationTuningProfile createDefaultGenerationTuningProfile();
    ShipGenerationProfile applyGenerationTuningProfile(const ShipGenerationProfile& baseProfile, ShipStyle style, const GenerationTuningProfile& tuningProfile);

    GenerationWeightGroupKind getGenerationWeightGroupKind(GenerationWeightGroup group);
    uint32_t getGenerationWeightOptionCount(GenerationWeightGroup group);
    uint32_t getGenerationTuningWeight(const GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group, uint32_t optionIndex);
    void setGenerationTuningWeight(GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group, uint32_t optionIndex, uint32_t value);
    uint32_t getGenerationTuningGroupTotalWeight(const GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group);
    double getGenerationTuningNormalizedProbability(const GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group, uint32_t optionIndex);
}
