#include "jspp.hpp"
#include "values/promise.hpp"
#include "values/prototypes/promise.hpp"

namespace jspp {

// --- PromiseState Implementation ---

PromiseState::PromiseState() : result(Constants::UNDEFINED), handled(false) {}

PromiseState::~PromiseState()
{
    if (status == PromiseStatus::Rejected && !handled)
    {
        std::string msg;
        try
        {
            if (result.is_object() || result.is_function())
            {
                msg = result.call_own_property("toString", {}).to_std_string();
            }
            else
            {
                msg = result.to_std_string();
            }
        }
        catch (...)
        {
            msg = result.to_std_string();
        }
        std::cerr << "UnhandledPromiseRejection: " << msg << "\n";
        std::exit(1);
    }
}

// --- JsPromise Implementation ---

JsPromise::JsPromise() : state(std::make_shared<PromiseState>()) {}

void JsPromise::resolve(AnyValue value)
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
            [weak_state](AnyValue v)
            {
                if (auto s = weak_state.lock())
                {
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
            [weak_state](AnyValue r)
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

    auto callbacks = state->onFulfilled;
    state->onFulfilled.clear();
    state->onRejected.clear();

    for (auto &cb : callbacks)
    {
        jspp::Scheduler::instance().enqueue([cb, value]()
                                            { cb(value); });
    }
}

void JsPromise::reject(AnyValue reason)
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

void JsPromise::then(std::function<void(AnyValue)> onFulfilled, std::function<void(AnyValue)> onRejected)
{
    state->handled = true;
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

std::string JsPromise::to_std_string() const
{
    return "[object Promise]";
}

bool JsPromise::has_symbol_property(const AnyValue &key) const
{
    if (symbol_props.count(key) > 0)
        return true;
    if (PromisePrototypes::get(key).has_value())
        return true;
    return false;
}

AnyValue JsPromise::get_property(const std::string &key, AnyValue thisVal)
{
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

AnyValue JsPromise::get_symbol_property(const AnyValue &key, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it == symbol_props.end())
    {
        auto proto_it = PromisePrototypes::get(key);
        if (proto_it.has_value())
        {
            return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key.to_std_string());
        }
        return Constants::UNDEFINED;
    }
    return AnyValue::resolve_property_for_read(it->second, thisVal, key.to_std_string());
}

AnyValue JsPromise::set_property(const std::string &key, AnyValue value, AnyValue thisVal)
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

AnyValue JsPromise::set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal)
{
    auto it = symbol_props.find(key);
    if (it != symbol_props.end())
    {
        return AnyValue::resolve_property_for_write(it->second, thisVal, value, key.to_std_string());
    }
    else
    {
        symbol_props[key] = value;
        return value;
    }
}

// --- JsPromisePromiseType Implementation ---

void JsPromisePromiseType::return_value(AnyValue val)
{
    promise.resolve(val);
}

void JsPromisePromiseType::unhandled_exception()
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

AnyValueAwaiter JsPromisePromiseType::await_transform(AnyValue value)
{
    return AnyValueAwaiter{value};
}

AnyValueAwaiter JsPromisePromiseType::await_transform(const JsPromise &value)
{
    return AnyValueAwaiter{AnyValue::make_promise(value)};
}

// --- AnyValueAwaiter Implementation ---

bool AnyValueAwaiter::await_ready()
{
    return false;
}

void AnyValueAwaiter::await_suspend(std::coroutine_handle<> h)
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

AnyValue AnyValueAwaiter::await_resume()
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

// --- PromisePrototypes Implementation ---

namespace PromisePrototypes {

AnyValue &get_then_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_promise();
                                                     AnyValue onFulfilled = (args.size() > 0 && args[0].is_function()) ? args[0] : Constants::UNDEFINED;
                                                     AnyValue onRejected = (args.size() > 1 && args[1].is_function()) ? args[1] : Constants::UNDEFINED;

                                                     JsPromise newPromise;
                                                     AnyValue newPromiseVal = AnyValue::make_promise(newPromise);

                                                     auto newPromiseState = newPromise.state;
                                                     auto resolveNew = [newPromiseState](const AnyValue &v)
                                                     {
                                                         if (newPromiseState->status != PromiseStatus::Pending)
                                                             return;
                                                         newPromiseState->status = PromiseStatus::Fulfilled;
                                                         newPromiseState->result = v;
                                                         auto callbacks = newPromiseState->onFulfilled;
                                                         newPromiseState->onFulfilled.clear();
                                                         newPromiseState->onRejected.clear();
                                                         for (auto &cb : callbacks)
                                                             jspp::Scheduler::instance().enqueue([cb, v]()
                                                                                                 { cb(v); });
                                                     };
                                                     auto rejectNew = [newPromiseState](const AnyValue &r)
                                                     {
                                                         if (newPromiseState->status != PromiseStatus::Pending)
                                                             return;
                                                         newPromiseState->status = PromiseStatus::Rejected;
                                                         newPromiseState->result = r;
                                                         auto callbacks = newPromiseState->onRejected;
                                                         newPromiseState->onFulfilled.clear();
                                                         newPromiseState->onRejected.clear();
                                                         for (auto &cb : callbacks)
                                                             jspp::Scheduler::instance().enqueue([cb, r]()
                                                                                                 { cb(r); });
                                                     };

