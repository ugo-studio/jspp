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
        inline std::optional<AnyValue> get(const std::string &key, JsArray *this_ptr)
        {

            // --- toString() method ---
            if (key == "toString")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               { return AnyValue::make_string(this_ptr->to_std_string()); },
                                                                                               "toString"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            // --- length property ---
            if (key == "length")
            {
                auto getter = [this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                {
                    return AnyValue::make_number(this_ptr->length);
                };

                auto setter = [this_ptr](const std::vector<AnyValue> &args) -> AnyValue
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
                    if (new_len < this_ptr->dense.size())
                    {
                        this_ptr->dense.resize(new_len);
                    }

                    // Remove sparse elements beyond the new length
                    for (auto it = this_ptr->sparse.begin(); it != this_ptr->sparse.end();)
                    {
                        if (it->first >= new_len)
                        {
                            it = this_ptr->sparse.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    this_ptr->length = new_len;
                    return new_len_val;
                };

                AnyValue proto = AnyValue::make_accessor_descriptor(getter,
                                                                    setter,
                                                                    false,
                                                                    false);
                return proto;
            }

            // --- push() method ---
            if (key == "push")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               {
                                                                                           for (const auto &arg : args)
                                                                                           {
                                                                                               this_ptr->set_property(static_cast<uint32_t>(this_ptr->length), arg);
                                                                                           }
                                                                                           return AnyValue::make_number(this_ptr->length); },
                                                                                               "push"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            // --- pop() method ---
            if (key == "pop")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               {
                                                                                           if (this_ptr->length == 0)
                                                                                           {
                                                                                               return AnyValue::make_undefined();
                                                                                           }
                                                                                           uint64_t last_idx = this_ptr->length - 1;
                                                                                           AnyValue last_val = this_ptr->get_property(static_cast<uint32_t>(last_idx));

                                                                                           // Remove from dense
                                                                                           if (last_idx < this_ptr->dense.size())
                                                                                           {
                                                                                               this_ptr->dense.pop_back();
                                                                                           }
                                                                                           // Remove from sparse
                                                                                           this_ptr->sparse.erase(static_cast<uint32_t>(last_idx));

                                                                                           this_ptr->length--;
                                                                                           return last_val; },
                                                                                               "pop"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            // --- shift() method ---
            if (key == "shift")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               {
                                                                                           if (this_ptr->length == 0)
                                                                                           {
                                                                                               return AnyValue::make_undefined();
                                                                                           }
                                                                                           AnyValue first_val = this_ptr->get_property(0u);

                                                                                           // Shift all elements to the left
                                                                                           for (uint64_t i = 0; i < this_ptr->length - 1; ++i)
                                                                                           {
                                                                                               this_ptr->set_property(static_cast<uint32_t>(i), this_ptr->get_property(static_cast<uint32_t>(i + 1)));
                                                                                           }

                                                                                           // remove last element
                                                                                           uint64_t last_idx = this_ptr->length - 1;
                                                                                           if (last_idx < this_ptr->dense.size())
                                                                                           {
                                                                                               this_ptr->dense.pop_back();
                                                                                           }
                                                                                           this_ptr->sparse.erase(static_cast<uint32_t>(last_idx));

                                                                                           this_ptr->length--;

                                                                                           return first_val; },
                                                                                               "shift"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            // --- unshift() method ---
            if (key == "unshift")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               {
                                                                                             size_t args_count = args.size();
                                                                                             if (args_count == 0)
                                                                                             {
                                                                                                 return AnyValue::make_number(this_ptr->length);
                                                                                             }

                                                                                             // Shift existing elements to the right
                                                                                             for (uint64_t i = this_ptr->length; i > 0; --i)
                                                                                             {
                                                                                                 this_ptr->set_property(static_cast<uint32_t>(i + args_count - 1), this_ptr->get_property(static_cast<uint32_t>(i - 1)));
                                                                                             }

                                                                                             // Insert new elements at the beginning
                                                                                             for (size_t i = 0; i < args_count; ++i)
                                                                                             {
                                                                                                 this_ptr->set_property(static_cast<uint32_t>(i), args[i]);
                                                                                             }

                                                                                             return AnyValue::make_number(this_ptr->length); },
                                                                                               "unshift"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            // --- join() method ---
            if (key == "join")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               {
                                                                                           std::string sep = ",";
                                                                                           if (!args.empty() && !args[0].is_undefined())
                                                                                           {
                                                                                               sep = args[0].to_std_string();
                                                                                           }

                                                                                           std::string result = "";
                                                                                           for (uint64_t i = 0; i < this_ptr->length; ++i)
                                                                                           {
                                                                                               AnyValue item = this_ptr->get_property(static_cast<uint32_t>(i));
                                                                                               if (!item.is_undefined() && !item.is_null())
                                                                                               {
                                                                                                   result += item.to_std_string();
                                                                                               }
                                                                                               if (i < this_ptr->length - 1)
                                                                                               {
                                                                                                   result += sep;
                                                                                               }
                                                                                           }
                                                                                           return AnyValue::make_string(result); },
                                                                                               "join"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            // --- forEach() method ---
            if (key == "forEach")
            {
                static AnyValue proto = AnyValue::make_data_descriptor(AnyValue::make_function([this_ptr](const std::vector<AnyValue> &args) -> AnyValue
                                                                                               {
                                                                                           if (args.empty() || !args[0].is_function())
                                                                                           {
                                                                                               throw RuntimeError::make_error("callback is not a function", "TypeError");
                                                                                           }
                                                                                           auto callback = args[0].as_function();
                                                                                           for (uint64_t i = 0; i < this_ptr->length; ++i)
                                                                                           {
                                                                                               AnyValue val = this_ptr->get_property(static_cast<uint32_t>(i));
                                                                                               if (!val.is_undefined())
                                                                                               { // forEach skips empty slots
                                                                                                   callback->call({val, AnyValue::make_number(i)});
                                                                                               }
                                                                                           }
                                                                                           return AnyValue::make_undefined(); },
                                                                                               "forEach"),
                                                                       true,
                                                                       false,
                                                                       true);
                return proto;
            }

            return std::nullopt;
        }
    }
}