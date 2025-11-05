#pragma once

#include "types.hpp"
#include "object.hpp"

inline auto console = jspp::Object::make_object({
    {"log", jspp::Object::make_function([](const std::vector<jspp::AnyValue> &args)
                                        {
        for (size_t i = 0; i < args.size(); ++i) {
            std::cout << jspp::Log::to_log_string(args[i]);
            if (i < args.size() - 1) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
        return undefined; })},

    {"warn", jspp::Object::make_function([](const std::vector<jspp::AnyValue> &args)
                                         {
        std::cerr << "\033[33m"; // Yellow
        for (const auto& arg : args) {
            std::cerr << arg << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined; })},

    {"error", jspp::Object::make_function([](const std::vector<jspp::AnyValue> &args)
                                          {
        std::cerr << "\033[31m"; // Red
        for (const auto& arg : args) {
            std::cerr << arg << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined; })},
});
