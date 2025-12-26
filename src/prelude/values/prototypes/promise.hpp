#pragma once

#include "types.hpp"
#include "values/promise.hpp"
#include "any_value.hpp"

namespace jspp {
    namespace PromisePrototypes {
        inline std::optional<AnyValue> get(const std::string& key, JsPromise* self) {
            
            if (key == "then") {
                return AnyValue::make_function([self](const AnyValue& thisVal, const std::vector<AnyValue>& args) -> AnyValue {
                    AnyValue onFulfilled = (args.size() > 0 && args[0].is_function()) ? args[0] : AnyValue::make_undefined();
                    AnyValue onRejected = (args.size() > 1 && args[1].is_function()) ? args[1] : AnyValue::make_undefined();
                    
                    // "then" returns a new Promise
                    JsPromise newPromise; 
                    AnyValue newPromiseVal = AnyValue::make_promise(newPromise);
                    
                    // Capture shared pointer to the new promise's state to keep it alive and modify it
                    auto newPromiseState = newPromise.state; 
                    // Helper wrapper to interact with state
                    auto resolveNew = [newPromiseState](const AnyValue& v) {
                        JsPromise p; p.state = newPromiseState;
                        p.resolve(v);
                    };
                    auto rejectNew = [newPromiseState](const AnyValue& r) {
                        JsPromise p; p.state = newPromiseState;
                        p.reject(r);
                    };


                    // Resolve handler
                    auto resolveHandler = [resolveNew, rejectNew, onFulfilled](AnyValue val) mutable {
                        if (onFulfilled.is_function()) {
                            try {
                                auto res = onFulfilled.as_function()->call(AnyValue::make_undefined(), {val});
                                if (res.is_promise()) {
                                    // Chaining: newPromise follows res
                                    auto chained = res.as_promise();
                                    chained->then(
                                        [resolveNew](AnyValue v) { resolveNew(v); },
                                        [rejectNew](AnyValue e) { rejectNew(e); }
                                    );
                                } else {
                                    resolveNew(res);
                                }
                            } catch (const Exception& e) {
                                rejectNew(*e.data);
                            } catch (...) {
                                rejectNew(AnyValue::make_string("Unknown error"));
                            }
                        } else {
                            resolveNew(val); // Fallthrough
                        }
                    };

                    // Reject handler
                    auto rejectHandler = [resolveNew, rejectNew, onRejected](AnyValue reason) mutable {
                         if (onRejected.is_function()) {
                            try {
                                auto res = onRejected.as_function()->call(AnyValue::make_undefined(), {reason});
                                if (res.is_promise()) {
                                    auto chained = res.as_promise();
                                    chained->then(
                                        [resolveNew](AnyValue v) { resolveNew(v); },
                                        [rejectNew](AnyValue e) { rejectNew(e); }
                                    );
                                } else {
                                    resolveNew(res); // Recovered
                                }
                            } catch (const Exception& e) {
                                rejectNew(*e.data);
                            } catch (...) {
                                rejectNew(AnyValue::make_string("Unknown error"));
                            }
                        } else {
                            rejectNew(reason); // Fallthrough
                        }
                    };

                    self->then(resolveHandler, rejectHandler);
                    return newPromiseVal;
                }, "then");
            }

            if (key == "catch") {
                 return AnyValue::make_function([self](const AnyValue& thisVal, const std::vector<AnyValue>& args) -> AnyValue {
                    // catch(onRejected) is then(undefined, onRejected)
                    AnyValue onRejected = (args.size() > 0 && args[0].is_function()) ? args[0] : AnyValue::make_undefined();
                    return thisVal.get_own_property("then").as_function()->call(thisVal, {AnyValue::make_undefined(), onRejected});
                 }, "catch");
            }

            if (key == "finally") {
                return AnyValue::make_function([self](const AnyValue& thisVal, const std::vector<AnyValue>& args) -> AnyValue {
                    AnyValue onFinally = (args.size() > 0 && args[0].is_function()) ? args[0] : AnyValue::make_undefined();
                    
                    // finally(onFinally) returns a promise that passes through value/reason, 
                    // but executes onFinally first.
                    
                    return thisVal.get_own_property("then").as_function()->call(thisVal, {
                        AnyValue::make_function([onFinally](const AnyValue&, const std::vector<AnyValue>& args) -> AnyValue {
                            AnyValue val = args.empty() ? AnyValue::make_undefined() : args[0];
                            if (onFinally.is_function()) {
                                onFinally.as_function()->call(AnyValue::make_undefined(), {});
                            }
                            return val;
                        }, ""),
                        AnyValue::make_function([onFinally](const AnyValue&, const std::vector<AnyValue>& args) -> AnyValue {
                            AnyValue reason = args.empty() ? AnyValue::make_undefined() : args[0];
                            if (onFinally.is_function()) {
                                onFinally.as_function()->call(AnyValue::make_undefined(), {});
                            }
                            throw Exception(reason);
                        }, "")
                    });
                }, "finally");
            }
            
            return std::nullopt;
        }
    }
}
