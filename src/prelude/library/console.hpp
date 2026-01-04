#pragma once

#include <chrono>
#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "utils/operators.hpp"
#include "exception.hpp"
#include "utils/log_any_value/log_any_value.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

static std::map<std::string, std::chrono::steady_clock::time_point> timers = {};

auto logFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args)
                                           {
                                               for (size_t i = 0; i < args.size(); ++i)
                                               {
                                                   std::cout << jspp::LogAnyValue::to_log_string(args[i]);
                                                   if (i < args.size() - 1)
                                                       std::cout << " ";
                                               }
                                               std::cout << "\n" << std::flush;
                                               return jspp::Constants::UNDEFINED; }, "log");
auto warnFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args)
                                            {
                                                std::cerr << "\033[33m";
                                                for (size_t i = 0; i < args.size(); ++i)
                                                {
                                                    std::cout << jspp::LogAnyValue::to_log_string(args[i]);
                                                    if (i < args.size() - 1)
                                                        std::cout << " ";
                                                }
                                                std::cerr << "\033[0m" << "\n" << std::flush; // reset
                                                return jspp::Constants::UNDEFINED; }, "warn");
auto errorFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args)
                                             {
                                                 std::cerr << "\033[31m";
                                                 for (size_t i = 0; i < args.size(); ++i)
                                                 {
                                                     std::cout << jspp::LogAnyValue::to_log_string(args[i]);
                                                     if (i < args.size() - 1)
                                                         std::cout << " ";
                                                 }
                                                 std::cerr << "\033[0m" << "\n" << std::flush; // reset
                                                 return jspp::Constants::UNDEFINED; }, "error");
auto timeFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args)
                                            {
                                                auto start = std::chrono::steady_clock::now(); // capture immediately
                                                auto key_str = args.size() > 0 ? args[0].to_std_string() : "default";
                                                timers[key_str] = start;
                                                return jspp::Constants::UNDEFINED; }, "time");

// helper to format duration in ms -> ms/s/m/h with nice precision
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
    { // less than a minute -> show seconds
        ss << std::fixed << std::setprecision(4) << total_s << "s";
        return ss.str();
    }
    if (ms < 3600000.0)
    { // less than an hour -> show minutes + seconds
        int minutes = static_cast<int>(ms / 60000.0);
        double seconds = (ms - minutes * 60000.0) / 1000.0;
        ss << minutes << "m " << std::fixed << std::setprecision(4) << seconds << "s";
        return ss.str();
    }
    // hours, minutes, seconds
    int hours = static_cast<int>(ms / 3600000.0);
    int minutes = static_cast<int>((ms - hours * 3600000.0) / 60000.0);
    double seconds = (ms - hours * 3600000.0 - minutes * 60000.0) / 1000.0;
    ss << hours << "h " << minutes << "m " << std::fixed << std::setprecision(4) << seconds << "s";
    return ss.str();
};

auto timeEndFn = jspp::AnyValue::make_function([](const jspp::AnyValue &thisVal, std::span<const jspp::AnyValue> args)
                                               {
                                                   auto end = std::chrono::steady_clock::now(); // capture immediately
                                                   auto key_str = args.size() > 0 ? args[0].to_std_string() : "default";
                                                   auto it = timers.find(key_str);
                                                   if (it != timers.end())
                                                   {
                                                       std::chrono::duration<double, std::milli> duration = end - it->second;
                                                       double ms = duration.count();
                                                       std::string formatted = format_duration(ms);
                                                       std::cout << "\033[90m" << "[" << format_duration(ms) << "] " << "\033[0m" << key_str << "\n";
                                                       timers.erase(it); // remove the timer after ending it
                                                   }
                                                   else
                                                   {
                                                       std::cout << "Timer '" << key_str << "' does not exist." << "\n";
                                                   }
                                                   return jspp::Constants::UNDEFINED; }, "timeEnd");

inline auto console = jspp::AnyValue::make_object({
    {"log", logFn},
    {"warn", warnFn},
    {"error", errorFn},
    {"time", timeFn},
    {"timeEnd", timeEndFn},
});