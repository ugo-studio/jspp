#pragma once

#include "types.hpp"
#include "values/array.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"
#include <algorithm>
#include <vector>

namespace jspp
{
    namespace ArrayPrototypes
    {
        inline std::optional<AnyValue> get(const std::string &key, JsArray *self)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                               { return AnyValue::make_string(self->to_std_string()); },
                                               key);
            }

            // --- [Symbol.iterator]() method ---
            if (key == WellKnownSymbols::iterator->key)
            {
                return AnyValue::make_generator([self](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                { return AnyValue::from_iterator(self->get_iterator()); },
                                                key);
            }

            // --- length property ---
            if (key == "length")
            {
                auto getter = [self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    return AnyValue::make_number(self->length);
                };

                auto setter = [self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                    {
                        return AnyValue::make_undefined();
                    }

                    const auto &new_len_val = args[0];
                    double new_len_double = Operators_Private::ToNumber(new_len_val);

                    if (new_len_double < 0 || std::isnan(new_len_double) || std::isinf(new_len_double) || new_len_double != static_cast<uint64_t>(new_len_double))
                    {
                        throw Exception::make_exception("Invalid array length", "RangeError");
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
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
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
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
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
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
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
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
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
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
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
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                                                                                           if (args.empty() || !args[0].is_function())
                                                                                           {
                                                                                               throw Exception::make_exception("callback is not a function", "TypeError");
                                                                                           }
                                                                                           auto callback = args[0].as_function();
                                                                                           for (uint64_t i = 0; i < self->length; ++i)
                                                                                           {
                                                                                               AnyValue val = self->get_property(static_cast<uint32_t>(i));
                                                                                               if (!val.is_undefined())
                                                                                               { // forEach skips empty slots
                                                                                                   AnyValue iVal = AnyValue::make_number(i);
                                                                                                   const AnyValue cbArgs[] = {val, iVal, thisVal};
                                                                                                   callback->call(thisVal, cbArgs);
                                                                                               }
                                                                                           }
                                                                                           return AnyValue::make_undefined(); },
                                               key);
            }

            // --- at(index) ---
            if (key == "at")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    double len = static_cast<double>(self->length);
                    double relativeIndex = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    double k;
                    if (relativeIndex >= 0) k = relativeIndex;
                    else k = len + relativeIndex;
                    if (k < 0 || k >= len) return AnyValue::make_undefined();
                    return self->get_property(static_cast<uint32_t>(k)); },
                                               key);
            }

            // --- includes(searchElement, fromIndex) ---
            if (key == "includes")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    AnyValue searchElement = args.empty() ? AnyValue::make_undefined() : args[0];
                    double len = static_cast<double>(self->length);
                    if (len == 0) return AnyValue::make_boolean(false);
                    double n = (args.size() > 1) ? Operators_Private::ToNumber(args[1]) : 0;
                    double k;
                    if (n >= 0) k = n;
                    else k = len + n;
                    if (k < 0) k = 0;

                    for (uint64_t i = static_cast<uint64_t>(k); i < self->length; ++i)
                    {
                        AnyValue element = self->get_property(static_cast<uint32_t>(i));
                        // SameValueZero algorithm (includes handles NaN)
                        if (element.is_number() && searchElement.is_number() && std::isnan(element.as_double()) && std::isnan(searchElement.as_double())) return AnyValue::make_boolean(true);
                        if (is_strictly_equal_to_primitive(element, searchElement)) return AnyValue::make_boolean(true);
                    }
                    return AnyValue::make_boolean(false); },
                                               key);
            }

            // --- indexOf(searchElement, fromIndex) ---
            if (key == "indexOf")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    AnyValue searchElement = args.empty() ? AnyValue::make_undefined() : args[0];
                    double len = static_cast<double>(self->length);
                    if (len == 0) return AnyValue::make_number(-1);
                    double n = (args.size() > 1) ? Operators_Private::ToNumber(args[1]) : 0;
                    double k;
                    if (n >= 0) k = n;
                    else k = len + n;
                    if (k < 0) k = 0;

                    for (uint64_t i = static_cast<uint64_t>(k); i < self->length; ++i)
                    {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue element = self->get_property(static_cast<uint32_t>(i));
                            if (is_strictly_equal_to_primitive(element, searchElement)) return AnyValue::make_number(i);
                        }
                    }
                    return AnyValue::make_number(-1); },
                                               key);
            }

            // --- lastIndexOf(searchElement, fromIndex) ---
            if (key == "lastIndexOf")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    AnyValue searchElement = args.empty() ? AnyValue::make_undefined() : args[0];
                    double len = static_cast<double>(self->length);
                    if (len == 0) return AnyValue::make_number(-1);
                    double n = (args.size() > 1) ? Operators_Private::ToNumber(args[1]) : len - 1;
                    double k;
                    if (n >= 0) k = std::min(n, len - 1);
                    else k = len + n;
                    
                    if (k < 0) return AnyValue::make_number(-1);

                    for (int64_t i = static_cast<int64_t>(k); i >= 0; --i)
                    {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue element = self->get_property(static_cast<uint32_t>(i));
                            if (is_strictly_equal_to_primitive(element, searchElement)) return AnyValue::make_number(i);
                        }
                    }
                    return AnyValue::make_number(-1); },
                                               key);
            }

            // --- find(callback, thisArg) ---
            if (key == "find")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    for (uint64_t i = 0; i < self->length; ++i)
                    {
                        AnyValue element = self->get_property(static_cast<uint32_t>(i));
                        AnyValue kVal = AnyValue::make_number(i);
                        const AnyValue cbArgs[] = {element, kVal, thisVal};
                        
                        if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                            return element;
                        }
                    }
                    return AnyValue::make_undefined(); },
                                               key);
            }

            // --- findIndex(callback, thisArg) ---
            if (key == "findIndex")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    for (uint64_t i = 0; i < self->length; ++i)
                    {
                        AnyValue element = self->get_property(static_cast<uint32_t>(i));
                        AnyValue kVal = AnyValue::make_number(i);
                        const AnyValue cbArgs[] = {element, kVal, thisVal};
                        
                        if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                            return AnyValue::make_number(i);
                        }
                    }
                    return AnyValue::make_number(-1); },
                                               key);
            }

            // --- findLast(callback, thisArg) ---
            if (key == "findLast")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    for (int64_t i = self->length - 1; i >= 0; --i)
                    {
                        AnyValue element = self->get_property(static_cast<uint32_t>(i));
                        AnyValue kVal = AnyValue::make_number(i);
                        const AnyValue cbArgs[] = {element, kVal, thisVal};
                        
                        if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                            return element;
                        }
                    }
                    return AnyValue::make_undefined(); },
                                               key);
            }

            // --- findLastIndex(callback, thisArg) ---
            if (key == "findLastIndex")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    for (int64_t i = self->length - 1; i >= 0; --i)
                    {
                        AnyValue element = self->get_property(static_cast<uint32_t>(i));
                        AnyValue kVal = AnyValue::make_number(i);
                        const AnyValue cbArgs[] = {element, kVal, thisVal};
                        
                        if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                            return AnyValue::make_number(i);
                        }
                    }
                    return AnyValue::make_number(-1); },
                                               key);
            }

            // --- values() ---
            if (key == "values")
            {
                return AnyValue::make_generator([self](const AnyValue &thisVal, std::span<const AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                { return self->get_iterator(); },
                                                key);
            }

            // --- keys() ---
            if (key == "keys")
            {
                return AnyValue::make_generator([self](const AnyValue &thisVal, std::span<const AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                { 
                                                    // Generator for keys
                                                    for (uint64_t i = 0; i < self->length; ++i) {
                                                        co_yield AnyValue::make_number(i);
                                                    }
                                                    co_return AnyValue::make_undefined();
                                                },
                                                key);
            }

            // --- entries() ---
            if (key == "entries")
            {
                return AnyValue::make_generator([self](const AnyValue &thisVal, std::span<const AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                { 
                                                    // Generator for [key, value]
                                                    for (uint64_t i = 0; i < self->length; ++i) {
                                                        std::vector<AnyValue> entry;
                                                        entry.push_back(AnyValue::make_number(i));
                                                        entry.push_back(self->get_property(static_cast<uint32_t>(i)));
                                                        co_yield AnyValue::make_array(std::move(entry));
                                                    }
                                                    co_return AnyValue::make_undefined();
                                                },
                                                key);
            }

            // --- map(callback, thisArg) ---
            if (key == "map")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    std::vector<AnyValue> result;
                    result.reserve(self->length);
                    
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {val, kVal, thisVal};
                            result.push_back(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)));
                        } else {
                            result.push_back(Constants::UNINITIALIZED);
                        }
                    }
                    return AnyValue::make_array(std::move(result)); },
                                               key);
            }

            // --- filter(callback, thisArg) ---
            if (key == "filter")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    std::vector<AnyValue> result;
                    
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {val, kVal, thisVal};
                            if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                                result.push_back(val);
                            }
                        }
                    }
                    return AnyValue::make_array(std::move(result)); },
                                               key);
            }

            // --- every(callback, thisArg) ---
            if (key == "every")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {val, kVal, thisVal};
                            if (!is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                                return AnyValue::make_boolean(false);
                            }
                        }
                    }
                    return AnyValue::make_boolean(true); },
                                               key);
            }

            // --- some(callback, thisArg) ---
            if (key == "some")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {val, kVal, thisVal};
                            if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                                return AnyValue::make_boolean(true);
                            }
                        }
                    }
                    return AnyValue::make_boolean(false); },
                                               key);
            }

            // --- reduce(callback, initialValue) ---
            if (key == "reduce")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    
                    uint64_t i = 0;
                    AnyValue accumulator;
                    
                    if (args.size() > 1) {
                        accumulator = args[1];
                    } else {
                        if (self->length == 0) throw Exception::make_exception("Reduce of empty array with no initial value", "TypeError");
                        bool found = false;
                        for (; i < self->length; ++i) {
                            if (self->has_property(std::to_string(i))) {
                                accumulator = self->get_property(static_cast<uint32_t>(i));
                                found = true;
                                i++;
                                break;
                            }
                        }
                        if (!found) throw Exception::make_exception("Reduce of empty array with no initial value", "TypeError");
                    }
                    
                    for (; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {accumulator, val, kVal, thisVal};
                            accumulator = callback->call(Constants::UNDEFINED, std::span<const AnyValue>(cbArgs, 4));
                        }
                    }
                    return accumulator; },
                                               key);
            }

            // --- reduceRight(callback, initialValue) ---
            if (key == "reduceRight")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    
                    int64_t i = self->length - 1;
                    AnyValue accumulator;
                    
                    if (args.size() > 1) {
                        accumulator = args[1];
                    } else {
                        if (self->length == 0) throw Exception::make_exception("Reduce of empty array with no initial value", "TypeError");
                        bool found = false;
                        for (; i >= 0; --i) {
                            if (self->has_property(std::to_string(i))) {
                                accumulator = self->get_property(static_cast<uint32_t>(i));
                                found = true;
                                i--;
                                break;
                            }
                        }
                        if (!found) throw Exception::make_exception("Reduce of empty array with no initial value", "TypeError");
                    }
                    
                    for (; i >= 0; --i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {accumulator, val, kVal, thisVal};
                            accumulator = callback->call(Constants::UNDEFINED, std::span<const AnyValue>(cbArgs, 4));
                        }
                    }
                    return accumulator; },
                                               key);
            }

            // --- flat(depth) ---
            if (key == "flat")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    double depthVal = (args.size() > 0 && !args[0].is_undefined()) ? Operators_Private::ToNumber(args[0]) : 1;
                    int depth = static_cast<int>(depthVal);
                    if (depth < 0) depth = 0;
                    
                    std::vector<AnyValue> result;
                    std::function<void(const AnyValue&, int)> flatten;
                    flatten = [&result, &flatten](const AnyValue& item, int d) {
                        if (d > 0 && item.is_array()) {
                            auto arr = item.as_array();
                            for (uint64_t i = 0; i < arr->length; ++i) {
                                if (arr->has_property(std::to_string(i))) {
                                    flatten(arr->get_property(static_cast<uint32_t>(i)), d - 1);
                                }
                            }
                        } else {
                            result.push_back(item);
                        }
                    };
                    
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            flatten(self->get_property(static_cast<uint32_t>(i)), depth);
                        }
                    }
                    return AnyValue::make_array(std::move(result)); },
                                               key);
            }

            // --- flatMap(callback, thisArg) ---
            if (key == "flatMap")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                    auto callback = args[0].as_function();
                    auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    
                    std::vector<AnyValue> result;
                    
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            AnyValue val = self->get_property(static_cast<uint32_t>(i));
                            AnyValue kVal = AnyValue::make_number(i);
                            const AnyValue cbArgs[] = {val, kVal, thisVal};
                            AnyValue mapped = callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3));
                            
                            if (mapped.is_array()) {
                                auto arr = mapped.as_array();
                                for (uint64_t j = 0; j < arr->length; ++j) {
                                    if (arr->has_property(std::to_string(j))) {
                                        result.push_back(arr->get_property(static_cast<uint32_t>(j)));
                                    }
                                }
                            } else {
                                result.push_back(mapped);
                            }
                        }
                    }
                    return AnyValue::make_array(std::move(result)); },
                                               key);
            }

            // --- fill(value, start, end) ---
            if (key == "fill")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    AnyValue value = args.empty() ? AnyValue::make_undefined() : args[0];
                    double len = static_cast<double>(self->length);
                    double start = (args.size() > 1) ? Operators_Private::ToNumber(args[1]) : 0;
                    double end = (args.size() > 2 && !args[2].is_undefined()) ? Operators_Private::ToNumber(args[2]) : len;
                    
                    double k;
                    if (start >= 0) k = start; else k = len + start;
                    if (k < 0) k = 0;
                    
                    double final;
                    if (end >= 0) final = end; else final = len + end;
                    if (final > len) final = len;
                    
                    for (uint64_t i = static_cast<uint64_t>(k); i < static_cast<uint64_t>(final); ++i) {
                        self->set_property(static_cast<uint32_t>(i), value);
                    }
                    return thisVal; },
                                               key);
            }

            // --- reverse() ---
            if (key == "reverse")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    uint64_t len = self->length;
                    for (uint64_t i = 0; i < len / 2; ++i) {
                        uint64_t j = len - 1 - i;
                        bool hasI = self->has_property(std::to_string(i));
                        bool hasJ = self->has_property(std::to_string(j));
                        
                        if (hasI && hasJ) {
                            AnyValue valI = self->get_property(static_cast<uint32_t>(i));
                            AnyValue valJ = self->get_property(static_cast<uint32_t>(j));
                            self->set_property(static_cast<uint32_t>(i), valJ);
                            self->set_property(static_cast<uint32_t>(j), valI);
                        } else if (hasI && !hasJ) {
                            AnyValue valI = self->get_property(static_cast<uint32_t>(i));
                            self->set_property(static_cast<uint32_t>(j), valI);
                            if (i < self->dense.size()) self->dense[i] = Constants::UNINITIALIZED;
                            else self->sparse.erase(static_cast<uint32_t>(i));
                        } else if (!hasI && hasJ) {
                            AnyValue valJ = self->get_property(static_cast<uint32_t>(j));
                            self->set_property(static_cast<uint32_t>(i), valJ);
                            if (j < self->dense.size()) self->dense[j] = Constants::UNINITIALIZED;
                            else self->sparse.erase(static_cast<uint32_t>(j));
                        }
                    }
                    return thisVal; },
                                               key);
            }

            // --- sort(compareFn) ---
            if (key == "sort")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    AnyValue compareFn = args.empty() ? AnyValue::make_undefined() : args[0];
                    
                    std::vector<AnyValue> items;
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            items.push_back(self->get_property(static_cast<uint32_t>(i)));
                        }
                    }
                    
                    std::sort(items.begin(), items.end(), [&](const AnyValue& a, const AnyValue& b) {
                        if (a.is_undefined() && b.is_undefined()) return false;
                        if (a.is_undefined()) return false; 
                        if (b.is_undefined()) return true;
                        
                        if (compareFn.is_function()) {
                            const AnyValue cmpArgs[] = {a, b};
                            double res = Operators_Private::ToNumber(compareFn.as_function()->call(Constants::UNDEFINED, std::span<const AnyValue>(cmpArgs, 2)));
                            return res < 0;
                        } else {
                            std::string sA = a.to_std_string();
                            std::string sB = b.to_std_string();
                            return sA < sB;
                        }
                    });
                    
                    for (uint64_t i = 0; i < items.size(); ++i) {
                        self->set_property(static_cast<uint32_t>(i), items[i]);
                    }
                    for (uint64_t i = items.size(); i < self->length; ++i) {
                        if (i < self->dense.size()) self->dense[i] = Constants::UNINITIALIZED;
                        else self->sparse.erase(static_cast<uint32_t>(i));
                    }
                    
                    return thisVal; },
                                               key);
            }

            // --- splice(start, deleteCount, ...items) ---
            if (key == "splice")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    double len = static_cast<double>(self->length);
                    double start = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    double actualStart = (start < 0) ? std::max(len + start, 0.0) : std::min(start, len);
                    
                    uint64_t startIdx = static_cast<uint64_t>(actualStart);
                    uint64_t deleteCount = 0;
                    if (args.size() >= 2) {
                        double dc = Operators_Private::ToNumber(args[1]);
                        deleteCount = static_cast<uint64_t>(std::max(0.0, std::min(dc, len - startIdx)));
                    } else if (args.size() == 1) {
                        deleteCount = len - startIdx;
                    }
                    
                    std::vector<AnyValue> deletedItems;
                    for (uint64_t i = 0; i < deleteCount; ++i) {
                        if (self->has_property(std::to_string(startIdx + i))) {
                            deletedItems.push_back(self->get_property(static_cast<uint32_t>(startIdx + i)));
                        } else {
                            deletedItems.push_back(Constants::UNINITIALIZED);
                        }
                    }
                    
                    std::vector<AnyValue> insertItems;
                    for (size_t i = 2; i < args.size(); ++i) {
                        insertItems.push_back(args[i]);
                    }
                    uint64_t insertCount = insertItems.size();
                    
                    if (insertCount < deleteCount) {
                        for (uint64_t i = startIdx; i < len - deleteCount; ++i) {
                            uint64_t from = i + deleteCount;
                            uint64_t to = i + insertCount;
                            if (self->has_property(std::to_string(from))) {
                                self->set_property(static_cast<uint32_t>(to), self->get_property(static_cast<uint32_t>(from)));
                            } else {
                                if (to < self->dense.size()) self->dense[to] = Constants::UNINITIALIZED;
                                else self->sparse.erase(static_cast<uint32_t>(to));
                            }
                        }
                        for (uint64_t i = len; i > len - deleteCount + insertCount; --i) {
                             uint64_t idx = i - 1;
                             if (idx < self->dense.size()) self->dense[idx] = Constants::UNINITIALIZED;
                             else self->sparse.erase(static_cast<uint32_t>(idx));
                        }
                    } else if (insertCount > deleteCount) {
                        for (uint64_t i = len; i > startIdx + deleteCount; --i) {
                            uint64_t from = i - 1;
                            uint64_t to = from - deleteCount + insertCount;
                             if (self->has_property(std::to_string(from))) {
                                self->set_property(static_cast<uint32_t>(to), self->get_property(static_cast<uint32_t>(from)));
                            } else {
                                if (to < self->dense.size()) self->dense[to] = Constants::UNINITIALIZED;
                                else self->sparse.erase(static_cast<uint32_t>(to));
                            }
                        }
                    }
                    
                    for (uint64_t i = 0; i < insertCount; ++i) {
                        self->set_property(static_cast<uint32_t>(startIdx + i), insertItems[i]);
                    }
                    
                    self->length = len - deleteCount + insertCount;
                    return AnyValue::make_array(std::move(deletedItems)); },
                                               key);
            }

            // --- copyWithin(target, start, end) ---
            if (key == "copyWithin")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    double len = static_cast<double>(self->length);
                    double target = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    double start = (args.size() > 1) ? Operators_Private::ToNumber(args[1]) : 0;
                    double end = (args.size() > 2 && !args[2].is_undefined()) ? Operators_Private::ToNumber(args[2]) : len;
                    
                    double to;
                    if (target >= 0) to = target; else to = len + target;
                    if (to < 0) to = 0; else if (to > len) to = len;
                    
                    double from;
                    if (start >= 0) from = start; else from = len + start;
                    if (from < 0) from = 0; else if (from > len) from = len;
                    
                    double final;
                    if (end >= 0) final = end; else final = len + end;
                    if (final < 0) final = 0; else if (final > len) final = len;
                    
                    double count = std::min(final - from, len - to);
                    
                    if (from < to && to < from + count) {
                        for (double i = count - 1; i >= 0; --i) {
                            uint64_t f = static_cast<uint64_t>(from + i);
                            uint64_t t = static_cast<uint64_t>(to + i);
                            if (self->has_property(std::to_string(f))) {
                                self->set_property(static_cast<uint32_t>(t), self->get_property(static_cast<uint32_t>(f)));
                            } else {
                                if (t < self->dense.size()) self->dense[t] = Constants::UNINITIALIZED;
                                else self->sparse.erase(static_cast<uint32_t>(t));
                            }
                        }
                    } else {
                        for (double i = 0; i < count; ++i) {
                            uint64_t f = static_cast<uint64_t>(from + i);
                            uint64_t t = static_cast<uint64_t>(to + i);
                            if (self->has_property(std::to_string(f))) {
                                self->set_property(static_cast<uint32_t>(t), self->get_property(static_cast<uint32_t>(f)));
                            } else {
                                if (t < self->dense.size()) self->dense[t] = Constants::UNINITIALIZED;
                                else self->sparse.erase(static_cast<uint32_t>(t));
                            }
                        }
                    }
                    return thisVal; },
                                               key);
            }

            // --- concat(...items) ---
            if (key == "concat")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    std::vector<AnyValue> result;
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (self->has_property(std::to_string(i))) {
                            result.push_back(self->get_property(static_cast<uint32_t>(i)));
                        } else {
                            result.push_back(Constants::UNINITIALIZED);
                        }
                    }
                    
                    for (const auto& item : args) {
                        bool spreadable = false;
                        if (item.is_array()) {
                            spreadable = true;
                            auto sym = WellKnownSymbols::isConcatSpreadable;
                            if (item.has_property(sym->key)) {
                                spreadable = is_truthy(item.get_property_with_receiver(sym->key, item));
                            }
                        } else if (item.is_object()) {
                             auto sym = WellKnownSymbols::isConcatSpreadable;
                             if (item.has_property(sym->key)) {
                                spreadable = is_truthy(item.get_property_with_receiver(sym->key, item));
                             }
                        }
                        
                        if (spreadable && item.is_array()) {
                            auto arr = item.as_array();
                            for (uint64_t i = 0; i < arr->length; ++i) {
                                if (arr->has_property(std::to_string(i))) {
                                    result.push_back(arr->get_property(static_cast<uint32_t>(i)));
                                } else {
                                    result.push_back(Constants::UNINITIALIZED);
                                }
                            }
                        } else {
                            result.push_back(item);
                        }
                    }
                    return AnyValue::make_array(std::move(result)); },
                                               key);
            }

            // --- slice(start, end) ---
            if (key == "slice")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    double len = static_cast<double>(self->length);
                    double start = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    double end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToNumber(args[1]);
                    
                    double k;
                    if (start >= 0) k = start; else k = len + start;
                    if (k < 0) k = 0;
                    
                    double final;
                    if (end >= 0) final = end; else final = len + end;
                    if (final > len) final = len;
                    
                    if (final < k) final = k;
                    
                    std::vector<AnyValue> result;
                    for (uint64_t i = static_cast<uint64_t>(k); i < static_cast<uint64_t>(final); ++i) {
                        if (self->has_property(std::to_string(i))) {
                            result.push_back(self->get_property(static_cast<uint32_t>(i)));
                        } else {
                            result.push_back(Constants::UNINITIALIZED);
                        }
                    }
                    return AnyValue::make_array(std::move(result)); },
                                               key);
            }

            // --- toReversed() ---
            if (key == "toReversed")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    auto copy = self->get_property("slice", thisVal).as_function()->call(thisVal, {});
                    copy.get_own_property("reverse").as_function()->call(copy, {});
                    return copy; },
                                               key);
            }

            // --- toSorted(compareFn) ---
            if (key == "toSorted")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    auto copy = self->get_property("slice", thisVal).as_function()->call(thisVal, {});
                    copy.get_own_property("sort").as_function()->call(copy, args);
                    return copy; },
                                               key);
            }

            // --- toSpliced(start, deleteCount, ...items) ---
            if (key == "toSpliced")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    auto copy = self->get_property("slice", thisVal).as_function()->call(thisVal, {});
                    copy.get_own_property("splice").as_function()->call(copy, args);
                    return copy; },
                                               key);
            }

            // --- with(index, value) ---
            if (key == "with")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    auto copy = self->get_property("slice", thisVal).as_function()->call(thisVal, {});
                    
                    double len = static_cast<double>(self->length);
                    double idx = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                    double k;
                    if (idx >= 0) k = idx; else k = len + idx;
                    
                    if (k < 0 || k >= len) throw Exception::make_exception("Invalid index", "RangeError");
                    
                    AnyValue value = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                    copy.set_own_property(static_cast<uint32_t>(k), value);
                    return copy; },
                                               key);
            }

            // --- toLocaleString() ---
            if (key == "toLocaleString")
            {
                return AnyValue::make_function([self](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                               {
                    std::string result = "";
                    for (uint64_t i = 0; i < self->length; ++i) {
                        if (i > 0) result += ",";
                        AnyValue element = self->get_property(static_cast<uint32_t>(i));
                        if (!element.is_null() && !element.is_undefined()) {
                            if (element.has_property("toLocaleString")) {
                                auto fn = element.get_property_with_receiver("toLocaleString", element);
                                if (fn.is_function()) {
                                    result += fn.as_function()->call(element, {}).to_std_string();
                                    continue;
                                }
                            }
                            result += element.to_std_string();
                        }
                    }
                    return AnyValue::make_string(result); },
                                               key);
            }

            return std::nullopt;
        }
    }
}