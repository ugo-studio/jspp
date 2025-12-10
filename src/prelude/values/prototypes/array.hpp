#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "any_value.hpp"
#include "error.hpp"
#include "operators.hpp"
#include <algorithm>
#include <vector>

namespace jspp
{
    namespace ArrayPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsArray *self)
        {
            // --- toString() method ---
            if (key == "toString" )
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &_) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            // --- [Symbol.iterator]() method ---
            if (key == WellKnownSymbols::iterator->key)
            {
                return AnyValue::make_generator([self](const AnyValue &thisVal, const std::vector<AnyValue> &_) -> AnyValue
                                                { return AnyValue::from_iterator(self->get_iterator()); },
                                                key);
            }

            // --- length property ---
            if (key == "length")
            {
                auto getter = [self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    return AnyValue::make_number(self->length);
                };

                auto setter = [self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    if (args.empty())
                    {
                        return AnyValue::make_undefined();
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

                return AnyValue::make_accessor_descriptor(getter,
                                                          setter,
                                                          false,
                                                          false);
            }

            // --- push() method ---
            if (key == "push")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                                               {
                                                                                           for (const auto &arg : args)
                                                                                           {
                                                                                               self->set_property(static_cast<uint32_t>(self->length), arg);
                                                                                           }
                                                                                           return AnyValue::make_number(self->length); },
                                               key);
            }

            // --- pop() method ---
            if (key == "pop")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                                               {
                                                                                           if (self->length == 0)
                                                                                           {
                                                                                               return AnyValue::make_undefined();
                                                                                           }
                                                                                           uint64_t last_idx = self->length - 1;
                                                                                           AnyValue last_val = self->get_property(static_cast<uint32_t>(last_idx));

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
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                                               {
                                                                                           if (self->length == 0)
                                                                                           {
                                                                                               return AnyValue::make_undefined();
                                                                                           }
                                                                                           AnyValue first_val = self->get_property(0u);

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
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                                               {
                                                                                             size_t args_count = args.size();
                                                                                             if (args_count == 0)
                                                                                             {
                                                                                                 return AnyValue::make_number(self->length);
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

                                                                                             return AnyValue::make_number(self->length); },
                                               key);
            }

            // --- join() method ---
            if (key == "join")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                                               {
                                                                                           std::string sep = ",";
                                                                                           if (!args.empty() && !args[0].is_undefined())
                                                                                           {
                                                                                               sep = args[0].to_std_string();
                                                                                           }

                                                                                           std::string result = "";
                                                                                           for (uint64_t i = 0; i < self->length; ++i)
                                                                                           {
                                                                                               AnyValue item = self->get_property(static_cast<uint32_t>(i));
                                                                                               if (!item.is_undefined() && !item.is_null())
                                                                                               {
                                                                                                   result += item.to_std_string();
                                                                                               }
                                                                                               if (i < self->length - 1)
                                                                                               {
                                                                                                   result += sep;
                                                                                               }
                                                                                           }
                                                                                           return AnyValue::make_string(result); },
                                               key);
            }

            // --- forEach() method ---
            if (key == "forEach")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                                               {
                                                                                           if (args.empty() || !args[0].is_function())
                                                                                           {
                                                                                               throw RuntimeError::make_error("callback is not a function", "TypeError");
                                                                                           }
                                                                                           auto callback = args[0].as_function();
                                                                                           for (uint64_t i = 0; i < self->length; ++i)
                                                                                           {
                                                                                               AnyValue val = self->get_property(static_cast<uint32_t>(i));
                                                                                               if (!val.is_undefined())
                                                                                               { // forEach skips empty slots
                                                                                                   callback->call(thisVal, {val, AnyValue::make_number(i)});
                                                                                               }
                                                                                           }
                                                                                           return AnyValue::make_undefined(); },
                                               key);
            }

            return std::nullopt;
        }
    }
}