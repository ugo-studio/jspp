#include "jspp.hpp"
#include "exception.hpp"
#include "any_value.hpp"

namespace jspp {

const char *Exception::what() const noexcept
{
    static thread_local std::string last_msg;
    last_msg = data.to_std_string();
    return last_msg.c_str();
}

Exception Exception::make_exception(const std::string &message, const std::string &name)
{
    std::vector<AnyValue> args = {AnyValue::make_string(message)};
    AnyValue errorObj = ::Error.construct(args, name);
    errorObj.define_data_property("name", AnyValue::make_string(name), true, false, true);

    return Exception(errorObj);
}

AnyValue Exception::exception_to_any_value(const std::exception &ex)
{
    if (const jspp::Exception *err = dynamic_cast<const jspp::Exception *>(&ex))
    {
        return err->data;
    }
    else
    {
        return AnyValue::make_string(ex.what());
    }
}

// --- THROWERS
AnyValue Exception::throw_unresolved_reference(const std::string &var_name)
{
    throw Exception::make_exception(var_name + " is not defined", "ReferenceError");
}
AnyValue Exception::throw_uninitialized_reference(const std::string &var_name)
{
    throw Exception::make_exception("Cannot access '" + var_name + "' before initialization", "ReferenceError");
}
AnyValue Exception::throw_immutable_assignment()
{
    throw Exception::make_exception("Assignment to constant variable.", "TypeError");
}
AnyValue Exception::throw_invalid_return_statement()
{
    throw Exception::make_exception("Return statements are only valid inside functions.", "SyntaxError");
}

} // namespace jspp
