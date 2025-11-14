//////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2025 Elecard.
// All rights are reserved.  Reproduction in whole or in part is prohibited
// without the written consent of the copyright owner.
//
// Elecard reserves the right to make changes without
// notice at any time. Elecard makes no warranty, expressed,
// implied or statutory, including but not limited to any implied
// warranty of merchantability of fitness for any particular purpose,
// or that the use will not infringe any third party patent, copyright
// or trademark.
//
// Elecard must not be liable for any loss or damage arising
// from its use.
//
//////////////////////////////////////////////////////////////////////////

// C++17, GStreamer 1.0+, header-only
// Поддержка: BOOL, INT32, ..., STRING, FRACTION, CAPS, ENUM, FLAGS, STRUCTURE
#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace FlexValue
{

// ============================================================
// Structured exceptions with path information
// ============================================================
class ValueException : public std::runtime_error
{
public:
    std::string path;
    std::string type_info;

    ValueException(
        const std::string& msg,
        const std::string& p = "",
        const std::string& ti = ""
    ) :
        std::runtime_error(format_message(msg, p, ti)),
        path(p),
        type_info(ti)
    {}

private:
    static std::string format_message(
        const std::string& msg,
        const std::string& path,
        const std::string& type_info
    )
    {
        std::string result = msg;
        if (!path.empty()) result += " at path: " + path;
        if (!type_info.empty()) result += " (type: " + type_info + ")";
        return result;
    }
};

class KeyNotFoundException : public ValueException
{
public:
    KeyNotFoundException(const std::string& key, const std::string& path = "") :
        ValueException("Key not found: " + key, path, "Map")
    {}
};

class TypeMismatchException : public ValueException
{
public:
    TypeMismatchException(
        const std::string& expected,
        const std::string& actual,
        const std::string& path = ""
    ) :
        ValueException(
            "Type mismatch: expected " + expected + ", got " + actual, path
        )
    {}
};

class NotContainerException : public ValueException
{
public:
    NotContainerException(
        const std::string& container_type, const std::string& path = ""
    ) :
        ValueException("Value is not a " + container_type, path)
    {}
};

class InvalidArgumentException : public ValueException
{
public:
    InvalidArgumentException(
        const std::string& msg, const std::string& path = ""
    ) :
        ValueException(msg, path)
    {}
};

// ============================================================
// Типы
// ============================================================
struct Value; // объявляем, но определение будет выше первого использования в
              // контейнерах

struct Fraction
{
    int32_t num = 0;
    int32_t den = 1;

    Fraction() = default;

    Fraction(int32_t n, int32_t d) :
        num(n),
        den(d)
    {
        if (d == 0) {
            throw InvalidArgumentException("Fraction denominator cannot be zero"
            );
        }
    }

    bool operator==(const Fraction& o) const noexcept
    {
        return num == o.num && den == o.den;
    }

    bool operator!=(const Fraction& o) const noexcept { return !(*this == o); }
};

struct EnumValue
{
    int32_t value = 0;
    std::string type_name;

    EnumValue() = default;

    EnumValue(int32_t v, std::string t = "") :
        value(v),
        type_name(std::move(t))
    {}

    bool operator==(const EnumValue& o) const noexcept
    {
        return value == o.value && type_name == o.type_name;
    }

    bool operator!=(const EnumValue& o) const noexcept { return !(*this == o); }
};

struct FlagsValue
{
    uint32_t value = 0;
    std::string type_name;

    FlagsValue() = default;

    FlagsValue(uint32_t v, std::string t = "") :
        value(v),
        type_name(std::move(t))
    {}

    bool operator==(const FlagsValue& o) const noexcept
    {
        return value == o.value && type_name == o.type_name;
    }

    bool operator!=(const FlagsValue& o) const noexcept
    {
        return !(*this == o);
    }
};

// Copy-on-write Box
template<typename T>
class CowBox
{
    std::shared_ptr<T> ptr_;

public:
    CowBox() :
        ptr_(std::make_shared<T>())
    {}

    explicit CowBox(T v) :
        ptr_(std::make_shared<T>(std::move(v)))
    {}

    CowBox(const CowBox& other) :
        ptr_(other.ptr_)
    {}

    CowBox(CowBox&&) noexcept = default;

    CowBox& operator=(const CowBox& other)
    {
        if (this != &other) ptr_ = other.ptr_;
        return *this;
    }

    CowBox& operator=(CowBox&&) noexcept = default;

    T& get()
    {
        if (ptr_.use_count() > 1) {
            ptr_ = std::make_shared<T>(*ptr_);
        }
        return *ptr_;
    }

    const T& get() const { return *ptr_; }

    T* operator->() { return &get(); }

    const T* operator->() const { return &get(); }

    friend bool operator==(const CowBox& a, const CowBox& b) noexcept
    {
        if (!a.ptr_ || !b.ptr_) return a.ptr_ == b.ptr_;
        return *a.ptr_ == *b.ptr_;
    }

    friend bool operator!=(const CowBox& a, const CowBox& b) noexcept
    {
        return !(a == b);
    }

    long ref_count() const { return ptr_.use_count(); }
};

// Вперёд объявляем структуры, которые зависят от Value, но определим их ПОСЛЕ
// Value, чтобы std::map<std::string, Value> видел полный тип.
struct Array;
struct Map;

// ============================================================
// Main Value type with COW
// ============================================================
struct Value
{
    std::variant<
        std::monostate,
        bool,
        int32_t,
        uint32_t,
        int64_t,
        uint64_t,
        float,
        double,
        std::string,
        Fraction,
        EnumValue,
        FlagsValue,
        CowBox<Array>,
        CowBox<Map>>
        data;

    mutable std::string last_access_path;

    Value() :
        data(std::monostate{})
    {}

    explicit Value(bool v) :
        data(v)
    {}

    explicit Value(int32_t v) :
        data(v)
    {}

    explicit Value(uint32_t v) :
        data(v)
    {}

    explicit Value(int64_t v) :
        data(v)
    {}

    explicit Value(uint64_t v) :
        data(v)
    {}

    explicit Value(float v) :
        data(v)
    {}

    explicit Value(double v) :
        data(v)
    {}

    explicit Value(std::string v) :
        data(std::move(v))
    {}

    explicit Value(const char* v) :
        data(std::string(v))
    {}

    explicit Value(Fraction v) :
        data(v)
    {}

    explicit Value(EnumValue v) :
        data(std::move(v))
    {}

    explicit Value(FlagsValue v) :
        data(std::move(v))
    {}

    explicit Value(CowBox<Array> v) :
        data(std::move(v))
    {}

    explicit Value(CowBox<Map> v) :
        data(std::move(v))
    {}

    template<class T>
    bool is() const
    {
        return std::holds_alternative<T>(data);
    }

    template<class T>
    const T& as(const std::string& path = "") const
    {
        if (!std::holds_alternative<T>(data)) {
            throw TypeMismatchException(typeid(T).name(), typeid_name(), path);
        }
        return std::get<T>(data);
    }

    template<class T>
    T& as(const std::string& path = "")
    {
        if (!std::holds_alternative<T>(data)) {
            throw TypeMismatchException(typeid(T).name(), typeid_name(), path);
        }
        return std::get<T>(data);
    }

    bool is_null() const
    {
        return std::holds_alternative<std::monostate>(data);
    }

    std::string typeid_name() const
    {
        return std::visit(
            [](auto const& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::monostate>) return "null";
                else if constexpr (std::is_same_v<T, bool>) return "bool";
                else if constexpr (std::is_same_v<T, int32_t>) return "int32";
                else if constexpr (std::is_same_v<T, uint32_t>) return "uint32";
                else if constexpr (std::is_same_v<T, int64_t>) return "int64";
                else if constexpr (std::is_same_v<T, uint64_t>) return "uint64";
                else if constexpr (std::is_same_v<T, float>) return "float";
                else if constexpr (std::is_same_v<T, double>) return "double";
                else if constexpr (std::is_same_v<T, std::string>)
                    return "string";
                else if constexpr (std::is_same_v<T, Fraction>)
                    return "fraction";
                else if constexpr (std::is_same_v<T, EnumValue>) return "enum";
                else if constexpr (std::is_same_v<T, FlagsValue>)
                    return "flags";
                else if constexpr (std::is_same_v<T, CowBox<Array>>)
                    return "array";
                else if constexpr (std::is_same_v<T, CowBox<Map>>) return "map";
                else return "unknown";
            },
            data
        );
    }

    bool operator==(const Value& other) const noexcept
    {
        return data == other.data;
    }

    bool operator!=(const Value& other) const noexcept
    {
        return !(*this == other);
    }
};

