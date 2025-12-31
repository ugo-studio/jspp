#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/operators.hpp"
#include "utils/access.hpp"

inline auto Array = jspp::AnyValue::make_class([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                               {
    if (args.size() == 1 && args[0].is_number()) {
        double len = args[0].as_double();
        if (len < 0 || len > 4294967295.0) { // Max uint32
             throw jspp::Exception::make_exception("Invalid array length", "RangeError");
        }
        auto arr = jspp::AnyValue::make_array(std::vector<jspp::AnyValue>());
        arr.as_array()->length = static_cast<uint64_t>(len);
        arr.as_array()->dense.resize(static_cast<size_t>(len), jspp::AnyValue::make_uninitialized());
        return arr;
    }
    std::vector<jspp::AnyValue> elements;
    for(const auto& arg : args) {
        elements.push_back(arg);
    }
    return jspp::AnyValue::make_array(std::move(elements)); }, "Array");

struct ArrayInit
{
    ArrayInit()
    {
        // Set Array.prototype.proto to Object.prototype
        // Array.get_own_property("prototype").set_prototype(::Object.get_own_property("prototype"));

        // Array.isArray(value)
        Array.define_data_property("isArray", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                            {
            if (args.empty()) return jspp::Constants::FALSE;
            return jspp::AnyValue::make_boolean(args[0].is_array()); }, "isArray"));

        // Array.of(...elements)
        Array.define_data_property("of", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                       {
            std::vector<jspp::AnyValue> elements;
            for(const auto& arg : args) {
                elements.push_back(arg);
            }
            return jspp::AnyValue::make_array(std::move(elements)); }, "of"));

        // Array.from(arrayLike, mapFn?, thisArg?)
        Array.define_data_property("from", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                         {
            if (args.empty() || args[0].is_null() || args[0].is_undefined()) {
                throw jspp::Exception::make_exception("Array.from requires an array-like object", "TypeError");
            }

            const auto& items = args[0];
            const auto& mapFn = (args.size() > 1 && args[1].is_function()) ? args[1] : jspp::Constants::UNDEFINED;
            const auto& thisArg = (args.size() > 2) ? args[2] : jspp::Constants::UNDEFINED;

            std::vector<jspp::AnyValue> result;

            // Check if iterable
            // Simple check: does it have [Symbol.iterator]?
            auto iteratorSym = jspp::WellKnownSymbols::iterator;
            if (items.has_property(iteratorSym->key)) {
                auto iter = jspp::Access::get_object_value_iterator(items, "Array.from source");
                auto nextFn = iter.get_own_property("next").as_function();
                
                size_t k = 0;
                while (true) {
                    auto nextRes = nextFn->call(iter, std::span<const jspp::AnyValue>{});
                    if (jspp::is_truthy(nextRes.get_own_property("done"))) break;
                    
                    auto val = nextRes.get_own_property("value");
                    if (mapFn.is_function()) {
                        jspp::AnyValue kVal = jspp::AnyValue::make_number(k);
                        const jspp::AnyValue mapArgs[] = {val, kVal};
                        val = mapFn.as_function()->call(thisArg, std::span<const jspp::AnyValue>(mapArgs, 2));
                    }
                    result.push_back(val);
                    k++;
                }
            } else {
                // Array-like (length property)
                auto lenVal = items.get_property_with_receiver("length", items);
                size_t len = static_cast<size_t>(jspp::Operators_Private::ToUint32(lenVal));
                
                for (size_t k = 0; k < len; ++k) {
                    auto kVal = items.get_property_with_receiver(std::to_string(k), items);
                    if (mapFn.is_function()) {
                        jspp::AnyValue kNum = jspp::AnyValue::make_number(k);
                        const jspp::AnyValue mapArgs[] = {kVal, kNum};
                        kVal = mapFn.as_function()->call(thisArg, std::span<const jspp::AnyValue>(mapArgs, 2));
                    }
                    result.push_back(kVal);
                }
            }
            
            return jspp::AnyValue::make_array(std::move(result)); }, "from"));
    }
};
