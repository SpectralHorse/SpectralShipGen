#include <PixelShipGenerator/GenerationTuningProfile.h>
#include <PixelShipGenerator/BuiltInPresetCatalog.h>

#include <algorithm>
#include <stdexcept>

namespace PixelShipGenerator
{
    namespace
    {
        std::size_t styleIndex(ShipStyle style)
        {
            return static_cast<std::size_t>(style);
        }

        GenerationTuningStyleProfile createStyleTuningProfile(ShipStyle style)
        {
            const ShipGenerationProfile& source = getShipGenerationProfile(style);
            GenerationTuningStyleProfile result;

            result.EngineLayoutWeights = { source.CentralEngineWeight, source.TwinEngineWeight, source.QuadEngineWeight, source.CentralAuxiliaryEngineWeight, source.EngineBankWeight };
            result.EngineSizeWeights = { source.SmallEngineSizeWeight, source.MediumEngineSizeWeight, source.LargeEngineSizeWeight };
            result.HullModifierWeights = { source.BroaderShouldersModifierWeight, source.SideLobesModifierWeight, source.SteppedWingModifierWeight, source.NarrowWaistModifierWeight, source.WingCutoutModifierWeight, source.SplitNoseModifierWeight };
            result.MajorFeatureWeights = { source.MajorFeatureWeights.CentralSpine, source.MajorFeatureWeights.ArmorPlate, source.MajorFeatureWeights.RecessedBay, source.MajorFeatureWeights.VentBank, source.MajorFeatureWeights.WingPlate, source.MajorFeatureWeights.TechCore };
            result.AttachmentWeights = { source.AttachmentWeights.WeaponMount, source.AttachmentWeights.SensorArray, source.AttachmentWeights.AuxiliaryPod, source.AttachmentWeights.Radiator, source.AttachmentWeights.ArmorFin, source.AttachmentWeights.TechnologyNode };
            result.LargeWeaponWeights = { source.LargeWeaponWeights.SingleCannon, source.LargeWeaponWeights.TwinCannon, source.LargeWeaponWeights.CompactTurret, source.LargeWeaponWeights.RailWeapon, source.LargeWeaponWeights.WeaponPod };
            result.EngineNacelleChance = source.EngineNacelleChance;
            result.MajorFeatureChance = source.MajorFeatureChance;
            result.AttachmentChance = source.AttachmentChance;
            result.LargeWeaponChance = source.LargeWeaponChance;
            return result;
        }

        uint32_t getProbabilityWeight(uint32_t chance, uint32_t optionIndex)
        {
            return optionIndex == 0u ? 100u - std::min(100u, chance) : std::min(100u, chance);
        }

        void setProbabilityWeight(uint32_t& chance, uint32_t optionIndex, uint32_t value)
        {
            const uint32_t clamped = std::min(100u, value);
            chance = optionIndex == 0u ? 100u - clamped : clamped;
        }
    }

    GenerationTuningProfile createDefaultGenerationTuningProfile()
    {
        GenerationTuningProfile result;
        for (const BuiltInStructuralPreset& preset : getBuiltInStructuralPresetCatalog())
        {
            result.Styles[styleIndex(preset.Preset)] = createStyleTuningProfile(preset.Preset);
        }
        return result;
    }