// Теперь, когда Value ОПРЕДЕЛЕН, можно определять контейнеры, которые его
// содержат.
struct Array
{
    std::vector<Value> items;

    bool operator==(const Array& o) const noexcept { return items == o.items; }

    bool operator!=(const Array& o) const noexcept { return !((*this) == o); }
};

struct Map
{
    std::map<std::string, Value> fields;

    bool operator==(const Map& o) const noexcept { return fields == o.fields; }

    bool operator!=(const Map& o) const noexcept { return !((*this) == o); }
};

// ============================================================
// Фабрики и утилиты
// ============================================================
inline Value make_null() { return Value{}; }

inline Value make_fraction(int32_t n, int32_t d)
{
    return Value{
        Fraction{n, d}
    };
}

inline Value make_enum(int32_t v, std::string t = "")
{
    return Value{
        EnumValue{v, std::move(t)}
    };
}

inline Value make_flags(uint32_t v, std::string t = "")
{
    return Value{
        FlagsValue{v, std::move(t)}
    };
}

inline Value make_array(std::vector<Value> items = {})
{
    Array a;
    a.items = std::move(items);
    return Value{CowBox<Array>{std::move(a)}};
}

inline Value make_map(std::map<std::string, Value> f = {})
{
    Map m;
    m.fields = std::move(f);
    return Value{CowBox<Map>{std::move(m)}};
}

