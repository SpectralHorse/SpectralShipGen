#include "ShipGenerationRecipeSerializer.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <PixelShipGenerator/BuiltInPresetCatalog.h>
#include <PixelShipGenerator/ShipFactionProfileValidation.h>
#include <PixelShipGenerator/ShipGenerationProfileValidation.h>
#include <PixelShipGenerator/Serialization/ShipGenerationRecipeJson.h>
#include <PixelShipGenerator/Serialization/ShipGenerationRecipeProfileSerialization.h>
#include <PixelShipGenerator/ShipPaletteGenerationProfileValidation.h>

namespace PixelShipGenerator
{
    namespace
    {
        using RecipeJson::Type;
        using RecipeJson::Value;

        ShipGenerationRecipeLoadResult errorResult(const std::string& error)
        {
            ShipGenerationRecipeLoadResult result;
            result.Error = error;
            return result;
        }

        const char* profileSourceString(ShipGenerationRecipeProfileSource source)
        {
            switch (source)
            {
            case ShipGenerationRecipeProfileSource::BUILT_IN_PRESET: return "BUILT_IN_PRESET";
            case ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM: return "EMBEDDED_CUSTOM";
            default: throw std::invalid_argument("Unknown recipe profile source.");
            }
        }

        bool profileSourceFromString(const std::string& value, ShipGenerationRecipeProfileSource& source)
        {
            if (value == "BUILT_IN_PRESET") { source = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET; return true; }
            if (value == "EMBEDDED_CUSTOM") { source = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM; return true; }
            return false;
        }

        const char* paletteSourceString(ShipPaletteSourceMode mode)
        {
            switch (mode)
            {
            case ShipPaletteSourceMode::FACTION_PROFILE_GENERATED: return "FACTION_PROFILE_GENERATED";
            case ShipPaletteSourceMode::EXPLICIT_GENERATED: return "EXPLICIT_GENERATED";
            case ShipPaletteSourceMode::FIXED: return "FIXED";
            default: throw std::invalid_argument("Unknown palette source mode.");
            }
        }

        bool paletteSourceFromString(const std::string& value, ShipPaletteSourceMode& mode)
        {
            if (value == "FACTION_PROFILE_GENERATED") { mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED; return true; }
            if (value == "EXPLICIT_GENERATED") { mode = ShipPaletteSourceMode::EXPLICIT_GENERATED; return true; }
            if (value == "FIXED") { mode = ShipPaletteSourceMode::FIXED; return true; }
            return false;
        }

        const char* randomStreamModeString(GenerationRandomStreamMode mode)
        {
            switch (mode)
            {
            case GenerationRandomStreamMode::DOMAIN_SUBSTREAMS: return "DOMAIN_SUBSTREAMS";
            case GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS: return "LEGACY_TOP_LEVEL_STREAMS";
            default: throw std::invalid_argument("Unknown generation random stream mode.");
            }
        }

        bool randomStreamModeFromString(const std::string& value, GenerationRandomStreamMode& mode)
        {
            if (value == "DOMAIN_SUBSTREAMS") { mode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS; return true; }
            if (value == "LEGACY_TOP_LEVEL_STREAMS") { mode = GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS; return true; }
            return false;
        }

        const char* animationSamplingModeString(AnimationSamplingMode mode)
        {
            switch (mode)
            {
            case AnimationSamplingMode::ADAPTIVE: return "ADAPTIVE";
            case AnimationSamplingMode::EXACT_FRAME_COUNT: return "EXACT_FRAME_COUNT";
            default: throw std::invalid_argument("Unknown animation sampling mode.");
            }
        }

        bool animationSamplingModeFromString(const std::string& value, AnimationSamplingMode& mode)
        {
            if (value == "ADAPTIVE") { mode = AnimationSamplingMode::ADAPTIVE; return true; }
            if (value == "EXACT_FRAME_COUNT") { mode = AnimationSamplingMode::EXACT_FRAME_COUNT; return true; }
            return false;
        }

