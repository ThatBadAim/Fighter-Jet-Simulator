#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/// @file
/// @brief Minimal, dependency-free JSON document model used for user settings.
///
/// Supports the full JSON grammar (RFC 8259) including string escapes and
/// \\u surrogate pairs. Numbers are parsed with std::from_chars, so the result
/// does not depend on the process locale. Object member order is preserved so
/// that saved files stay diff-friendly.

namespace fastjet::ui::json {

class Value;
using Array = std::vector<Value>;
using Member = std::pair<std::string, Value>;
using Object = std::vector<Member>;

class Value {
public:
    enum class Type : uint8_t { NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT };

    Value() = default;
    Value(std::nullptr_t) {}
    Value(bool b) : data_(b) {}
    Value(double d) : data_(d) {}
    Value(int i) : data_(static_cast<double>(i)) {}
    Value(const char* s) : data_(std::string(s)) {}
    Value(std::string s) : data_(std::move(s)) {}
    Value(Array a) : data_(std::move(a)) {}
    Value(Object o) : data_(std::move(o)) {}

    [[nodiscard]] Type type() const noexcept { return static_cast<Type>(data_.index()); }
    [[nodiscard]] bool is_null() const noexcept { return type() == Type::NUL; }
    [[nodiscard]] bool is_bool() const noexcept { return type() == Type::BOOL; }
    [[nodiscard]] bool is_number() const noexcept { return type() == Type::NUMBER; }
    [[nodiscard]] bool is_string() const noexcept { return type() == Type::STRING; }
    [[nodiscard]] bool is_array() const noexcept { return type() == Type::ARRAY; }
    [[nodiscard]] bool is_object() const noexcept { return type() == Type::OBJECT; }

    [[nodiscard]] bool as_bool(bool fallback) const noexcept {
        const bool* b = std::get_if<bool>(&data_);
        return b ? *b : fallback;
    }
    [[nodiscard]] double as_number(double fallback) const noexcept {
        const double* d = std::get_if<double>(&data_);
        return (d && std::isfinite(*d)) ? *d : fallback;
    }
    [[nodiscard]] int as_int(int fallback) const noexcept {
        const double d = as_number(static_cast<double>(fallback));
        if (d < -2147483648.0 || d > 2147483647.0) return fallback;
        return static_cast<int>(std::lround(d));
    }
    [[nodiscard]] std::string as_string(std::string fallback) const {
        const std::string* s = std::get_if<std::string>(&data_);
        return s ? *s : std::move(fallback);
    }

    [[nodiscard]] const Array* array() const noexcept { return std::get_if<Array>(&data_); }
    [[nodiscard]] const Object* object() const noexcept { return std::get_if<Object>(&data_); }
    [[nodiscard]] Object* object() noexcept { return std::get_if<Object>(&data_); }

    /// @brief Member lookup; nullptr if this is not an object or the key is absent.
    [[nodiscard]] const Value* find(std::string_view key) const noexcept {
        const Object* o = object();
        if (!o) return nullptr;
        for (const auto& [k, v] : *o) {
            if (k == key) return &v;
        }
        return nullptr;
    }

    /// @brief Inserts or replaces a member. Converts a null value to an object first.
    Value& set(std::string key, Value v) {
        if (is_null()) data_ = Object{};
        Object* o = object();
        if (!o) return *this;
        for (auto& [k, existing] : *o) {
            if (k == key) {
                existing = std::move(v);
                return *this;
            }
        }
        o->emplace_back(std::move(key), std::move(v));
        return *this;
    }

private:
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> data_{nullptr};

    friend class Writer;
};

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

class Parser {
public:
    explicit Parser(std::string_view text) noexcept : s_(text) {}

    std::optional<Value> parse(std::string* error) {
        skip_ws();
        std::optional<Value> v = parse_value(0);
        if (v) {
            skip_ws();
            if (pos_ != s_.size()) {
                v.reset();
                fail("unexpected trailing characters");
            }
        }
        if (!v && error) *error = error_ + " at offset " + std::to_string(pos_);
        return v;
    }

private:
    static constexpr int kMaxDepth = 64; ///< Guards against stack exhaustion on hostile input

    std::string_view s_;
    size_t pos_ = 0;
    std::string error_ = "invalid JSON";

    void fail(const char* msg) { error_ = msg; }

    [[nodiscard]] bool at_end() const noexcept { return pos_ >= s_.size(); }
    [[nodiscard]] char peek() const noexcept { return at_end() ? '\0' : s_[pos_]; }

    void skip_ws() noexcept {
        while (!at_end()) {
            const char c = s_[pos_];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
            ++pos_;
        }
    }

