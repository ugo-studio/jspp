#pragma once

#include <exception>
#include "types.hpp"
#include "any_value.hpp"

namespace jspp
{
    struct Exception : std::exception
    {
        AnyValue data;

        explicit Exception(const AnyValue &value)
            : data(value) {}
        explicit Exception(AnyValue &&value)
            : data(std::move(value)) {}

        const char *what() const noexcept override;
        static Exception make_exception(const std::string &message, const std::string &name);
        static AnyValue exception_to_any_value(const std::exception &ex);

        // --- THROWERS
        static AnyValue throw_unresolved_reference(const std::string &var_name);
        static AnyValue throw_uninitialized_reference(const std::string &var_name);
        static AnyValue throw_immutable_assignment();
        static AnyValue throw_invalid_return_statement();
    };
}