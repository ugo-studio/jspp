#pragma once

#include "types.hpp"
#include "well_known_symbols.hpp"

namespace jspp
{
    namespace Object
    {
        inline std::shared_ptr<jspp::JsObject> make_object(const std::map<std::string, AnyValue> &properties);
        inline std::shared_ptr<jspp::JsArray> make_array(const std::vector<AnyValue> &properties);
        inline std::shared_ptr<jspp::JsFunction> make_function(const std::function<AnyValue(const std::vector<AnyValue> &)> &callable);
        inline std::shared_ptr<jspp::JsString> make_string(const std::string &value);
        inline std::shared_ptr<jspp::JsBoolean> make_boolean(const bool &value);

        inline std::shared_ptr<jspp::JsObject> make_object(const std::map<std::string, AnyValue> &properties)
        {

            return std::make_shared<jspp::JsObject>(jspp::JsObject{properties, {}});
        }

        inline std::shared_ptr<jspp::JsArray> make_array(const std::vector<AnyValue> &properties)
        {
            return std::make_shared<jspp::JsArray>(jspp::JsArray{properties, {}});
        }

        inline std::shared_ptr<jspp::JsFunction> make_function(const std::function<AnyValue(const std::vector<AnyValue> &)> &callable)
        {
            return std::make_shared<jspp::JsFunction>(jspp::JsFunction{callable, {}});
        }

        inline std::shared_ptr<jspp::JsString> make_string(const std::string &value)
        {
            return std::make_shared<jspp::JsString>(jspp::JsString{value, {}});
        }

        inline std::shared_ptr<jspp::JsNumber> make_number(const NumberValue &value)
        {
            return std::make_shared<jspp::JsNumber>(jspp::JsNumber{value, {}});
        }

        inline std::shared_ptr<jspp::JsBoolean> make_boolean(const bool &value)
        {
            return std::make_shared<jspp::JsBoolean>(jspp::JsBoolean{value, {}});
        }

    }
}
