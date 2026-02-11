#pragma once

#include "types.hpp"
#include "values/promise.hpp"
#include "any_value.hpp"
#include "values/prototypes/promise.hpp"

namespace jspp
{

    inline PromiseState::PromiseState() : result(Constants::UNDEFINED) {}

    inline JsPromise::JsPromise() : state(std::make_shared<PromiseState>()) {}

    inline void JsPromise::resolve(const AnyValue &value)
    {
        if (state->status != PromiseStatus::Pending)
            return;

        if (value.is_promise())
        {
            auto p = value.as_promise();
            if (p->state == state)
            {
                reject(AnyValue::make_string("TypeError: Chaining cycle detected for promise"));
                return;
            }

            auto weak_state = std::weak_ptr<PromiseState>(state);

            p->then(
                [weak_state](const AnyValue &v)
                {
                    if (auto s = weak_state.lock())
                    {
                        // We can't easily use JsPromise here because it's a HeapObject now.
                        // But we can manually resolve the state if we have a way.
                        // Actually, we can create a temporary AnyValue from the state.
                        // But AnyValue expects a JsPromise* which was allocated with new.
                        // This is tricky.
                        // Let's assume we can just modify the status and result of the state directly.
                        s->status = PromiseStatus::Fulfilled;
                        s->result = v;
                        auto callbacks = s->onFulfilled;
                        s->onFulfilled.clear();
                        s->onRejected.clear();
                        for (auto &cb : callbacks)
                            jspp::Scheduler::instance().enqueue([cb, v]()
                                                                { cb(v); });
                    }
                },
                [weak_state](const AnyValue &r)
                {
                    if (auto s = weak_state.lock())
                    {
                        s->status = PromiseStatus::Rejected;
                        s->result = r;
                        auto callbacks = s->onRejected;
                        s->onFulfilled.clear();
                        s->onRejected.clear();
                        for (auto &cb : callbacks)
                            jspp::Scheduler::instance().enqueue([cb, r]()
                                                                { cb(r); });
                    }
                });
            return;
        }

        state->status = PromiseStatus::Fulfilled;
        state->result = value;

        // Schedule callbacks
        auto callbacks = state->onFulfilled;
        state->onFulfilled.clear();
        state->onRejected.clear();

        for (auto &cb : callbacks)
        {
            jspp::Scheduler::instance().enqueue([cb, value]()
                                                { cb(value); });
        }
    }

    inline void JsPromise::reject(const AnyValue &reason)
    {
        if (state->status != PromiseStatus::Pending)
            return;
        state->status = PromiseStatus::Rejected;
        state->result = reason;

        auto callbacks = state->onRejected;
        state->onFulfilled.clear();
        state->onRejected.clear();

        for (auto &cb : callbacks)
        {
            jspp::Scheduler::instance().enqueue([cb, reason]()
                                                { cb(reason); });
        }
    }

    inline void JsPromise::then(std::function<void(const AnyValue &)> onFulfilled, std::function<void(const AnyValue &)> onRejected)
    {
        if (state->status == PromiseStatus::Fulfilled)
        {
            if (onFulfilled)
            {
                AnyValue val = state->result;
                jspp::Scheduler::instance().enqueue([onFulfilled, val]()
                                                    { onFulfilled(val); });
            }
        }
        else if (state->status == PromiseStatus::Rejected)
        {
            if (onRejected)
            {
                AnyValue val = state->result;
                jspp::Scheduler::instance().enqueue([onRejected, val]()
                                                    { onRejected(val); });
            }
        }
        else
        {
            if (onFulfilled)
                state->onFulfilled.push_back(onFulfilled);
            if (onRejected)
                state->onRejected.push_back(onRejected);
        }
    }

    inline auto JsPromise::operator co_await() const
    {
        // This is safe because AnyValue::make_promise copies the JsPromise (which is a HeapObject)
        // Actually, AnyValue::make_promise(const JsPromise&) does from_ptr(new JsPromise(promise)).
        return AnyValueAwaiter{AnyValue::make_promise(*this)};
    }

    inline std::string JsPromise::to_std_string() const
    {
        return "[object Promise]";
    }

    inline AnyValue JsPromise::get_property(const std::string &key, const AnyValue &thisVal)
    {
        // Prototype lookup
        auto proto_it = PromisePrototypes::get(key);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }

        auto it = props.find(key);
        if (it != props.end())
        {
            return AnyValue::resolve_property_for_read(it->second, thisVal, key);
        }
        return Constants::UNDEFINED;
    }

    inline AnyValue JsPromise::set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal)
    {
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

    // --- Coroutine Methods ---

    inline void JsPromisePromiseType::return_value(const AnyValue &val)
    {
        promise.resolve(val);
    }

    inline void JsPromisePromiseType::unhandled_exception()
    {
        try
        {
            throw;
        }
        catch (const Exception &e)
        {
            promise.reject(e.data);
        }
        catch (const std::exception &e)
        {
            promise.reject(AnyValue::make_string(e.what()));
        }
        catch (...)
        {
            promise.reject(AnyValue::make_string("Unknown exception"));
        }
    }

    inline auto JsPromisePromiseType::await_transform(const AnyValue &value)
    {
        return AnyValueAwaiter{value};
    }

    inline auto JsPromisePromiseType::await_transform(const JsPromise &value)
    {
        // value is a JsPromise& which is a HeapObject.
        // We wrap it in a temporary AnyValue for Awaiter.
        // Wait, AnyValue::make_promise(value) will allocate a new JsPromise on heap.
        // This is fine for now.
        return AnyValueAwaiter{AnyValue::make_promise(value)};
    }

    // --- AnyValueAwaiter ---

    inline bool AnyValueAwaiter::await_ready()
    {
        return false;
    }

    inline void AnyValueAwaiter::await_suspend(std::coroutine_handle<> h)
    {
        if (!value.is_promise())
        {
            jspp::Scheduler::instance().enqueue([h]() mutable
                                                { h.resume(); });
            return;
        }
        auto p = value.as_promise();

        p->then(
            [h](AnyValue v) mutable
            { h.resume(); },
            [h](AnyValue e) mutable
            { h.resume(); });
    }

    inline AnyValue AnyValueAwaiter::await_resume()
    {
        if (!value.is_promise())
            return value;
        auto p = value.as_promise();
        if (p->state->status == PromiseStatus::Fulfilled)
        {
            return p->state->result;
        }
        else
        {
            throw Exception(p->state->result);
        }
    }

}