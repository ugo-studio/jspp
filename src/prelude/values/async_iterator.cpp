#include "jspp.hpp"
#include "values/async_iterator.hpp"
#include "values/prototypes/async_iterator.hpp"

namespace jspp {

template <typename T>
JsAsyncIterator<T>::JsAsyncIterator(handle_type h) : handle(h) {}

template <typename T>
JsAsyncIterator<T>::JsAsyncIterator(JsAsyncIterator &&other) noexcept 
    : handle(std::exchange(other.handle, nullptr)),
      props(std::move(other.props)),
      symbol_props(std::move(other.symbol_props)) {}

template <typename T>
JsAsyncIterator<T>::~JsAsyncIterator() { if (handle) handle.destroy(); }

template <typename T>
JsAsyncIterator<T> JsAsyncIterator<T>::promise_type::get_return_object() {
    return JsAsyncIterator{std::coroutine_handle<promise_type>::from_promise(*this)};
}

template <typename T>
std::suspend_always JsAsyncIterator<T>::promise_type::initial_suspend() noexcept { return {}; }

template <typename T>
std::suspend_always JsAsyncIterator<T>::promise_type::final_suspend() noexcept { return {}; }

template <typename T>
bool JsAsyncIterator<T>::promise_type::YieldAwaiter::await_ready() { return false; }

template <typename T>
void JsAsyncIterator<T>::promise_type::YieldAwaiter::await_suspend(std::coroutine_handle<promise_type> h) {}

template <typename T>
T JsAsyncIterator<T>::promise_type::YieldAwaiter::await_resume() { return p.current_input; }

template <typename T>
void JsAsyncIterator<T>::promise_type::unhandled_exception() {
    try {
        std::rethrow_exception(std::current_exception());
    } catch (const Exception &e) {
        fail_all(e.data);
    } catch (const std::exception &e) {
        fail_all(AnyValue::make_string(e.what()));
    } catch (...) {
        fail_all(AnyValue::make_string("Unknown error in async generator"));
    }
}

template <typename T>
bool JsAsyncIterator<T>::promise_type::AsyncIterAwaiter::await_ready() { return base_awaiter.await_ready(); }

template <typename T>
void JsAsyncIterator<T>::promise_type::AsyncIterAwaiter::await_suspend(std::coroutine_handle<promise_type> h) {
    if (!base_awaiter.value.is_promise()) {
        jspp::Scheduler::instance().enqueue([h]() mutable {
            auto &pr = h.promise();
            pr.is_awaiting = false;
            pr.is_running = true;
            h.resume();
            pr.is_running = false;
            if (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty()) {
                while (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty()) {
                    pr.is_running = true;
                    pr.current_input = pr.pending_calls.front().second;
                    h.resume();
                    pr.is_running = false;
                }
            }
        });
        return;
    }
    auto p = base_awaiter.value.as_promise();
    p->then(
        [h](AnyValue v) mutable {
            auto &pr = h.promise();
            pr.is_awaiting = false;
            pr.is_running = true;
            h.resume();
            pr.is_running = false;
            if (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty()) {
                while (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty()) {
                    pr.is_running = true;
                    pr.current_input = pr.pending_calls.front().second;
                    h.resume();
                    pr.is_running = false;
                }
            }
        },
        [h](AnyValue e) mutable {
            auto &pr = h.promise();
            pr.is_awaiting = false;
            pr.is_running = true;
            h.resume();
            pr.is_running = false;
        }
    );
}

template <typename T>
AnyValue JsAsyncIterator<T>::promise_type::AsyncIterAwaiter::await_resume() { return base_awaiter.await_resume(); }

template <typename T>
typename JsAsyncIterator<T>::promise_type::AsyncIterAwaiter JsAsyncIterator<T>::promise_type::await_transform(AnyValue value) {
    is_awaiting = true;
    return AsyncIterAwaiter{AnyValueAwaiter{std::move(value)}, *this};
}

template <typename T>
std::string JsAsyncIterator<T>::to_std_string() const { return "[object AsyncGenerator]"; }

template <typename T>
void JsAsyncIterator<T>::resume_next()
{
    if (!handle || handle.done()) return;
    auto &p = handle.promise();
    if (p.is_awaiting || p.is_running) return;
    if (p.pending_calls.empty()) return;
    p.is_running = true;
    auto &next_call = p.pending_calls.front();
    p.current_input = next_call.second;
    handle.resume();
    p.is_running = false;
    if (!p.pending_calls.empty() && !p.is_awaiting && !handle.done()) {
        this->ref();
        Scheduler::instance().enqueue([this]() { this->resume_next(); this->deref(); });
    }
}

template <typename T>
JsPromise JsAsyncIterator<T>::next(const T &val)
{
    JsPromise p;
    if (handle) {
        if (handle.done()) p.resolve(AnyValue::make_object({{"value", Constants::UNDEFINED}, {"done", Constants::TRUE}}));
        else { handle.promise().pending_calls.push({p, val}); resume_next(); }
    } else p.resolve(AnyValue::make_object({{"value", Constants::UNDEFINED}, {"done", Constants::TRUE}}));
    return p;
}

template <typename T>
bool JsAsyncIterator<T>::has_symbol_property(const AnyValue &key) const { return symbol_props.count(key) > 0; }

template <typename T>
AnyValue JsAsyncIterator<T>::get_property(const std::string &key, AnyValue thisVal)
{
    auto it = props.find(key);
    if (it == props.end()) {
        if constexpr (std::is_same_v<T, AnyValue>) {
            auto proto_it = AsyncIteratorPrototypes::get(key);
            if (proto_it.has_value()) return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

template <typename T>
AnyValue JsAsyncIterator<T>::get_symbol_property(const AnyValue &key, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it == symbol_props.end()) {
        if constexpr (std::is_same_v<T, AnyValue>) {
            auto proto_it = AsyncIteratorPrototypes::get(key);
            if (proto_it.has_value()) return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key.to_std_string());
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key.to_std_string());
}

template <typename T>
AnyValue JsAsyncIterator<T>::set_property(const std::string &key, AnyValue value, AnyValue thisVal)
{
    if constexpr (std::is_same_v<T, AnyValue>) {
        auto proto_it = AsyncIteratorPrototypes::get(key);
        if (proto_it.has_value()) {
            auto proto_value = proto_it.value();
            if (proto_value.is_accessor_descriptor()) return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
            if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable) return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
        }
    }
    auto it = props.find(key);
    if (it != props.end()) return jspp::AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
    else { props[key] = value; return value; }
}

template <typename T>
AnyValue JsAsyncIterator<T>::set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it != symbol_props.end()) return AnyValue::resolve_property_for_write(it->second, thisVal, value, key.to_std_string());
    else { symbol_props[key] = value; return value; }
}

// Explicit template instantiation
template class JsAsyncIterator<AnyValue>;

namespace AsyncIteratorPrototypes {

AnyValue &get_toString_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                 { return AnyValue::make_string(thisVal.as_async_iterator()->to_std_string()); },
                                                 "toString");
    return fn;
}

AnyValue &get_asyncIterator_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                 { return thisVal; },
                                                 "Symbol.asyncIterator");
    return fn;
}

AnyValue &get_next_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                     auto res = thisVal.as_async_iterator()->next(val);
                                                     return AnyValue::make_promise(res); },
                                                 "next");
    return fn;
}

std::optional<AnyValue> get(const std::string &key)
{
    if (key == "toString") return get_toString_fn();
    if (key == "next") return get_next_fn();
    return std::nullopt;
}

std::optional<AnyValue> get(const AnyValue &key)
{
    if (key.is_string())
        return get(key.as_string()->value);

    if (key == AnyValue::from_symbol(WellKnownSymbols::toStringTag)) return get_toString_fn();
    if (key == AnyValue::from_symbol(WellKnownSymbols::asyncIterator)) return get_asyncIterator_fn();
    if (key == "next") return get_next_fn();

    return std::nullopt;
}

} // namespace AsyncIteratorPrototypes

} // namespace jspp
