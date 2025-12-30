#include "index.hpp"

jspp::AnyValue __container__() {
  jspp::AnyValue __this_val__0 = global;
  jspp::AnyValue a = jspp::UNDEFINED;
  jspp::AnyValue b = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue c = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue d = jspp::AnyValue::make_uninitialized();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Declarations ---")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("- var -")}); })();
  a = 1;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(a)}); })();
  a = 2;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(a)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("- let -")}); })();
  b = 3;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(b)}); })();
  b = 4;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("- const -")}); })();
  c = 5;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(c)}); })();
  try {
    jspp::Exception::throw_immutable_assignment();
  }
 catch (const std::exception& __caught_exception_1) {
    {
      jspp::AnyValue e = jspp::Exception::exception_to_any_value(__caught_exception_1);
      auto __caught_exception_1 = std::make_shared<jspp::AnyValue>(jspp::UNDEFINED);
{
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Caught expected error")}); })();
      }
    }
  }
  d = ((c) + 1);
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(d)}); })();
  return jspp::UNDEFINED;
}

int main() {
  try {
    __container__();
    jspp::Scheduler::instance().run();
  } catch (const std::exception& ex) {
    auto error = std::make_shared<jspp::AnyValue>(jspp::Exception::exception_to_any_value(ex));
{
    ([&](){ auto __obj = console; return __obj.get_own_property("error").as_function("console.error")->call(__obj, {*error}); })();
    return 1;
}
  }
  return 0;
}