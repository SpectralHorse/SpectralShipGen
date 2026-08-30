#include "ShipGenerationRecipeJson.h"

#include <cctype>
#include <charconv>
#include <limits>
#include <sstream>
#include <utility>

namespace PixelShipGenerator::RecipeJson
{
    Value Value::null() { return {}; }
    Value Value::boolean(bool value) { Value result; result.ValueType = Type::Boolean; result.Boolean = value; return result; }
    Value Value::number(uint64_t value) { Value result; result.ValueType = Type::Number; result.Text = std::to_string(value); return result; }
    Value Value::number(int64_t value) { Value result; result.ValueType = Type::Number; result.Text = std::to_string(value); return result; }
    Value Value::string(std::string value) { Value result; result.ValueType = Type::String; result.Text = std::move(value); return result; }
    Value Value::object() { Value result; result.ValueType = Type::Object; return result; }
    Value Value::array() { Value result; result.ValueType = Type::Array; return result; }

    const Value* Value::find(const std::string& key) const
    {
        const auto iterator = Object.find(key);
        return iterator == Object.end() ? nullptr : &iterator->second;
    }

    Value* Value::find(const std::string& key)
    {
        const auto iterator = Object.find(key);
        return iterator == Object.end() ? nullptr : &iterator->second;
    }

    namespace
    {
        class Parser
        {
        public:
            explicit Parser(const std::string& text) : m_Text(text) {}

            ParseResult run()
            {
                ParseResult result;
                skipWhitespace();
                if (!parseValue(result.Root, result.Error)) { return result; }
                skipWhitespace();
                if (m_Position != m_Text.size())
                {
                    result.Error = "Unexpected trailing JSON content at byte " + std::to_string(m_Position) + ".";
                    return result;
                }
                result.Success = true;
                return result;
            }

        private:
            void skipWhitespace()
            {
                while (m_Position < m_Text.size() && std::isspace(static_cast<unsigned char>(m_Text[m_Position]))) { ++m_Position; }
            }

            bool parseValue(Value& out, std::string& error)
            {
                skipWhitespace();
                if (m_Position >= m_Text.size()) { error = "Unexpected end of JSON input."; return false; }
                const char c = m_Text[m_Position];
                if (c == '{') { return parseObject(out, error); }
                if (c == '[') { return parseArray(out, error); }
                if (c == '"') { out.ValueType = Type::String; return parseString(out.Text, error); }
                if (c == 't') { return parseLiteral("true", Value::boolean(true), out, error); }
                if (c == 'f') { return parseLiteral("false", Value::boolean(false), out, error); }
                if (c == 'n') { return parseLiteral("null", Value::null(), out, error); }
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) { return parseNumber(out, error); }
                error = "Unexpected JSON token at byte " + std::to_string(m_Position) + ".";
                return false;
            }

            bool parseObject(Value& out, std::string& error)
            {
                ++m_Position;
                out = Value::object();
                skipWhitespace();
                if (m_Position < m_Text.size() && m_Text[m_Position] == '}') { ++m_Position; return true; }
                while (m_Position < m_Text.size())
                {
                    skipWhitespace();
                    if (m_Position >= m_Text.size() || m_Text[m_Position] != '"') { error = "Expected JSON object key at byte " + std::to_string(m_Position) + "."; return false; }
                    std::string key;
                    if (!parseString(key, error)) { return false; }
                    skipWhitespace();
                    if (m_Position >= m_Text.size() || m_Text[m_Position] != ':') { error = "Expected ':' after JSON object key at byte " + std::to_string(m_Position) + "."; return false; }
                    ++m_Position;
                    Value value;
                    if (!parseValue(value, error)) { return false; }
                    if (!out.Object.emplace(std::move(key), std::move(value)).second) { error = "Duplicate JSON object key."; return false; }
                    skipWhitespace();
                    if (m_Position >= m_Text.size()) { error = "Unterminated JSON object."; return false; }
                    if (m_Text[m_Position] == '}') { ++m_Position; return true; }
                    if (m_Text[m_Position] != ',') { error = "Expected ',' in JSON object at byte " + std::to_string(m_Position) + "."; return false; }
                    ++m_Position;
                }
                error = "Unterminated JSON object.";
                return false;
            }

