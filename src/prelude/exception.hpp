
#pragma once

#include "types.hpp"

namespace jspp
{
    class AnyValue;

    struct Exception : std::exception
    {
        std::shared_ptr<AnyValue> data;

        explicit Exception(std::shared_ptr<AnyValue> d)
            : data(std::move(d)) {}
        explicit Exception(const AnyValue &value)
            : data(std::make_shared<AnyValue>(value)) {}
        explicit Exception(AnyValue &&value)
            : data(std::make_shared<AnyValue>(std::move(value))) {}

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