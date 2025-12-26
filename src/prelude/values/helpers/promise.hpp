#pragma once

#include "types.hpp"
#include "values/promise.hpp"
#include "any_value.hpp"
#include "values/prototypes/promise.hpp"

namespace jspp {

    inline PromiseState::PromiseState() : result(std::make_shared<AnyValue>(AnyValue::make_undefined())) {}

    inline JsPromise::JsPromise() : state(std::make_shared<PromiseState>()) {}

    inline void JsPromise::resolve(const AnyValue& value) {
        if (state->status != PromiseStatus::Pending) return;

        if (value.is_promise()) {
             auto p = value.as_promise();
             if (p->state == state) {
                 reject(AnyValue::make_string("TypeError: Chaining cycle detected for promise"));
                 return;
             }
             
             auto weak_state = std::weak_ptr<PromiseState>(state);
             
             p->then(
                 [weak_state](AnyValue v) {
                     if (auto s = weak_state.lock()) {
                         JsPromise localP; localP.state = s;
                         localP.resolve(v); 
                     }
                 },
                 [weak_state](AnyValue r) {
                     if (auto s = weak_state.lock()) {
                         JsPromise localP; localP.state = s;
                         localP.reject(r);
                     }
                 }
             );
             return;
        }

        state->status = PromiseStatus::Fulfilled;
        *(state->result) = value;
        
        // Schedule callbacks
        auto callbacks = state->onFulfilled;
        state->onFulfilled.clear();
        state->onRejected.clear();
        
        for (auto& cb : callbacks) {
            jspp::Scheduler::instance().enqueue([cb, value]() {
                cb(value);
            });
        }
    }

    inline void JsPromise::reject(const AnyValue& reason) {
        if (state->status != PromiseStatus::Pending) return;
        state->status = PromiseStatus::Rejected;
        *(state->result) = reason;
        
        auto callbacks = state->onRejected;
        state->onFulfilled.clear();
        state->onRejected.clear();
        
        for (auto& cb : callbacks) {
            jspp::Scheduler::instance().enqueue([cb, reason]() {
                cb(reason);
            });
        }
    }

    inline void JsPromise::then(std::function<void(AnyValue)> onFulfilled, std::function<void(AnyValue)> onRejected) {
        if (state->status == PromiseStatus::Fulfilled) {
            if (onFulfilled) {
                AnyValue val = *(state->result);
                jspp::Scheduler::instance().enqueue([onFulfilled, val]() {
                    onFulfilled(val);
                });
            }
        } else if (state->status == PromiseStatus::Rejected) {
            if (onRejected) {
                AnyValue val = *(state->result);
                jspp::Scheduler::instance().enqueue([onRejected, val]() {
                    onRejected(val);
                });
            }
        } else {
            if (onFulfilled) state->onFulfilled.push_back(onFulfilled);
            if (onRejected) state->onRejected.push_back(onRejected);
        }
    }

    inline std::string JsPromise::to_std_string() const {
        return "[object Promise]";
    }

    inline AnyValue JsPromise::get_property(const std::string& key, const AnyValue& thisVal) {
        // Prototype lookup
        auto proto_it = PromisePrototypes::get(key, const_cast<JsPromise*>(this));
        if (proto_it.has_value()) {
             return AnyValue::resolve_property_for_read(proto_it.value(), thisVal, key);
        }
        
        auto it = props.find(key);
        if (it != props.end()) {
             return AnyValue::resolve_property_for_read(it->second, thisVal, key);
        }
        return AnyValue::make_undefined();
    }

    inline AnyValue JsPromise::set_property(const std::string& key, const AnyValue& value, const AnyValue& thisVal) {
         // Prototype check (if we had setters on prototype)
         
         auto it = props.find(key);
         if (it != props.end()) {
             return AnyValue::resolve_property_for_write(it->second, thisVal, value, key);
         } else {
             props[key] = value;
             return value;
         }
    }
    
    // --- Coroutine Methods ---
    
    inline void JsPromisePromiseType::return_value(const AnyValue& val) {
        promise.resolve(val);
    }
    
    inline void JsPromisePromiseType::unhandled_exception() {
         try {
             throw;
         } catch (const Exception& e) {
             promise.reject(*(e.data));
         } catch (const std::exception& e) {
             promise.reject(AnyValue::make_string(e.what()));
         } catch (...) {
             promise.reject(AnyValue::make_string("Unknown exception"));
         }
    }
    
    inline auto JsPromisePromiseType::await_transform(const AnyValue& value) {
         return AnyValueAwaiter{value};
    }

    // --- AnyValueAwaiter ---

    inline bool AnyValueAwaiter::await_ready() {
        if (!value.is_promise()) return true; // Non-promise values are ready immediately (for now)
        // Always suspend for promises to ensure microtask interleaving, even if already resolved.
        return false;
    }

    inline void AnyValueAwaiter::await_suspend(std::coroutine_handle<> h) {
        if (!value.is_promise()) {
            h.resume();
            return;
        }
        auto p = value.as_promise();
        
        // Attach resume to promise resolution.
        // Since we are using the Scheduler in .then(), this will be queued.
        p->then(
            [h](AnyValue v) mutable { h.resume(); },
            [h](AnyValue e) mutable { h.resume(); } 
        );
    }

    inline AnyValue AnyValueAwaiter::await_resume() {
        if (!value.is_promise()) return value;
        auto p = value.as_promise();
        if (p->state->status == PromiseStatus::Fulfilled) {
            return *(p->state->result);
        } else {
            // Throw exception to be caught by try/catch in coroutine
            throw Exception(*(p->state->result));
        }
    }

}
