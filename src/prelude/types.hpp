#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <any>
#include <memory>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <cmath>

struct Undefined
{
};
inline Undefined undefined;
struct Null
{
};
inline Null null;

// JSPP standard library
namespace jspp
{
    // Dynamic AnyValue
    using AnyValue = std::any;
    using NumberValue = std::variant<int, double>;

    // Temporal Dead Zone
    struct Uninitialized
    {
    };
    inline constexpr Uninitialized uninitialized;

    // Forward declarations
    struct JsObject;
    struct JsArray;
    struct JsString;
    struct JsFunction;
    struct JsNumber;
    struct JsBoolean;

    // Object and array prototypes
    struct DataDescriptor
    {
        AnyValue value = undefined;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;
    };
    struct AccessorDescriptor
    {
        std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>, Undefined> get = undefined; // getter
        std::variant<std::function<AnyValue(const std::vector<AnyValue> &)>, Undefined> set = undefined; // setter
        bool enumerable = false;
        bool configurable = true;
    };

    // Objects
    struct JsObject
    {
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> prototype;
    };

    // Arrays
    struct JsArray
    {
        std::vector<AnyValue> items;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> prototype;
    };

    // Strings
    struct JsString
    {
        std::string value;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> prototype;
    };

    // Functions
    struct JsFunction
    {
        std::function<AnyValue(const std::vector<AnyValue> &)> call;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> prototype;
    };

    // Numbers
    struct JsNumber
    {
        NumberValue value;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> prototype;
    };

    // Booleans
    struct JsBoolean
    {
        bool value;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> prototype;
    };

    // Operators
    inline bool is_truthy(const AnyValue &val);
    inline bool equals(const AnyValue &lhs, const AnyValue &rhs);
    inline bool strict_equals(const AnyValue &lhs, const AnyValue &rhs);
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs);

}
