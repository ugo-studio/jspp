#pragma once

#include "types.hpp"
#include <variant>
#include <optional>

namespace jspp
{
  // Forward declaration of AnyValue
  class AnyValue;

  using JsFunctionCallable = std::variant<std::function<AnyValue(const AnyValue &, std::span<const AnyValue>)>,                               // 0: Normal
                                          std::function<jspp::JsIterator<jspp::AnyValue>(const AnyValue &, std::span<const AnyValue>)>, // 1: Generator
                                          std::function<jspp::JsPromise(const AnyValue &, std::span<const AnyValue>)>>;                 // 2: Async

  struct JsFunction
  {
    JsFunctionCallable callable;
    std::optional<std::string> name;
    std::unordered_map<std::string, AnyValue> props;
    std::shared_ptr<AnyValue> proto = nullptr;
    bool is_generator;
    bool is_async;
    bool is_class;
    bool is_constructor;

    // ---- Constructor A: infer flags ----
    JsFunction(const JsFunctionCallable &c,
               std::optional<std::string> n = std::nullopt,
               std::unordered_map<std::string, AnyValue> p = {},
               bool is_cls = false,
               bool is_ctor = true)
        : callable(c),
          name(std::move(n)),
          props(std::move(p)),
          is_generator(callable.index() == 1),
          is_async(callable.index() == 2),
          is_class(is_cls),
          is_constructor(is_ctor && !is_generator && !is_async) // Generators and asyncs are never constructors
    {
    }

    // ---- Constructor B: explicit generator flag (backward compat) ----
    JsFunction(const JsFunctionCallable &c,
               bool is_gen,
               std::optional<std::string> n = std::nullopt,
               std::unordered_map<std::string, AnyValue> p = {},
               bool is_cls = false,
               bool is_ctor = true)
        : callable(c),
          name(std::move(n)),
          props(std::move(p)),
          is_generator(is_gen),
          is_async(callable.index() == 2),
          is_class(is_cls),
          is_constructor(is_ctor && !is_gen && !is_async)
    {
    }

    // ---- Constructor C: explicit async flag ----
    JsFunction(const JsFunctionCallable &c,
               bool is_gen,
               bool is_async_func,
               std::optional<std::string> n = std::nullopt,
               std::unordered_map<std::string, AnyValue> p = {},
               bool is_cls = false,
               bool is_ctor = true)
        : callable(c),
          name(std::move(n)),
          props(std::move(p)),
          is_generator(is_gen),
          is_async(is_async_func),
          is_class(is_cls),
          is_constructor(is_ctor && !is_gen && !is_async_func)
    {
    }

    std::string to_std_string() const;
    AnyValue call(const AnyValue &thisVal, std::span<const AnyValue> args);

    bool has_property(const std::string &key) const;
    AnyValue get_property(const std::string &key, const AnyValue &thisVal);
    AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
  };
}