        const char* generationDomainKey(GenerationDomain domain)
        {
            switch (domain)
            {
            case GenerationDomain::HULL: return "hull";
            case GenerationDomain::WINGS: return "wings";
            case GenerationDomain::COCKPIT: return "cockpit";
            case GenerationDomain::ENGINES: return "engines";
            case GenerationDomain::HULL_LAYERS: return "hull_layers";
            case GenerationDomain::MAJOR_FEATURES: return "major_features";
            case GenerationDomain::MACRO_ASYMMETRY: return "macro_asymmetry";
            case GenerationDomain::WEAPONS: return "weapons";
            case GenerationDomain::ATTACHMENTS: return "attachments";
            case GenerationDomain::PALETTE: return "palette";
            case GenerationDomain::DETAILS: return "details";
            default: return "unknown";
            }
        }

        Value serializeSeeds(const ShipGenerationRecipe& recipe)
        {
            Value seeds = Value::object();
            seeds.Object["master"] = Value::number(recipe.Seeds.Master);
            seeds.Object["structure"] = Value::number(recipe.Seeds.Structure);
            seeds.Object["palette"] = Value::number(recipe.Seeds.Palette);
            seeds.Object["details"] = Value::number(recipe.Seeds.Details);
            seeds.Object["attachments"] = Value::number(recipe.Seeds.Attachments);
            seeds.Object["rng_mode"] = Value::string(randomStreamModeString(recipe.RandomStreamMode));

            if (recipe.DomainSeedOverrides.hasAny())
            {
                Value domains = Value::object();
                for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
                {
                    const std::optional<uint64_t>& seed = recipe.DomainSeedOverrides.Values[index];
                    if (seed.has_value()) { domains.Object[generationDomainKey(static_cast<GenerationDomain>(index))] = Value::number(*seed); }
                }
                seeds.Object["domains"] = std::move(domains);
            }
            return seeds;
        }

