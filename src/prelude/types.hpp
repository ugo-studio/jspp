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
    // Dynamic JsValue
    using JsValue = std::any;

    inline JsValue pow(const JsValue &lhs, const JsValue &rhs);

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

    // Object and array prototypes
    struct DataDescriptor
    {
        JsValue value = undefined;
        bool writable = true;
        bool enumerable = false;
        bool configurable = true;
    };
    struct AccessorDescriptor
    {
        std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> get = undefined; // getter
        std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> set = undefined; // setter
        bool enumerable = false;
        bool configurable = true;
    };

    // Objects
    struct JsObject
    {
        std::map<std::string, JsValue> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    // Arrays
    struct JsArray
    {
        std::vector<JsValue> properties;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    // Strings
    struct JsString
    {
        std::string value;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };

    // Functions
    struct JsFunction
    {
        std::function<JsValue(const std::vector<JsValue> &)> call;
        std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>> prototype;
    };
}
