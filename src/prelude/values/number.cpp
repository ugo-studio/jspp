#include "jspp.hpp"
#include "values/prototypes/number.hpp"

namespace jspp
{

    // --- JsNumber Implementation ---

    namespace JsNumber
    {
        std::string to_std_string(double num)
        {
            if (std::isnan(num))
                return "NaN";
            if (std::abs(num) >= 1e21 || (std::abs(num) > 0 && std::abs(num) < 1e-6))
            {
                std::ostringstream oss;
                oss << std::scientific << std::setprecision(4) << num;
                return oss.str();
            }
            else
            {
                std::ostringstream oss;
                oss << std::setprecision(6) << std::fixed << num;
                std::string s = oss.str();
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (!s.empty() && s.back() == '.')
                    s.pop_back();
                return s;
            }
        }

        std::string to_std_string(const AnyValue &value)
        {
            return to_std_string(value.as_double());
        }

        std::string to_radix_string(double value, int radix)
        {
            if (std::isnan(value))
                return "NaN";
            if (!std::isfinite(value))
                return value > 0 ? "Infinity" : "-Infinity";

            long long intPart = static_cast<long long>(value);

            if (radix == 10)
            {
                return AnyValue::make_number(value).to_std_string();
            }

            bool negative = intPart < 0;
            if (negative)
                intPart = -intPart;

            std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
            std::string res = "";

            if (intPart == 0)
                res = "0";
            else
            {
                while (intPart > 0)
                {
                    res += chars[intPart % radix];
                    intPart /= radix;
                }
            }
            if (negative)
                res += "-";
            std::reverse(res.begin(), res.end());
            return res;
        }

    }

    // --- NumberPrototypes Implementation ---

    namespace NumberPrototypes
    {

        AnyValue &get_toExponential_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                     double self = thisVal.as_double();
                                                     int digits = -1;
                                                     if (!args.empty() && !args[0].is_undefined())
                                                     {
                                                         digits = Operators_Private::ToInt32(args[0]);
                                                         if (digits < 0 || digits > 100)
                                                         {
                                                             throw Exception::make_exception("toExponential() digits argument must be between 0 and 100", "RangeError");
                                                         }
                                                     }

                                                     std::ostringstream ss;
                                                     if (digits >= 0)
                                                     {
                                                         ss << std::scientific << std::setprecision(digits) << self;
                                                     }
                                                     else
                                                     {
                                                         ss << std::scientific << self;
                                                     }

                                                     std::string res = ss.str();
                                                     return AnyValue::make_string(res); },
                                                         "toExponential");
            return fn;
        }

        AnyValue &get_toFixed_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                     double self = thisVal.as_double();
                                                     int digits = 0;
                                                     if (!args.empty() && !args[0].is_undefined())
                                                     {
                                                         digits = Operators_Private::ToInt32(args[0]);
                                                     }
                                                     if (digits < 0 || digits > 100)
                                                     {
                                                         throw Exception::make_exception("toFixed() digits argument must be between 0 and 100", "RangeError");
                                                     }

                                                     std::ostringstream ss;
                                                     ss << std::fixed << std::setprecision(digits) << self;
                                                     return AnyValue::make_string(ss.str()); },
                                                         "toFixed");
            return fn;
        }

        AnyValue &get_toPrecision_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                     double self = thisVal.as_double();
                                                     if (args.empty() || args[0].is_undefined())
                                                     {
                                                         return AnyValue::make_number(self).get_own_property("toString").call(AnyValue::make_number(self), {}, "toString");
                                                     }
                                                     int precision = Operators_Private::ToInt32(args[0]);
                                                     if (precision < 1 || precision > 100)
                                                     {
                                                         throw Exception::make_exception("toPrecision() precision argument must be between 1 and 100", "RangeError");
                                                     }

                                                     std::ostringstream ss;
                                                     ss << std::setprecision(precision) << self;
                                                     return AnyValue::make_string(ss.str()); },
                                                         "toPrecision");
            return fn;
        }

        AnyValue &get_toString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                         {
                                                     double self = thisVal.as_double();
                                                     int radix = 10;
                                                     if (!args.empty() && !args[0].is_undefined())
                                                     {
                                                         radix = Operators_Private::ToInt32(args[0]);
                                                     }
                                                     if (radix < 2 || radix > 36)
                                                     {
                                                         throw Exception::make_exception("toString() radix argument must be between 2 and 36", "RangeError");
                                                     }

                                                     return AnyValue::make_string(JsNumber::to_radix_string(self, radix)); },
                                                         "toString");
            return fn;
        }

        AnyValue &get_valueOf_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_number(thisVal.as_double()); },
                                                         "valueOf");
            return fn;
        }

        AnyValue &get_toLocaleString_fn()
        {
            static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
                                                         { return AnyValue::make_string(AnyValue::make_number(thisVal.as_double()).to_std_string()); },
                                                         "toLocaleString");
            return fn;
        }

        std::optional<AnyValue> get(const std::string &key)
        {
            if (key == "toExponential")
                return get_toExponential_fn();
            if (key == "toFixed")
                return get_toFixed_fn();
            if (key == "toPrecision")
                return get_toPrecision_fn();
            if (key == "toString")
                return get_toString_fn();
            if (key == "valueOf")
                return get_valueOf_fn();
            if (key == "toLocaleString")
                return get_toLocaleString_fn();
            return std::nullopt;
        }

        std::optional<AnyValue> get(const AnyValue &key)
        {
            if (key == "toString" || key == AnyValue::from_symbol(WellKnownSymbols::toStringTag))
                return get_toString_fn();
            if (key == "valueOf")
                return get_valueOf_fn();
            return std::nullopt;
        }

    } // namespace NumberPrototypes

} // namespace jspp
