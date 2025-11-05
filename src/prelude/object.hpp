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
            // Create a new map with the correct type
            std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, AnyValue>> props_variant;
            for (const auto &pair : properties)
            {
                props_variant[pair.first] = pair.second;
            }
            return std::make_shared<jspp::JsObject>(jspp::JsObject{props_variant});
        }

        inline std::shared_ptr<jspp::JsArray> make_array(const std::vector<AnyValue> &items)
        {
            return std::make_shared<jspp::JsArray>(jspp::JsArray{items, {}});
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