template<class U>
inline Value wrap(U&& u)
{
    using D = std::decay_t<U>;
    if constexpr (std::is_same_v<D, const char*>) {
        return Value{std::string(u)};
    } else if constexpr (std::is_same_v<D, std::string>) {
        return Value{std::forward<U>(u)};
    } else if constexpr (std::is_same_v<D, bool>) {
        return Value{u};
    } else if constexpr (std::is_arithmetic_v<D>) {
        return Value{std::forward<U>(u)};
    } else if constexpr (std::is_same_v<D, Fraction>) {
        return Value{std::forward<U>(u)};
    } else if constexpr (std::is_same_v<D, EnumValue>) {
        return Value{std::forward<U>(u)};
    } else if constexpr (std::is_same_v<D, FlagsValue>) {
        return Value{std::forward<U>(u)};
    } else {
        return Value{std::forward<U>(u)};
    }
}

// ============================================================
// Map access with path tracking
// ============================================================
inline Value& map_get(
    Value& val, const std::string& key, const std::string& parent_path = ""
)
{
    const std::string path
        = (parent_path.empty() ? "/" : parent_path) + "/" + key;
    if (!val.is<CowBox<Map>>()) {
        throw NotContainerException("Map", path);
    }
    auto& m = val.as<CowBox<Map>>(path).get();
    auto it = m.fields.find(key);
    if (it == m.fields.end()) {
        throw KeyNotFoundException(key, path);
    }
    val.last_access_path = path;
    return it->second;
}

inline const Value& map_get(
    const Value& val,
    const std::string& key,
    const std::string& parent_path = ""
)
{
    const std::string path
        = (parent_path.empty() ? "/" : parent_path) + "/" + key;
    if (!val.is<CowBox<Map>>()) {
        throw NotContainerException("Map", path);
    }
    const auto& fields = val.as<CowBox<Map>>(path).get().fields;
    auto it = fields.find(key);
    if (it == fields.end()) {
        throw KeyNotFoundException(key, path);
    }
    val.last_access_path = path;
    return it->second;
}

inline bool map_has_key(const Value& val, const std::string& key)
{
    if (!val.is<CowBox<Map>>()) return false;
    return val.as<CowBox<Map>>().get().fields.count(key) > 0;
}

inline void map_set(
    Value& val,
    const std::string& key,
    Value v,
    const std::string& parent_path = ""
)
{
    const std::string path
        = (parent_path.empty() ? "/" : parent_path) + "/" + key;
    if (!val.is<CowBox<Map>>()) {
        throw NotContainerException("Map", path);
    }
    auto& m = val.as<CowBox<Map>>(path).get();
    m.fields[key] = std::move(v);
}

template<class U>
inline void map_set(
    Value& val,
    const std::string& key,
    U&& u,
    const std::string& parent_path = ""
)
{
    map_set(val, key, wrap(std::forward<U>(u)), parent_path);
}

inline bool map_set_strict(
    Value& val,
    const std::string& key,
    Value v,
    const std::string& parent_path = ""
)
{
    const std::string path
        = (parent_path.empty() ? "/" : parent_path) + "/" + key;
    if (!val.is<CowBox<Map>>()) return false;
    auto& m = val.as<CowBox<Map>>(path).get();
    auto it = m.fields.find(key);
    if (it == m.fields.end()) return false;
    it->second = std::move(v);
    return true;
}

