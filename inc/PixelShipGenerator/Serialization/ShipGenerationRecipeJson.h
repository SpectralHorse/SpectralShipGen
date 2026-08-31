#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace PixelShipGenerator::RecipeJson
{
    enum class Type : uint32_t
    {
        Null = 0u,
        Boolean,
        Number,
        String,
        Object,
        Array
    };

    struct Value
    {
        Type ValueType = Type::Null;
        bool Boolean = false;
        std::string Text;
        std::map<std::string, Value> Object;
        std::vector<Value> Array;

        static Value null();
        static Value boolean(bool value);
        static Value number(uint64_t value);
        static Value number(int64_t value);
        static Value string(std::string value);
        static Value object();
        static Value array();

        const Value* find(const std::string& key) const;
        Value* find(const std::string& key);
    };

    struct ParseResult
    {
        bool Success = false;
        Value Root;
        std::string Error;
    };

    ParseResult parse(const std::string& text);
    std::string stringify(const Value& value, uint32_t indentSize = 2u);

    bool getObject(const Value& parent, const std::string& key, const Value*& out, std::string& error, const std::string& path);
    bool getString(const Value& parent, const std::string& key, std::string& out, std::string& error, const std::string& path);
    bool getBool(const Value& parent, const std::string& key, bool& out, std::string& error, const std::string& path);
    bool getUInt32(const Value& parent, const std::string& key, uint32_t& out, std::string& error, const std::string& path);
    bool getUInt64(const Value& parent, const std::string& key, uint64_t& out, std::string& error, const std::string& path);
    bool getInt32(const Value& parent, const std::string& key, int32_t& out, std::string& error, const std::string& path);
}
