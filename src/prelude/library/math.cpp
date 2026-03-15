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
            auto defMutable = [](const std::string& key, AnyValue val) {
                jspp::Math.define_data_property(key, val, true, false, true);
            };

            defConst("E", std::numbers::e);
            defConst("LN10", std::numbers::ln10);
            defConst("LN2", std::numbers::ln2);
            defConst("LOG10E", std::numbers::log10e);
            defConst("LOG2E", std::numbers::log2e);
            defConst("PI", std::numbers::pi);
            defConst("SQRT1_2", std::numbers::sqrt2 / 2.0);
            defConst("SQRT2", std::numbers::sqrt2);

            defMutable("abs", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return AnyValue::make_number(std::abs(GetArgAsDouble(args, 0)));
            }, "abs"));

            defMutable("acos", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::acos);
            }, "acos"));

            defMutable("acosh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::acosh);
            }, "acosh"));

            defMutable("asin", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::asin);
            }, "asin"));

            defMutable("asinh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::asinh);
            }, "asinh"));

            defMutable("atan", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::atan);
            }, "atan"));

            defMutable("atan2", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc2(args, std::atan2);
            }, "atan2"));

            defMutable("atanh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::atanh);
            }, "atanh"));

            defMutable("cbrt", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::cbrt);
            }, "cbrt"));

            defMutable("ceil", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::ceil);
            }, "ceil"));

            defMutable("clz32", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                uint32_t val = Operators_Private::ToInt32(args.empty() ? Constants::UNDEFINED : args[0]);
                if (val == 0) return AnyValue::make_number(32);
                return AnyValue::make_number(std::countl_zero(val));
            }, "clz32"));

            defMutable("cos", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::cos);
            }, "cos"));

            defMutable("cosh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::cosh);
            }, "cosh"));

            defMutable("exp", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::exp);
            }, "exp"));

            defMutable("expm1", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::expm1);
            }, "expm1"));

            defMutable("floor", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::floor);
            }, "floor"));

            defMutable("fround", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                return AnyValue::make_number(static_cast<double>(static_cast<float>(d)));
            }, "fround"));

            defMutable("hypot", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double result = 0;
                for (const auto& arg : args) {
                    double val = Operators_Private::ToNumber(arg);
                    if (std::isinf(val)) return AnyValue::make_number(std::numeric_limits<double>::infinity());
                    result = std::hypot(result, val);
                }
                return AnyValue::make_number(result);
            }, "hypot"));

            defMutable("imul", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                int32_t a = Operators_Private::ToInt32(args.empty() ? Constants::UNDEFINED : args[0]);
                int32_t b = Operators_Private::ToInt32(args.size() < 2 ? Constants::UNDEFINED : args[1]);
                return AnyValue::make_number(a * b);
            }, "imul"));

            defMutable("log", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log);
            }, "log"));

            defMutable("log10", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log10);
            }, "log10"));

            defMutable("log1p", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log1p);
            }, "log1p"));

            defMutable("log2", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::log2);
            }, "log2"));

            defMutable("max", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double maxVal = -std::numeric_limits<double>::infinity();
                for (const auto& arg : args) {
                    double val = Operators_Private::ToNumber(arg);
                    if (std::isnan(val)) return AnyValue::make_nan();
                    if (val > maxVal) maxVal = val;
                }
                return AnyValue::make_number(maxVal);
            }, "max"));

            defMutable("min", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double minVal = std::numeric_limits<double>::infinity();
                for (const auto& arg : args) {
                    double val = Operators_Private::ToNumber(arg);
                    if (std::isnan(val)) return AnyValue::make_nan();
                    if (val < minVal) minVal = val;
                }
                return AnyValue::make_number(minVal);
            }, "min"));

            defMutable("pow", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc2(args, std::pow);
            }, "pow"));

            defMutable("random", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return AnyValue::make_number(dis(gen));
            }, "random"));

            defMutable("round", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                if (std::isnan(d)) return AnyValue::make_nan();
                return AnyValue::make_number(std::floor(d + 0.5)); 
            }, "round"));

            defMutable("sign", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                if (std::isnan(d)) return AnyValue::make_nan();
                if (d == 0) return AnyValue::make_number(d);
                return AnyValue::make_number(d > 0 ? 1.0 : -1.0);
            }, "sign"));

            defMutable("sin", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::sin);
            }, "sin"));

            defMutable("sinh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::sinh);
            }, "sinh"));

            defMutable("sqrt", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::sqrt);
            }, "sqrt"));

            defMutable("tan", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::tan);
            }, "tan"));

            defMutable("tanh", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::tanh);
            }, "tanh"));

            defMutable("trunc", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                return MathFunc1(args, std::trunc);
            }, "trunc"));

            // Math.f16round(x)
            defMutable("f16round", AnyValue::make_function([](AnyValue, std::span<const AnyValue> args) -> AnyValue {
                double d = GetArgAsDouble(args, 0);
                #if defined(__STDCPP_FLOAT16_T__)
                                return AnyValue::make_number(static_cast<double>(static_cast<std::float16_t>(d)));
                #else
                                float f = static_cast<float>(d);
                                return AnyValue::make_number(static_cast<double>(f));
                #endif
            }, "f16round"));
            // Math.sumPrecise(iterable)
            defMutable("sumPrecise", AnyValue::make_function([](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue {
                if (args.empty()) throw Exception::make_exception("Math.sumPrecise requires an iterable", "TypeError");
                
                auto iterObj = jspp::Access::get_object_iterator(args[0], "iterable");
                auto nextFunc = iterObj.get_own_property("next");
                
                double sum = 0;
                // Kahan summation algorithm for better precision
                double c = 0; 
                
                while (true) {
                    auto nextRes = nextFunc.call(iterObj, std::span<const jspp::AnyValue>{}, "next");
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
        }
    };
    void init_math()
    {
        static MathInit mathInit;
    }
}