    ShipGenerationProfile applyGenerationTuningProfile(const ShipGenerationProfile& baseProfile, const GenerationTuningStyleProfile& tuned)
    {
        ShipGenerationProfile result = baseProfile;

        result.CentralEngineWeight = tuned.EngineLayoutWeights[0u];
        result.TwinEngineWeight = tuned.EngineLayoutWeights[1u];
        result.QuadEngineWeight = tuned.EngineLayoutWeights[2u];
        result.CentralAuxiliaryEngineWeight = tuned.EngineLayoutWeights[3u];
        result.EngineBankWeight = tuned.EngineLayoutWeights[4u];
        result.SmallEngineSizeWeight = tuned.EngineSizeWeights[0u];
        result.MediumEngineSizeWeight = tuned.EngineSizeWeights[1u];
        result.LargeEngineSizeWeight = tuned.EngineSizeWeights[2u];
        result.BroaderShouldersModifierWeight = tuned.HullModifierWeights[0u];
        result.SideLobesModifierWeight = tuned.HullModifierWeights[1u];
        result.SteppedWingModifierWeight = tuned.HullModifierWeights[2u];
        result.NarrowWaistModifierWeight = tuned.HullModifierWeights[3u];
        result.WingCutoutModifierWeight = tuned.HullModifierWeights[4u];
        result.SplitNoseModifierWeight = tuned.HullModifierWeights[5u];
        result.MajorFeatureWeights.CentralSpine = tuned.MajorFeatureWeights[0u];
        result.MajorFeatureWeights.ArmorPlate = tuned.MajorFeatureWeights[1u];
        result.MajorFeatureWeights.RecessedBay = tuned.MajorFeatureWeights[2u];
        result.MajorFeatureWeights.VentBank = tuned.MajorFeatureWeights[3u];
        result.MajorFeatureWeights.WingPlate = tuned.MajorFeatureWeights[4u];
        result.MajorFeatureWeights.TechCore = tuned.MajorFeatureWeights[5u];
        result.AttachmentWeights.WeaponMount = tuned.AttachmentWeights[0u];
        result.AttachmentWeights.SensorArray = tuned.AttachmentWeights[1u];
        result.AttachmentWeights.AuxiliaryPod = tuned.AttachmentWeights[2u];
        result.AttachmentWeights.Radiator = tuned.AttachmentWeights[3u];
        result.AttachmentWeights.ArmorFin = tuned.AttachmentWeights[4u];
        result.AttachmentWeights.TechnologyNode = tuned.AttachmentWeights[5u];
        result.LargeWeaponWeights.SingleCannon = tuned.LargeWeaponWeights[0u];
        result.LargeWeaponWeights.TwinCannon = tuned.LargeWeaponWeights[1u];
        result.LargeWeaponWeights.CompactTurret = tuned.LargeWeaponWeights[2u];
        result.LargeWeaponWeights.RailWeapon = tuned.LargeWeaponWeights[3u];
        result.LargeWeaponWeights.WeaponPod = tuned.LargeWeaponWeights[4u];
        result.EngineNacelleChance = tuned.EngineNacelleChance;
        result.MajorFeatureChance = tuned.MajorFeatureChance;
        result.AttachmentChance = tuned.AttachmentChance;
        result.LargeWeaponChance = tuned.LargeWeaponChance;
        return result;
    }

    ShipGenerationProfile applyGenerationTuningProfile(const ShipGenerationProfile& baseProfile, ShipStyle style, const GenerationTuningProfile& tuningProfile)
    {
        if (style >= ShipStyle::SHIP_STYLE_END)
        {
            throw std::invalid_argument("Generation tuning requires a valid built-in ShipStyle.");
        }
        return applyGenerationTuningProfile(baseProfile, tuningProfile.Styles[styleIndex(style)]);
    }

