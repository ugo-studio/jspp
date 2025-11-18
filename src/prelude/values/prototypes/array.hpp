#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "js_value.hpp"
#include "error.hpp"
#include "operators.hpp"
#include <algorithm>
#include <vector>

namespace jspp
{
    namespace ArrayPrototypes
    {
        inline std::optional<JsValue> get(const std::string &key, JsArray *self)
        {

            // --- toString() method ---
            if (key == "toString")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               { return JsValue::make_string(self->to_std_string()); },
                                               key);
            }

            // --- length property ---
            if (key == "length")
            {
                auto getter = [&self](const std::vector<JsValue> &args) -> JsValue
                {
                    return JsValue::make_number(self->length);
                };

                auto setter = [&self](const std::vector<JsValue> &args) -> JsValue
                {
                    if (args.empty())
                    {
                        return JsValue::make_undefined();
                    }

                    const auto &new_len_val = args[0];
                    double new_len_double = Operators_Private::ToNumber(new_len_val);

                    if (new_len_double < 0 || std::isnan(new_len_double) || std::isinf(new_len_double) || new_len_double != static_cast<uint64_t>(new_len_double))
                    {
                        throw RuntimeError::make_error("Invalid array length", "RangeError");
                    }
                    uint64_t new_len = static_cast<uint64_t>(new_len_double);

                    // Truncate dense part
                    if (new_len < self->dense.size())
                    {
                        self->dense.resize(new_len);
                    }

                    // Remove sparse elements beyond the new length
                    for (auto it = self->sparse.begin(); it != self->sparse.end();)
                    {
                        if (it->first >= new_len)
                        {
                            it = self->sparse.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    self->length = new_len;
                    return new_len_val;
                };

                return JsValue::make_accessor_descriptor(getter,
                                                          setter,
                                                          false,
                                                          false);
            }

            // --- push() method ---
            if (key == "push")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                                                                                           for (const auto &arg : args)
                                                                                           {
                                                                                               self->set_property(static_cast<uint32_t>(self->length), arg);
                                                                                           }
                                                                                           return JsValue::make_number(self->length); },
                                               key);
            }

            // --- pop() method ---
            if (key == "pop")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                                                                                           if (self->length == 0)
                                                                                           {
                                                                                               return JsValue::make_undefined();
                                                                                           }
                                                                                           uint64_t last_idx = self->length - 1;
                                                                                           JsValue last_val = self->get_property(static_cast<uint32_t>(last_idx));

                                                                                           // Remove from dense
                                                                                           if (last_idx < self->dense.size())
                                                                                           {
                                                                                               self->dense.pop_back();
                                                                                           }
                                                                                           // Remove from sparse
                                                                                           self->sparse.erase(static_cast<uint32_t>(last_idx));

                                                                                           self->length--;
                                                                                           return last_val; },
                                               key);
            }

            // --- shift() method ---
            if (key == "shift")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                                                                                           if (self->length == 0)
                                                                                           {
                                                                                               return JsValue::make_undefined();
                                                                                           }
                                                                                           JsValue first_val = self->get_property(0u);

                                                                                           // Shift all elements to the left
                                                                                           for (uint64_t i = 0; i < self->length - 1; ++i)
                                                                                           {
                                                                                               self->set_property(static_cast<uint32_t>(i), self->get_property(static_cast<uint32_t>(i + 1)));
                                                                                           }

                                                                                           // remove last element
                                                                                           uint64_t last_idx = self->length - 1;
                                                                                           if (last_idx < self->dense.size())
                                                                                           {
                                                                                               self->dense.pop_back();
                                                                                           }
                                                                                           self->sparse.erase(static_cast<uint32_t>(last_idx));

                                                                                           self->length--;

                                                                                           return first_val; },
                                               key);
            }

            // --- unshift() method ---
            if (key == "unshift")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                                                                                             size_t args_count = args.size();
                                                                                             if (args_count == 0)
                                                                                             {
                                                                                                 return JsValue::make_number(self->length);
                                                                                             }

                                                                                             // Shift existing elements to the right
                                                                                             for (uint64_t i = self->length; i > 0; --i)
                                                                                             {
                                                                                                 self->set_property(static_cast<uint32_t>(i + args_count - 1), self->get_property(static_cast<uint32_t>(i - 1)));
                                                                                             }

                                                                                             // Insert new elements at the beginning
                                                                                             for (size_t i = 0; i < args_count; ++i)
                                                                                             {
                                                                                                 self->set_property(static_cast<uint32_t>(i), args[i]);
                                                                                             }

                                                                                             return JsValue::make_number(self->length); },
                                               key);
            }

            // --- join() method ---
            if (key == "join")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                                                                                           std::string sep = ",";
                                                                                           if (!args.empty() && !args[0].is_undefined())
                                                                                           {
                                                                                               sep = args[0].to_std_string();
                                                                                           }

                                                                                           std::string result = "";
                                                                                           for (uint64_t i = 0; i < self->length; ++i)
                                                                                           {
                                                                                               JsValue item = self->get_property(static_cast<uint32_t>(i));
                                                                                               if (!item.is_undefined() && !item.is_null())
                                                                                               {
                                                                                                   result += item.to_std_string();
                                                                                               }
                                                                                               if (i < self->length - 1)
                                                                                               {
                                                                                                   result += sep;
                                                                                               }
                                                                                           }
                                                                                           return JsValue::make_string(result); },
                                               key);
            }

            // --- forEach() method ---
            if (key == "forEach")
            {
                return JsValue::make_function([&self](const std::vector<JsValue> &args) -> JsValue
                                               {
                                                                                           if (args.empty() || !args[0].is_function())
                                                                                           {
                                                                                               throw RuntimeError::make_error("callback is not a function", "TypeError");
                                                                                           }
                                                                                           auto callback = args[0].as_function();
                                                                                           for (uint64_t i = 0; i < self->length; ++i)
                                                                                           {
                                                                                               JsValue val = self->get_property(static_cast<uint32_t>(i));
                                                                                               if (!val.is_undefined())
                                                                                               { // forEach skips empty slots
                                                                                                   callback->call({val, JsValue::make_number(i)});
                                                                                               }
                                                                                           }
                                                                                           return JsValue::make_undefined(); },
                                               key);
            }

            return std::nullopt;
        }
    }
}