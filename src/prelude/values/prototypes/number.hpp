#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "exception.hpp"
#include "utils/operators.hpp"
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace jspp
{
    namespace NumberPrototypes
    {
        // Helper for radix conversion
        inline std::string number_to_radix_string(double value, int radix)
        {
            if (std::isnan(value)) return "NaN";
            if (!std::isfinite(value)) return value > 0 ? "Infinity" : "-Infinity";

            // For radix != 10, we only support integer part for now
            // as implementing full float-to-radix is complex.
            long long intPart = static_cast<long long>(value);
            
            if (radix == 10) {
                return AnyValue::make_number(value).to_std_string();
            }

            bool negative = intPart < 0;
            if (negative) intPart = -intPart;
            
            std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
            std::string res = "";
            
            if (intPart == 0) res = "0";
            else {
                while (intPart > 0) {
                    res += chars[intPart % radix];
                    intPart /= radix;
                }
            }
            if (negative) res += "-";
            std::reverse(res.begin(), res.end());
            return res;
        }

        inline std::optional<AnyValue> get(const std::string &key, double self)
        {
            // --- toExponential(fractionDigits) ---
            if (key == "toExponential")
            {
                return AnyValue::make_function([self](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
                    int digits = -1;
                    if (!args.empty() && !args[0].is_undefined()) {
                        digits = Operators_Private::ToInt32(args[0]);
                        if (digits < 0 || digits > 100) {
                            throw Exception::make_exception("toExponential() digits argument must be between 0 and 100", "RangeError");
                        }
                    }

                    std::ostringstream ss;
                    if (digits >= 0) {
                        ss << std::scientific << std::setprecision(digits) << self;
                    } else {
                        ss << std::scientific << self;
                    }
                    
                    std::string res = ss.str();
                    return AnyValue::make_string(res);
                }, "toExponential");
            }

            // --- toFixed(digits) ---
            if (key == "toFixed")
            {
                return AnyValue::make_function([self](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
                    int digits = 0;
                    if (!args.empty() && !args[0].is_undefined()) {
                        digits = Operators_Private::ToInt32(args[0]);
                    }
                    if (digits < 0 || digits > 100) {
                        throw Exception::make_exception("toFixed() digits argument must be between 0 and 100", "RangeError");
                    }

                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(digits) << self;
                    return AnyValue::make_string(ss.str());
                }, "toFixed");
            }

            // --- toPrecision(precision) ---
            if (key == "toPrecision")
            {
                return AnyValue::make_function([self](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
                    if (args.empty() || args[0].is_undefined()) {
                        return AnyValue::make_number(self).get_own_property("toString").as_function()->call(AnyValue::make_number(self), {});
                    }
                    int precision = Operators_Private::ToInt32(args[0]);
                    if (precision < 1 || precision > 100) {
                        throw Exception::make_exception("toPrecision() precision argument must be between 1 and 100", "RangeError");
                    }

                    std::ostringstream ss;
                    ss << std::setprecision(precision) << self;
                    return AnyValue::make_string(ss.str());
                }, "toPrecision");
            }

            // --- toString(radix) ---
            if (key == "toString")
            {
                return AnyValue::make_function([self](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
                    int radix = 10;
                    if (!args.empty() && !args[0].is_undefined()) {
                        radix = Operators_Private::ToInt32(args[0]);
                    }
                    if (radix < 2 || radix > 36) {
                        throw Exception::make_exception("toString() radix argument must be between 2 and 36", "RangeError");
                    }
                    
                    return AnyValue::make_string(number_to_radix_string(self, radix));
                }, "toString");
            }

            // --- valueOf() ---
            if (key == "valueOf")
            {
                return AnyValue::make_function([self](const AnyValue&, std::span<const AnyValue>) -> AnyValue {
                    return AnyValue::make_number(self);
                }, "valueOf");
            }

            // --- toLocaleString() ---
            if (key == "toLocaleString")
            {
                return AnyValue::make_function([self](const AnyValue&, std::span<const AnyValue>) -> AnyValue {
                    return AnyValue::make_string(AnyValue::make_number(self).to_std_string());
                }, "toLocaleString");
            }

            return std::nullopt;
        }
    }
}