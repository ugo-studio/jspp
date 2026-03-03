#include "jspp.hpp"
#include "library/console.hpp"
#include <chrono>
#include <map>

static std::map<std::string, std::chrono::steady_clock::time_point> timers = {};

namespace jspp {
    jspp::AnyValue logFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args)
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::cout << jspp::LogAnyValue::to_log_string(args[i]);
                if (i < args.size() - 1)
                    std::cout << " ";
            }
            std::cout << "\n" << std::flush;
            return jspp::Constants::UNDEFINED;
        }), "log");

    jspp::AnyValue warnFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args)
        {
            std::cerr << "\033[33m";
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::cout << jspp::LogAnyValue::to_log_string(args[i]);
                if (i < args.size() - 1)
                    std::cout << " ";
            }
            std::cerr << "\033[0m" << "\n" << std::flush;
            return jspp::Constants::UNDEFINED;
        }), "warn");

    jspp::AnyValue errorFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args)
        {
            std::cerr << "\033[31m";
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::cout << jspp::LogAnyValue::to_log_string(args[i]);
                if (i < args.size() - 1)
                    std::cout << " ";
            }
            std::cerr << "\033[0m" << "\n" << std::flush;
            return jspp::Constants::UNDEFINED;
        }), "error");

    jspp::AnyValue timeFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args)
        {
            auto start = std::chrono::steady_clock::now();
            auto key_str = args.size() > 0 ? args[0].to_std_string() : "default";
            timers[key_str] = start;
            return jspp::Constants::UNDEFINED;
        }), "time");

    static auto format_duration = [](double ms) -> std::string
    {
        std::ostringstream ss;
        if (ms < 1000.0)
        {
            ss << std::fixed << std::setprecision(4) << ms << "ms";
            return ss.str();
        }
        double total_s = ms / 1000.0;
        if (ms < 60000.0)
        {
            ss << std::fixed << std::setprecision(4) << total_s << "s";
            return ss.str();
        }
        if (ms < 3600000.0)
        {
            int minutes = static_cast<int>(ms / 60000.0);
            double seconds = (ms - minutes * 60000.0) / 1000.0;
            ss << minutes << "m " << std::fixed << std::setprecision(4) << seconds << "s";
            return ss.str();
        }
        int hours = static_cast<int>(ms / 3600000.0);
        int minutes = static_cast<int>((ms - hours * 3600000.0) / 60000.0);
        double seconds = (ms - hours * 3600000.0 - minutes * 60000.0) / 1000.0;
        ss << hours << "h " << minutes << "m " << std::fixed << std::setprecision(4) << seconds << "s";
        return ss.str();
    };

    jspp::AnyValue timeEndFn = jspp::AnyValue::make_function(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args)
        {
            auto end = std::chrono::steady_clock::now();
            auto key_str = args.size() > 0 ? args[0].to_std_string() : "default";
            auto it = timers.find(key_str);
            if (it != timers.end())
            {
                std::chrono::duration<double, std::milli> duration = end - it->second;
                double ms = duration.count();
                std::cout << "\033[90m" << "[" << format_duration(ms) << "] " << "\033[0m" << key_str << "\n";
                timers.erase(it);
            }
            else
            {
                std::cout << "Timer '" << key_str << "' does not exist.\n";
            }
            return jspp::Constants::UNDEFINED;
        }), "timeEnd");

    jspp::AnyValue console = jspp::AnyValue::make_object({
        {"log", logFn},
        {"warn", warnFn},
        {"error", errorFn},
        {"time", timeFn},
        {"timeEnd", timeEndFn},
    });
}
