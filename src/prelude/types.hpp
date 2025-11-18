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
    struct JsObject;        // can set property
    struct JsArray;         // can set property
    struct JsFunction;      // can set property

    template <typename T>
    class JsGenerator; // can set property

    // Object property configuration forward declarations
    struct DataDescriptor;
    struct AccessorDescriptor;

    // Custom runtime exception
    struct RuntimeError;

    // Dynamic AnyValue
    class AnyValue;

    // Arithemetic operators
    inline AnyValue pow(const AnyValue &lhs, const AnyValue &rhs);

    // AnyValue prototypes
    namespace StringPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, const std::unique_ptr<std::string> &self);
    }
    namespace ArrayPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsArray *self);
    }
    namespace GeneratorPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsArray *self);
    }
}
