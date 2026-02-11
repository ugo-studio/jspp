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
#include <span>

// JSPP standard library
namespace jspp
{
    enum class JsType : uint8_t
    {
        Undefined = 0,
        Null = 1,
        Uninitialized = 2,
        Boolean = 3,
        Number = 4,
        String = 5,
        Object = 6,
        Array = 7,
        Function = 8,
        Iterator = 9,
        Symbol = 10,
        Promise = 11,
        DataDescriptor = 12,
        AccessorDescriptor = 13,
        AsyncIterator = 14,
    };

    struct HeapObject {
        mutable uint32_t ref_count = 0;
        
        HeapObject() noexcept : ref_count(0) {}
        
        // Disable copying/assignment of ref_count
        HeapObject(const HeapObject&) noexcept : ref_count(0) {}
        HeapObject& operator=(const HeapObject&) noexcept { return *this; }
        
        virtual ~HeapObject() = default;
        virtual JsType get_heap_type() const = 0;
        
        void ref() const {
            ++ref_count;
        }
        
        void deref() const {
            if (--ref_count == 0) {
                delete this;
            }
        }
    };

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

    template <typename T>
    class JsAsyncIterator; // can set property

    // Object property configuration forward declarations
    struct DataDescriptor;
    struct AccessorDescriptor;

    // Custom runtime exception
    struct Exception;

    // Dynamic AnyValue
    class AnyValue;

    using JsFunctionCallable = std::variant<
        std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>,
        std::function<JsIterator<AnyValue>(const AnyValue &, std::span<const AnyValue>)>,
        std::function<JsPromise(const AnyValue &, std::span<const AnyValue>)>,
        std::function<JsAsyncIterator<AnyValue>(const AnyValue &, std::span<const AnyValue>)>>;

    // Awaiter for AnyValue
    struct AnyValueAwaiter;

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
    inline const AnyValue not_equal_to(const AnyValue &lhs, const AnyValue &rhs) noexcept;

    // Bitwise operators
    inline AnyValue unsigned_right_shift(const AnyValue &lhs, const AnyValue &rhs);
    inline AnyValue unsigned_right_shift(const AnyValue &lhs, const double &rhs);
    inline AnyValue unsigned_right_shift(const double &lhs, const AnyValue &rhs);

    // Arithemetic operators

    inline AnyValue pow(const double &lhs, const double &rhs);
    inline AnyValue pow(const AnyValue &lhs, const double &rhs);
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs);

    // AnyValue prototypes
    namespace ObjectPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace StringPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace NumberPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace ArrayPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace FunctionPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace PromisePrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace IteratorPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
    namespace SymbolPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key);
    }
}