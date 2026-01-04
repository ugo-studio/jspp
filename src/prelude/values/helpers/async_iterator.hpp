#pragma once

#include "types.hpp"
#include "values/async_iterator.hpp"
#include "any_value.hpp"
#include "values/prototypes/async_iterator.hpp"

// --- JsAsyncIterator methods ---

template <typename T>
std::string jspp::JsAsyncIterator<T>::to_std_string() const
{
    return "[object AsyncGenerator]";
}

template <typename T>
jspp::AnyValue jspp::JsAsyncIterator<T>::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        if constexpr (std::is_same_v<T, AnyValue>)
        {
            auto proto_it = AsyncIteratorPrototypes::get(key, this);
            if (proto_it.has_value())
            {
                return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
            }
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key);
}

template <typename T>
jspp::AnyValue jspp::JsAsyncIterator<T>::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
{
    if constexpr (std::is_same_v<T, AnyValue>)
    {
        auto proto_it = AsyncIteratorPrototypes::get(key, this);
        if (proto_it.has_value())
        {
            auto proto_value = proto_it.value();
            if (proto_value.is_accessor_descriptor())
            {
                return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
            }
            if (proto_value.is_data_descriptor() && !proto_value.as_data_descriptor()->writable)
            {
                return AnyValue::resolve_property_for_write(proto_value, thisVal, value, key);
            }
        }
    }

    auto it = props.find(key);
    if (it != props.end())
    {
        return jspp::AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
    }
    else
    {
        props[key] = value;
        return value;
    }
}

template <typename T>
void jspp::JsAsyncIterator<T>::resume_next()
{
    if (!handle || handle.done())
        return;
    auto &p = handle.promise();
    if (p.is_awaiting || p.is_running)
        return;
    if (p.pending_calls.empty())
        return;

    p.is_running = true;

    auto &next_call = p.pending_calls.front();
    p.current_input = next_call.second;

    handle.resume();

    p.is_running = false;

    // After yield/return, if more calls are pending, handle them.
    if (!p.pending_calls.empty() && !p.is_awaiting && !handle.done())
    {
        Scheduler::instance().enqueue([this]()
                                      { this->resume_next(); });
    }
}

template <typename T>
jspp::JsPromise jspp::JsAsyncIterator<T>::next(const T &val)
{
    // JsPromise is now a HeapObject.
    // We should return it by value (which AnyValue will then wrap).
    // Wait, coroutines return JsPromise by value.
    JsPromise p;
    if (handle)
    {
        if (handle.done())
        {
            p.resolve(AnyValue::make_object({{"value", Constants::UNDEFINED}, {"done", Constants::TRUE}}));
        }
        else
        {
            handle.promise().pending_calls.push({p, val});
            resume_next();
        }
    }
    else
    {
        p.resolve(AnyValue::make_object({{"value", Constants::UNDEFINED}, {"done", Constants::TRUE}}));
    }
    return p;
}

// --- JsAsyncIterator::promise_type methods ---

template <typename T>
template <typename From>
auto jspp::JsAsyncIterator<T>::promise_type::yield_value(From &&from)
{
    if (!pending_calls.empty())
    {
        auto call = pending_calls.front();
        pending_calls.pop();
        AnyValue result = AnyValue::make_object({{"value", std::forward<From>(from)}, {"done", Constants::FALSE}});
        call.first.resolve(result);
    }

    struct YieldAwaiter
    {
        promise_type &p;
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h)
        {
            // Suspended at yield.
        }
        T await_resume() { return p.current_input; }
    };
    return YieldAwaiter{*this};
}

template <typename T>
template <typename From>
void jspp::JsAsyncIterator<T>::promise_type::return_value(From &&from)
{
    if (!pending_calls.empty())
    {
        auto call = pending_calls.front();
        pending_calls.pop();
        AnyValue result = AnyValue::make_object({{"value", std::forward<From>(from)}, {"done", Constants::TRUE}});
        call.first.resolve(result);
    }

    while (!pending_calls.empty())
    {
        auto call = pending_calls.front();
        pending_calls.pop();
        AnyValue result = AnyValue::make_object({{"value", Constants::UNDEFINED}, {"done", Constants::TRUE}});
        call.first.resolve(result);
    }
}

template <typename T>
void jspp::JsAsyncIterator<T>::promise_type::fail_all(const AnyValue &reason)
{
    while (!pending_calls.empty())
    {
        auto call = pending_calls.front();
        pending_calls.pop();
        call.first.reject(reason);
    }
}

template <typename T>
void jspp::JsAsyncIterator<T>::promise_type::unhandled_exception()
{
    try
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (const Exception &e)
    {
        fail_all(e.data);
    }
    catch (const std::exception &e)
    {
        fail_all(AnyValue::make_string(e.what()));
    }
    catch (...)
    {
        fail_all(AnyValue::make_string("Unknown error in async generator"));
    }
}

template <typename T>
auto jspp::JsAsyncIterator<T>::promise_type::await_transform(AnyValue value)
{
    is_awaiting = true;
    struct AsyncIterAwaiter
    {
        AnyValueAwaiter base_awaiter;
        promise_type &p_ref;

        bool await_ready() { return base_awaiter.await_ready(); }
        void await_suspend(std::coroutine_handle<promise_type> h)
        {
            if (!base_awaiter.value.is_promise())
            {
                jspp::Scheduler::instance().enqueue([h]() mutable
                                              {
                    auto &pr = h.promise();
                    pr.is_awaiting = false;
                    pr.is_running = true;
                    h.resume();
                    pr.is_running = false;

                    if (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty())
                    {
                        while (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty())
                        {
                            pr.is_running = true;
                            pr.current_input = pr.pending_calls.front().second;
                            h.resume();
                            pr.is_running = false;
                        }
                    } });
                return;
            }
            auto p = base_awaiter.value.as_promise();
            p->then(
                [h](AnyValue v) mutable
                {
                    auto &pr = h.promise();
                    pr.is_awaiting = false;
                    pr.is_running = true;
                    h.resume();
                    pr.is_running = false;

                    if (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty())
                    {
                        while (!h.done() && !pr.is_awaiting && !pr.pending_calls.empty())
                        {
                            pr.is_running = true;
                            pr.current_input = pr.pending_calls.front().second;
                            h.resume();
                            pr.is_running = false;
                        }
                    }
                },
                [h](AnyValue e) mutable
                {
                    auto &pr = h.promise();
                    pr.is_awaiting = false;
                    pr.is_running = true;
                    h.resume();
                    pr.is_running = false;
                });
        }
        AnyValue await_resume()
        {
            return base_awaiter.await_resume();
        }
    };
    return AsyncIterAwaiter{AnyValueAwaiter{std::move(value)}, *this};
}