        Value serializeStructuralSource(const ShipGenerationRecipe& recipe)
        {
            Value structural = Value::object();
            structural.Object["source"] = Value::string(profileSourceString(recipe.StructuralSource));
            if (recipe.StructuralSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
            {
                structural.Object["preset"] = Value::string(shipStyleToRecipeString(recipe.Style));
            }
            else
            {
                structural.Object["profile"] = RecipeProfileSerialization::serialize(recipe.StructuralProfile);
            }
            return structural;
        }

        Value serializeFactionSource(const ShipGenerationRecipe& recipe)
        {
            Value faction = Value::object();
            faction.Object["source"] = Value::string(profileSourceString(recipe.FactionSource));
            if (recipe.FactionSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
            {
                faction.Object["preset"] = Value::string(shipFactionToRecipeString(recipe.Faction));
            }
            else
            {
                faction.Object["profile"] = RecipeProfileSerialization::serialize(recipe.FactionProfile);
            }
            return faction;
        }

        Value serializePaletteSource(const ShipPaletteConfiguration& configuration)
        {
            Value palette = Value::object();
            palette.Object["source"] = Value::string(paletteSourceString(configuration.Mode));
            if (configuration.Mode == ShipPaletteSourceMode::EXPLICIT_GENERATED)
            {
                palette.Object["profile"] = RecipeProfileSerialization::serialize(configuration.Generated);
            }
            else if (configuration.Mode == ShipPaletteSourceMode::FIXED)
            {
                palette.Object["colors"] = RecipeProfileSerialization::serialize(configuration.Fixed);
            }
            return palette;
        }

        Value serializeAnimation(const ShipIdleAnimationSettings& animation)
        {
            Value object = Value::object();
            object.Object["seed"] = animation.Seed.has_value() ? Value::number(*animation.Seed) : Value::null();
            object.Object["duration_milliseconds"] = Value::number(static_cast<uint64_t>(animation.AnimationDurationMilliseconds));
            object.Object["exact_frame_count"] = Value::number(static_cast<uint64_t>(animation.FrameCount));
            object.Object["minimum_frame_count"] = Value::number(static_cast<uint64_t>(animation.MinimumFrameCount));
            object.Object["maximum_frame_count"] = Value::number(static_cast<uint64_t>(animation.MaximumFrameCount));
            object.Object["sampling_mode"] = Value::string(animationSamplingModeString(animation.SamplingMode));
            object.Object["engine_flicker"] = Value::boolean(animation.EngineFlicker);
            object.Object["light_blinking"] = Value::boolean(animation.LightBlinking);
            object.Object["mechanical_micro_movement"] = Value::boolean(animation.MechanicalMicroMovement);
            object.Object["hover_offset"] = Value::boolean(animation.HoverOffset);
            object.Object["small_detail_variation"] = Value::boolean(animation.SmallDetailVariation);
            return object;
        }

        bool validateCommonRecipe(const ShipGenerationRecipe& recipe, std::string& error)
        {
            const ValidationResult validation = validateShipGenerationRecipe(recipe);
            if (validation.isValid()) { return true; }
            error = "Invalid ship generation recipe: " + validation.Errors.front().Field + " - " + validation.Errors.front().Message;
            return false;
        }

        bool validateAnimation(const ShipIdleAnimationSettings& settings, std::string& error)
        {
            if (settings.AnimationDurationMilliseconds == 0u || settings.AnimationDurationMilliseconds > 120000u) { error = "animation.duration_milliseconds must be in the range 1-120000."; return false; }
            if (settings.FrameCount == 0u || settings.FrameCount > 1000u) { error = "animation.exact_frame_count must be in the range 1-1000."; return false; }
            if (settings.MinimumFrameCount == 0u || settings.MinimumFrameCount > 1000u) { error = "animation.minimum_frame_count must be in the range 1-1000."; return false; }
            if (settings.MaximumFrameCount == 0u || settings.MaximumFrameCount > 1000u) { error = "animation.maximum_frame_count must be in the range 1-1000."; return false; }
            if (settings.MinimumFrameCount > settings.MaximumFrameCount) { error = "animation.minimum_frame_count must not exceed animation.maximum_frame_count."; return false; }
            if (settings.SamplingMode >= AnimationSamplingMode::ANIMATION_SAMPLING_MODE_END) { error = "animation.sampling_mode is invalid."; return false; }
            if (settings.SamplingMode == AnimationSamplingMode::EXACT_FRAME_COUNT && (settings.FrameCount < settings.MinimumFrameCount || settings.FrameCount > settings.MaximumFrameCount)) { error = "animation.exact_frame_count must be within animation frame limits in EXACT_FRAME_COUNT mode."; return false; }
            return true;
        }

        bool parseOptionalSeed(const Value& object, const char* key, std::optional<uint64_t>& out, std::string& error, const std::string& path)
        {
            if (object.ValueType != Type::Object) { error = path + " must be an object."; return false; }
            const Value* value = object.find(key);
            if (value == nullptr || value->ValueType == Type::Null) { out.reset(); return true; }
            Value wrapper = Value::object(); wrapper.Object[key] = *value;
            uint64_t parsed = 0u;
            if (!RecipeJson::getUInt64(wrapper, key, parsed, error, path)) { return false; }
            out = parsed;
            return true;
        }

        bool parseSeeds(const Value& object, uint32_t formatVersion, ShipGenerationRecipe& recipe, std::string& error)
        {
            if (!RecipeJson::getUInt64(object, "master", recipe.Seeds.Master, error, "ship.seeds")) { return false; }
            if (!RecipeJson::getUInt64(object, "structure", recipe.Seeds.Structure, error, "ship.seeds")) { return false; }
            if (!RecipeJson::getUInt64(object, "palette", recipe.Seeds.Palette, error, "ship.seeds")) { return false; }
            if (!RecipeJson::getUInt64(object, "details", recipe.Seeds.Details, error, "ship.seeds")) { return false; }
            if (!RecipeJson::getUInt64(object, "attachments", recipe.Seeds.Attachments, error, "ship.seeds")) { return false; }

            if (formatVersion >= 3u)
            {
                std::string mode;
                if (!RecipeJson::getString(object, "rng_mode", mode, error, "ship.seeds")) { return false; }
                if (!randomStreamModeFromString(mode, recipe.RandomStreamMode)) { error = "Unknown ship.seeds.rng_mode: " + mode + "."; return false; }

                recipe.DomainSeedOverrides.clearAll();
                const Value* domains = object.find("domains");
                if (domains != nullptr)
                {
                    if (domains->ValueType != Type::Object) { error = "ship.seeds.domains must be an object."; return false; }
                    for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
                    {
                        const char* key = generationDomainKey(static_cast<GenerationDomain>(index));
                        const Value* domainValue = domains->find(key);
                        if (domainValue == nullptr) { continue; }
                        Value wrapper = Value::object(); wrapper.Object[key] = *domainValue;
                        uint64_t parsed = 0u;
                        if (!RecipeJson::getUInt64(wrapper, key, parsed, error, "ship.seeds.domains")) { return false; }
                        recipe.DomainSeedOverrides.Values[index] = parsed;
                    }
                }
            }
            else
            {
                recipe.RandomStreamMode = GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS;
                recipe.DomainSeedOverrides.clearAll();
            }
            return true;
        }

        bool parseAnimation(const Value& object, uint32_t formatVersion, ShipIdleAnimationSettings& animation, std::string& error)
        {
            if (!parseOptionalSeed(object, "seed", animation.Seed, error, "animation")) { return false; }
            if (formatVersion >= 4u)
            {
                std::string mode;
                if (!RecipeJson::getUInt32(object, "duration_milliseconds", animation.AnimationDurationMilliseconds, error, "animation")) { return false; }
                if (!RecipeJson::getUInt32(object, "exact_frame_count", animation.FrameCount, error, "animation")) { return false; }
                if (!RecipeJson::getUInt32(object, "minimum_frame_count", animation.MinimumFrameCount, error, "animation")) { return false; }
                if (!RecipeJson::getUInt32(object, "maximum_frame_count", animation.MaximumFrameCount, error, "animation")) { return false; }
                if (!RecipeJson::getString(object, "sampling_mode", mode, error, "animation")) { return false; }
                if (!animationSamplingModeFromString(mode, animation.SamplingMode)) { error = "Unknown animation.sampling_mode: " + mode + "."; return false; }
            }
            else
            {
                if (!RecipeJson::getUInt32(object, "frame_count", animation.FrameCount, error, "animation")) { return false; }
                animation.AnimationDurationMilliseconds = animation.FrameCount * 100u;
                animation.MinimumFrameCount = animation.FrameCount;
                animation.MaximumFrameCount = animation.FrameCount;
                animation.SamplingMode = AnimationSamplingMode::EXACT_FRAME_COUNT;
            }
            if (!RecipeJson::getBool(object, "engine_flicker", animation.EngineFlicker, error, "animation")) { return false; }
            if (!RecipeJson::getBool(object, "light_blinking", animation.LightBlinking, error, "animation")) { return false; }
            if (!RecipeJson::getBool(object, "mechanical_micro_movement", animation.MechanicalMicroMovement, error, "animation")) { return false; }
            if (!RecipeJson::getBool(object, "hover_offset", animation.HoverOffset, error, "animation")) { return false; }
            if (!RecipeJson::getBool(object, "small_detail_variation", animation.SmallDetailVariation, error, "animation")) { return false; }
            return validateAnimation(animation, error);
        }

        bool parseLegacyShip(const Value& ship, uint32_t formatVersion, ShipGenerationRecipe& recipe, std::string& error)
        {
            const Value* dimensions = nullptr;
            if (formatVersion >= 2u)
            {
                if (!RecipeJson::getObject(ship, "dimensions", dimensions, error, "ship")) { return false; }
                if (!RecipeJson::getUInt32(*dimensions, "width", recipe.Dimensions.Width, error, "ship.dimensions")) { return false; }
                if (!RecipeJson::getUInt32(*dimensions, "height", recipe.Dimensions.Height, error, "ship.dimensions")) { return false; }
            }
            else
            {
                const Value* resolution = ship.find("resolution");
                if (resolution == nullptr) { error = "Missing required field: ship.resolution."; return false; }
                if (resolution->ValueType == Type::Object)
                {
                    if (!RecipeJson::getUInt32(*resolution, "width", recipe.Dimensions.Width, error, "ship.resolution")) { return false; }
                    if (!RecipeJson::getUInt32(*resolution, "height", recipe.Dimensions.Height, error, "ship.resolution")) { return false; }
                }
                else if (resolution->ValueType == Type::Number)
                {
                    Value wrapper = Value::object(); wrapper.Object["resolution"] = *resolution;
                    uint32_t square = 0u;
                    if (!RecipeJson::getUInt32(wrapper, "resolution", square, error, "ship")) { return false; }
                    recipe.Dimensions = { square, square };
                }
                else { error = "ship.resolution has the wrong JSON type."; return false; }
            }

            std::string style;
            std::string faction;
            if (!RecipeJson::getString(ship, "style", style, error, "ship") || !shipStyleFromRecipeString(style, recipe.Style)) { error = "Unknown or invalid ship.style: " + style + "."; return false; }
            if (!RecipeJson::getString(ship, "faction", faction, error, "ship") || !shipFactionFromRecipeString(faction, recipe.Faction)) { error = "Unknown or invalid ship.faction: " + faction + "."; return false; }
            recipe.StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
            recipe.FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
            recipe.PaletteConfiguration.Mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;

            const Value* seeds = nullptr;
            const Value* settings = nullptr;
            if (!RecipeJson::getObject(ship, "seeds", seeds, error, "ship")) { return false; }
            if (!RecipeJson::getObject(ship, "settings", settings, error, "ship")) { return false; }
            if (!parseSeeds(*seeds, formatVersion, recipe, error)) { return false; }
            if (!RecipeJson::getUInt32(*settings, "detail_density", recipe.DetailDensity, error, "ship.settings")) { return false; }
            if (!RecipeJson::getUInt32(*settings, "asymmetric_detail_chance", recipe.AsymmetricDetailChance, error, "ship.settings")) { return false; }
            if (!RecipeJson::getBool(*settings, "attachments_enabled", recipe.AttachmentsEnabled, error, "ship.settings")) { return false; }
            return true;
        }

        bool parseVersion5Ship(const Value& ship, ShipGenerationRecipe& recipe, std::string& error)
        {
            const Value* dimensions = nullptr;
            const Value* structural = nullptr;
            const Value* faction = nullptr;
            const Value* palette = nullptr;
            const Value* seeds = nullptr;
            const Value* settings = nullptr;
            if (!RecipeJson::getObject(ship, "dimensions", dimensions, error, "ship")) { return false; }
            if (!RecipeJson::getUInt32(*dimensions, "width", recipe.Dimensions.Width, error, "ship.dimensions")) { return false; }
            if (!RecipeJson::getUInt32(*dimensions, "height", recipe.Dimensions.Height, error, "ship.dimensions")) { return false; }
            if (!RecipeJson::getObject(ship, "structural", structural, error, "ship")) { return false; }
            if (!RecipeJson::getObject(ship, "faction", faction, error, "ship")) { return false; }
            if (!RecipeJson::getObject(ship, "palette", palette, error, "ship")) { return false; }
            if (!RecipeJson::getObject(ship, "seeds", seeds, error, "ship")) { return false; }
            if (!RecipeJson::getObject(ship, "settings", settings, error, "ship")) { return false; }

            std::string source;
            if (!RecipeJson::getString(*structural, "source", source, error, "ship.structural") || !profileSourceFromString(source, recipe.StructuralSource)) { error = "Unknown ship.structural.source: " + source + "."; return false; }
            if (recipe.StructuralSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
            {
                std::string preset;
                if (!RecipeJson::getString(*structural, "preset", preset, error, "ship.structural") || !shipStyleFromRecipeString(preset, recipe.Style)) { error = "Unknown ship.structural.preset: " + preset + "."; return false; }
            }
            else
            {
                recipe.Style = ShipStyle::SHIP_STYLE_END;
                const Value* profile = nullptr;
                if (!RecipeJson::getObject(*structural, "profile", profile, error, "ship.structural")) { return false; }
                if (!RecipeProfileSerialization::deserialize(*profile, recipe.StructuralProfile, error, "ship.structural.profile")) { return false; }
            }

            if (!RecipeJson::getString(*faction, "source", source, error, "ship.faction") || !profileSourceFromString(source, recipe.FactionSource)) { error = "Unknown ship.faction.source: " + source + "."; return false; }
            if (recipe.FactionSource == ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
            {
                std::string preset;
                if (!RecipeJson::getString(*faction, "preset", preset, error, "ship.faction") || !shipFactionFromRecipeString(preset, recipe.Faction)) { error = "Unknown ship.faction.preset: " + preset + "."; return false; }
            }
            else
            {
                recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
                const Value* profile = nullptr;
                if (!RecipeJson::getObject(*faction, "profile", profile, error, "ship.faction")) { return false; }
                if (!RecipeProfileSerialization::deserialize(*profile, recipe.FactionProfile, error, "ship.faction.profile")) { return false; }
            }

            if (!RecipeJson::getString(*palette, "source", source, error, "ship.palette") || !paletteSourceFromString(source, recipe.PaletteConfiguration.Mode)) { error = "Unknown ship.palette.source: " + source + "."; return false; }
            if (recipe.PaletteConfiguration.Mode == ShipPaletteSourceMode::EXPLICIT_GENERATED)
            {
                const Value* profile = nullptr;
                if (!RecipeJson::getObject(*palette, "profile", profile, error, "ship.palette")) { return false; }
                if (!RecipeProfileSerialization::deserialize(*profile, recipe.PaletteConfiguration.Generated, error, "ship.palette.profile")) { return false; }
            }
            else if (recipe.PaletteConfiguration.Mode == ShipPaletteSourceMode::FIXED)
            {
                const Value* colors = nullptr;
                if (!RecipeJson::getObject(*palette, "colors", colors, error, "ship.palette")) { return false; }
                if (!RecipeProfileSerialization::deserialize(*colors, recipe.PaletteConfiguration.Fixed, error, "ship.palette.colors")) { return false; }
            }

            if (!parseSeeds(*seeds, ShipGenerationRecipeFormatVersion, recipe, error)) { return false; }
            if (!RecipeJson::getUInt32(*settings, "detail_density", recipe.DetailDensity, error, "ship.settings")) { return false; }
            if (!RecipeJson::getUInt32(*settings, "asymmetric_detail_chance", recipe.AsymmetricDetailChance, error, "ship.settings")) { return false; }
            if (!RecipeJson::getBool(*settings, "attachments_enabled", recipe.AttachmentsEnabled, error, "ship.settings")) { return false; }
            return true;
        }
    }

    std::string shipStyleToRecipeString(ShipStyle style)
    {
        return std::string(getBuiltInStructuralPresetId(style));
    }

    std::string shipFactionToRecipeString(ShipFactionType faction)
    {
        return std::string(getBuiltInFactionPresetId(faction));
    }

    bool shipStyleFromRecipeString(const std::string& value, ShipStyle& style)
    {
        return tryGetBuiltInStructuralPreset(value, style);
    }

    bool shipFactionFromRecipeString(const std::string& value, ShipFactionType& faction)
    {
        return tryGetBuiltInFactionPreset(value, faction);
    }

    std::string serializeShipGenerationRecipe(const ShipGenerationRecipeDocument& document)
    {
        std::string error;
        if (!validateCommonRecipe(document.Recipe, error)) { throw std::invalid_argument(error); }
        if (document.AnimationSettings.has_value() && !validateAnimation(*document.AnimationSettings, error)) { throw std::invalid_argument(error); }

        Value root = Value::object();
        root.Object["format_version"] = Value::number(static_cast<uint64_t>(ShipGenerationRecipeFormatVersion));

        Value ship = Value::object();
        Value dimensions = Value::object();
        dimensions.Object["width"] = Value::number(static_cast<uint64_t>(document.Recipe.Dimensions.Width));
        dimensions.Object["height"] = Value::number(static_cast<uint64_t>(document.Recipe.Dimensions.Height));
        ship.Object["dimensions"] = std::move(dimensions);
        ship.Object["structural"] = serializeStructuralSource(document.Recipe);
        ship.Object["faction"] = serializeFactionSource(document.Recipe);
        ship.Object["palette"] = serializePaletteSource(document.Recipe.PaletteConfiguration);
        ship.Object["seeds"] = serializeSeeds(document.Recipe);

        Value settings = Value::object();
        settings.Object["detail_density"] = Value::number(static_cast<uint64_t>(document.Recipe.DetailDensity));
        settings.Object["asymmetric_detail_chance"] = Value::number(static_cast<uint64_t>(document.Recipe.AsymmetricDetailChance));
        settings.Object["attachments_enabled"] = Value::boolean(document.Recipe.AttachmentsEnabled);
        ship.Object["settings"] = std::move(settings);
        root.Object["ship"] = std::move(ship);

        if (document.AnimationSettings.has_value()) { root.Object["animation"] = serializeAnimation(*document.AnimationSettings); }
        return RecipeJson::stringify(root);
    }

    ShipGenerationRecipeLoadResult deserializeShipGenerationRecipe(const std::string& jsonText)
    {
        const RecipeJson::ParseResult parsed = RecipeJson::parse(jsonText);
        if (!parsed.Success) { return errorResult("Failed to parse recipe JSON: " + parsed.Error); }
        if (parsed.Root.ValueType != Type::Object) { return errorResult("Recipe root must be a JSON object."); }

        uint32_t formatVersion = 0u;
        std::string error;
        if (!RecipeJson::getUInt32(parsed.Root, "format_version", formatVersion, error, "recipe")) { return errorResult(error); }
        if (formatVersion < 1u || formatVersion > ShipGenerationRecipeFormatVersion) { return errorResult("Unsupported format version: " + std::to_string(formatVersion) + "."); }

        const Value* ship = nullptr;
        if (!RecipeJson::getObject(parsed.Root, "ship", ship, error, "recipe")) { return errorResult(error); }

        ShipGenerationRecipeDocument document;
        if (formatVersion <= 4u)
        {
            if (!parseLegacyShip(*ship, formatVersion, document.Recipe, error)) { return errorResult(error); }
        }
        else
        {
            if (!parseVersion5Ship(*ship, document.Recipe, error)) { return errorResult(error); }
        }

        if (!validateCommonRecipe(document.Recipe, error)) { return errorResult(error); }

        const Value* animation = parsed.Root.find("animation");
        if (animation != nullptr)
        {
            if (animation->ValueType != Type::Object) { return errorResult("animation must be a JSON object."); }
            ShipIdleAnimationSettings settings;
            if (!parseAnimation(*animation, formatVersion, settings, error)) { return errorResult(error); }
            document.AnimationSettings = settings;
        }

        ShipGenerationRecipeLoadResult result;
        result.Success = true;
        result.Document = std::move(document);
        return result;
    }

    bool saveShipGenerationRecipe(const ShipGenerationRecipeDocument& document, const std::filesystem::path& path, std::string& error)
    {
        error.clear();
        try
        {
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            if (!stream) { error = "Failed to open recipe file for writing: " + path.string(); return false; }
            stream << serializeShipGenerationRecipe(document);
            stream.flush();
            if (!stream) { error = "Failed while writing recipe file: " + path.string(); return false; }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = std::string("Recipe export failed: ") + exception.what();
            return false;
        }
    }

    ShipGenerationRecipeLoadResult loadShipGenerationRecipe(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) { return errorResult("Failed to open recipe file: " + path.string()); }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof()) { return errorResult("Failed while reading recipe file: " + path.string()); }
        return deserializeShipGenerationRecipe(buffer.str());
    }

    bool operator==(const ShipGenerationRecipe& first, const ShipGenerationRecipe& second)
    {
        try
        {
            ShipGenerationRecipeDocument firstDocument;
            firstDocument.Recipe = first;
            ShipGenerationRecipeDocument secondDocument;
            secondDocument.Recipe = second;
            return serializeShipGenerationRecipe(firstDocument) == serializeShipGenerationRecipe(secondDocument);
        }
        catch (...) { return false; }
    }

    bool operator!=(const ShipGenerationRecipe& first, const ShipGenerationRecipe& second) { return !(first == second); }
}