template<class U>
inline bool map_set_strict(
    Value& val,
    const std::string& key,
    U&& u,
    const std::string& parent_path = ""
)
{
    return map_set_strict(val, key, wrap(std::forward<U>(u)), parent_path);
}

// ============================================================
// Array access with path tracking
// ============================================================
inline void
    array_push(Value& val, Value item, const std::string& parent_path = "")
{
    const std::string path = (parent_path.empty() ? "/" : parent_path) + "[]";
    if (!val.is<CowBox<Array>>()) {
        throw NotContainerException("Array", path);
    }
    val.as<CowBox<Array>>(path).get().items.push_back(std::move(item));
}

template<class U>
inline void array_push(Value& val, U&& u, const std::string& parent_path = "")
{
    array_push(val, wrap(std::forward<U>(u)), parent_path);
}

inline size_t array_size(const Value& val, const std::string& parent_path = "")
{
    if (!val.is<CowBox<Array>>()) {
        throw NotContainerException("Array", parent_path);
    }
    return val.as<CowBox<Array>>(parent_path).get().items.size();
}

inline Value&
    array_at(Value& val, size_t idx, const std::string& parent_path = "")
{
    const std::string path = (parent_path.empty() ? "/" : parent_path) + "["
                           + std::to_string(idx) + "]";
    if (!val.is<CowBox<Array>>()) {
        throw NotContainerException("Array", path);
    }
    auto& arr = val.as<CowBox<Array>>(path).get();
    if (idx >= arr.items.size()) {
        throw std::out_of_range(
            "Array index out of bounds: " + std::to_string(idx)
            + " >= " + std::to_string(arr.items.size()) + " at " + path
        );
    }
    return arr.items[idx];
}

inline const Value&
    array_at(const Value& val, size_t idx, const std::string& parent_path = "")
{
    const std::string path = (parent_path.empty() ? "/" : parent_path) + "["
                           + std::to_string(idx) + "]";
    if (!val.is<CowBox<Array>>()) {
        throw NotContainerException("Array", path);
    }
    const auto& arr = val.as<CowBox<Array>>(path).get();
    if (idx >= arr.items.size()) {
        throw std::out_of_range("Array index out of bounds");
    }
    return arr.items[idx];
}

// ============================================================
// Builder pattern
// ============================================================
class ValueBuilder
{
    Value root_;
    std::vector<std::string> path_;

    // Вспомогательный метод для получения текущего контекста (куда писать)
    Value& current()
    {
        Value* current = &root_;
        for (const auto& key: path_) {
            if (!map_has_key(*current, key)) {
                map_set(*current, key, make_map());
            }
            current = &map_get(*current, key);
        }
        return *current;
    }

public:
    ValueBuilder() :
        root_(make_map())
    {}

    explicit ValueBuilder(Value v) :
        root_(std::move(v))
    {}

    ValueBuilder& set(const std::string& key, Value v)
    {
        map_set(current(), key, std::move(v));
        return *this;
    }

    template<class U>
    ValueBuilder& set(const std::string& key, U&& u)
    {
        map_set(current(), key, wrap(std::forward<U>(u)));
        return *this;
    }

    ValueBuilder& push(Value v)
    {
        array_push(current(), std::move(v));
        return *this;
    }

    template<class U>
    ValueBuilder& push(U&& u)
    {
        array_push(current(), wrap(std::forward<U>(u)));
        return *this;
    }

    ValueBuilder& enter(const std::string& key)
    {
        path_.push_back(key);
        return *this;
    }

    ValueBuilder& exit()
    {
        if (!path_.empty()) path_.pop_back();
        return *this;
    }

    Value build() { return root_; }

    // Для отладки
    const Value& get_root() const { return root_; }
};

// ============================================================
// Traversal и поиск
// ============================================================
using Visitor = std::function<void(const Value&, const std::string& path)>;
using Predicate = std::function<bool(const Value&)>;

inline void
    traverse(const Value& val, Visitor visitor, const std::string& path = "/")
{
    visitor(val, path);
    if (val.is<CowBox<Array>>()) {
        const auto& arr = val.as<CowBox<Array>>().get().items;
        for (size_t i = 0; i < arr.size(); ++i) {
            traverse(arr[i], visitor, path + "[" + std::to_string(i) + "]");
        }
    } else if (val.is<CowBox<Map>>()) {
        const auto& m = val.as<CowBox<Map>>().get().fields;
        for (const auto& [k, v]: m) {
            traverse(v, visitor, path + "/" + k);
        }
    }
}

