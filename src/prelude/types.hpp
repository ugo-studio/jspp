#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <memory>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <cmath>
#include <optional>

// JSPP standard library
namespace jspp
{
    // Js value forward declarations
    struct JsUndefined;     // cannot set property
    struct JsNull;          // cannot set property
    struct JsUninitialized; // cannot set property
    struct JsString;        // can set property
    struct JsObject;        // can set property
    struct JsArray;         // can set property
    struct JsFunction;      // can set property
    struct JsPromise;       // can set property
    struct JsSymbol;        // can set property (but usually doesn't have own props)

    template <typename T>
    class JsIterator; // can set property

    // Object property configuration forward declarations
    struct DataDescriptor;
    struct AccessorDescriptor;

    // Custom runtime exception
    struct Exception;

    // Dynamic AnyValue
    class AnyValue;

    // Truthiness checker
    const bool is_truthy(const double &val) noexcept;
    const bool is_truthy(const std::string &val) noexcept;
    const bool is_truthy(const AnyValue &val) noexcept;

    // Basic equality operators
    inline const bool is_strictly_equal_to_primitive(const AnyValue &lhs, const double &rhs) noexcept;
    inline const bool is_strictly_equal_to_primitive(const double &lhs, const AnyValue &rhs) noexcept;
    inline const bool is_strictly_equal_to_primitive(const double &lhs, const double &rhs) noexcept;
    inline const bool is_strictly_equal_to_primitive(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    inline const bool is_equal_to_primitive(const AnyValue &lhs, const double &rhs) noexcept;
    inline const bool is_equal_to_primitive(const double &lhs, const AnyValue &rhs) noexcept;
    inline const bool is_equal_to_primitive(const double &lhs, const double &rhs) noexcept;
    inline const bool is_equal_to_primitive(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    inline const AnyValue is_strictly_equal_to(const AnyValue &lhs, const double &rhs) noexcept;
    inline const AnyValue is_strictly_equal_to(const double &lhs, const AnyValue &rhs) noexcept;
    inline const AnyValue is_strictly_equal_to(const double &lhs, const double &rhs) noexcept;
    inline const AnyValue is_strictly_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    inline const AnyValue is_equal_to(const AnyValue &lhs, const double &rhs) noexcept;
    inline const AnyValue is_equal_to(const double &lhs, const AnyValue &rhs) noexcept;
    inline const AnyValue is_equal_to(const double &lhs, const double &rhs) noexcept;
    inline const AnyValue is_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    inline const AnyValue not_strictly_equal_to(const AnyValue &lhs, const double &rhs) noexcept;
    inline const AnyValue not_strictly_equal_to(const double &lhs, const AnyValue &rhs) noexcept;
    inline const AnyValue not_strictly_equal_to(const double &lhs, const double &rhs) noexcept;
    inline const AnyValue not_strictly_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    inline const AnyValue not_equal_to(const AnyValue &lhs, const double &rhs) noexcept;
    inline const AnyValue not_equal_to(const double &lhs, const AnyValue &rhs) noexcept;
    inline const AnyValue not_equal_to(const double &lhs, const double &rhs) noexcept;
    inline const AnyValue not_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    // Arithemetic operators
    inline AnyValue pow(const double &lhs, const double &rhs);
    inline AnyValue pow(const AnyValue &lhs, const double &rhs);
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs);

    // AnyValue prototypes
    namespace ObjectPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsObject *self);
    }
    namespace StringPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsString *self);
    }
    namespace NumberPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, double self);
    }
    namespace ArrayPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsArray *self);
    }
    namespace FunctionPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsFunction *self);
    }
    namespace PromisePrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsPromise *self);
    }
    namespace IteratorPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsIterator<AnyValue> *self);
    }
    namespace SymbolPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsSymbol *self);
    }
}