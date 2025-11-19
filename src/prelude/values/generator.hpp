#pragma once

#include "types.hpp"
#include <coroutine>
#include <optional>
#include <iostream>
#include <utility>
#include <exception>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    template <typename T>
    class JsGenerator
    {
    public:
        struct NextResult
        {
            std::optional<T> value;
            bool done;
        };
        struct promise_type
        {
            std::optional<T> current_value;
            std::exception_ptr exception_;

            JsGenerator get_return_object()
            {
                return JsGenerator{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept { return {}; }

            // valid js generators allow access to the return value after completion,
            // so we must suspend at the end to keep the promise (and value) alive.
            std::suspend_always final_suspend() noexcept { return {}; }

            // Handle co_yield
            template <typename From>
            std::suspend_always yield_value(From &&from)
            {
                current_value = std::forward<From>(from);
                return {};
            }

            // Handle co_return
            // This replaces return_void.
            // It captures the final value and moves to final_suspend (implicit).
            template <typename From>
            void return_value(From &&from)
            {
                current_value = std::forward<From>(from);
            }

            void unhandled_exception()
            {
                exception_ = std::current_exception();
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle;

        explicit JsGenerator(handle_type h) : handle(h) {}
        JsGenerator(JsGenerator &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

        // Delete copy constructor/assignment to ensure unique ownership of the handle
        JsGenerator(const JsGenerator &) = delete;
        JsGenerator &operator=(const JsGenerator &) = delete;

        ~JsGenerator()
        {
            if (handle)
                handle.destroy();
        }

        NextResult next()
        {
            // If the generator is already finished or invalid, return {undefined, true}
            if (!handle || handle.done())
                return {std::nullopt, true};

            // Resume execution until next co_yield or co_return
            handle.resume();

            if (handle.promise().exception_)
            {
                std::rethrow_exception(handle.promise().exception_);
            }

            // If handle.done() is TRUE, we hit co_return (value: X, done: true)
            // If handle.done() is FALSE, we hit co_yield (value: X, done: false)
            bool is_done = handle.done();

            return {std::move(handle.promise().current_value), is_done};
        }

        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const;
        AnyValue get_property(const std::string &key);
        AnyValue set_property(const std::string &key, const AnyValue &value);
    };
}