inline std::vector<std::pair<std::string, Value>>
    find_all(const Value& val, Predicate pred, const std::string& path = "/")
{
    std::vector<std::pair<std::string, Value>> results;
    traverse(
        val,
        [&](const Value& v, const std::string& p) {
            if (pred(v)) results.push_back({p, v});
        },
        path
    );
    return results;
}

// ============================================================
// Сериализация в строку (для отладки)
// ============================================================
inline std::string to_string(const Value& val, size_t indent = 0)
{
    std::string ind(indent, ' ');
    std::string next_ind(indent + 2, ' ');

    return std::visit(
        [&](auto const& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "null";
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(arg);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + arg + "\"";
            } else if constexpr (std::is_same_v<T, Fraction>) {
                return std::to_string(arg.num) + "/" + std::to_string(arg.den);
            } else if constexpr (std::is_same_v<T, EnumValue>) {
                return "enum(" + std::to_string(arg.value)
                     + (arg.type_name.empty() ? "" : ("," + arg.type_name))
                     + ")";
            } else if constexpr (std::is_same_v<T, FlagsValue>) {
                return "flags(" + std::to_string(arg.value)
                     + (arg.type_name.empty() ? "" : ("," + arg.type_name))
                     + ")";
            } else if constexpr (std::is_same_v<T, CowBox<Array>>) {
                std::string res = "[\n";
                const auto& items = arg.get().items;
                for (size_t i = 0; i < items.size(); ++i) {
                    res += next_ind + to_string(items[i], indent + 2);
                    if (i < items.size() - 1) res += ",";
                    res += "\n";
                }
                res += ind + "]";
                return res;
            } else if constexpr (std::is_same_v<T, CowBox<Map>>) {
                std::string res = "{\n";
                const auto& fields = arg.get().fields;
                size_t i = 0;
                for (const auto& [k, v]: fields) {
                    res += next_ind + "\"" + k
                         + "\": " + to_string(v, indent + 2);
                    if (++i < fields.size()) res += ",";
                    res += "\n";
                }
                res += ind + "}";
                return res;
            } else {
                return "<unknown>";
            }
        },
        val.data
    );
}

// ============================================================
// Schema validation (простой вариант)
// ============================================================
struct Schema
{
    std::string type; // "map", "array", "int32", "string", etc.
    std::map<std::string, std::unique_ptr<Schema>> properties; // для map
    std::unique_ptr<Schema> items;                             // для array
    bool required = false;

    bool validate(
        const Value& val, std::string& error, const std::string& path = "/"
    ) const
    {
        if (val.is_null() && !required) return true;
        if (val.is_null() && required) {
            error = "Required field is null at " + path;
            return false;
        }

        if (type == "map") {
            if (!val.is<CowBox<Map>>()) {
                error
                    = "Expected map, got " + val.typeid_name() + " at " + path;
                return false;
            }
            const auto& m = val.as<CowBox<Map>>().get().fields;
            for (const auto& [k, v]: m) {
                if (properties.count(k)) {
                    if (!properties.at(k)->validate(v, error, path + "/" + k))
                        return false;
                }
            }
            return true;
        } else if (type == "array") {
            if (!val.is<CowBox<Array>>()) {
                error = "Expected array, got " + val.typeid_name() + " at "
                      + path;
                return false;
            }
            if (items) {
                const auto& arr = val.as<CowBox<Array>>().get().items;
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (!items->validate(
                            arr[i], error, path + "[" + std::to_string(i) + "]"
                        ))
                        return false;
                }
            }
            return true;
        } else if (type == "int32") {
            if (!val.is<int32_t>()) {
                error = "Expected int32, got " + val.typeid_name() + " at "
                      + path;
                return false;
            }
        } else if (type == "string") {
            if (!val.is<std::string>()) {
                error = "Expected string, got " + val.typeid_name() + " at "
                      + path;
                return false;
            }
        }
        return true;
    }
};

// ============================================================
// Интроспекция
// ============================================================
class ValueIntrospector
{
public:
    static std::vector<std::string> map_keys(const Value& val)
    {
        if (!val.is<CowBox<Map>>()) return {};
        std::vector<std::string> keys;
        for (const auto& [k, _]: val.as<CowBox<Map>>().get().fields) {
            keys.push_back(k);
        }
        return keys;
    }

    static size_t array_length(const Value& val)
    {
        if (!val.is<CowBox<Array>>()) return 0;
        return val.as<CowBox<Array>>().get().items.size();
    }

    static std::string type_name(const Value& val) { return val.typeid_name(); }
};

} // namespace FlexValue
