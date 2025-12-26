#pragma once

#include "types.hpp"
#include <variant> // Ensure variant is included

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    using JsFunctionCallable = std::variant<std::function<AnyValue(const AnyValue &, const std::vector<AnyValue> &)>,                                // 0: Normal
                                            std::function<jspp::JsIterator<jspp::AnyValue>(const AnyValue &, const std::vector<jspp::AnyValue> &)>, // 1: Generator
                                            std::function<jspp::JsPromise(const AnyValue &, const std::vector<jspp::AnyValue> &)>>;                  // 2: Async

    struct JsFunction
    {
        JsFunctionCallable callable;
        std::string name;
        std::unordered_map<std::string, AnyValue> props;
        std::shared_ptr<AnyValue> proto = nullptr;
        bool is_generator;
        bool is_async;
        bool is_class;

        // ---- Constructor A: infer flags ----
        JsFunction(const JsFunctionCallable &c,
                   std::string n = {},
                   std::unordered_map<std::string, AnyValue> p = {},
                   bool is_cls = false)
            : callable(c),
              name(std::move(n)),
              props(std::move(p)),
              is_generator(callable.index() == 1),
              is_async(callable.index() == 2),
              is_class(is_cls)
        {
        }

        // ---- Constructor B: explicit generator flag (backward compat) ----
        JsFunction(const JsFunctionCallable &c,
                   bool is_gen,
                   std::string n = {},
                   std::unordered_map<std::string, AnyValue> p = {},
                   bool is_cls = false)
            : callable(c),
              name(std::move(n)),
              props(std::move(p)),
              is_generator(is_gen),
              is_async(callable.index() == 2),
              is_class(is_cls)
        {
        }
        
         // ---- Constructor C: explicit async flag ----
        JsFunction(const JsFunctionCallable &c,
                   bool is_gen,
                   bool is_async_func,
                   std::string n = {},
                   std::unordered_map<std::string, AnyValue> p = {},
                   bool is_cls = false)
            : callable(c),
              name(std::move(n)),
              props(std::move(p)),
              is_generator(is_gen),
              is_async(is_async_func),
              is_class(is_cls)
        {
        }

        std::string to_std_string() const;
        AnyValue call(const AnyValue &thisVal, const std::vector<AnyValue> &args);

        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
    };
}