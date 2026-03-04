#include "jspp.hpp"
#include "values/array.hpp"
#include "values/prototypes/array.hpp"

namespace jspp {

// --- JsArray Implementation ---

JsArray::JsArray() : proto(Constants::Null), length(0) {}
JsArray::JsArray(const std::vector<jspp::AnyValue> &items) : dense(items), proto(Constants::Null), length(items.size()) {}
JsArray::JsArray(std::vector<jspp::AnyValue> &&items) : dense(std::move(items)), proto(Constants::Null), length(dense.size()) {}

std::string JsArray::to_std_string() const
{
    if (length == 0)
    {
        return "";
    }

    std::string result = "";
    for (uint64_t i = 0; i < length; ++i)
    {
        AnyValue itemVal = Constants::UNINITIALIZED;
        if (i < dense.size())
        {
            itemVal = dense[i];
        }
        else
        {
            auto it = sparse.find(static_cast<uint32_t>(i));
            if (it != sparse.end())
            {
                itemVal = it->second;
            }
        }

        if (!itemVal.is_uninitialized())
        {
            if (!itemVal.is_undefined() && !itemVal.is_null())
            {
                result += itemVal.to_std_string();
            }
        }

        if (i < length - 1)
        {
            result += ",";
        }
    }
    return result;
}

bool JsArray::has_property(const std::string &key) const
{
    if (key == "length")
        return true;
    if (is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        if (idx < dense.size())
            return !dense[idx].is_uninitialized();
        if (sparse.find(idx) != sparse.end())
            return !sparse.at(idx).is_uninitialized();
    }
    if (props.find(key) != props.end())
        return true;

    if (!proto.is_null() && !proto.is_undefined())
    {
        if (proto.has_property(key))
            return true;
    }

    if (ArrayPrototypes::get(key).has_value())
        return true;
    return false;
}

bool JsArray::has_symbol_property(const AnyValue &key) const
{
    if (symbol_props.count(key))
        return true;
    if (!proto.is_null() && !proto.is_undefined())
    {
        if (proto.has_property(key))
            return true;
    }
    if (ArrayPrototypes::get(key).has_value())
        return true;
    return false;
}

jspp::AnyValue JsArray::get_property(const std::string &key, const AnyValue &thisVal)
{
    if (!key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) && is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return get_property(idx);
    }
    else
    {
        auto it = props.find(key);
        if (it == props.end())
        {
            if (key == "length")
            {
                auto proto_it = ArrayPrototypes::get(key);
                if (proto_it.has_value())
                {
                    return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
                }
            }

            if (!proto.is_null() && !proto.is_undefined())
            {
                if (proto.has_property(key))
                {
                    return proto.get_property_with_receiver(key, thisVal);
                }
            }

            auto proto_it = ArrayPrototypes::get(key);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
            }
            return Constants::UNDEFINED;
        }
        return AnyValue::resolve_property_for_read(it->second, thisVal, key);
    }
}

jspp::AnyValue JsArray::get_symbol_property(const AnyValue &key, const AnyValue &thisVal)
{
    auto it = symbol_props.find(key);
    if (it == symbol_props.end())
    {
        if (!proto.is_null() && !proto.is_undefined())
        {
            auto res = proto.get_symbol_property_with_receiver(key, thisVal);
            if (!res.is_undefined())
                return res;
        }

        auto proto_it = ArrayPrototypes::get(key);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key.to_std_string());
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key.to_std_string());
}

jspp::AnyValue JsArray::get_property(uint32_t idx)
{
    if (idx < dense.size())
    {
        const auto &val = dense[idx];
        return val.is_uninitialized() ? Constants::UNDEFINED : val;
    }
    const auto &it = sparse.find(idx);
    if (it != sparse.end())
    {
        const auto &val = it->second;
        return val.is_uninitialized() ? Constants::UNDEFINED : val;
    }
    return Constants::UNDEFINED;
}

