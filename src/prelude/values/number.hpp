#pragma once

#include "types.hpp"

#include <sstream>
#include <iomanip>

namespace jspp
{
    struct JsNumber
    {
        double value;

        inline std::string to_std_string() const
        {
            if (std::abs(value) >= 1e21 || (std::abs(value) > 0 && std::abs(value) < 1e-6))
            {
                std::ostringstream oss;
                oss << std::scientific << std::setprecision(4) << value;
                return oss.str();
            }
            else
            {
                std::ostringstream oss;
                oss << std::setprecision(6) << std::fixed << value;
                std::string s = oss.str();
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.')
                {
                    s.pop_back();
                }
                return s;
            }
        }

        // AnyValue &operator[](const AnyValue &_)
        // {
        //     return AnyValue{NonValues::undefined};
        // }
    };
}
