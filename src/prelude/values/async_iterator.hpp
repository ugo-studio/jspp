#pragma once

#include "types.hpp"
#include <coroutine>
#include <optional>
#include <queue>
#include <vector>

namespace jspp
{
    class AnyValue;
    struct JsPromise;

    template <typename T>
    class JsAsyncIterator : public HeapObject
    {
    public:
        JsType get_heap_type() const override { return JsType::AsyncIterator; }

        struct promise_type
        {
            std::queue<std::pair<JsPromise, T>> pending_calls;
            bool is_awaiting = false;
            bool is_running = false;
            T current_input;

            JsAsyncIterator get_return_object();
            std::suspend_always initial_suspend() noexcept;
            std::suspend_always final_suspend() noexcept;

            struct YieldAwaiter
            {
                promise_type &p;
                bool await_ready();
                void await_suspend(std::coroutine_handle<promise_type> h);
                T await_resume();
            };

            template <typename From>
            YieldAwaiter yield_value(From &&from) {
                if (!pending_calls.empty()) {
                    auto call = pending_calls.front();
                    pending_calls.pop();
                    AnyValue result = AnyValue::make_object({{"value", std::forward<From>(from)}, {"done", Constants::FALSE}});
                    call.first.resolve(result);
                }
                return YieldAwaiter{*this};
            }

            template <typename From>
            void return_value(From &&from) {
                if (!pending_calls.empty()) {
                    auto call = pending_calls.front();
                    pending_calls.pop();
                    AnyValue result = AnyValue::make_object({{"value", std::forward<From>(from)}, {"done", Constants::TRUE}});
                    call.first.resolve(result);
                }
                while (!pending_calls.empty()) {
                    auto call = pending_calls.front();
                    pending_calls.pop();
                    AnyValue result = AnyValue::make_object({{"value", Constants::UNDEFINED}, {"done", Constants::TRUE}});
                    call.first.resolve(result);
                }
            }

            void unhandled_exception();
            
            void fail_all(const AnyValue &reason) {
                while (!pending_calls.empty()) {
                    auto call = pending_calls.front();
                    pending_calls.pop();
                    call.first.reject(reason);
                }
            }

            struct AsyncIterAwaiter
            {
                AnyValueAwaiter base_awaiter;
                promise_type &p_ref;

                bool await_ready();
                void await_suspend(std::coroutine_handle<promise_type> h);
                AnyValue await_resume();
            };

            AsyncIterAwaiter await_transform(AnyValue value);
        };

        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle;

        explicit JsAsyncIterator(handle_type h);
        JsAsyncIterator(JsAsyncIterator &&other) noexcept;
        JsAsyncIterator(const JsAsyncIterator &) = delete;
        JsAsyncIterator &operator=(const JsAsyncIterator &) = delete;
        ~JsAsyncIterator();

        std::unordered_map<std::string, AnyValue> props;
        std::map<AnyValue, AnyValue> symbol_props;

        std::string to_std_string() const;
        JsPromise next(const T &val = T());
        bool has_symbol_property(const AnyValue &key) const;
        AnyValue get_property(const std::string &key, AnyValue thisVal);
        AnyValue get_symbol_property(const AnyValue &key, AnyValue thisVal);
        AnyValue set_property(const std::string &key, AnyValue value, AnyValue thisVal);
        AnyValue set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal);

        void resume_next();
    };
}