            bool parseArray(Value& out, std::string& error)
            {
                ++m_Position;
                out = Value::array();
                skipWhitespace();
                if (m_Position < m_Text.size() && m_Text[m_Position] == ']') { ++m_Position; return true; }
                while (m_Position < m_Text.size())
                {
                    Value value;
                    if (!parseValue(value, error)) { return false; }
                    out.Array.push_back(std::move(value));
                    skipWhitespace();
                    if (m_Position >= m_Text.size()) { error = "Unterminated JSON array."; return false; }
                    if (m_Text[m_Position] == ']') { ++m_Position; return true; }
                    if (m_Text[m_Position] != ',') { error = "Expected ',' in JSON array at byte " + std::to_string(m_Position) + "."; return false; }
                    ++m_Position;
                }
                error = "Unterminated JSON array.";
                return false;
            }

            bool parseString(std::string& out, std::string& error)
            {
                if (m_Text[m_Position] != '"') { return false; }
                ++m_Position;
                out.clear();
                while (m_Position < m_Text.size())
                {
                    const char c = m_Text[m_Position++];
                    if (c == '"') { return true; }
                    if (static_cast<unsigned char>(c) < 0x20u) { error = "Control character in JSON string."; return false; }
                    if (c != '\\') { out.push_back(c); continue; }
                    if (m_Position >= m_Text.size()) { error = "Unterminated JSON string escape."; return false; }
                    const char escape = m_Text[m_Position++];
                    switch (escape)
                    {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: error = "Unsupported JSON string escape."; return false;
                    }
                }
                error = "Unterminated JSON string.";
                return false;
            }

