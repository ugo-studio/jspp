#pragma once

#include "types.hpp"

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    using JsFunctionCallable = std::variant<std::function<AnyValue(const AnyValue &, const std::vector<AnyValue> &)>,                                // 0: Normal
                                            std::function<jspp::JsIterator<jspp::AnyValue>(const AnyValue &, const std::vector<jspp::AnyValue> &)>>; // 1: Generator

    struct JsFunction
    {
        JsFunctionCallable callable;
        std::string name;
        std::unordered_map<std::string, AnyValue> props;
        std::shared_ptr<AnyValue> proto = nullptr;
        bool is_generator;
        bool is_class;

        // ---- Constructor A: infer is_generator using index() ----
        JsFunction(const JsFunctionCallable &c,
                   std::string n = {},
                   std::unordered_map<std::string, AnyValue> p = {},
                   bool is_cls = false)
            : callable(c),
              name(std::move(n)),
              props(std::move(p)),
              is_generator(callable.index() == 1), // 1 = generator
              is_class(is_cls)
        {
        }

        // ---- Constructor B: explicitly set is_generator ----
        JsFunction(const JsFunctionCallable &c,
                   bool is_gen,
                   std::string n = {},
                   std::unordered_map<std::string, AnyValue> p = {},
                   bool is_cls = false)
            : callable(c),
              name(std::move(n)),
              props(std::move(p)),
              is_generator(is_gen),
              is_class(is_cls)
        {
            // Optional debug check (no RTTI, no variant visitation):
            // if (callable.index() == 1 && !is_gen) { ... }
            // if (callable.index() == 0 && is_gen) { ... }
        }

        std::string to_std_string() const;
        AnyValue call(const AnyValue &thisVal, const std::vector<AnyValue> &args);

        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
    };
}
