#include "index.hpp"

jspp::AnyValue __container__() {
  jspp::AnyValue __this_val__0 = global;
  auto funcWithReturn = std::make_shared<jspp::AnyValue>(jspp::AnyValue::make_uninitialized());
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Try-Catch-Finally ---")}); })();
  try {
    throw jspp::Exception(jspp::AnyValue::make_string("This is an error"));
  }
 catch (const std::exception& __caught_exception_1) {
    {
      jspp::AnyValue e = jspp::Exception::exception_to_any_value(__caught_exception_1);
      auto __caught_exception_1 = std::make_shared<jspp::AnyValue>(jspp::UNDEFINED);
{
        jspp::AnyValue ex = jspp::AnyValue::make_uninitialized();
        jspp::AnyValue __caught_exception = jspp::AnyValue::make_uninitialized();
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Caught error:"), (e)}); })();
        ex = jspp::AnyValue::make_string("This is a new variable");
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Inner variable:"), (ex)}); })();
        __caught_exception = jspp::AnyValue::make_string("This is another new variable");
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Assigned leaked variable:"), (__caught_exception)}); })();
      }
    }
  }
  try {
    throw jspp::Exception(jspp::AnyValue::make_string("This is an error"));
  }
 catch (const std::exception& __caught_exception_2) {
    {
      jspp::AnyValue __caught_exception = jspp::Exception::exception_to_any_value(__caught_exception_2);
      auto __caught_exception_2 = std::make_shared<jspp::AnyValue>(jspp::UNDEFINED);
{
        jspp::AnyValue ex = jspp::AnyValue::make_uninitialized();
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Caught error:"), (__caught_exception)}); })();
        ex = jspp::AnyValue::make_string("This is a new variable");
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Inner variable:"), (ex)}); })();
      }
    }
  }
  try {
    throw jspp::Exception(jspp::AnyValue::make_string("This is an error"));
  }
 catch (const std::exception& __caught_exception_3) {
{
    ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Caught error without exception variable")}); })();
  }
  }
  {
    jspp::AnyValue __try_result_5;
    bool __try_has_returned_6 = false;
    auto __finally_4 = [=]() {
      ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("This is the finally block")}); })();
    };
    try {
      __try_result_5 = ([=, &__try_has_returned_6]() -> jspp::AnyValue {
        try {
{
            throw jspp::Exception(jspp::AnyValue::make_string("This is an error"));
          }
        }
        catch (const std::exception& __caught_exception_7) {
{
            ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Caught error before finally")}); })();
          }
        }
        return jspp::UNDEFINED;
      })();
    } catch (...) {
      __finally_4();
      throw;
    }
    __finally_4();
    if (__try_has_returned_6) {
      return __try_result_5;
    }
  }
  try {
    {
      jspp::AnyValue __try_result_10;
      bool __try_has_returned_11 = false;
      auto __finally_9 = [=]() {
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("This is the finally block")}); })();
      };
      try {
        __try_result_10 = ([=, &__try_has_returned_11]() -> jspp::AnyValue {
          try {
{
              throw jspp::Exception(jspp::AnyValue::make_string("This is an error"));
            }
          }
          catch (...) { throw; }
          return jspp::UNDEFINED;
        })();
      } catch (...) {
        __finally_9();
        throw;
      }
      __finally_9();
      if (__try_has_returned_11) {
        return __try_result_10;
      }
    }
  }
 catch (const std::exception& __caught_exception_8) {
    {
      jspp::AnyValue e = jspp::Exception::exception_to_any_value(__caught_exception_8);
      auto __caught_exception_8 = std::make_shared<jspp::AnyValue>(jspp::UNDEFINED);
{
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("Caught propagated error:"), (e)}); })();
      }
    }
  }
  *funcWithReturn = jspp::AnyValue::make_function(std::function<jspp::AnyValue(const jspp::AnyValue&, const std::vector<jspp::AnyValue>&)>([=](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& __args_12) mutable -> jspp::AnyValue {
    {
      jspp::AnyValue __try_result_14;
      bool __try_has_returned_15 = false;
      auto __finally_13 = [=]() {
        ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("This is the finally block after a return statement")}); })();
      };
      try {
        __try_result_14 = ([=, &__try_has_returned_15]() -> jspp::AnyValue {
          try {
{
              __try_has_returned_15 = true;
              return jspp::AnyValue::make_string("This is the return value");
            }
          }
          catch (...) { throw; }
          return jspp::UNDEFINED;
        })();
      } catch (...) {
        __finally_13();
        throw;
      }
      __finally_13();
      if (__try_has_returned_15) {
        return __try_result_14;
      }
    }
    return jspp::UNDEFINED;
  }
), "funcWithReturn", false);
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {(*funcWithReturn).as_function("funcWithReturn")->call(jspp::UNDEFINED, {})}); })();
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