jspp::AnyValue JsArray::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    if (!key.empty() && std::isdigit(static_cast<unsigned char>(key[0])) && is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return set_property(idx, value);
    }
    else
    {
        auto proto_val_opt = ArrayPrototypes::get(key);

        if (proto_val_opt.has_value())
        {
            auto proto_value = proto_val_opt.value();
            if (proto_value.is_accessor_descriptor())
            {
                return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
            }
            if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable)
            {
                return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
            }
        }

        auto it = props.find(key);
        if (it != props.end())
        {
            return AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
        }
        else
        {
            props[key] = value;
            return value;
        }
    }
}

jspp::AnyValue JsArray::set_symbol_property(const AnyValue &key, const AnyValue &value, const AnyValue &thisVal)
{
    auto it = symbol_props.find(key);
    if (it != symbol_props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, thisVal, value, key.to_std_string());
    }
    else
    {
        symbol_props[key] = value;
        return value;
    }
}

jspp::AnyValue JsArray::set_property(uint32_t idx, const AnyValue &value)
{
    uint64_t newLen = static_cast<uint64_t>(idx) + 1;
    if (newLen > length)
        length = newLen;

    const uint32_t DENSE_GROW_THRESHOLD = 1024;
    if (idx < dense.size())
    {
        dense[idx] = value;
        return value;
    }
    else if (idx <= dense.size() + DENSE_GROW_THRESHOLD)
    {
        dense.resize(idx + 1, Constants::UNINITIALIZED);
        dense[idx] = value;
        return value;
    }
    else
    {
        sparse[idx] = value;
        return value;
    }
}

// --- ArrayPrototypes Implementation ---

namespace ArrayPrototypes {

AnyValue &get_toString_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> _) -> AnyValue
                                                 { return AnyValue::make_string(thisVal.as_array()->to_std_string()); },
                                                 "toString");
    return fn;
}

AnyValue &get_iterator_fn()
{
    static AnyValue fn = AnyValue::make_generator([](AnyValue thisVal, std::vector<AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                  {
                                                      auto arr = thisVal.as_array();
                                                      for (uint64_t idx = 0; idx < arr->length; ++idx)
                                                      {
                                                          co_yield arr->get_property(static_cast<uint32_t>(idx));
                                                      }
                                                      co_return Constants::UNDEFINED; },
                                                  "Symbol.iterator");
    return fn;
}

AnyValue &get_length_desc()
{
    static auto getter = [](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
    {
        return AnyValue::make_number(thisVal.as_array()->length);
    };

    static auto setter = [](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
    {
        if (args.empty())
        {
            return Constants::UNDEFINED;
        }

        auto self = thisVal.as_array();
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

    static AnyValue desc = AnyValue::make_accessor_descriptor(getter,
                                                              setter,
                                                              false,
                                                              false);
    return desc;
}

AnyValue &get_push_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     for (const auto &arg : args)
                                                     {
                                                         self->set_property(static_cast<uint32_t>(self->length), arg);
                                                     }
                                                     return AnyValue::make_number(self->length); },
                                                 "push");
    return fn;
}

AnyValue &get_pop_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     if (self->length == 0)
                                                     {
                                                         return Constants::UNDEFINED;
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
                                                 "pop");
    return fn;
}

AnyValue &get_shift_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     if (self->length == 0)
                                                     {
                                                         return Constants::UNDEFINED;
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
                                                 "shift");
    return fn;
}

AnyValue &get_unshift_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "unshift");
    return fn;
}

AnyValue &get_join_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "join");
    return fn;
}

AnyValue &get_forEach_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                     return Constants::UNDEFINED; },
                                                 "forEach");
    return fn;
}

AnyValue &get_at_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     double len = static_cast<double>(self->length);
                                                     double relativeIndex = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                     double k;
                                                     if (relativeIndex >= 0) k = relativeIndex;
                                                     else k = len + relativeIndex;
                                                     if (k < 0 || k >= len) return Constants::UNDEFINED;
                                                     return self->get_property(static_cast<uint32_t>(k)); },
                                                 "at");
    return fn;
}