                                                     auto resolveHandler = [resolveNew, rejectNew, onFulfilled](const AnyValue &val) mutable
                                                     {
                                                         if (onFulfilled.is_function())
                                                         {
                                                             try
                                                             {
                                                                 const AnyValue cbArgs[] = {val};
                                                                 auto res = onFulfilled.call(Constants::UNDEFINED, cbArgs, "onFulfilled");
                                                                 if (res.is_promise())
                                                                 {
                                                                     auto chained = res.as_promise();
                                                                     chained->then(
                                                                         [resolveNew](const AnyValue &v)
                                                                         { resolveNew(v); },
                                                                         [rejectNew](const AnyValue &e)
                                                                         { rejectNew(e); });
                                                                 }
                                                                 else
                                                                 {
                                                                     resolveNew(res);
                                                                 }
                                                             }
                                                             catch (const Exception &e)
                                                             {
                                                                 rejectNew(e.data);
                                                             }
                                                             catch (...)
                                                             {
                                                                 rejectNew(AnyValue::make_string("Unknown error"));
                                                             }
                                                         }
                                                         else
                                                         {
                                                             resolveNew(val);
                                                         }
                                                     };

                                                     auto rejectHandler = [resolveNew, rejectNew, onRejected](const AnyValue &reason) mutable
                                                     {
                                                         if (onRejected.is_function())
                                                         {
                                                             try
                                                             {
                                                                 const AnyValue cbArgs[] = {reason};
                                                                 auto res = onRejected.call(Constants::UNDEFINED, cbArgs, "onRejected");
                                                                 if (res.is_promise())
                                                                 {
                                                                     auto chained = res.as_promise();
                                                                     chained->then(
                                                                         [resolveNew](const AnyValue &v)
                                                                         { resolveNew(v); },
                                                                         [rejectNew](const AnyValue &e)
                                                                         { rejectNew(e); });
                                                                 }
                                                                 else
                                                                 {
                                                                     resolveNew(res);
                                                                 }
                                                             }
                                                             catch (const Exception &e)
                                                             {
                                                                 rejectNew(e.data);
                                                             }
                                                             catch (...)
                                                             {
                                                                 rejectNew(AnyValue::make_string("Unknown error"));
                                                             }
                                                         }
                                                         else
                                                         {
                                                             rejectNew(reason);
                                                         }
                                                     };

                                                     self->then(resolveHandler, rejectHandler);
                                                     return newPromiseVal; },
                                                 "then");
    return fn;
}

AnyValue &get_catch_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     AnyValue onRejected = (args.size() > 0 && args[0].is_function()) ? args[0] : Constants::UNDEFINED;
                                                     const AnyValue thenArgs[] = {Constants::UNDEFINED, onRejected};
                                                     return thisVal.get_own_property("then").call(thisVal, thenArgs, "then"); },
                                                 "catch");
    return fn;
}

AnyValue &get_finally_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     AnyValue onFinally = (args.size() > 0 && args[0].is_function()) ? args[0] : Constants::UNDEFINED;

                                                     const AnyValue thenArgs[] = {
                                                         AnyValue::make_function([onFinally](const AnyValue &, std::span<const AnyValue> args) -> AnyValue
                                                                                 {
                                                                                     AnyValue val = args.empty() ? Constants::UNDEFINED : args[0];
                                                                                     if (onFinally.is_function())
                                                                                     {
                                                                                         onFinally.call(Constants::UNDEFINED, {}, "onFinally");
                                                                                     }
                                                                                     return val; },
                                                                                 ""),
                                                         AnyValue::make_function([onFinally](const AnyValue &, std::span<const AnyValue> args) -> AnyValue
                                                                                 {
                                                                                     AnyValue reason = args.empty() ? Constants::UNDEFINED : args[0];
                                                                                     if (onFinally.is_function())
                                                                                     {
                                                                                         onFinally.call(Constants::UNDEFINED, {}, "onFinally");
                                                                                     }
                                                                                     throw Exception(reason); },
                                                                                 "")};
                                                     return thisVal.get_own_property("then").call(thisVal, thenArgs, "then"); },
                                                         "finally");
    return fn;
}

std::optional<AnyValue> get(const std::string &key)
{
    if (key == "then") return get_then_fn();
    if (key == "catch") return get_catch_fn();
    if (key == "finally") return get_finally_fn();
    return std::nullopt;
}

std::optional<AnyValue> get(const AnyValue &key)
{
    // Well-known symbols could be added here if needed
    return std::nullopt;
}

} // namespace PromisePrototypes

} // namespace jspp
