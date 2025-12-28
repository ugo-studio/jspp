#pragma once

#include <string>

namespace jspp
{
    namespace LogAnyValue
    {
        // --- Configuration for Logging Verbosity ---
        const int MAX_DEPTH = 5;
        const size_t MAX_STRING_LENGTH = 100;
        const size_t MAX_ARRAY_ITEMS = 50;
        const size_t MAX_OBJECT_PROPS = 30;

        // --- Configuration for Horizontal Layout ---
        const size_t HORIZONTAL_ARRAY_MAX_ITEMS = 10;
        const size_t HORIZONTAL_OBJECT_MAX_PROPS = 5;

        // ANSI Color Codes for terminal output
        namespace Color
        {
            const std::string RESET = "\033[0m";
            const std::string RED = "\033[31m";
            const std::string GREEN = "\033[32m";
            const std::string YELLOW = "\033[33m";
            const std::string BLUE = "\033[94m";
            const std::string CYAN = "\033[36m";
            const std::string MAGENTA = "\033[35m";
            const std::string BRIGHT_BLACK = "\033[90m"; // Grey
        }
    }
}