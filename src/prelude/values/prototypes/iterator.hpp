#pragma once

#include "types.hpp"
#include "values/iterator.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"

namespace jspp
{
    namespace IteratorPrototypes
    {
        inline AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_string(thisVal.as_iterator()->to_std_string()); },
                                                         "toString");
            return fn;
        }

        inline AnyValue &get_iterator_fn()
        {
            static AnyValue fn = AnyValue::make_generator([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                          { return thisVal; },
                                                          "Symbol.iterator");
            return fn;
        }

        inline AnyValue &get_next_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                             auto res = thisVal.as_iterator()->next(val);
                                                             return AnyValue::make_object({{"value", res.value.value_or(Constants::UNDEFINED)}, {"done", AnyValue::make_boolean(res.done)}}); },
                                                         "next");
            return fn;
        }

        inline AnyValue &get_return_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                             auto res = thisVal.as_iterator()->return_(val);
                                                             return AnyValue::make_object({{"value", res.value.value_or(Constants::UNDEFINED)}, {"done", AnyValue::make_boolean(res.done)}}); },
                                                         "return");
            return fn;
        }

        inline AnyValue &get_throw_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue err = args.empty() ? Constants::UNDEFINED : args[0];
                                                             auto res = thisVal.as_iterator()->throw_(err);
                                                             return AnyValue::make_object({{"value", res.value.value_or(Constants::UNDEFINED)}, {"done", AnyValue::make_boolean(res.done)}}); },
                                                         "throw");
            return fn;
        }

        inline AnyValue &get_toArray_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_array(thisVal.as_iterator()->to_vector()); },
                                                         "drop");
            return fn;
        }

        inline AnyValue &get_drop_fn()
        {
            static AnyValue fn = AnyValue::make_generator([](const AnyValue &thisVal, std::span<const AnyValue> args) -> JsIterator<AnyValue>
                                                          { 
                                                            auto self = thisVal.as_iterator();
                                                            size_t skip_count = 0;
                                                            if (!args.empty()) {
                                                                skip_count = static_cast<size_t>(args[0].as_double());
                                                            }
                                                            size_t skipped = 0;
                                                            while (true)
                                                            {
                                                                auto next_res = self->next();
                                                                if (next_res.done) { break; }
                                                                if (skipped < skip_count)
                                                                {
                                                                    skipped++;
                                                                    continue;
                                                                }
                                                                co_yield next_res.value.value_or(Constants::UNDEFINED);
                                                            }
                                                            co_return jspp::Constants::UNDEFINED; },
                                                          "drop");
            return fn;
        }

        inline AnyValue &get_take_fn()
        {
            static AnyValue fn = AnyValue::make_generator([](const AnyValue &thisVal, std::span<const AnyValue> args) -> JsIterator<AnyValue>
                                                          { 
                                                            auto self = thisVal.as_iterator();
                                                            size_t take_count = 0;
                                                            if (!args.empty()) {
                                                                take_count = static_cast<size_t>(args[0].as_double());
                                                            }
                                                            size_t taken = 0;
                                                            while (true)
                                                            {
                                                                auto next_res = self->next();
                                                                if (next_res.done) { break; }
                                                                if (taken < take_count)
                                                                {
                                                                    taken++;
                                                                    co_yield next_res.value.value_or(Constants::UNDEFINED);
                                                                }
                                                                if (taken >= take_count) 
                                                                { 
                                                                    // Call the iterator's return() method for early cleanup of resources
                                                                    self->return_();
                                                                    break; 
                                                                }
                                                            }
                                                            co_return jspp::Constants::UNDEFINED; },
                                                          "take");
            return fn;
        }

        inline AnyValue &get_some_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         { 
                                                            auto self = thisVal.as_iterator();
                                                            if (args.empty() || !args[0].is_function()) throw Exception::make_exception("callback is not a function", "TypeError");
                                                            auto callback = args[0].as_function();
                                                            while (true)
                                                            {
                                                                auto next_res = self->next();
                                                                if (next_res.done) { break; }
                                                                if (is_truthy(callback->call(thisVal, std::span<const AnyValue>((const jspp::AnyValue[]){next_res.value.value_or(Constants::UNDEFINED)}, 1))))
                                                                {
                                                                    // Call the iterator's return() method for early cleanup of resources
                                                                    self->return_();
                                                                    return Constants::TRUE;
                                                                }
                                                            }
                                                            return jspp::Constants::FALSE; },
                                                         "some");
            return fn;
        }

        inline std::optional<AnyValue> get(const std::string &key)
        {
            // --- toString() method ---
            if (key == "toString" || key == WellKnownSymbols::toStringTag->key)
            {
                return get_toString_fn();
            }
            // --- [Symbol.iterator]() method ---
            if (key == WellKnownSymbols::iterator->key)
            {
                return get_iterator_fn();
            }
            // --- next() method ---
            if (key == "next")
            {
                return get_next_fn();
            }
            // --- return() method ---
            if (key == "return")
            {
                return get_return_fn();
            }
            // --- throw() method ---
            if (key == "throw")
            {
                return get_throw_fn();
            }
            // --- toArray() method ---
            if (key == "toArray")
            {
                return get_toArray_fn();
            }
            // --- drop() method ---
            if (key == "drop")
            {
                return get_drop_fn();
            }
            // --- take() method ---
            if (key == "take")
            {
                return get_take_fn();
            }
            // --- some() method ---
            if (key == "some")
            {
                return get_some_fn();
            }

            return std::nullopt;
        }
    }
}
