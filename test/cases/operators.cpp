#include "index.hpp"

jspp::AnyValue __container__() {
  jspp::AnyValue __this_val__0 = global;
  jspp::AnyValue a = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue b = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue c = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue d = jspp::AnyValue::make_uninitialized();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Operators ---")}); })();
  a = 10;
  b = 5;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a:"), (a), jspp::AnyValue::make_string("b:"), (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a + b ="), ((a) + (b))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a - b ="), ((a) - (b))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a * b ="), ((a) * (b))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a / b ="), ((a) / (b))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a % b ="), ((a) % (b))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a ** 2 ="), jspp::pow((a), 2)}); })();
  (a)++;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a++:"), (a)}); })();
  (b)--;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("b--:"), (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("++a:"), ++(a)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--b:"), --(b)}); })();
  a += 5;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a += 5:"), (a)}); })();
  a -= 5;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a -= 5:"), (a)}); })();
  a *= 2;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a *= 2:"), (a)}); })();
  a /= 2;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a /= 2:"), (a)}); })();
  a %= 3;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a %= 3:"), (a)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a > b:"), (a) > (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a < b:"), (a) < (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a >= b:"), (a) >= (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a <= b:"), (a) <= (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a != b:"), jspp::not_equal_to((a), (b))}); })();
  c = 5;
  d = 3;
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("c & d:"), ((c) & (d))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("c | d:"), ((c) | (d))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("c ^ d:"), ((c) ^ (d))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("~c:"), ~(c)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("c << 1:"), ((c) << 1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("c >> 1:"), ((c) >> 1)}); })();
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