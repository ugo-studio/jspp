#include "jspp.hpp"
#include "library/process.hpp"

#ifdef _WIN32
#define JSPP_PLATFORM "win32"
#else
#define JSPP_PLATFORM "linux"
#endif

namespace jspp {

AnyValue process = jspp::AnyValue::make_object({
    {"argv", jspp::AnyValue::make_array(std::vector<jspp::AnyValue>{})},
    {"env", jspp::AnyValue::make_object({})},
    {"platform", jspp::AnyValue::make_string(JSPP_PLATFORM)},
    {"exit", jspp::AnyValue::make_function([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue {
        int code = 0;
        if (!args.empty() && args[0].is_number()) {
            code = static_cast<int>(args[0].as_double());
        }
        std::exit(code);
        return jspp::Constants::UNDEFINED;
    }, "exit")}
});

void setup_process_argv(int argc, char** argv) {
    std::vector<jspp::AnyValue> args;
    if (argc > 0) {
        args.push_back(jspp::AnyValue::make_string(argv[0]));
        args.push_back(jspp::AnyValue::make_string("index.js"));
        for (int i = 1; i < argc; ++i) {
            args.push_back(jspp::AnyValue::make_string(argv[i]));
        }
    }
    process.set_own_property("argv", jspp::AnyValue::make_array(std::move(args)));
}

} // namespace jspp
