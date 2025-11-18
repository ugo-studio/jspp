
#pragma once

#include "types.hpp"

namespace jspp
{
    class JsValue;

    struct RuntimeError : std::exception
    {
        std::shared_ptr<JsValue> data;

        explicit RuntimeError(std::shared_ptr<JsValue> d)
            : data(std::move(d)) {}
        explicit RuntimeError(const JsValue &value)
            : data(std::make_shared<JsValue>(value)) {}
        explicit RuntimeError(JsValue &&value)
            : data(std::make_shared<JsValue>(std::move(value))) {}

        const char *what() const noexcept override;
        static RuntimeError make_error(const std::string &message, const std::string &name);
        static JsValue error_to_value(const std::exception &ex);

        // --- THROWERS
        static JsValue throw_unresolved_reference_error(const std::string &var_name);
        static JsValue throw_uninitialized_reference_error(const std::string &var_name);
        static JsValue throw_immutable_assignment_error();
        static JsValue throw_invalid_return_statement_error();
    };
}