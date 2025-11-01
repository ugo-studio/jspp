#pragma once

#include "types.hpp"
#include "object.hpp"
#include "prototype.hpp"

inline auto console = jspp::Object::make_object({
    {"log", jspp::Object::make_function([](const std::vector<jspp::JsValue>& args) {
        for (const auto& arg : args) {
            std::cout << arg << " ";
        }
        std::cout << std::endl;
        return undefined;
    })},
    {"warn", jspp::Object::make_function([](const std::vector<jspp::JsValue>& args) {
        std::cerr << "\033[33m"; // Yellow
        for (const auto& arg : args) {
            std::cerr << arg << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    })},
    {"error", jspp::Object::make_function([](const std::vector<jspp::JsValue>& args) {
        std::cerr << "\033[31m"; // Red
        for (const auto& arg : args) {
            std::cerr << arg << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset
        return undefined;
    })}
});
