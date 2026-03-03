#pragma once

#include "types.hpp"
#include <coroutine>
#include <optional>
#include <vector>

namespace jspp
{
    class AnyValue;

    struct GeneratorReturnException {};

    template <typename T>
    class JsIterator : public HeapObject
    {
    public:
        struct NextResult
        {
            std::optional<T> value;
            bool done;
        };

        JsType get_heap_type() const override { return JsType::Iterator; }

        struct promise_type
        {
            std::optional<T> current_value;
            std::exception_ptr exception_;
            T input_value;

            std::exception_ptr pending_exception = nullptr;
            bool pending_return = false;

            JsIterator get_return_object();
            std::suspend_always initial_suspend() noexcept;
            std::suspend_always final_suspend() noexcept;

            struct Awaiter
            {
                promise_type &p;
                bool await_ready() { return false; }
                void await_suspend(std::coroutine_handle<promise_type>) {}
                T await_resume() {
                    if (p.pending_exception) {
                        auto ex = p.pending_exception;
                        p.pending_exception = nullptr;
                        std::rethrow_exception(ex);
                    }
                    if (p.pending_return) {
                        p.pending_return = false;
                        throw GeneratorReturnException{};
                    }
                    return p.input_value;
                }
            };

            template <typename From>
            Awaiter yield_value(From &&from) {
                current_value = std::forward<From>(from);
                return Awaiter{*this};
            }

            template <typename From>
            void return_value(From &&from) {
                current_value = std::forward<From>(from);
            }

            void unhandled_exception();
        };

        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle;

        explicit JsIterator(handle_type h);
        JsIterator(JsIterator &&other) noexcept;
        JsIterator(const JsIterator &) = delete;
        JsIterator &operator=(const JsIterator &) = delete;
        ~JsIterator();

        std::unordered_map<std::string, AnyValue> props;
        std::map<AnyValue, AnyValue> symbol_props;

        std::string to_std_string() const;
        NextResult next(const T &val = T());
        NextResult return_(const T &val = T());
        NextResult throw_(const AnyValue &err);
        std::vector<T> to_vector();
        bool has_symbol_property(const AnyValue &key) const;
        AnyValue get_property(const std::string &key, AnyValue thisVal);
        AnyValue get_symbol_property(const AnyValue &key, AnyValue thisVal);
        AnyValue set_property(const std::string &key, AnyValue value, AnyValue thisVal);
        AnyValue set_symbol_property(const AnyValue &key, AnyValue value, AnyValue thisVal);
    };
}
