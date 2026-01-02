#pragma once

#include "types.hpp"
#include <coroutine>
#include <optional>
#include <queue>
#include <iostream>
#include <utility>
#include <exception>
#include "values/promise.hpp"
#include "scheduler.hpp"

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    template <typename T>
    class JsAsyncIterator
    {
    public:
        struct promise_type
        {
            std::queue<std::pair<JsPromise, T>> pending_calls;
            bool is_awaiting = false;
            bool is_running = false;
            T current_input;

            JsAsyncIterator get_return_object()
            {
                return JsAsyncIterator{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept { return {}; }

            std::suspend_always final_suspend() noexcept { return {}; }

            // Declarations
            template <typename From>
            auto yield_value(From &&from);

            template <typename From>
            void return_value(From &&from);

            void unhandled_exception();

            void fail_all(const AnyValue &reason);

            auto await_transform(AnyValue value);
        };

        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle;

        explicit JsAsyncIterator(handle_type h) : handle(h) {}
        JsAsyncIterator(JsAsyncIterator &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

        JsAsyncIterator(const JsAsyncIterator &) = delete;
        JsAsyncIterator &operator=(const JsAsyncIterator &) = delete;

        ~JsAsyncIterator()
        {
            if (handle)
                handle.destroy();
        }

        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const;

        JsPromise next(const T &val = T());

        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);

        void resume_next();
    };
}