    bool consume_literal(std::string_view lit) noexcept {
        if (s_.substr(pos_, lit.size()) != lit) return false;
        pos_ += lit.size();
        return true;
    }

    std::optional<Value> parse_value(int depth) {
        if (depth > kMaxDepth) { fail("nesting too deep"); return std::nullopt; }
        switch (peek()) {
            case '{': return parse_object(depth);
            case '[': return parse_array(depth);
            case '"': {
                auto s = parse_string();
                if (!s) return std::nullopt;
                return Value(std::move(*s));
            }
            case 't': if (consume_literal("true")) return Value(true); break;
            case 'f': if (consume_literal("false")) return Value(false); break;
            case 'n': if (consume_literal("null")) return Value(nullptr); break;
            default:
                if (peek() == '-' || (peek() >= '0' && peek() <= '9')) return parse_number();
                break;
        }
        fail("unexpected token");
        return std::nullopt;
    }

    std::optional<Value> parse_object(int depth) {
        ++pos_; // '{'
        Object obj;
        skip_ws();
        if (peek() == '}') { ++pos_; return Value(std::move(obj)); }
        while (true) {
            skip_ws();
            if (peek() != '"') { fail("expected object key"); return std::nullopt; }
            auto key = parse_string();
            if (!key) return std::nullopt;
            skip_ws();
            if (peek() != ':') { fail("expected ':'"); return std::nullopt; }
            ++pos_;
            skip_ws();
            auto v = parse_value(depth + 1);
            if (!v) return std::nullopt;
            obj.emplace_back(std::move(*key), std::move(*v));
            skip_ws();
            if (peek() == ',') { ++pos_; continue; }
            if (peek() == '}') { ++pos_; return Value(std::move(obj)); }
            fail("expected ',' or '}'");
            return std::nullopt;
        }
    }

    std::optional<Value> parse_array(int depth) {
        ++pos_; // '['
        Array arr;
        skip_ws();
        if (peek() == ']') { ++pos_; return Value(std::move(arr)); }
        while (true) {
            skip_ws();
            auto v = parse_value(depth + 1);
            if (!v) return std::nullopt;
            arr.push_back(std::move(*v));
            skip_ws();
            if (peek() == ',') { ++pos_; continue; }
            if (peek() == ']') { ++pos_; return Value(std::move(arr)); }
            fail("expected ',' or ']'");
            return std::nullopt;
        }
    }

    std::optional<Value> parse_number() {
        const size_t start = pos_;
        if (peek() == '-') ++pos_;
        if (peek() == '0') {
            ++pos_;
        } else if (peek() >= '1' && peek() <= '9') {
            while (peek() >= '0' && peek() <= '9') ++pos_;
        } else {
            fail("invalid number");
            return std::nullopt;
        }
        if (peek() == '.') {
            ++pos_;
            if (!(peek() >= '0' && peek() <= '9')) { fail("invalid fraction"); return std::nullopt; }
            while (peek() >= '0' && peek() <= '9') ++pos_;
        }
        if (peek() == 'e' || peek() == 'E') {
            ++pos_;
            if (peek() == '+' || peek() == '-') ++pos_;
            if (!(peek() >= '0' && peek() <= '9')) { fail("invalid exponent"); return std::nullopt; }
            while (peek() >= '0' && peek() <= '9') ++pos_;
        }
        double d = 0.0;
        const char* first = s_.data() + start;
        const char* last = s_.data() + pos_;
        const auto res = std::from_chars(first, last, d);
        if (res.ec != std::errc{} || res.ptr != last) { fail("number out of range"); return std::nullopt; }
        return Value(d);
    }

    static void append_utf8(std::string& out, uint32_t cp) {
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    std::optional<uint32_t> parse_hex4() {
        if (pos_ + 4 > s_.size()) { fail("truncated \\u escape"); return std::nullopt; }
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = s_[pos_++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<uint32_t>(c - 'A' + 10);
            else { fail("invalid \\u escape"); return std::nullopt; }
        }
        return v;
    }

    std::optional<std::string> parse_string() {
        ++pos_; // opening quote
        std::string out;
        while (true) {
            if (at_end()) { fail("unterminated string"); return std::nullopt; }
            const char c = s_[pos_++];
            if (c == '"') return out;
            if (static_cast<unsigned char>(c) < 0x20) { fail("control character in string"); return std::nullopt; }
            if (c != '\\') { out.push_back(c); continue; }
            if (at_end()) { fail("unterminated escape"); return std::nullopt; }
            const char e = s_[pos_++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    auto hi = parse_hex4();
                    if (!hi) return std::nullopt;
                    uint32_t cp = *hi;
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (!consume_literal("\\u")) { fail("unpaired surrogate"); return std::nullopt; }
                        auto lo = parse_hex4();
                        if (!lo || *lo < 0xDC00 || *lo > 0xDFFF) { fail("invalid surrogate pair"); return std::nullopt; }
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (*lo - 0xDC00);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        fail("unpaired surrogate");
                        return std::nullopt;
                    }
                    append_utf8(out, cp);
                    break;
                }
                default: fail("invalid escape"); return std::nullopt;
            }
        }
    }
};

