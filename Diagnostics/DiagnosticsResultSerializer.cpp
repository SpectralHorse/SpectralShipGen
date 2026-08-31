#include <PixelShipGenerator/Diagnostics/DiagnosticsResultSerializer.h>

#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace PixelShipGeneratorDiagnostics
{
    namespace
    {
        enum class JsonType { NIL, BOOL, NUMBER, STRING, ARRAY, OBJECT };
        struct JsonValue
        {
            JsonType Type = JsonType::NIL;
            bool Bool = false;
            double Number = 0.0;
            std::string NumberToken;
            std::string String;
            std::vector<JsonValue> Array;
            std::map<std::string, JsonValue> Object;
        };

        class JsonParser
        {
        public:
            explicit JsonParser(const std::string& text) : m_Text(text) {}
            JsonValue parse()
            {
                skip();
                JsonValue result = parseValue();
                skip();
                if (m_Position != m_Text.size()) { fail("Unexpected trailing JSON content."); }
                return result;
            }
        private:
            [[noreturn]] void fail(const char* message) const { throw std::runtime_error(std::string(message) + " At offset " + std::to_string(m_Position) + "."); }
            void skip() { while (m_Position < m_Text.size() && std::isspace(static_cast<unsigned char>(m_Text[m_Position]))) { ++m_Position; } }
            bool take(char c) { skip(); if (m_Position < m_Text.size() && m_Text[m_Position] == c) { ++m_Position; return true; } return false; }
            JsonValue parseValue()
            {
                skip();
                if (m_Position >= m_Text.size()) { fail("Unexpected end of JSON."); }
                const char c = m_Text[m_Position];
                if (c == '{') { return parseObject(); }
                if (c == '[') { return parseArray(); }
                if (c == '"') { JsonValue v; v.Type = JsonType::STRING; v.String = parseString(); return v; }
                if (c == 't' && m_Text.compare(m_Position, 4u, "true") == 0) { m_Position += 4u; JsonValue v; v.Type = JsonType::BOOL; v.Bool = true; return v; }
                if (c == 'f' && m_Text.compare(m_Position, 5u, "false") == 0) { m_Position += 5u; JsonValue v; v.Type = JsonType::BOOL; return v; }
                if (c == 'n' && m_Text.compare(m_Position, 4u, "null") == 0) { m_Position += 4u; return {}; }
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) { JsonValue v; v.Type = JsonType::NUMBER; v.NumberToken = parseNumberToken(); try { v.Number = std::stod(v.NumberToken); } catch (...) { fail("Invalid number."); } return v; }
                fail("Unsupported JSON value.");
            }
            JsonValue parseObject()
            {
                JsonValue v; v.Type = JsonType::OBJECT; if (!take('{')) { fail("Expected object."); }
                skip(); if (take('}')) { return v; }
                while (true)
                {
                    skip(); if (m_Position >= m_Text.size() || m_Text[m_Position] != '"') { fail("Expected object key."); }
                    const std::string key = parseString(); if (!take(':')) { fail("Expected colon."); }
                    v.Object.emplace(key, parseValue());
                    if (take('}')) { return v; }
                    if (!take(',')) { fail("Expected comma in object."); }
                }
            }
            JsonValue parseArray()
            {
                JsonValue v; v.Type = JsonType::ARRAY; if (!take('[')) { fail("Expected array."); }
                skip(); if (take(']')) { return v; }
                while (true)
                {
                    v.Array.push_back(parseValue());
                    if (take(']')) { return v; }
                    if (!take(',')) { fail("Expected comma in array."); }
                }
            }
            std::string parseString()
            {
                if (m_Text[m_Position++] != '"') { fail("Expected string."); }
                std::string result;
                while (m_Position < m_Text.size())
                {
                    const char c = m_Text[m_Position++];
                    if (c == '"') { return result; }
                    if (c != '\\') { result.push_back(c); continue; }
                    if (m_Position >= m_Text.size()) { fail("Invalid string escape."); }
                    const char e = m_Text[m_Position++];
                    switch (e)
                    {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    default: fail("Unsupported JSON string escape.");
                    }
                }
                fail("Unterminated JSON string.");
            }
            std::string parseNumberToken()
            {
                const std::size_t start = m_Position;
                if (m_Text[m_Position] == '-') { ++m_Position; }
                while (m_Position < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Position]))) { ++m_Position; }
                if (m_Position < m_Text.size() && m_Text[m_Position] == '.') { ++m_Position; while (m_Position < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Position]))) { ++m_Position; } }
                if (m_Position < m_Text.size() && (m_Text[m_Position] == 'e' || m_Text[m_Position] == 'E')) { ++m_Position; if (m_Position < m_Text.size() && (m_Text[m_Position] == '+' || m_Text[m_Position] == '-')) { ++m_Position; } while (m_Position < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Position]))) { ++m_Position; } }
                return m_Text.substr(start, m_Position - start);
            }
            const std::string& m_Text;
            std::size_t m_Position = 0u;
        };

        std::string escaped(const std::string& value)
        {
            std::ostringstream out;
            for (const unsigned char c : value)
            {
                switch (c)
                {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (c < 0x20u) { out << '?'; }
                    else { out << static_cast<char>(c); }
                    break;
                }
            }
            return out.str();
        }

        const JsonValue* optionalMember(const JsonValue& object, const char* key)
        {
            if (object.Type != JsonType::OBJECT) { throw std::runtime_error("Expected JSON object."); }
            const auto it = object.Object.find(key);
            return it == object.Object.end() ? nullptr : &it->second;
        }

        const JsonValue& member(const JsonValue& object, const char* key)
        {
            if (object.Type != JsonType::OBJECT) { throw std::runtime_error("Expected JSON object."); }
            const auto it = object.Object.find(key);
            if (it == object.Object.end()) { throw std::runtime_error(std::string("Missing JSON field: ") + key); }
            return it->second;
        }
        uint64_t u64(const JsonValue& value)
        {
            if (value.Type != JsonType::NUMBER || value.NumberToken.empty() || value.NumberToken[0] == '-' || value.NumberToken.find_first_of(".eE") != std::string::npos) { throw std::runtime_error("Expected unsigned JSON integer."); }
            try
            {
                std::size_t parsed = 0u;
                const unsigned long long result = std::stoull(value.NumberToken, &parsed, 10);
                if (parsed != value.NumberToken.size()) { throw std::runtime_error("Expected unsigned JSON integer."); }
                return static_cast<uint64_t>(result);
            }
            catch (const std::out_of_range&) { throw std::runtime_error("Unsigned JSON integer overflow."); }
            catch (const std::invalid_argument&) { throw std::runtime_error("Expected unsigned JSON integer."); }
        }
        uint32_t u32(const JsonValue& value)
        {
            const uint64_t value64 = u64(value); if (value64 > std::numeric_limits<uint32_t>::max()) { throw std::runtime_error("JSON uint32 overflow."); } return static_cast<uint32_t>(value64);
        }
        double number(const JsonValue& value) { if (value.Type != JsonType::NUMBER || !std::isfinite(value.Number)) { throw std::runtime_error("Expected finite JSON number."); } return value.Number; }
        bool boolean(const JsonValue& value) { if (value.Type != JsonType::BOOL) { throw std::runtime_error("Expected JSON boolean."); } return value.Bool; }
        std::string stringValue(const JsonValue& value) { if (value.Type != JsonType::STRING) { throw std::runtime_error("Expected JSON string."); } return value.String; }

        template <typename T>
        void writeIntegerArray(std::ostream& out, const T& values)
        {
            out << '[';
            for (std::size_t i = 0u; i < values.size(); ++i) { if (i) { out << ','; } out << static_cast<uint64_t>(values[i]); }
            out << ']';
        }

        void rebuildLoadedAggregates(DiagnosticsResult& result)
        {
            result.OverallSummary = aggregateDiagnosticsSamples(result.Samples);
            result.ConfigurationResults.clear();
            uint64_t configurationIndex = 0u;
            for (const auto dimensions : result.Configuration.Dimensions)
            {
                for (const auto style : result.Configuration.Styles)
                {
                    for (const auto faction : result.Configuration.Factions)
                    {
                        DiagnosticsConfigurationResult configurationResult;
                        configurationResult.Configuration.Width = dimensions.Width;
                        configurationResult.Configuration.Height = dimensions.Height;
                        configurationResult.Configuration.Style = style;
                        configurationResult.Configuration.Faction = faction;
                        configurationResult.Configuration.DetailDensity = result.Configuration.DetailDensity;
                        configurationResult.Configuration.AsymmetricDetailChance = result.Configuration.AsymmetricDetailChance;
                        configurationResult.Configuration.AttachmentsEnabled = result.Configuration.AttachmentsEnabled;
                        configurationResult.Configuration.Samples = result.Configuration.SamplesPerConfiguration;
                        configurationResult.Configuration.DiagnosticSeed = result.Configuration.DiagnosticSeed;
                        std::vector<DiagnosticsRawSampleResult> selected;
                        for (const auto& sample : result.Samples) { if (sample.WorkItem.ConfigurationIndex == configurationIndex) { selected.push_back(sample); } }
                        configurationResult.PerformanceSummary = aggregateDiagnosticsSamples(selected);
                        result.ConfigurationResults.push_back(std::move(configurationResult));
                        ++configurationIndex;
                    }
                }
            }
        }
    }

    std::string serializeDiagnosticsResultJson(const DiagnosticsResult& result)
    {
        std::ostringstream csv;
        writeDiagnosticsResultCsv(csv, result);
        std::ostringstream out;
        out << std::setprecision(17);
        out << "{\n  \"schema_version\":" << DiagnosticsResultSchemaVersion << ",\n";
        out << "  \"completed\":" << (result.Completed ? "true" : "false") << ",\n";
        out << "  \"cancelled\":" << (result.Cancelled ? "true" : "false") << ",\n";
        out << "  \"scheduled_work_items\":" << result.ScheduledWorkItems << ",\n";
        out << "  \"completed_work_items\":" << result.CompletedWorkItems << ",\n";
        out << "  \"elapsed_ns\":" << result.ElapsedNanoseconds << ",\n";
        out << "  \"configuration\":{\n";
        out << "    \"dimensions\":[";
        for (std::size_t i = 0u; i < result.Configuration.Dimensions.size(); ++i) { if (i) { out << ','; } out << "{\"width\":" << result.Configuration.Dimensions[i].Width << ",\"height\":" << result.Configuration.Dimensions[i].Height << '}'; }
        out << "],\n    \"styles\":[";
        for (std::size_t i = 0u; i < result.Configuration.Styles.size(); ++i) { if (i) { out << ','; } out << static_cast<uint32_t>(result.Configuration.Styles[i]); }
        out << "],\n    \"factions\":[";
        for (std::size_t i = 0u; i < result.Configuration.Factions.size(); ++i) { if (i) { out << ','; } out << static_cast<uint32_t>(result.Configuration.Factions[i]); }
        out << "],\n";
        out << "    \"samples_per_configuration\":" << result.Configuration.SamplesPerConfiguration << ",\n";
        out << "    \"diagnostic_seed\":" << result.Configuration.DiagnosticSeed << ",\n";
        out << "    \"detail_density\":" << result.Configuration.DetailDensity << ",\n";
        out << "    \"asymmetric_detail_chance\":" << result.Configuration.AsymmetricDetailChance << ",\n";
        out << "    \"attachments_enabled\":" << (result.Configuration.AttachmentsEnabled ? "true" : "false") << ",\n";
        out << "    \"enabled_categories\":" << result.Configuration.EnabledCategories << ",\n";
        out << "    \"detailed_timing\":" << (result.Configuration.DetailedPerformanceInstrumentation ? "true" : "false") << ",\n";
        out << "    \"detail_level\":" << static_cast<uint32_t>(result.Configuration.DetailLevel) << ",\n";
        out << "    \"build_configuration\":\"" << escaped(result.Configuration.BuildConfiguration) << "\",\n";
        out << "    \"version_identifier\":\"" << escaped(result.Configuration.VersionIdentifier) << "\",\n";
        out << "    \"configuration_label\":\"" << escaped(result.Configuration.ConfigurationLabel) << "\",\n";
        out << "    \"palette_source_mode\":" << static_cast<uint32_t>(result.Configuration.PaletteSourceMode) << "\n  },\n";
        out << "  \"samples\":[\n";
        for (std::size_t i = 0u; i < result.Samples.size(); ++i)
        {
            const auto& s = result.Samples[i];
            out << "    {\"work_index\":" << s.WorkItem.WorkIndex << ",\"configuration_index\":" << s.WorkItem.ConfigurationIndex << ",\"sample_index\":" << s.WorkItem.SampleIndex << ",\"seed\":" << s.WorkItem.Seed
                << ",\"width\":" << s.WorkItem.Dimensions.Width << ",\"height\":" << s.WorkItem.Dimensions.Height << ",\"style\":" << static_cast<uint32_t>(s.WorkItem.Style) << ",\"faction\":" << static_cast<uint32_t>(s.WorkItem.Faction)
                << ",\"success\":" << (s.Success ? "true" : "false") << ",\"error\":\"" << escaped(s.ErrorMessage) << "\",\"total_ns\":" << s.TotalGenerationNanoseconds
                << ",\"stage_ns\":"; writeIntegerArray(out, s.Performance.StageDurationNanoseconds);
            out << ",\"hull_attempts\":" << s.HullAttemptCount << ",\"hull_rejections\":" << s.HullValidationRejectionCount << ",\"silhouette_rejections\":"; writeIntegerArray(out, s.SilhouetteRejectionCounts);
            out << ",\"negative_space_attempts\":" << s.StructuralNegativeSpaceAttemptCount << ",\"negative_space_successes\":" << s.StructuralNegativeSpaceSuccessCount
                << ",\"major_feature_attempts\":" << s.MajorFeaturePlacementAttemptCount << ",\"major_feature_rejections\":" << s.MajorFeaturePlacementRejectionCount
                << ",\"weapon_attempts\":" << s.WeaponPlacementAttemptCount << ",\"weapon_rejections\":" << s.WeaponPlacementRejectionCount
                << ",\"weapon_requested_groups\":" << s.WeaponRequestedGroupCount << ",\"weapon_realized_groups\":" << s.WeaponRealizedGroupCount
                << ",\"weapon_chance_skips\":" << s.WeaponGenerationChanceSkipCount << ",\"weapon_no_hardpoint\":" << s.WeaponNoHardpointFailureCount
                << ",\"weapon_type_selection_failures\":" << s.WeaponTypeSelectionFailureCount
                << ",\"weapon_geometry_failures\":" << s.WeaponCandidateGeometryFailureCount << ",\"weapon_semantic_failures\":" << s.WeaponSemanticCollisionFailureCount
                << ",\"weapon_connectivity_failures\":" << s.WeaponConnectivityFailureCount << ",\"weapon_clearance_failures\":" << s.WeaponFiringClearanceFailureCount
                << ",\"weapon_pair_failures\":" << s.WeaponSymmetryPairFailureCount << ",\"weapon_spatial_rejections\":" << s.WeaponSpatialBudgetRejectionCount
                << ",\"weapon_complexity_rejections\":" << s.WeaponComplexityBudgetRejectionCount << ",\"weapon_coverage_permille\":" << s.WeaponCoveragePermille
                << ",\"weapon_anchor_opportunity\":" << (s.WeaponVisualAnchorOpportunity ? "true" : "false") << ",\"weapon_anchor_realized\":" << (s.WeaponVisualAnchorRealized ? "true" : "false")
                << ",\"attachment_attempts\":" << s.AttachmentPlacementAttemptCount << ",\"attachment_failures\":" << s.AttachmentPlacementFailureCount
                << ",\"material_zones\":" << s.MaterialZoneCount << ",\"livery_markings\":" << s.LiveryMarkingCount
                << ",\"livery_primary_coverage_permille\":" << s.LiveryPrimaryCoveragePermille << ",\"livery_secondary_coverage_permille\":" << s.LiverySecondaryCoveragePermille
                << ",\"livery_coverage_permille\":" << s.LiveryCoveragePermille << ",\"livery_largest_connected_coverage_permille\":" << s.LiveryLargestConnectedCoveragePermille
                << ",\"livery_secondary_material_coverage_permille\":" << s.LiverySecondaryMaterialCoveragePermille << ",\"livery_mechanical_material_coverage_permille\":" << s.LiveryMechanicalMaterialCoveragePermille
                << ",\"livery_coverage_rejections\":" << s.LiveryCoverageRejectionCount << ",\"livery_material_rejections\":" << s.LiveryMaterialPreservationRejectionCount
                << ",\"detail_motif_occurrences\":" << s.DetailMotifOccurrenceCount
                << ",\"major_features\":" << s.MajorFeatureCount << ",\"weapons\":" << s.WeaponCount << ",\"engines\":" << s.EngineCount
                << ",\"complexity_utilization_percent\":" << s.ComplexityUtilizationPercent << ",\"primary_visual_anchor\":" << static_cast<uint32_t>(s.PrimaryVisualAnchor)
                << ",\"secondary_visual_anchor\":" << static_cast<uint32_t>(s.SecondaryVisualAnchor) << ",\"visual_hierarchy_fallback\":" << (s.VisualHierarchyFallbackOccurred ? "true" : "false")
                << ",\"image_signature\":" << s.FinalImageSignature << '}' << (i + 1u < result.Samples.size() ? "," : "") << '\n';
        }
        out << "  ],\n  \"csv_snapshot\":\"" << escaped(csv.str()) << "\"\n}\n";
        return out.str();
    }

    DiagnosticsResultLoadResult deserializeDiagnosticsResultJson(const std::string& jsonText)
    {
        DiagnosticsResultLoadResult load;
        try
        {
            const JsonValue root = JsonParser(jsonText).parse();
            const uint32_t schema = u32(member(root, "schema_version"));
            if (schema != DiagnosticsResultSchemaVersion) { throw std::runtime_error("Unsupported .shipdiag.json schema version: " + std::to_string(schema)); }
            DiagnosticsResult result;
            result.Configuration.Dimensions.clear();
            result.Configuration.Styles.clear();
            result.Configuration.Factions.clear();
            result.Completed = boolean(member(root, "completed"));
            result.Cancelled = boolean(member(root, "cancelled"));
            result.ScheduledWorkItems = u64(member(root, "scheduled_work_items"));
            result.CompletedWorkItems = u64(member(root, "completed_work_items"));
            result.ElapsedNanoseconds = u64(member(root, "elapsed_ns"));
            const JsonValue& config = member(root, "configuration");
            const JsonValue& dims = member(config, "dimensions"); if (dims.Type != JsonType::ARRAY) { throw std::runtime_error("configuration.dimensions must be an array."); }
            for (const auto& d : dims.Array) { result.Configuration.Dimensions.push_back({ u32(member(d, "width")), u32(member(d, "height")) }); }
            const JsonValue& styles = member(config, "styles"); if (styles.Type != JsonType::ARRAY) { throw std::runtime_error("configuration.styles must be an array."); }
            for (const auto& value : styles.Array) { const uint32_t index = u32(value); if (index > static_cast<uint32_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)) { throw std::runtime_error("Invalid style in diagnostics file."); } result.Configuration.Styles.push_back(static_cast<PixelShipGenerator::ShipStyle>(index)); }
            const JsonValue& factions = member(config, "factions"); if (factions.Type != JsonType::ARRAY) { throw std::runtime_error("configuration.factions must be an array."); }
            for (const auto& value : factions.Array) { const uint32_t index = u32(value); if (index > static_cast<uint32_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)) { throw std::runtime_error("Invalid faction in diagnostics file."); } result.Configuration.Factions.push_back(static_cast<PixelShipGenerator::ShipFactionType>(index)); }
            result.Configuration.SamplesPerConfiguration = u64(member(config, "samples_per_configuration"));
            result.Configuration.DiagnosticSeed = u64(member(config, "diagnostic_seed"));
            result.Configuration.DetailDensity = u32(member(config, "detail_density"));
            result.Configuration.AsymmetricDetailChance = u32(member(config, "asymmetric_detail_chance"));
            result.Configuration.AttachmentsEnabled = boolean(member(config, "attachments_enabled"));
            result.Configuration.EnabledCategories = u32(member(config, "enabled_categories"));
            result.Configuration.DetailedPerformanceInstrumentation = boolean(member(config, "detailed_timing"));
            result.Configuration.DetailLevel = static_cast<DiagnosticsDetailLevel>(u32(member(config, "detail_level")));
            result.Configuration.BuildConfiguration = stringValue(member(config, "build_configuration"));
            result.Configuration.VersionIdentifier = stringValue(member(config, "version_identifier"));
            if (const JsonValue* value = optionalMember(config, "configuration_label")) { result.Configuration.ConfigurationLabel = stringValue(*value); }
            if (const JsonValue* value = optionalMember(config, "palette_source_mode"))
            {
                const uint32_t mode = u32(*value);
                if (mode >= static_cast<uint32_t>(PixelShipGenerator::ShipPaletteSourceMode::SHIP_PALETTE_SOURCE_MODE_END)) { throw std::runtime_error("Invalid diagnostics palette source mode."); }
                result.Configuration.PaletteSourceMode = static_cast<PixelShipGenerator::ShipPaletteSourceMode>(mode);
            }
            const JsonValue& samples = member(root, "samples"); if (samples.Type != JsonType::ARRAY) { throw std::runtime_error("samples must be an array."); }
            result.Samples.reserve(samples.Array.size());
            for (const auto& j : samples.Array)
            {
                DiagnosticsRawSampleResult s;
                s.WorkItem.WorkIndex = u64(member(j, "work_index")); s.WorkItem.ConfigurationIndex = u64(member(j, "configuration_index")); s.WorkItem.SampleIndex = u64(member(j, "sample_index")); s.WorkItem.Seed = u64(member(j, "seed"));
                s.WorkItem.Dimensions = { u32(member(j, "width")), u32(member(j, "height")) };
                const uint32_t style = u32(member(j, "style")); const uint32_t faction = u32(member(j, "faction"));
                if (style > static_cast<uint32_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END) || faction > static_cast<uint32_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)) { throw std::runtime_error("Invalid sample style/faction."); }
                s.WorkItem.Style = static_cast<PixelShipGenerator::ShipStyle>(style); s.WorkItem.Faction = static_cast<PixelShipGenerator::ShipFactionType>(faction);
                s.Success = boolean(member(j, "success")); s.ErrorMessage = stringValue(member(j, "error")); s.TotalGenerationNanoseconds = u64(member(j, "total_ns"));
                const JsonValue& stages = member(j, "stage_ns"); if (stages.Type != JsonType::ARRAY || stages.Array.size() != s.Performance.StageDurationNanoseconds.size()) { throw std::runtime_error("Invalid stage timing array."); }
                for (std::size_t index = 0u; index < stages.Array.size(); ++index) { s.Performance.StageDurationNanoseconds[index] = u64(stages.Array[index]); }
                s.Performance.TotalDurationNanoseconds = s.TotalGenerationNanoseconds;
                s.HullAttemptCount = u32(member(j, "hull_attempts")); s.HullValidationRejectionCount = u32(member(j, "hull_rejections"));
                const JsonValue& rejections = member(j, "silhouette_rejections"); if (rejections.Type != JsonType::ARRAY || rejections.Array.size() != s.SilhouetteRejectionCounts.size()) { throw std::runtime_error("Invalid silhouette rejection array."); }
                for (std::size_t index = 0u; index < rejections.Array.size(); ++index) { s.SilhouetteRejectionCounts[index] = u32(rejections.Array[index]); }
                s.StructuralNegativeSpaceAttemptCount = u32(member(j, "negative_space_attempts")); s.StructuralNegativeSpaceSuccessCount = u32(member(j, "negative_space_successes"));
                s.MajorFeaturePlacementAttemptCount = u32(member(j, "major_feature_attempts")); s.MajorFeaturePlacementRejectionCount = u32(member(j, "major_feature_rejections"));
                s.WeaponPlacementAttemptCount = u32(member(j, "weapon_attempts")); s.WeaponPlacementRejectionCount = u32(member(j, "weapon_rejections"));
                if (const JsonValue* value = optionalMember(j, "weapon_requested_groups")) { s.WeaponRequestedGroupCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_realized_groups")) { s.WeaponRealizedGroupCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_chance_skips")) { s.WeaponGenerationChanceSkipCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_no_hardpoint")) { s.WeaponNoHardpointFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_type_selection_failures")) { s.WeaponTypeSelectionFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_geometry_failures")) { s.WeaponCandidateGeometryFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_semantic_failures")) { s.WeaponSemanticCollisionFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_connectivity_failures")) { s.WeaponConnectivityFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_clearance_failures")) { s.WeaponFiringClearanceFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_pair_failures")) { s.WeaponSymmetryPairFailureCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_spatial_rejections")) { s.WeaponSpatialBudgetRejectionCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_complexity_rejections")) { s.WeaponComplexityBudgetRejectionCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_coverage_permille")) { s.WeaponCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_anchor_opportunity")) { s.WeaponVisualAnchorOpportunity = boolean(*value); }
                if (const JsonValue* value = optionalMember(j, "weapon_anchor_realized")) { s.WeaponVisualAnchorRealized = boolean(*value); }
                s.AttachmentPlacementAttemptCount = u32(member(j, "attachment_attempts")); s.AttachmentPlacementFailureCount = u32(member(j, "attachment_failures"));
                s.MaterialZoneCount = u32(member(j, "material_zones")); s.LiveryMarkingCount = u32(member(j, "livery_markings"));
                if (const JsonValue* value = optionalMember(j, "livery_primary_coverage_permille")) { s.LiveryPrimaryCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_secondary_coverage_permille")) { s.LiverySecondaryCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_coverage_permille")) { s.LiveryCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_largest_connected_coverage_permille")) { s.LiveryLargestConnectedCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_secondary_material_coverage_permille")) { s.LiverySecondaryMaterialCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_mechanical_material_coverage_permille")) { s.LiveryMechanicalMaterialCoveragePermille = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_coverage_rejections")) { s.LiveryCoverageRejectionCount = u32(*value); }
                if (const JsonValue* value = optionalMember(j, "livery_material_rejections")) { s.LiveryMaterialPreservationRejectionCount = u32(*value); }
                s.DetailMotifOccurrenceCount = u32(member(j, "detail_motif_occurrences"));
                s.MajorFeatureCount = u32(member(j, "major_features")); s.WeaponCount = u32(member(j, "weapons")); s.EngineCount = u32(member(j, "engines")); s.ComplexityUtilizationPercent = number(member(j, "complexity_utilization_percent"));
                const uint32_t primary = u32(member(j, "primary_visual_anchor")); const uint32_t secondary = u32(member(j, "secondary_visual_anchor"));
                if (primary > static_cast<uint32_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END) || secondary > static_cast<uint32_t>(PixelShipGenerator::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)) { throw std::runtime_error("Invalid visual anchor."); }
                s.PrimaryVisualAnchor = static_cast<PixelShipGenerator::ShipVisualAnchorType>(primary); s.SecondaryVisualAnchor = static_cast<PixelShipGenerator::ShipVisualAnchorType>(secondary); s.VisualHierarchyFallbackOccurred = boolean(member(j, "visual_hierarchy_fallback"));
                s.FinalImageSignature = u64(member(j, "image_signature"));
                result.Samples.push_back(std::move(s));
            }
            result.PersistedCsvSnapshot = stringValue(member(root, "csv_snapshot"));
            if (result.CompletedWorkItems != result.Samples.size()) { throw std::runtime_error("completed_work_items does not match saved sample count."); }
            rebuildLoadedAggregates(result);
            load.Success = true; load.Result = std::move(result);
        }
        catch (const std::exception& exception) { load.Error = exception.what(); }
        return load;
    }

    bool saveDiagnosticsResultJson(const std::filesystem::path& path, const DiagnosticsResult& result, std::string& errorMessage)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) { errorMessage = "Unable to open diagnostics run path: " + path.string(); return false; }
        stream << serializeDiagnosticsResultJson(result);
        if (!stream.good()) { errorMessage = "Failed while writing diagnostics run: " + path.string(); return false; }
        errorMessage.clear(); return true;
    }

    DiagnosticsResultLoadResult loadDiagnosticsResultJson(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) { DiagnosticsResultLoadResult result; result.Error = "Unable to open diagnostics run path: " + path.string(); return result; }
        std::ostringstream buffer; buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof()) { DiagnosticsResultLoadResult result; result.Error = "Failed while reading diagnostics run: " + path.string(); return result; }
        return deserializeDiagnosticsResultJson(buffer.str());
    }
}
