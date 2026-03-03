#pragma once

#include "types.hpp"
#include "values/iterator.hpp"
#include "any_value.hpp"
#include "values/prototypes/iterator.hpp"

namespace jspp {

template <typename T>
JsIterator<T>::JsIterator(handle_type h) : handle(h) {}

template <typename T>
JsIterator<T>::JsIterator(JsIterator &&other) noexcept 
    : handle(std::exchange(other.handle, nullptr)),
      props(std::move(other.props)),
      symbol_props(std::move(other.symbol_props)) {}

template <typename T>
JsIterator<T>::~JsIterator() { if (handle) handle.destroy(); }

template <typename T>
JsIterator<T> JsIterator<T>::promise_type::get_return_object() {
    return JsIterator{std::coroutine_handle<promise_type>::from_promise(*this)};
}

template <typename T>
std::suspend_always JsIterator<T>::promise_type::initial_suspend() noexcept { return {}; }

template <typename T>
std::suspend_always JsIterator<T>::promise_type::final_suspend() noexcept { return {}; }

template <typename T>
void JsIterator<T>::promise_type::unhandled_exception() {
    try {
        throw;
    } catch (const GeneratorReturnException&) {
    } catch (...) {
        exception_ = std::current_exception();
    }
}

template <typename T>
std::string JsIterator<T>::to_std_string() const { return "[object Generator]"; }

template <typename T>
typename JsIterator<T>::NextResult JsIterator<T>::next(const T &val)
{
    if (!handle || handle.done()) return {std::nullopt, true};
    handle.promise().input_value = val;
    handle.resume();
    if (handle.promise().exception_) std::rethrow_exception(handle.promise().exception_);
    bool is_done = handle.done();
    return {handle.promise().current_value, is_done};
}

template <typename T>
typename JsIterator<T>::NextResult JsIterator<T>::return_(const T &val)
{
    if (!handle || handle.done()) return {val, true};
    handle.promise().pending_return = true;
    handle.promise().current_value = val;
    handle.resume();
    if (handle.promise().exception_) std::rethrow_exception(handle.promise().exception_);
    return {handle.promise().current_value, true};
}

template <typename T>
typename JsIterator<T>::NextResult JsIterator<T>::throw_(const AnyValue &err)
{
    if (!handle || handle.done()) throw Exception(err);
    handle.promise().pending_exception = std::make_exception_ptr(Exception(err));
    handle.resume();
    if (handle.promise().exception_) std::rethrow_exception(handle.promise().exception_);
    bool is_done = handle.done();
    return {handle.promise().current_value, is_done};
}

template <typename T>
std::vector<T> JsIterator<T>::to_vector()
{
    std::vector<T> result;
    while (true) {
        auto next_res = this->next();
        if (next_res.done) break;
        result.push_back(next_res.value.value_or(Constants::UNDEFINED));
    }
    return result;
}

template <typename T>
bool JsIterator<T>::has_symbol_property(const AnyValue &key) const { return symbol_props.count(key) > 0; }

template <typename T>
AnyValue JsIterator<T>::get_property(const std::string &key, AnyValue thisVal)
{
    auto it = props.find(key);
    if (it == props.end()) {
        if constexpr (std::is_same_v<T, AnyValue>) {
            auto proto_it = IteratorPrototypes::get(key);
            if (proto_it.has_value()) return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

template <typename T>
AnyValue JsIterator<T>::get_symbol_property(const AnyValue &key, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it == symbol_props.end()) {
        if constexpr (std::is_same_v<T, AnyValue>) {
            auto proto_it = IteratorPrototypes::get(key);
            if (proto_it.has_value()) return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key.to_std_string());
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key.to_std_string());
}

template <typename T>
AnyValue JsIterator<T>::set_property(const std::string &key, AnyValue value, AnyValue thisVal)
{
    if constexpr (std::is_same_v<T, AnyValue>) {
        auto proto_it = IteratorPrototypes::get(key);
        if (proto_it.has_value()) {
            auto proto_value = proto_it.value();
            if (proto_value.is_accessor_descriptor()) return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
            if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable) return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
    }
    auto it = props.find(key);
    if (it != props.end()) return AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
    else { props[key] = value; return value; }
}

template <typename T>
AnyValue JsIterator<T>::set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it != symbol_props.end()) return AnyValue::resolve_property_for_write(it->second, thisVal, value, key.to_std_string());
    else { symbol_props[key] = value; return value; }
}

}