/// @brief Parses a complete JSON document. On failure returns nullopt and,
/// if `error` is non-null, a human-readable reason with the byte offset.
[[nodiscard]] inline std::optional<Value> parse(std::string_view text, std::string* error = nullptr) {
    return Parser(text).parse(error);
}

// ---------------------------------------------------------------------------
// Writer
// ---------------------------------------------------------------------------

class Writer {
public:
    explicit Writer(int indent) noexcept : indent_(indent) {}

    std::string write(const Value& v) {
        out_.clear();
        emit(v, 0);
        out_.push_back('\n');
        return std::move(out_);
    }

private:
    int indent_;
    std::string out_;

    void newline(int depth) {
        if (indent_ <= 0) return;
        out_.push_back('\n');
        out_.append(static_cast<size_t>(depth * indent_), ' ');
    }

    void emit_string(const std::string& s) {
        static constexpr char kHex[] = "0123456789abcdef";
        out_.push_back('"');
        for (const char c : s) {
            switch (c) {
                case '"': out_ += "\\\""; break;
                case '\\': out_ += "\\\\"; break;
                case '\n': out_ += "\\n"; break;
                case '\r': out_ += "\\r"; break;
                case '\t': out_ += "\\t"; break;
                case '\b': out_ += "\\b"; break;
                case '\f': out_ += "\\f"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        out_ += "\\u00";
                        out_.push_back(kHex[(c >> 4) & 0xF]);
                        out_.push_back(kHex[c & 0xF]);
                    } else {
                        out_.push_back(c);
                    }
            }
        }
        out_.push_back('"');
    }

    void emit_number(double d) {
        if (!std::isfinite(d)) { out_ += "null"; return; } // JSON has no NaN/Inf
        char buf[32];
        // Integral values print without a fraction so hand-edited files stay tidy.
        constexpr double kMaxExactInteger = 9007199254740992.0; // 2^53
        const auto res = (std::trunc(d) == d && std::fabs(d) < kMaxExactInteger)
            ? std::to_chars(buf, buf + sizeof(buf), static_cast<long long>(d))
            : std::to_chars(buf, buf + sizeof(buf), d);
        out_.append(buf, res.ptr);
    }

    void emit(const Value& v, int depth) {
        switch (v.type()) {
            case Value::Type::NUL: out_ += "null"; break;
            case Value::Type::BOOL: out_ += std::get<bool>(v.data_) ? "true" : "false"; break;
            case Value::Type::NUMBER: emit_number(std::get<double>(v.data_)); break;
            case Value::Type::STRING: emit_string(std::get<std::string>(v.data_)); break;
            case Value::Type::ARRAY: {
                const Array& a = std::get<Array>(v.data_);
                // Short scalar arrays (e.g. key binding pairs) stay on one line.
                bool inline_array = a.size() <= 4;
                for (const Value& e : a) inline_array = inline_array && !e.is_array() && !e.is_object();
                out_.push_back('[');
                for (size_t i = 0; i < a.size(); ++i) {
                    if (i) out_.push_back(',');
                    if (inline_array) { if (i) out_.push_back(' '); }
                    else newline(depth + 1);
                    emit(a[i], depth + 1);
                }
                if (!inline_array && !a.empty()) newline(depth);
                out_.push_back(']');
                break;
            }
            case Value::Type::OBJECT: {
                const Object& o = std::get<Object>(v.data_);
                out_.push_back('{');
                for (size_t i = 0; i < o.size(); ++i) {
                    if (i) out_.push_back(',');
                    newline(depth + 1);
                    emit_string(o[i].first);
                    out_ += indent_ > 0 ? ": " : ":";
                    emit(o[i].second, depth + 1);
                }
                if (!o.empty()) newline(depth);
                out_.push_back('}');
                break;
            }
        }
    }
};

/// @brief Serialises a document; `indent` spaces per level (0 = compact).
[[nodiscard]] inline std::string serialize(const Value& v, int indent = 2) {
    return Writer(indent).write(v);
}

} // namespace fastjet::ui::json
