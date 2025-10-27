#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <any>

struct Undefined {};
inline Undefined undefined;

struct Null {};
inline Null null;

using JsVariant = std::variant<Undefined, Null, bool, int, double, std::string, std::function<std::any()>>;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

inline std::ostream& operator<<(std::ostream& os, const JsVariant& v) {
    std::visit(overloaded {
        [&](const Undefined& arg) { os << "undefined"; },
        [&](const Null& arg) { os << "null"; },
        [&](bool arg) { os << std::boolalpha << arg; },
        [&](int arg) { os << arg; },
        [&](double arg) { os << arg; },
        [&](const std::string& arg) { os << arg; },
        [&](const std::function<std::any()>& arg) { os << "function"; },
        [&](const auto& arg) { os << "Unprintable"; }
    }, v);
    return os;
}

struct Console {
    template<typename... Args>
    Undefined log(Args... args) {
        ( (std::cout << args << " "), ... );
        std::cout << std::endl;
        return undefined;
    }

    template<typename... Args>
    Undefined warn(Args... args) {
        std::cerr << "Warning: ";
        ( (std::cerr << args << " "), ... );
        std::cerr << std::endl;
        return undefined;
    }

    template<typename... Args>
    Undefined error(Args... args) {
        std::cerr << "Error: ";
        ( (std::cerr << args << " "), ... );
        std::cerr << std::endl;
        return undefined;
    }
};

inline Console console;
