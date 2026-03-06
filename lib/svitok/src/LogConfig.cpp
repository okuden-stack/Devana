/**
 * @file    LogConfig.cpp
 * @brief   Logging configuration implementation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#include "LogConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace devana::logging
{

    // ───────────────────────────── minimal JSON tokeniser ─────────────────────────
    //
    // Handles the subset of JSON that LogConfig produces:
    //   - string values (with \" and \\ escapes)
    //   - integer / size_t numeric values (no floats needed)
    //   - boolean literals (true / false)
    //   - a single level of object nesting (for "rotation")
    //   - no arrays, no null
    //
    // On any parse error, the function writes to std::cerr and returns std::nullopt.

    namespace
    {

        // ── helpers ──────────────────────────────────────────────────────────────

        void skipWhitespace(std::string_view sv, size_t &pos) noexcept
        {
            while (pos < sv.size() && std::isspace(static_cast<unsigned char>(sv[pos])))
                ++pos;
        }

        // Consume and return a JSON-quoted string (the opening '"' must be at sv[pos]).
        // Returns std::nullopt on error.
        std::optional<std::string> parseString(std::string_view sv, size_t &pos)
        {
            if (pos >= sv.size() || sv[pos] != '"')
                return std::nullopt;

            ++pos; // skip opening quote
            std::string result;
            while (pos < sv.size())
            {
                char c = sv[pos];
                if (c == '\\' && pos + 1 < sv.size())
                {
                    char next = sv[pos + 1];
                    if (next == '"' || next == '\\' || next == '/')
                    {
                        result += next;
                        pos += 2;
                        continue;
                    }
                    if (next == 'n') { result += '\n'; pos += 2; continue; }
                    if (next == 't') { result += '\t'; pos += 2; continue; }
                    if (next == 'r') { result += '\r'; pos += 2; continue; }
                    // Unknown escape — keep as-is
                    result += c;
                    ++pos;
                    continue;
                }
                if (c == '"')
                {
                    ++pos; // skip closing quote
                    return result;
                }
                result += c;
                ++pos;
            }
            return std::nullopt; // unterminated string
        }

        // A very small variant that holds the types we care about.
        struct JsonValue
        {
            enum class Type { String, Integer, Boolean, Object, None };
            Type type = Type::None;

            std::string stringVal;
            int64_t intVal = 0;
            bool boolVal = false;

            // For the "rotation" sub-object only — flat key→value map.
            std::unordered_map<std::string, JsonValue> objectVal;
        };

        // Forward declaration.
        std::optional<JsonValue> parseValue(std::string_view sv, size_t &pos);

        // Parse a JSON object ( { ... } ) into a flat string→JsonValue map.
        std::optional<std::unordered_map<std::string, JsonValue>>
        parseObject(std::string_view sv, size_t &pos)
        {
            skipWhitespace(sv, pos);
            if (pos >= sv.size() || sv[pos] != '{')
                return std::nullopt;
            ++pos; // skip '{'

            std::unordered_map<std::string, JsonValue> map;

            skipWhitespace(sv, pos);
            if (pos < sv.size() && sv[pos] == '}')
            {
                ++pos;
                return map; // empty object
            }

            while (pos < sv.size())
            {
                skipWhitespace(sv, pos);
                auto key = parseString(sv, pos);
                if (!key)
                    return std::nullopt;

                skipWhitespace(sv, pos);
                if (pos >= sv.size() || sv[pos] != ':')
                    return std::nullopt;
                ++pos; // skip ':'

                auto val = parseValue(sv, pos);
                if (!val)
                    return std::nullopt;

                map[std::move(*key)] = std::move(*val);

                skipWhitespace(sv, pos);
                if (pos < sv.size() && sv[pos] == ',')
                {
                    ++pos;
                    continue;
                }
                if (pos < sv.size() && sv[pos] == '}')
                {
                    ++pos;
                    return map;
                }
                return std::nullopt; // unexpected token
            }
            return std::nullopt;
        }

        std::optional<JsonValue> parseValue(std::string_view sv, size_t &pos)
        {
            skipWhitespace(sv, pos);
            if (pos >= sv.size())
                return std::nullopt;

            char c = sv[pos];

            // String
            if (c == '"')
            {
                auto s = parseString(sv, pos);
                if (!s)
                    return std::nullopt;
                JsonValue v;
                v.type = JsonValue::Type::String;
                v.stringVal = std::move(*s);
                return v;
            }

            // Boolean true
            if (sv.substr(pos).substr(0, 4) == "true")
            {
                pos += 4;
                JsonValue v;
                v.type = JsonValue::Type::Boolean;
                v.boolVal = true;
                return v;
            }

            // Boolean false
            if (sv.substr(pos).substr(0, 5) == "false")
            {
                pos += 5;
                JsonValue v;
                v.type = JsonValue::Type::Boolean;
                v.boolVal = false;
                return v;
            }

            // Nested object
            if (c == '{')
            {
                auto obj = parseObject(sv, pos);
                if (!obj)
                    return std::nullopt;
                JsonValue v;
                v.type = JsonValue::Type::Object;
                v.objectVal = std::move(*obj);
                return v;
            }

            // Number (integer only — no floats in our schema)
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
            {
                size_t start = pos;
                if (c == '-')
                    ++pos;
                while (pos < sv.size() && std::isdigit(static_cast<unsigned char>(sv[pos])))
                    ++pos;
                std::string numStr(sv.substr(start, pos - start));
                JsonValue v;
                v.type = JsonValue::Type::Integer;
                char *end = nullptr;
                v.intVal = std::strtoll(numStr.c_str(), &end, 10);
                if (end != numStr.c_str() + numStr.size())
                    return std::nullopt;
                return v;
            }

            return std::nullopt; // unexpected character
        }

        // ── value extraction helpers ─────────────────────────────────────────────

        std::string getString(const std::unordered_map<std::string, JsonValue> &m,
                              const std::string &key, const std::string &fallback)
        {
            auto it = m.find(key);
            if (it != m.end() && it->second.type == JsonValue::Type::String)
                return it->second.stringVal;
            return fallback;
        }

        bool getBool(const std::unordered_map<std::string, JsonValue> &m,
                     const std::string &key, bool fallback)
        {
            auto it = m.find(key);
            if (it != m.end() && it->second.type == JsonValue::Type::Boolean)
                return it->second.boolVal;
            return fallback;
        }

        int64_t getInt(const std::unordered_map<std::string, JsonValue> &m,
                       const std::string &key, int64_t fallback)
        {
            auto it = m.find(key);
            if (it != m.end() && it->second.type == JsonValue::Type::Integer)
                return it->second.intVal;
            return fallback;
        }

        // ── JSON string escaping (for saveToFile) ───────────────────────────────

        std::string escapeJsonString(const std::string &s)
        {
            std::string out;
            out.reserve(s.size() + 4);
            for (char c : s)
            {
                switch (c)
                {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
                }
            }
            return out;
        }

    } // anonymous namespace

    // ─────────────────────────────── loadFromFile ─────────────────────────────────

    std::optional<LogConfig> LogConfig::loadFromFile(const std::filesystem::path &path)
    {
        if (!std::filesystem::exists(path))
        {
            std::cerr << "[Svitok] Config file not found: " << path << std::endl;
            return std::nullopt;
        }

        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[Svitok] Failed to open config file: " << path << std::endl;
            return std::nullopt;
        }

        // Read the entire file into a string.
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string contents = ss.str();

        // Parse the root JSON object.
        size_t pos = 0;
        auto root = parseObject(contents, pos);
        if (!root)
        {
            std::cerr << "[Svitok] Malformed JSON in config file: " << path << std::endl;
            return std::nullopt;
        }

        const auto &m = *root;
        LogConfig defaults = LogConfig::getDefault();
        LogConfig config;

        // level
        config.level = fromString(getString(m, "level",
                                            std::string(toString(defaults.level))));

        // output — validate against known values
        {
            std::string raw = getString(m, "output", defaults.output);
            if (raw == "console" || raw == "file" || raw == "both" || raw == "none")
            {
                config.output = raw;
            }
            else
            {
                std::cerr << "[Svitok] Unrecognised output mode \"" << raw
                          << "\", falling back to \"both\"" << std::endl;
                config.output = "both";
            }
        }

        // filepath
        config.filepath = getString(m, "filepath", defaults.filepath.string());

        // rotation (nested object)
        {
            auto it = m.find("rotation");
            if (it != m.end() && it->second.type == JsonValue::Type::Object)
            {
                const auto &rm = it->second.objectVal;
                config.rotation.enabled      = getBool(rm, "enabled",      defaults.rotation.enabled);
                config.rotation.maxSizeBytes  = static_cast<size_t>(
                    getInt(rm, "maxSizeBytes", static_cast<int64_t>(defaults.rotation.maxSizeBytes)));
                config.rotation.maxFiles      = static_cast<size_t>(
                    getInt(rm, "maxFiles", static_cast<int64_t>(defaults.rotation.maxFiles)));
                config.rotation.compressOld   = getBool(rm, "compressOld", defaults.rotation.compressOld);
            }
            // else: keep defaults
        }

        // format
        config.format = getString(m, "format", defaults.format);

        // useColors
        config.useColors = getBool(m, "useColors", defaults.useColors);

        // flushOnWrite
        config.flushOnWrite = getBool(m, "flushOnWrite", defaults.flushOnWrite);

        // component
        config.component = getString(m, "component", defaults.component);

        return config;
    }

    // ─────────────────────────────── saveToFile ──────────────────────────────────

    bool LogConfig::saveToFile(const std::filesystem::path &path) const
    {
        // Ensure parent directories exist.
        if (path.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec)
            {
                std::cerr << "[Svitok] Failed to create directories for: "
                          << path << " — " << ec.message() << std::endl;
                return false;
            }
        }

        std::ofstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[Svitok] Failed to open config file for writing: "
                       << path << std::endl;
            return false;
        }

        // Emit pretty-printed JSON that loadFromFile() can round-trip.
        file << "{\n";
        file << "    \"level\": \""        << toString(level)                        << "\",\n";
        file << "    \"output\": \""       << escapeJsonString(output)               << "\",\n";
        file << "    \"filepath\": \""     << escapeJsonString(filepath.string())    << "\",\n";
        file << "    \"rotation\": {\n";
        file << "        \"enabled\": "    << (rotation.enabled ? "true" : "false")  << ",\n";
        file << "        \"maxSizeBytes\": " << rotation.maxSizeBytes               << ",\n";
        file << "        \"maxFiles\": "   << rotation.maxFiles                     << ",\n";
        file << "        \"compressOld\": " << (rotation.compressOld ? "true" : "false") << "\n";
        file << "    },\n";
        file << "    \"format\": \""       << escapeJsonString(format)               << "\",\n";
        file << "    \"useColors\": "      << (useColors ? "true" : "false")        << ",\n";
        file << "    \"flushOnWrite\": "   << (flushOnWrite ? "true" : "false")     << ",\n";
        file << "    \"component\": \""    << escapeJsonString(component)            << "\"\n";
        file << "}\n";

        return file.good();
    }

} // namespace devana::logging
