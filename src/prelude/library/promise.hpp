#pragma once

#include "types.hpp"
#include "values/promise.hpp"
#include "any_value.hpp"
#include "exception.hpp"

inline auto Promise = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                    {
                                                        if (args.empty() || !args[0].is_function())
                                                        {
                                                            throw jspp::Exception::make_exception("Promise resolver undefined is not a function", "TypeError");
                                                        }
                                                        auto executor = args[0].as_function();

                                                        jspp::JsPromise promise;
                                                        auto state = promise.state; // Share state

                                                        // resolve function
                                                        auto resolveFn = jspp::AnyValue::make_function([state](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                                                       {
        jspp::JsPromise p; p.state = state;
        p.resolve(args.empty() ? jspp::AnyValue::make_undefined() : args[0]);
        return jspp::AnyValue::make_undefined(); }, "resolve");

                                                        // reject function
                                                        auto rejectFn = jspp::AnyValue::make_function([state](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                                                      {
        jspp::JsPromise p; p.state = state;
        p.reject(args.empty() ? jspp::AnyValue::make_undefined() : args[0]);
        return jspp::AnyValue::make_undefined(); }, "reject");

                                                        try
                                                        {
                                                            const jspp::AnyValue executorArgs[] = {resolveFn, rejectFn};
                                                            executor->call(jspp::Constants::UNDEFINED, std::span<const jspp::AnyValue>(executorArgs, 2));
                                                        }
                                                        catch (const jspp::Exception &e)
                                                        {
                                                            promise.reject(*e.data);
                                                        }
                                                        catch (...)
                                                        {
                                                            promise.reject(jspp::AnyValue::make_string("Unknown error during Promise execution"));
                                                        }

                                                        return jspp::AnyValue::make_promise(promise); },
                                                    "Promise");

struct PromiseInit
{
    PromiseInit()
    {
        // Promise.resolve(value)
        Promise.define_data_property("resolve", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                              {
            jspp::JsPromise p;
            p.resolve(args.empty() ? jspp::AnyValue::make_undefined() : args[0]);
            return jspp::AnyValue::make_promise(p); }, "resolve"));

        // Promise.reject(reason)
        Promise.define_data_property("reject", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                             {
            jspp::JsPromise p;
            p.reject(args.empty() ? jspp::AnyValue::make_undefined() : args[0]);
            return jspp::AnyValue::make_promise(p); }, "reject"));

        // Promise.all(iterable)
        Promise.define_data_property("all", jspp::AnyValue::make_function([](const jspp::AnyValue &, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                                          {
             // Basic implementation for arrays
             if (args.empty() || !args[0].is_array()) {
                  // If not array, reject? Or treat as non-iterable?
                  // Should throw TypeError if not iterable. For now assume array.
                  // If empty array, return resolved empty array.
                  // TODO: Strict iterable check
             }
             
             // Handle non-array iterable or empty args
             if (args.empty() || !args[0].is_array()) {
                 jspp::JsPromise p; p.reject(jspp::AnyValue::make_string("Promise.all argument must be an array"));
                 return jspp::AnyValue::make_promise(p);
             }

             auto arr = args[0].as_array();
             size_t len = static_cast<size_t>(arr->length);
             if (len == 0) {
                 jspp::JsPromise p; p.resolve(jspp::AnyValue::make_array(std::vector<jspp::AnyValue>()));
                 return jspp::AnyValue::make_promise(p);
             }

             jspp::JsPromise masterPromise;
             auto masterState = masterPromise.state;
             
             auto results = std::make_shared<std::vector<jspp::AnyValue>>(len, jspp::Constants::UNDEFINED);
             auto count = std::make_shared<size_t>(len);
             
             // Check if already rejected to avoid further processing
             auto rejected = std::make_shared<bool>(false);

             for (size_t i = 0; i < len; ++i) {
                 jspp::AnyValue item = arr->get_property(static_cast<uint32_t>(i));
                 
                 auto handleResult = [masterState, results, count, i, rejected](const jspp::AnyValue& res) {
                      if (*rejected) return;
                      (*results)[i] = res;
                      (*count)--;
                      if (*count == 0) {
                          jspp::JsPromise p; p.state = masterState;
                          p.resolve(jspp::AnyValue::make_array(std::move(*results))); 
                      }
                 };
                 
                 auto handleReject = [masterState, rejected](const jspp::AnyValue& reason) {
                      if (*rejected) return;
                      *rejected = true;
                      jspp::JsPromise p; p.state = masterState;
                      masterState->status =jspp::PromiseStatus::Rejected; // ensure master state updated
                      p.reject(reason);
                 };

                 if (item.is_promise()) {
                     item.as_promise()->then(handleResult, handleReject);
                 } else {
                     handleResult(item);
                 }
             }
             
             return jspp::AnyValue::make_promise(masterPromise); }, "all"));
    }
} promiseInit;