AnyValue &get_includes_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     AnyValue searchElement = args.empty() ? Constants::UNDEFINED : args[0];
                                                     double len = static_cast<double>(self->length);
                                                     if (len == 0) return Constants::FALSE;
                                                     double n = (args.size() > 1) ? Operators_Private::ToNumber(args[1]) : 0;
                                                     double k;
                                                     if (n >= 0) k = n;
                                                     else k = len + n;
                                                     if (k < 0) k = 0;

                                                     for (uint64_t i = static_cast<uint64_t>(k); i < self->length; ++i)
                                                     {
                                                         AnyValue element = self->get_property(static_cast<uint32_t>(i));
                                                         // SameValueZero algorithm (includes handles NaN)
                                                         if (element.is_number() && searchElement.is_number() && std::isnan(element.as_double()) && std::isnan(searchElement.as_double())) return Constants::TRUE;
                                                         if (is_strictly_equal_to_primitive(element, searchElement)) return Constants::TRUE;
                                                     }
                                                     return Constants::FALSE; },
                                                 "includes");
    return fn;
}

AnyValue &get_indexOf_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     AnyValue searchElement = args.empty() ? Constants::UNDEFINED : args[0];
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
                                                 "indexOf");
    return fn;
}

AnyValue &get_lastIndexOf_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     AnyValue searchElement = args.empty() ? Constants::UNDEFINED : args[0];
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
                                                 "lastIndexOf");
    return fn;
}

AnyValue &get_find_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                     return Constants::UNDEFINED; },
                                                 "find");
    return fn;
}

AnyValue &get_findIndex_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                                                     auto callback = args[0].as_function();
                                                     auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                                                     
                                                     for (uint64_t i = 0; i < self->length; ++i) {
                                                         AnyValue element = self->get_property(static_cast<uint32_t>(i));
                                                         AnyValue kVal = AnyValue::make_number(i);
                                                         const AnyValue cbArgs[] = {element, kVal, thisVal};
                                                         
                                                         if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                                                             return AnyValue::make_number(i);
                                                         }
                                                     }
                                                     return AnyValue::make_number(-1); },
                                                 "findIndex");
    return fn;
}

AnyValue &get_findLast_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                     return Constants::UNDEFINED; },
                                                 "findLast");
    return fn;
}

AnyValue &get_findLastIndex_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "findLastIndex");
    return fn;
}

AnyValue &get_values_fn()
{
    static AnyValue fn = AnyValue::make_generator([](AnyValue thisVal, std::vector<AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                  {
                                                      auto arr = thisVal.as_array();
                                                      for (uint64_t idx = 0; idx < arr->length; ++idx)
                                                      {
                                                          co_yield arr->get_property(static_cast<uint32_t>(idx));
                                                      }
                                                      co_return Constants::UNDEFINED; },
                                                  "values");
    return fn;
}

AnyValue &get_keys_fn()
{
    static AnyValue fn = AnyValue::make_generator([](AnyValue thisVal, std::vector<AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                  { 
                                                      auto self = thisVal.as_array();
                                                      for (uint64_t i = 0; i < self->length; ++i) {
                                                          co_yield AnyValue::make_number(i);
                                                      }
                                                      co_return Constants::UNDEFINED; },
                                                  "keys");
    return fn;
}

AnyValue &get_entries_fn()
{
    static AnyValue fn = AnyValue::make_generator([](AnyValue thisVal, std::vector<AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                  { 
                                                      auto self = thisVal.as_array();
                                                      for (uint64_t i = 0; i < self->length; ++i) {
                                                          std::vector<AnyValue> entry;
                                                          entry.push_back(AnyValue::make_number(i));
                                                          entry.push_back(self->get_property(static_cast<uint32_t>(i)));
                                                          co_yield AnyValue::make_array(std::move(entry));
                                                      }
                                                      co_return Constants::UNDEFINED; },
                                                  "entries");
    return fn;
}

AnyValue &get_map_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "map");
    return fn;
}

AnyValue &get_filter_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "filter");
    return fn;
}

AnyValue &get_every_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                                                     auto callback = args[0].as_function();
                                                     auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                                                     
                                                     for (uint64_t i = 0; i < self->length; ++i) {
                                                         if (self->has_property(std::to_string(i))) {
                                                             AnyValue val = self->get_property(static_cast<uint32_t>(i));
                                                             AnyValue kVal = AnyValue::make_number(i);
                                                             const AnyValue cbArgs[] = {val, kVal, thisVal};
                                                             if (!is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                                                                 return Constants::FALSE;
                                                             }
                                                         }
                                                     }
                                                     return Constants::TRUE; },
                                                 "every");
    return fn;
}