            bool parseNumber(Value& out, std::string& error)
            {
                const std::size_t start = m_Position;
                if (m_Text[m_Position] == '-') { ++m_Position; }
                const std::size_t digitStart = m_Position;
                while (m_Position < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Position]))) { ++m_Position; }
                if (digitStart == m_Position) { error = "Invalid JSON number at byte " + std::to_string(start) + "."; return false; }
                if (m_Position < m_Text.size() && (m_Text[m_Position] == '.' || m_Text[m_Position] == 'e' || m_Text[m_Position] == 'E'))
                {
                    error = "Recipe JSON only accepts integral numeric values.";
                    return false;
                }
                out.ValueType = Type::Number;
                out.Text = m_Text.substr(start, m_Position - start);
                return true;
            }

            bool parseLiteral(const char* literal, const Value& value, Value& out, std::string& error)
            {
                const std::string token(literal);
                if (m_Text.compare(m_Position, token.size(), token) != 0) { error = "Invalid JSON literal at byte " + std::to_string(m_Position) + "."; return false; }
                m_Position += token.size();
                out = value;
                return true;
            }

            const std::string& m_Text;
            std::size_t m_Position = 0u;
        };

        std::string escapeString(const std::string& input)
        {
            std::string result;
            result.reserve(input.size() + 8u);
            for (const char c : input)
            {
                switch (c)
                {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result.push_back(c); break;
                }
            }
            return result;
        }

        void writeValue(const Value& value, std::ostringstream& stream, uint32_t indentSize, uint32_t depth)
        {
            switch (value.ValueType)
            {
            case Type::Null: stream << "null"; break;
            case Type::Boolean: stream << (value.Boolean ? "true" : "false"); break;
            case Type::Number: stream << value.Text; break;
            case Type::String: stream << '"' << escapeString(value.Text) << '"'; break;
            case Type::Object:
            {
                stream << '{';
                if (!value.Object.empty()) { stream << '\n'; }
                std::size_t index = 0u;
                for (const auto& [key, child] : value.Object)
                {
                    stream << std::string((depth + 1u) * indentSize, ' ') << '"' << escapeString(key) << "\": ";
                    writeValue(child, stream, indentSize, depth + 1u);
                    if (++index < value.Object.size()) { stream << ','; }
                    stream << '\n';
                }
                if (!value.Object.empty()) { stream << std::string(depth * indentSize, ' '); }
                stream << '}';
                break;
            }
            case Type::Array:
            {
                stream << '[';
                if (!value.Array.empty()) { stream << '\n'; }
                for (std::size_t index = 0u; index < value.Array.size(); ++index)
                {
                    stream << std::string((depth + 1u) * indentSize, ' ');
                    writeValue(value.Array[index], stream, indentSize, depth + 1u);
                    if (index + 1u < value.Array.size()) { stream << ','; }
                    stream << '\n';
                }
                if (!value.Array.empty()) { stream << std::string(depth * indentSize, ' '); }
                stream << ']';
                break;
            }
            }
        }

        bool requireType(const Value& value, Type expected, std::string& error, const std::string& path)
        {
            if (value.ValueType == expected) { return true; }
            error = path + " has the wrong JSON type.";
            return false;
        }

        const Value* getRequired(const Value& parent, const std::string& key, std::string& error, const std::string& path)
        {
            if (!requireType(parent, Type::Object, error, path)) { return nullptr; }
            const Value* value = parent.find(key);
            if (value == nullptr) { error = "Missing required field: " + path + "." + key + "."; }
            return value;
        }
    }

    ParseResult parse(const std::string& text) { return Parser(text).run(); }

    std::string stringify(const Value& value, uint32_t indentSize)
    {
        std::ostringstream stream;
        writeValue(value, stream, indentSize, 0u);
        stream << '\n';
        return stream.str();
    }

    bool getObject(const Value& parent, const std::string& key, const Value*& out, std::string& error, const std::string& path)
    {
        out = getRequired(parent, key, error, path);
        return out != nullptr && requireType(*out, Type::Object, error, path + "." + key);
    }

    bool getString(const Value& parent, const std::string& key, std::string& out, std::string& error, const std::string& path)
    {
        const Value* value = getRequired(parent, key, error, path);
        if (value == nullptr || !requireType(*value, Type::String, error, path + "." + key)) { return false; }
        out = value->Text;
        return true;
    }

    bool getBool(const Value& parent, const std::string& key, bool& out, std::string& error, const std::string& path)
    {
        const Value* value = getRequired(parent, key, error, path);
        if (value == nullptr || !requireType(*value, Type::Boolean, error, path + "." + key)) { return false; }
        out = value->Boolean;
        return true;
    }

    bool getUInt64(const Value& parent, const std::string& key, uint64_t& out, std::string& error, const std::string& path)
    {
        const Value* value = getRequired(parent, key, error, path);
        if (value == nullptr || !requireType(*value, Type::Number, error, path + "." + key)) { return false; }
        if (!value->Text.empty() && value->Text.front() == '-') { error = path + "." + key + " must be non-negative."; return false; }
        uint64_t parsed = 0u;
        const auto conversion = std::from_chars(value->Text.data(), value->Text.data() + value->Text.size(), parsed);
        if (conversion.ec != std::errc() || conversion.ptr != value->Text.data() + value->Text.size()) { error = "Invalid unsigned integer at " + path + "." + key + "."; return false; }
        out = parsed;
        return true;
    }

    bool getUInt32(const Value& parent, const std::string& key, uint32_t& out, std::string& error, const std::string& path)
    {
        uint64_t value = 0u;
        if (!getUInt64(parent, key, value, error, path)) { return false; }
        if (value > std::numeric_limits<uint32_t>::max()) { error = path + "." + key + " exceeds uint32 range."; return false; }
        out = static_cast<uint32_t>(value);
        return true;
    }

    bool getInt32(const Value& parent, const std::string& key, int32_t& out, std::string& error, const std::string& path)
    {
        const Value* value = getRequired(parent, key, error, path);
        if (value == nullptr || !requireType(*value, Type::Number, error, path + "." + key)) { return false; }
        int64_t parsed = 0;
        const auto conversion = std::from_chars(value->Text.data(), value->Text.data() + value->Text.size(), parsed);
        if (conversion.ec != std::errc() || conversion.ptr != value->Text.data() + value->Text.size() || parsed < std::numeric_limits<int32_t>::min() || parsed > std::numeric_limits<int32_t>::max())
        {
            error = "Invalid int32 at " + path + "." + key + ".";
            return false;
        }
        out = static_cast<int32_t>(parsed);
        return true;
    }
}
