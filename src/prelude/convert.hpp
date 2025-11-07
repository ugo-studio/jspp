#pragma once

#include "types.hpp"

#include "values/non-values.hpp"
#include "values/boolean.hpp"
#include "values/number.hpp"
#include "values/string.hpp"
#include "values/object-config.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"

namespace jspp
{
    namespace Convert
    {
        inline std::string to_string(const AnyValue &val)
        {
            if (std::holds_alternative<JsString>(val))
                return std::get<JsString>(val).to_std_string();
            if (std::holds_alternative<JsUndefined>(val))
                return std::get<JsUndefined>(val).to_std_string();
            if (std::holds_alternative<JsNull>(val))
                return std::get<JsNull>(val).to_std_string();
            if (std::holds_alternative<JsUninitialized>(val))
                return std::get<JsUninitialized>(val).to_std_string(); // This should ideally not be returned if the TDZ logic is correct
            if (std::holds_alternative<JsNumber>(val))
                return std::get<JsNumber>(val).to_std_string();
            if (std::holds_alternative<JsBoolean>(val))
                return std::get<JsBoolean>(val).to_std_string();
            if (std::holds_alternative<std::shared_ptr<jspp::JsObject>>(val))
                return std::get<std::shared_ptr<jspp::JsObject>>(val)->to_std_string();
            if (std::holds_alternative<std::shared_ptr<jspp::JsArray>>(val))
                return std::get<std::shared_ptr<jspp::JsArray>>(val)->to_std_string();
            if (std::holds_alternative<std::shared_ptr<jspp::JsFunction>>(val))
                return std::get<std::shared_ptr<jspp::JsFunction>>(val)->to_std_string();
            return "[Object Object]";
        }

    }
}