AnyValue &get_some_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                                                     auto callback = args[0].as_function();
                                                     auto thisArg = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                                                     
                                                     for (uint64_t i = 0; i < self->length; ++i) {
                                                         if (self->has_property(std::to_string(i))) {
                                                             AnyValue val = self->get_property(static_cast<uint32_t>(i));
                                                             AnyValue kVal = AnyValue::make_number(i);
                                                             const AnyValue cbArgs[] = {val, kVal, thisVal};
                                                             if (is_truthy(callback->call(thisArg, std::span<const AnyValue>(cbArgs, 3)))) {
                                                                 return Constants::TRUE;
                                                             }
                                                         }
                                                     }
                                                     return Constants::FALSE; },
                                                 "some");
    return fn;
}

AnyValue &get_reduce_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "reduce");
    return fn;
}

AnyValue &get_reduceRight_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "reduceRight");
    return fn;
}

AnyValue &get_flat_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "flat");
    return fn;
}

AnyValue &get_flatMap_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "flatMap");
    return fn;
}

AnyValue &get_fill_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     AnyValue value = args.empty() ? Constants::UNDEFINED : args[0];
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
                                                 "fill");
    return fn;
}

AnyValue &get_reverse_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                 "reverse");
    return fn;
}

AnyValue &get_sort_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     AnyValue compareFn = args.empty() ? Constants::UNDEFINED : args[0];
                                                     
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
                                                             double res = Operators_Private::ToNumber(compareFn.call(Constants::UNDEFINED, std::span<const AnyValue>(cmpArgs, 2)));
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
                                                 "sort");
    return fn;
}

AnyValue &get_splice_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                         "splice");
    return fn;
}

AnyValue &get_copyWithin_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                         "copyWithin");
    return fn;
}

AnyValue &get_concat_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
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
                                                         auto spreadableSym = AnyValue::from_symbol(WellKnownSymbols::isConcatSpreadable);
                                                         if (item.is_array()) {
                                                             spreadable = true;
                                                             if (item.has_property(spreadableSym)) {
                                                                 spreadable = is_truthy(item.get_own_property(spreadableSym));
                                                             }
                                                         } else if (item.is_object()) {
                                                              if (item.has_property(spreadableSym)) {
                                                                 spreadable = is_truthy(item.get_own_property(spreadableSym));
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
                                                         "concat");
    return fn;
}

AnyValue &get_slice_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     double len = static_cast<double>(self->length);
                                                     double start = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                     double actualStart = (start < 0) ? std::max(len + start, 0.0) : std::min(start, len);
                                                     double end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToNumber(args[1]);
                                                     double actualEnd = (end < 0) ? std::max(len + end, 0.0) : std::min(end, len);
                                                     
                                                     std::vector<AnyValue> result;
                                                     for (uint64_t i = static_cast<uint64_t>(actualStart); i < static_cast<uint64_t>(actualEnd); ++i) {
                                                         if (self->has_property(std::to_string(i))) {
                                                             result.push_back(self->get_property(static_cast<uint32_t>(i)));
                                                         } else {
                                                             result.push_back(Constants::UNINITIALIZED);
                                                         }
                                                     }
                                                     return AnyValue::make_array(std::move(result)); },
                                                         "slice");
    return fn;
}

AnyValue &get_toReversed_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto copy = thisVal.get_property_with_receiver("slice", thisVal).call(thisVal, {});
                                                     copy.get_own_property("reverse").call(copy, {});
                                                     return copy; },
                                                         "toReversed");
    return fn;
}

AnyValue &get_toSorted_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto copy = thisVal.get_property_with_receiver("slice", thisVal).call(thisVal, {});
                                                     copy.get_own_property("sort").call(copy, args);
                                                     return copy; },
                                                         "toSorted");
    return fn;
}

AnyValue &get_toSpliced_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto copy = thisVal.get_property_with_receiver("slice", thisVal).call(thisVal, {});
                                                     copy.get_own_property("splice").call(copy, args);
                                                     return copy; },
                                                         "toSpliced");
    return fn;
}

