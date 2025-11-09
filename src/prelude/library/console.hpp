#pragma once

#include "chrono"
#include "types.hpp"
#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "operators.hpp"

static std::unordered_map<std::string, std::chrono::steady_clock::time_point> timers = {};

auto logFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue> &args)
                                           {
                                                                     for (size_t i = 0; i < args.size(); ++i)
                                                                     {
                                                                         std::cout << args[i].convert_to_raw_string();
                                                                         if (i < args.size() - 1)
                                                                             std::cout << " ";
                                                                     }
                                                                     std::cout << std::endl;
                                                                     return jspp::AnyValue::make_undefined(); }, "");
auto warnFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue> &args)
                                            {
                                                                      std::cerr << "\033[33m";
                                                                      for (size_t i = 0; i < args.size(); ++i)
                                                                      {
                                                                          std::cout << args[i].convert_to_raw_string();
                                                                          if (i < args.size() - 1)
                                                                              std::cout << " ";
                                                                      }
                                                                      std::cerr << "\033[0m" << std::endl; // reset
                                                                      return jspp::AnyValue::make_undefined(); }, "");
auto errorFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue> &args)
                                             {
                                                                       std::cerr << "\033[31m";
                                                                       for (size_t i = 0; i < args.size(); ++i)
                                                                       {
                                                                           std::cout << args[i].convert_to_raw_string();
                                                                           if (i < args.size() - 1)
                                                                               std::cout << " ";
                                                                       }
                                                                       std::cerr << "\033[0m" << std::endl; // reset
                                                                       return jspp::AnyValue::make_undefined(); }, "");
auto timeFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue> &args)
                                            {
                                                                      auto start = std::chrono::steady_clock::now(); // capture immediately
                                                                      auto key_str = args.size() > 0 ? args[0].convert_to_raw_string() : "";
                                                                      timers[key_str] = start;
                                                                      return jspp::AnyValue::make_undefined(); }, "");
auto timeEndFn = jspp::AnyValue::make_function([](const std::vector<jspp::AnyValue> &args)
                                               {
                                                                         auto end = std::chrono::steady_clock::now(); // capture immediately
                                                                         auto key_str = args.size() > 0 ? args[0].convert_to_raw_string() : "";
                                                                         auto it = timers.find(key_str);
                                                                         if (it != timers.end())
                                                                         {
                                                                             std::chrono::duration<double, std::milli> duration = end - it->second;
                                                                             std::cout << "\033[90m";
                                                                             std::cout << "[" << duration.count() << "ms] ";
                                                                             std::cout << "\033[0m" << key_str << std::endl;
                                                                             timers.erase(it); // remove the timer after ending it
                                                                         }
                                                                         else
                                                                         {
                                                                             std::cout << "Timer '" << key_str << "' does not exist." << std::endl;
                                                                         }
                                                                         return jspp::AnyValue::make_undefined(); }, "");

inline auto console = jspp::AnyValue::make_object({
    {"log", logFn},
    {"warn", warnFn},
    {"error", errorFn},
    {"time", timeFn},
    {"timeEnd", timeEndFn},
});