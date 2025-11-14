#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "any_value.hpp"
#include "error.hpp"

std::optional<jspp::AnyValue> jspp::JsArray::get_prototype(const std::string &key) const
{
    if (key == "length")
    {
        static AnyValue proto = AnyValue::make_accessor_descriptor([this](const std::vector<AnyValue> &args) -> AnyValue
                                                                   { return AnyValue::make_number(this->length); },
                                                                   std::nullopt,
                                                                   false,
                                                                   false);
        return proto;
    }

    if (key == "toString")
    {
        static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this](const std::vector<AnyValue> &args) -> AnyValue
                                                                                       { return AnyValue::make_string(this->to_std_string()); }, "toString"),
                                                               false,
                                                               false,
                                                               false);
        return proto;
    }

    return std::nullopt;
}