AnyValue &get_with_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     auto copy = thisVal.get_property_with_receiver("slice", thisVal).call(thisVal, {});
                                                     
                                                     double len = static_cast<double>(self->length);
                                                     double idx = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                     double k;
                                                     if (idx >= 0) k = idx; else k = len + idx;
                                                     
                                                     if (k < 0 || k >= len) throw Exception::make_exception("Invalid index", "RangeError");
                                                     
                                                     AnyValue value = (args.size() > 1) ? args[1] : Constants::UNDEFINED;
                                                     copy.set_own_property(static_cast<uint32_t>(k), value);
                                                     return copy; },
                                                         "with");
    return fn;
}

AnyValue &get_toLocaleString_fn()
{
    static AnyValue fn = AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_array();
                                                     std::string result = "";
                                                     for (uint64_t i = 0; i < self->length; ++i) {
                                                         if (i > 0) result += ",";
                                                         AnyValue element = self->get_property(static_cast<uint32_t>(i));
                                                         if (!element.is_null() && !element.is_undefined()) {
                                                             if (element.has_property("toLocaleString")) {
                                                                 auto fn = element.get_property_with_receiver("toLocaleString", element);
                                                                 if (fn.is_function()) {
                                                                     result += fn.call(element, {}).to_std_string();
                                                                     continue;
                                                                 }
                                                             }
                                                             result += element.to_std_string();
                                                         }
                                                     }
                                                     return AnyValue::make_string(result); },
                                                         "toLocaleString");
    return fn;
}

std::optional<AnyValue> get(const std::string &key)
{
    if (key == "toString") return get_toString_fn();
    if (key == "length") return get_length_desc();
    if (key == "push") return get_push_fn();
    if (key == "pop") return get_pop_fn();
    if (key == "shift") return get_shift_fn();
    if (key == "unshift") return get_unshift_fn();
    if (key == "join") return get_join_fn();
    if (key == "forEach") return get_forEach_fn();
    if (key == "at") return get_at_fn();
    if (key == "includes") return get_includes_fn();
    if (key == "indexOf") return get_indexOf_fn();
    if (key == "lastIndexOf") return get_lastIndexOf_fn();
    if (key == "find") return get_find_fn();
    if (key == "findIndex") return get_findIndex_fn();
    if (key == "findLast") return get_findLast_fn();
    if (key == "findLastIndex") return get_findLastIndex_fn();
    if (key == "values") return get_values_fn();
    if (key == "keys") return get_keys_fn();
    if (key == "entries") return get_entries_fn();
    if (key == "map") return get_map_fn();
    if (key == "filter") return get_filter_fn();
    if (key == "every") return get_every_fn();
    if (key == "some") return get_some_fn();
    if (key == "reduce") return get_reduce_fn();
    if (key == "reduceRight") return get_reduceRight_fn();
    if (key == "flat") return get_flat_fn();
    if (key == "flatMap") return get_flatMap_fn();
    if (key == "fill") return get_fill_fn();
    if (key == "reverse") return get_reverse_fn();
    if (key == "sort") return get_sort_fn();
    if (key == "splice") return get_splice_fn();
    if (key == "copyWithin") return get_copyWithin_fn();
    if (key == "concat") return get_concat_fn();
    if (key == "slice") return get_slice_fn();
    if (key == "toReversed") return get_toReversed_fn();
    if (key == "toSorted") return get_toSorted_fn();
    if (key == "toSpliced") return get_toSpliced_fn();
    if (key == "with") return get_with_fn();
    if (key == "toLocaleString") return get_toLocaleString_fn();
    return std::nullopt;
}

std::optional<AnyValue> get(const AnyValue &key)
{
    if (key.is_string())
        return get(key.as_string()->value);

    auto toStringTagSym = AnyValue::from_symbol(WellKnownSymbols::toStringTag);
    if (key == toStringTagSym) return get_toString_fn();

    auto iteratorSym = AnyValue::from_symbol(WellKnownSymbols::iterator);
    if (key == iteratorSym) return get_iterator_fn();

    return std::nullopt;
}

} // namespace ArrayPrototypes

} // namespace jspp
