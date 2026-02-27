#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
  // Forward declaration of AnyValue
  class AnyValue;

  struct JsFunction : HeapObject
  {
    JsFunctionCallable callable;
    std::optional<std::string> name;
    std::unordered_map<std::string, AnyValue> props;
    std::map<AnyValue, AnyValue> symbol_props;
    AnyValue proto;
    bool is_generator;
    bool is_async;
    bool is_class;
    bool is_constructor;

    // ---- Constructor A: infer flags ----
    JsFunction(const JsFunctionCallable &c,
               std::optional<std::string> n = std::nullopt,
               std::unordered_map<std::string, AnyValue> p = {},
               std::map<AnyValue, AnyValue> sp = {},
               bool is_cls = false,
               bool is_ctor = true);

    // ---- Constructor B: explicit generator flag (backward compat) ----
    JsFunction(const JsFunctionCallable &c,
               bool is_gen,
               std::optional<std::string> n = std::nullopt,
               std::unordered_map<std::string, AnyValue> p = {},
               std::map<AnyValue, AnyValue> sp = {},
               bool is_cls = false,
               bool is_ctor = true);

    // ---- Constructor C: explicit async flag ----
    JsFunction(const JsFunctionCallable &c,
               bool is_gen,
               bool is_async_func,
               std::optional<std::string> n = std::nullopt,
               std::unordered_map<std::string, AnyValue> p = {},
               std::map<AnyValue, AnyValue> sp = {},
               bool is_cls = false,
               bool is_ctor = true);

    JsType get_heap_type() const override { return JsType::Function; }

    std::string to_std_string() const;
    AnyValue call(AnyValue thisVal, std::span<const AnyValue> args);

    bool has_property(const std::string &key) const;
    bool has_symbol_property(const AnyValue &key) const;
    AnyValue get_property(const std::string &key, AnyValue thisVal);
    AnyValue get_symbol_property(const AnyValue &key, AnyValue thisVal);
    AnyValue set_property(const std::string &key, AnyValue value, AnyValue thisVal);
    AnyValue set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal);
  };
}
