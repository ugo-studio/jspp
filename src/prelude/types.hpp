#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <cmath>

// JSPP standard library
namespace jspp
{
    // Forward declarations
    struct JsUndefined;     // cannot
    struct JsNull;          // cannot
    struct JsUninitialized; // cannot
    // struct JsBoolean;       // cannot
    // struct JsNumber;        // cannot
    // struct JsString;   // cannot
    struct JsObject;   // can set property
    struct JsArray;    // can set property
    struct JsFunction; // can set property

    // Object property configuration forward declarations
    struct DataDescriptor;
    struct AccessorDescriptor;

    // // Dynamic AnyValue
    class AnyValue;
    // // Use std::shared_ptr for types that can contain other AnyValues.
    // using AnyValue = std::variant<
    //     JsUndefined,
    //     JsNull,
    //     JsUninitialized,
    //     // JsBoolean,
    //     // JsNumber,
    //     JsString,
    //     std::shared_ptr<JsObject>,
    //     std::shared_ptr<JsArray>,
    //     std::shared_ptr<JsFunction>>;

    // Operators
    inline bool is_truthy(const AnyValue &val);
    inline bool equals(const AnyValue &lhs, const AnyValue &rhs);
    inline bool strict_equals(const AnyValue &lhs, const AnyValue &rhs);
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs);

}
