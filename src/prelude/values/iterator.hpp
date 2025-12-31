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
    class JsIterator
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
            T input_value;

            JsIterator get_return_object()
            {
                return JsIterator{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept { return {}; }

            // valid js generators allow access to the return value after completion,
            // so we must suspend at the end to keep the promise (and value) alive.
            std::suspend_always final_suspend() noexcept { return {}; }

            // Handle co_yield
            template <typename From>
            auto yield_value(From &&from)
            {
                current_value = std::forward<From>(from);
                struct Awaiter
                {
                    promise_type &p;
                    bool await_ready() { return false; }
                    void await_suspend(std::coroutine_handle<promise_type>) {}
                    T await_resume() { return p.input_value; }
                };
                return Awaiter{*this};
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

        explicit JsIterator(handle_type h) : handle(h) {}
        JsIterator(JsIterator &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

        // Delete copy constructor/assignment to ensure unique ownership of the handle
        JsIterator(const JsIterator &) = delete;
        JsIterator &operator=(const JsIterator &) = delete;

        ~JsIterator()
        {
            if (handle)
                handle.destroy();
        }

        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const;
        NextResult next(const T &val = T());
        std::vector<T> to_vector();
        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
        AnyValue set_property(const std::string &key, const AnyValue &value, const AnyValue &thisVal);
    };
}