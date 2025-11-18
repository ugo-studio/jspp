
#pragma once

#include "types.hpp"

namespace jspp
{
    class AnyValue;

    struct RuntimeError : std::exception
    {
        std::shared_ptr<AnyValue> data;

        explicit RuntimeError(std::shared_ptr<AnyValue> d)
            : data(std::move(d)) {}
        explicit RuntimeError(const AnyValue &value)
            : data(std::make_shared<AnyValue>(value)) {}
        explicit RuntimeError(AnyValue &&value)
            : data(std::make_shared<AnyValue>(std::move(value))) {}

        const char *what() const noexcept override;
        static RuntimeError make_error(const std::string &message, const std::string &name);
        static AnyValue error_to_value(const std::exception &ex);

        // --- THROWERS
        static AnyValue throw_unresolved_reference_error(const std::string &var_name);
        static AnyValue throw_uninitialized_reference_error(const std::string &var_name);
        static AnyValue throw_immutable_assignment_error();
        static AnyValue throw_invalid_return_statement_error();
    };
}