    GenerationWeightGroupKind getGenerationWeightGroupKind(GenerationWeightGroup group)
    {
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE:
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE:
        case GenerationWeightGroup::ATTACHMENT_PRESENCE:
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE:
            return GenerationWeightGroupKind::BINARY_PROBABILITY;
        default:
            return GenerationWeightGroupKind::RELATIVE_WEIGHTS;
        }
    }

    uint32_t getGenerationWeightOptionCount(GenerationWeightGroup group)
    {
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT: return static_cast<uint32_t>(EngineLayoutType::ENGINE_LAYOUT_TYPE_END);
        case GenerationWeightGroup::ENGINE_SIZE: return static_cast<uint32_t>(EngineSizeClass::ENGINE_SIZE_CLASS_END);
        case GenerationWeightGroup::HULL_MODIFIER: return static_cast<uint32_t>(HullModifierType::HULL_MODIFIER_TYPE_END);
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE: return static_cast<uint32_t>(ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END);
        case GenerationWeightGroup::ATTACHMENT_TYPE: return static_cast<uint32_t>(ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END);
        case GenerationWeightGroup::LARGE_WEAPON_TYPE: return static_cast<uint32_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END);
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE:
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE:
        case GenerationWeightGroup::ATTACHMENT_PRESENCE:
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE:
            return 2u;
        default: return 0u;
        }
    }

    uint32_t getGenerationTuningWeight(const GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group, uint32_t optionIndex)
    {
        const GenerationTuningStyleProfile& tuned = profile.Styles[styleIndex(style)];
        if (optionIndex >= getGenerationWeightOptionCount(group)) { return 0u; }
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT: return tuned.EngineLayoutWeights[optionIndex];
        case GenerationWeightGroup::ENGINE_SIZE: return tuned.EngineSizeWeights[optionIndex];
        case GenerationWeightGroup::HULL_MODIFIER: return tuned.HullModifierWeights[optionIndex];
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE: return tuned.MajorFeatureWeights[optionIndex];
        case GenerationWeightGroup::ATTACHMENT_TYPE: return tuned.AttachmentWeights[optionIndex];
        case GenerationWeightGroup::LARGE_WEAPON_TYPE: return tuned.LargeWeaponWeights[optionIndex];
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE: return getProbabilityWeight(tuned.EngineNacelleChance, optionIndex);
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE: return getProbabilityWeight(tuned.MajorFeatureChance, optionIndex);
        case GenerationWeightGroup::ATTACHMENT_PRESENCE: return getProbabilityWeight(tuned.AttachmentChance, optionIndex);
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE: return getProbabilityWeight(tuned.LargeWeaponChance, optionIndex);
        default: return 0u;
        }
    }

    void setGenerationTuningWeight(GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group, uint32_t optionIndex, uint32_t value)
    {
        GenerationTuningStyleProfile& tuned = profile.Styles[styleIndex(style)];
        if (optionIndex >= getGenerationWeightOptionCount(group)) { return; }
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT: tuned.EngineLayoutWeights[optionIndex] = value; break;
        case GenerationWeightGroup::ENGINE_SIZE: tuned.EngineSizeWeights[optionIndex] = value; break;
        case GenerationWeightGroup::HULL_MODIFIER: tuned.HullModifierWeights[optionIndex] = value; break;
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE: tuned.MajorFeatureWeights[optionIndex] = value; break;
        case GenerationWeightGroup::ATTACHMENT_TYPE: tuned.AttachmentWeights[optionIndex] = value; break;
        case GenerationWeightGroup::LARGE_WEAPON_TYPE: tuned.LargeWeaponWeights[optionIndex] = value; break;
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE: setProbabilityWeight(tuned.EngineNacelleChance, optionIndex, value); break;
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE: setProbabilityWeight(tuned.MajorFeatureChance, optionIndex, value); break;
        case GenerationWeightGroup::ATTACHMENT_PRESENCE: setProbabilityWeight(tuned.AttachmentChance, optionIndex, value); break;
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE: setProbabilityWeight(tuned.LargeWeaponChance, optionIndex, value); break;
        default: break;
        }
    }

    uint32_t getGenerationTuningGroupTotalWeight(const GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group)
    {
        uint64_t total = 0u;
        const uint32_t optionCount = getGenerationWeightOptionCount(group);
        for (uint32_t index = 0u; index < optionCount; ++index) { total += getGenerationTuningWeight(profile, style, group, index); }
        return static_cast<uint32_t>(std::min<uint64_t>(total, UINT32_MAX));
    }

    double getGenerationTuningNormalizedProbability(const GenerationTuningProfile& profile, ShipStyle style, GenerationWeightGroup group, uint32_t optionIndex)
    {
        const uint32_t total = getGenerationTuningGroupTotalWeight(profile, style, group);
        return total == 0u ? 0.0 : static_cast<double>(getGenerationTuningWeight(profile, style, group, optionIndex)) / static_cast<double>(total);
    }
}
