#pragma once

#include "types.hpp"
#include "values/generator.hpp"
#include "any_value.hpp"
#include "error.hpp"
#include "operators.hpp"

namespace jspp
{
    namespace GeneratorPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsGenerator<AnyValue> *self)
        {
            // --- toString() method ---
            if (key == "toString")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }
            // --- next() method ---
            if (key == "next")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &) -> AnyValue
                                               {
                                                auto result = self->next();
                                                return AnyValue::make_object({{"value",result.value.value_or(AnyValue::make_undefined())},{"done",AnyValue::make_boolean(result.done)}}); },
                                               key);
            }
            // --- toArray() method ---
            if (key == "toArray")
            {
                return AnyValue::make_function([&self](const std::vector<AnyValue> &) -> AnyValue
                                               {
                                                   std::vector<std::optional<AnyValue>> result;
                                                   while (true)
                                                   {
                                                       auto next = self->next();
                                                       if (next.done)
                                                       {
                                                           break;
                                                       }
                                                       result.push_back(next.value.value_or(AnyValue::make_undefined()));
                                                   }
                                                   return AnyValue::make_array(result); },
                                               key);
            }

            return std::nullopt;
        }
    }
}