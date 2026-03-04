#include "jspp.hpp"
#include "library/performance.hpp"
#include <chrono>

namespace jspp
{

    AnyValue performance = jspp::AnyValue::make_object({
        {"now",
         jspp::AnyValue::make_function(
             [startTime = std::chrono::steady_clock::now()](jspp::AnyValue, std::span<const jspp::AnyValue>)
             {
                 std::chrono::duration<double, std::milli> duration =
                     std::chrono::steady_clock::now() - startTime;

                 return jspp::AnyValue::make_number(duration.count());
             },
             "now")},
    });

} // namespace jspp
