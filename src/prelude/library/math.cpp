#include "jspp.hpp"
#include "library/math.hpp"
#include <numbers>
#include <random>

namespace jspp {
    namespace Library {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);

        double GetArgAsDouble(std::span<const jspp::AnyValue> args, size_t index) {
            if (index >= args.size()) return std::numeric_limits<double>::quiet_NaN();
            return Operators_Private::ToNumber(args[index]);
        }

        jspp::AnyValue MathFunc1(std::span<const jspp::AnyValue> args, double (*func)(double)) {
            return jspp::AnyValue::make_number(func(GetArgAsDouble(args, 0)));
        }

        jspp::AnyValue MathFunc2(std::span<const jspp::AnyValue> args, double (*func)(double, double)) {
            return jspp::AnyValue::make_number(func(GetArgAsDouble(args, 0), GetArgAsDouble(args, 1)));
        }
    }

    jspp::AnyValue Math = jspp::AnyValue::make_object({});

    struct MathInit {
        MathInit() {
            using namespace jspp::Library;

            auto defConst = [](const std::string& key, double val) {
                jspp::Math.define_data_property(key, AnyValue::make_number(val), false, false, false);
            };

            defConst("E", std::numbers::e);
            defConst("LN10", std::numbers::ln10);
            defConst("LN2", std::numbers::ln2);
            defConst("LOG10E", std::numbers::log10e);
            defConst("LOG2E", std::numbers::log2e);
            defConst("PI", std::numbers::pi);
            defConst("SQRT1_2", std::numbers::sqrt2 / 2.0);
            defConst("SQRT2", std::numbers::sqrt2);

            jspp::Math.define_data_property("abs", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return AnyValue::make_number(std::abs(GetArgAsDouble(args, 0)));
            }, "abs"));

            jspp::Math.define_data_property("acos", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::acos);
            }, "acos"));

            jspp::Math.define_data_property("acosh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::acosh);
            }, "acosh"));

            jspp::Math.define_data_property("asin", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::asin);
            }, "asin"));

            jspp::Math.define_data_property("asinh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::asinh);
            }, "asinh"));

            jspp::Math.define_data_property("atan", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::atan);
            }, "atan"));

            jspp::Math.define_data_property("atan2", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc2(args, std::atan2);
            }, "atan2"));

            jspp::Math.define_data_property("atanh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::atanh);
            }, "atanh"));

            jspp::Math.define_data_property("cbrt", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::cbrt);
            }, "cbrt"));

            jspp::Math.define_data_property("ceil", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::ceil);
            }, "ceil"));

            jspp::Math.define_data_property("clz32", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                uint32_t val = Operators_Private::ToInt32(args.empty() ? Constants::UNDEFINED : args[0]);
                if (val == 0) return AnyValue::make_number(32);
                return AnyValue::make_number(std::countl_zero(val));
            }, "clz32"));

            jspp::Math.define_data_property("cos", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::cos);
            }, "cos"));

            jspp::Math.define_data_property("cosh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::cosh);
            }, "cosh"));

            jspp::Math.define_data_property("exp", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::exp);
            }, "exp"));

            jspp::Math.define_data_property("expm1", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::expm1);
            }, "expm1"));

            jspp::Math.define_data_property("floor", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::floor);
            }, "floor"));

            jspp::Math.define_data_property("fround", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                return AnyValue::make_number(static_cast<double>(static_cast<float>(d)));
            }, "fround"));

            jspp::Math.define_data_property("hypot", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double result = 0;
                for (const auto& arg : args) {
                    double val = Operators_Private::ToNumber(arg);
                    if (std::isinf(val)) return AnyValue::make_number(std::numeric_limits<double>::infinity());
                    result = std::hypot(result, val);
                }
                return AnyValue::make_number(result);
            }, "hypot"));

            jspp::Math.define_data_property("imul", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                int32_t a = Operators_Private::ToInt32(args.empty() ? Constants::UNDEFINED : args[0]);
                int32_t b = Operators_Private::ToInt32(args.size() < 2 ? Constants::UNDEFINED : args[1]);
                return AnyValue::make_number(a * b);
            }, "imul"));

            jspp::Math.define_data_property("log", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log);
            }, "log"));

            jspp::Math.define_data_property("log10", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log10);
            }, "log10"));

            jspp::Math.define_data_property("log1p", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log1p);
            }, "log1p"));

            jspp::Math.define_data_property("log2", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log2);
            }, "log2"));

            jspp::Math.define_data_property("max", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double maxVal = -std::numeric_limits<double>::infinity();
                for (const auto& arg : args) {
                    double val = Operators_Private::ToNumber(arg);
                    if (std::isnan(val)) return AnyValue::make_nan();
                    if (val > maxVal) maxVal = val;
                }
                return AnyValue::make_number(maxVal);
            }, "max"));

            jspp::Math.define_data_property("min", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double minVal = std::numeric_limits<double>::infinity();
                for (const auto& arg : args) {
                    double val = Operators_Private::ToNumber(arg);
                    if (std::isnan(val)) return AnyValue::make_nan();
                    if (val < minVal) minVal = val;
                }
                return AnyValue::make_number(minVal);
            }, "min"));

            jspp::Math.define_data_property("pow", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc2(args, std::pow);
            }, "pow"));

            jspp::Math.define_data_property("random", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return AnyValue::make_number(dis(gen));
            }, "random"));

            jspp::Math.define_data_property("round", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                if (std::isnan(d)) return AnyValue::make_nan();
                return AnyValue::make_number(std::floor(d + 0.5)); 
            }, "round"));

            jspp::Math.define_data_property("sign", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                if (std::isnan(d)) return AnyValue::make_nan();
                if (d == 0) return AnyValue::make_number(d);
                return AnyValue::make_number(d > 0 ? 1.0 : -1.0);
            }, "sign"));

            jspp::Math.define_data_property("sin", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::sin);
            }, "sin"));

            jspp::Math.define_data_property("sinh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::sinh);
            }, "sinh"));

            jspp::Math.define_data_property("sqrt", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::sqrt);
            }, "sqrt"));

            jspp::Math.define_data_property("tan", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::tan);
            }, "tan"));

            jspp::Math.define_data_property("tanh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::tanh);
            }, "tanh"));

            jspp::Math.define_data_property("trunc", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::trunc);
            }, "trunc"));
        }
    };
    static MathInit mathInit;
}
