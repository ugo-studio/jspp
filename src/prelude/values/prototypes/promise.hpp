#pragma once

#include "types.hpp"
#include "values/promise.hpp"
#include "any_value.hpp"

namespace jspp
{
    namespace PromisePrototypes
    {
        inline AnyValue &get_then_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             auto self = thisVal.as_promise();
                                                             AnyValue onFulfilled = (args.size() > 0 && args[0].is_function()) ? args[0] : Constants::UNDEFINED;
                                                             AnyValue onRejected = (args.size() > 1 && args[1].is_function()) ? args[1] : Constants::UNDEFINED;

                                                             // "then" returns a new Promise
                                                             JsPromise newPromise;
                                                             AnyValue newPromiseVal = AnyValue::make_promise(newPromise);

                                                             auto newPromiseState = newPromise.state;
                                                             auto resolveNew = [newPromiseState](const AnyValue &v)
                                                             {
                                                                 // Manual state resolution to avoid HeapObject management issues here
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

                                                             // Resolve handler
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

                                                             // Reject handler
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

        inline AnyValue &get_catch_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                             AnyValue onRejected = (args.size() > 0 && args[0].is_function()) ? args[0] : Constants::UNDEFINED;
                                                             const AnyValue thenArgs[] = {Constants::UNDEFINED, onRejected};
                                                             return thisVal.get_own_property("then").call(thisVal, thenArgs, "then"); },
                                                         "catch");
            return fn;
        }

        inline AnyValue &get_finally_fn()
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

        inline std::optional<AnyValue> get(const std::string &key)
        {

            if (key == "then")
            {
                return get_then_fn();
            }

            if (key == "catch")
            {
                return get_catch_fn();
            }

            if (key == "finally")
            {
                return get_finally_fn();
            }

            return std::nullopt;
        }
    }
}
