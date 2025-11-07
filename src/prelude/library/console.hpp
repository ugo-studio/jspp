#pragma once

#include "chrono"
#include "types.hpp"
#include "values/non-values.hpp"
#include "values/object.hpp"
#include "values/function.hpp"
#include "convert.hpp"
#include "operators.hpp"

static std::unordered_map<std::string, std::chrono::steady_clock::time_point> timers = {};

inline auto console = jspp::JsObject{{
    {"log", std::make_shared<jspp::JsFunction>(jspp::JsFunction{[](const std::vector<jspp::AnyValue> &args)
                                                                {
                                                                    for (size_t i = 0; i < args.size(); ++i)
                                                                    {
                                                                        std::cout << jspp::Convert::to_string(args[i]);
                                                                        if (i < args.size() - 1)
                                                                            std::cout << " ";
                                                                    }
                                                                    std::cout << std::endl;
                                                                    return jspp::NonValues::undefined;
                                                                }})},

    {"warn", std::make_shared<jspp::JsFunction>(jspp::JsFunction{[](const std::vector<jspp::AnyValue> &args)
                                                                 {
                                                                     std::cerr << "\033[33m";
                                                                     for (const auto &arg : args)
                                                                         std::cerr << arg << " ";
                                                                     std::cerr << "\033[0m" << std::endl; // reset
                                                                     return jspp::NonValues::undefined;
                                                                 }})},

    {"error", std::make_shared<jspp::JsFunction>(jspp::JsFunction{[](const std::vector<jspp::AnyValue> &args)
                                                                  {
                                                                      std::cerr << "\033[31m";
                                                                      for (const auto &arg : args)
                                                                          std::cerr << arg << " ";
                                                                      std::cerr << "\033[0m" << std::endl; // reset
                                                                      return jspp::NonValues::undefined;
                                                                  }})},

    {"time", std::make_shared<jspp::JsFunction>(jspp::JsFunction{[](const std::vector<jspp::AnyValue> &args)
                                                                 {
                                                                     auto start = std::chrono::steady_clock::now(); // capture immediately
                                                                     auto key_str = args.size() > 0 ? jspp::Convert::to_string(args[0]) : "";
                                                                     timers[key_str] = start;
                                                                     return jspp::NonValues::undefined;
                                                                 }})},

    {"timeEnd", std::make_shared<jspp::JsFunction>(jspp::JsFunction{[](const std::vector<jspp::AnyValue> &args)
                                                                    {
                                                                        auto end = std::chrono::steady_clock::now(); // capture immediately
                                                                        auto key_str = args.size() > 0 ? jspp::Convert::to_string(args[0]) : "";
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
                                                                        return jspp::NonValues::undefined;
                                                                    }})},
}};