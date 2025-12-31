#pragma once

#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <bit>
#include <numbers>
#include <stdfloat>

#include "types.hpp"
#include "any_value.hpp"
#include "utils/operators.hpp"
#include "utils/access.hpp"

namespace jspp {
    namespace Library {
        // --- Random Number Generator ---
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);

        // --- Helpers ---
        inline double GetArgAsDouble(std::span<const jspp::AnyValue> args, size_t index) {
            if (index >= args.size()) return std::numeric_limits<double>::quiet_NaN();
            return Operators_Private::ToNumber(args[index]);
        }

        inline jspp::AnyValue MathFunc1(std::span<const jspp::AnyValue> args, double (*func)(double)) {
            return jspp::AnyValue::make_number(func(GetArgAsDouble(args, 0)));
        }

        inline jspp::AnyValue MathFunc2(std::span<const jspp::AnyValue> args, double (*func)(double, double)) {
            return jspp::AnyValue::make_number(func(GetArgAsDouble(args, 0), GetArgAsDouble(args, 1)));
        }
    }
}

inline auto Math = jspp::AnyValue::make_object({});

struct MathInit {
    MathInit() {
        using namespace jspp;
        using namespace jspp::Library;

        // --- Constants ---
        auto defConst = [](const std::string& key, double val) {
            Math.define_data_property(key, AnyValue::make_number(val), false, false, false);
        };

        defConst("E", std::numbers::e);
        defConst("LN10", std::numbers::ln10);
        defConst("LN2", std::numbers::ln2);
        defConst("LOG10E", std::numbers::log10e);
        defConst("LOG2E", std::numbers::log2e);
        defConst("PI", std::numbers::pi);
        defConst("SQRT1_2", std::numbers::sqrt2 / 2.0);
        defConst("SQRT2", std::numbers::sqrt2);

        // --- Methods ---
        
        // Math.abs(x)
        Math.define_data_property("abs", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return AnyValue::make_number(std::abs(GetArgAsDouble(args, 0)));
        }, "abs"));

        // Math.acos(x)
        Math.define_data_property("acos", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::acos);
        }, "acos"));

        // Math.acosh(x)
        Math.define_data_property("acosh", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::acosh);
        }, "acosh"));

        // Math.asin(x)
        Math.define_data_property("asin", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::asin);
        }, "asin"));

        // Math.asinh(x)
        Math.define_data_property("asinh", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::asinh);
        }, "asinh"));

        // Math.atan(x)
        Math.define_data_property("atan", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::atan);
        }, "atan"));

        // Math.atan2(y, x)
        Math.define_data_property("atan2", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc2(args, std::atan2);
        }, "atan2"));

        // Math.atanh(x)
        Math.define_data_property("atanh", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::atanh);
        }, "atanh"));

        // Math.cbrt(x)
        Math.define_data_property("cbrt", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::cbrt);
        }, "cbrt"));

        // Math.ceil(x)
        Math.define_data_property("ceil", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::ceil);
        }, "ceil"));

        // Math.clz32(x)
        Math.define_data_property("clz32", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            uint32_t val = Operators_Private::ToInt32(args.empty() ? AnyValue::make_undefined() : args[0]);
            if (val == 0) return AnyValue::make_number(32);
            return AnyValue::make_number(std::countl_zero(val));
        }, "clz32"));

        // Math.cos(x)
        Math.define_data_property("cos", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::cos);
        }, "cos"));

        // Math.cosh(x)
        Math.define_data_property("cosh", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::cosh);
        }, "cosh"));

        // Math.exp(x)
        Math.define_data_property("exp", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::exp);
        }, "exp"));

        // Math.expm1(x)
        Math.define_data_property("expm1", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::expm1);
        }, "expm1"));

        // Math.f16round(x)
        Math.define_data_property("f16round", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double d = GetArgAsDouble(args, 0);
#if defined(__STDCPP_FLOAT16_T__)
            return AnyValue::make_number(static_cast<double>(static_cast<std::float16_t>(d)));
#else
            // Manual fallback if float16_t is not available
            // This is a very rough approximation, actual f16round is more complex
            float f = static_cast<float>(d);
            return AnyValue::make_number(static_cast<double>(f));
#endif
        }, "f16round"));

        // Math.floor(x)
        Math.define_data_property("floor", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::floor);
        }, "floor"));

        // Math.fround(x)
        Math.define_data_property("fround", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double d = GetArgAsDouble(args, 0);
            return AnyValue::make_number(static_cast<double>(static_cast<float>(d)));
        }, "fround"));

        // Math.hypot(...args)
        Math.define_data_property("hypot", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double result = 0;
            for (const auto& arg : args) {
                double val = Operators_Private::ToNumber(arg);
                if (std::isinf(val)) return AnyValue::make_number(std::numeric_limits<double>::infinity());
                result = std::hypot(result, val);
            }
            return AnyValue::make_number(result);
        }, "hypot"));

        // Math.imul(x, y)
        Math.define_data_property("imul", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            int32_t a = Operators_Private::ToInt32(args.empty() ? AnyValue::make_undefined() : args[0]);
            int32_t b = Operators_Private::ToInt32(args.size() < 2 ? AnyValue::make_undefined() : args[1]);
            return AnyValue::make_number(a * b);
        }, "imul"));

        // Math.log(x)
        Math.define_data_property("log", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::log);
        }, "log"));

        // Math.log10(x)
        Math.define_data_property("log10", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::log10);
        }, "log10"));

        // Math.log1p(x)
        Math.define_data_property("log1p", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::log1p);
        }, "log1p"));

        // Math.log2(x)
        Math.define_data_property("log2", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::log2);
        }, "log2"));

        // Math.max(...args)
        Math.define_data_property("max", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double maxVal = -std::numeric_limits<double>::infinity();
            for (const auto& arg : args) {
                double val = Operators_Private::ToNumber(arg);
                if (std::isnan(val)) return AnyValue::make_nan();
                if (val > maxVal) maxVal = val;
            }
            return AnyValue::make_number(maxVal);
        }, "max"));

        // Math.min(...args)
        Math.define_data_property("min", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double minVal = std::numeric_limits<double>::infinity();
            for (const auto& arg : args) {
                double val = Operators_Private::ToNumber(arg);
                if (std::isnan(val)) return AnyValue::make_nan();
                if (val < minVal) minVal = val;
            }
            return AnyValue::make_number(minVal);
        }, "min"));

        // Math.pow(base, exponent)
        Math.define_data_property("pow", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc2(args, std::pow);
        }, "pow"));

        // Math.random()
        Math.define_data_property("random", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return AnyValue::make_number(dis(gen));
        }, "random"));

        // Math.round(x)
        Math.define_data_property("round", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double d = GetArgAsDouble(args, 0);
            if (std::isnan(d)) return AnyValue::make_nan();
            return AnyValue::make_number(std::floor(d + 0.5)); 
        }, "round"));

        // Math.sign(x)
        Math.define_data_property("sign", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            double d = GetArgAsDouble(args, 0);
            if (std::isnan(d)) return AnyValue::make_nan();
            if (d == 0) return AnyValue::make_number(d);
            return AnyValue::make_number(d > 0 ? 1.0 : -1.0);
        }, "sign"));

        // Math.sin(x)
        Math.define_data_property("sin", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::sin);
        }, "sin"));

        // Math.sinh(x)
        Math.define_data_property("sinh", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::sinh);
        }, "sinh"));

        // Math.sqrt(x)
        Math.define_data_property("sqrt", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::sqrt);
        }, "sqrt"));

        // Math.sumPrecise(iterable)
        Math.define_data_property("sumPrecise", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            if (args.empty()) throw Exception::make_exception("Math.sumPrecise requires an iterable", "TypeError");
            
            auto iterObj = jspp::Access::get_object_value_iterator(args[0], "iterable");
            auto nextFunc = iterObj.get_own_property("next").as_function();
            
            double sum = 0;
            // Kahan summation algorithm for better precision
            double c = 0; 
            
            while (true) {
                auto nextRes = nextFunc->call(iterObj, std::span<const jspp::AnyValue>{});
                if (is_truthy(nextRes.get_own_property("done"))) break;
                
                double val = Operators_Private::ToNumber(nextRes.get_own_property("value"));
                if (std::isnan(val)) {
                    sum = std::numeric_limits<double>::quiet_NaN();
                    break;
                }
                
                double y = val - c;
                double t = sum + y;
                c = (t - sum) - y;
                sum = t;
            }
            
            return AnyValue::make_number(sum);
        }, "sumPrecise"));

        // Math.tan(x)
        Math.define_data_property("tan", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::tan);
        }, "tan"));

        // Math.tanh(x)
        Math.define_data_property("tanh", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::tanh);
        }, "tanh"));

        // Math.trunc(x)
        Math.define_data_property("trunc", AnyValue::make_function([](const AnyValue&, std::span<const AnyValue> args) -> AnyValue {
            return MathFunc1(args, std::trunc);
        }, "trunc"));
    }
} mathInit;