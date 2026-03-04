#include "jspp.hpp"
#include "library/timer.hpp"

namespace jspp
{

    AnyValue setTimeout = jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                        {
    if (args.empty() || !args[0].is_function()) {
        throw jspp::Exception::make_exception("Callback must be a function", "TypeError");
    }
    
    auto callback = args[0];
    double delay = 0;
    if (args.size() > 1 && args[1].is_number()) {
        delay = args[1].as_double();
    }
    
    // Capture arguments
    std::vector<jspp::AnyValue> callArgs;
    for (size_t i = 2; i < args.size(); ++i) {
        callArgs.push_back(args[i]);
    }

    auto task = [callback, callArgs]() {
         try {
             callback.call(jspp::Constants::UNDEFINED, std::span<const jspp::AnyValue>(callArgs));
         } catch (const jspp::Exception& e) {
             std::cerr << "Uncaught exception in setTimeout: " << e.what() << "\n";
         } catch (const std::exception& e) {
             std::cerr << "Uncaught exception in setTimeout: " << e.what() << "\n";
         } catch (...) {
             std::cerr << "Uncaught unknown exception in setTimeout\n";
         }
    };

    size_t id = jspp::Scheduler::instance().set_timeout(task, static_cast<size_t>(delay));
    return jspp::AnyValue::make_number(static_cast<double>(id)); }, "setTimeout");

    AnyValue clearTimeout = jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                          {
    if (!args.empty() && args[0].is_number()) {
        size_t id = static_cast<size_t>(args[0].as_double());
        jspp::Scheduler::instance().clear_timer(id);
    }
    return jspp::Constants::UNDEFINED; }, "clearTimeout");

    AnyValue setInterval = jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                         {
    if (args.empty() || !args[0].is_function()) {
        throw jspp::Exception::make_exception("Callback must be a function", "TypeError");
    }
    
    auto callback = args[0];
    double delay = 0;
    if (args.size() > 1 && args[1].is_number()) {
        delay = args[1].as_double();
    }
    
    std::vector<jspp::AnyValue> callArgs;
    for (size_t i = 2; i < args.size(); ++i) {
        callArgs.push_back(args[i]);
    }

    auto task = [callback, callArgs]() {
         try {
             callback.call(jspp::Constants::UNDEFINED, std::span<const jspp::AnyValue>(callArgs));
         } catch (const jspp::Exception& e) {
             std::cerr << "Uncaught exception in setInterval: " << e.what() << "\n";
         } catch (const std::exception& e) {
             std::cerr << "Uncaught exception in setInterval: " << e.what() << "\n";
         } catch (...) {
             std::cerr << "Uncaught unknown exception in setInterval\n";
         }
    };

    size_t id = jspp::Scheduler::instance().set_interval(task, static_cast<size_t>(delay));
    return jspp::AnyValue::make_number(static_cast<double>(id)); }, "setInterval");

    AnyValue clearInterval = jspp::AnyValue::make_function([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                                                           {
    if (!args.empty() && args[0].is_number()) {
        size_t id = static_cast<size_t>(args[0].as_double());
        jspp::Scheduler::instance().clear_timer(id);
    }
    return jspp::Constants::UNDEFINED; }, "clearInterval");

} // namespace jspp
