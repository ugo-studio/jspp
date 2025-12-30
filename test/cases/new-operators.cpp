#include "index.hpp"

jspp::AnyValue __container__() {
  jspp::AnyValue __this_val__0 = global;
  auto MyClass = std::make_shared<jspp::AnyValue>(jspp::AnyValue::make_uninitialized());
  jspp::AnyValue a = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue b = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue obj = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue arr = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue myInstance = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue delObj = jspp::AnyValue::make_uninitialized();
  jspp::AnyValue delArr = jspp::AnyValue::make_uninitialized();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- New Operators Test ---")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Bitwise ---")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("5 ^ 3:"), jspp::AnyValue::make_number(5 ^ 3)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("5 & 3:"), jspp::AnyValue::make_number(5 & 3)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("5 | 3:"), jspp::AnyValue::make_number(5 | 3)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("5 << 1:"), jspp::AnyValue::make_number(5 << 1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("5 >> 1:"), jspp::AnyValue::make_number(5 >> 1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("5 >>> 1:"), jspp::unsigned_right_shift(5, 1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("-5 >>> 1:"), jspp::unsigned_right_shift(-jspp::AnyValue::make_number(5), 1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Logical Assignment ---")}); })();
  a = 1;
  jspp::logical_and_assign(a, [&](){ return jspp::AnyValue::make_number(2); });
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a &&= 2:"), (a)}); })();
  jspp::logical_and_assign(a, [&](){ return jspp::ZERO; });
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a &&= 0:"), (a)}); })();
  jspp::logical_or_assign(a, [&](){ return jspp::AnyValue::make_number(3); });
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("a ||= 3:"), (a)}); })();
  b = jspp::Null;
  jspp::nullish_coalesce_assign(b, [&](){ return jspp::AnyValue::make_number(4); });
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("b ??= 4:"), (b)}); })();
  jspp::nullish_coalesce_assign(b, [&](){ return jspp::AnyValue::make_number(5); });
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("b ??= 5:"), (b)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Optional Chaining ---")}); })();
  obj = ([&]() {
    auto __obj_1 = jspp::AnyValue::make_object({});
    __obj_1.define_data_property("prop", ([&]() {
      auto __obj_2 = jspp::AnyValue::make_object({});
      __obj_2.define_data_property("val", jspp::AnyValue::make_number(10));
      return __obj_2;
    })());
    __obj_1.define_data_property("method", jspp::AnyValue::make_function(std::function<jspp::AnyValue(const jspp::AnyValue&, const std::vector<jspp::AnyValue>&)>([=](const jspp::AnyValue& __this_val__0, const std::vector<jspp::AnyValue>& __args_3) mutable -> jspp::AnyValue {
      return jspp::AnyValue::make_string("method result");
    }
), "method", false));
    return __obj_1;
  })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("obj?.prop?.val:"), jspp::Access::optional_get_property(jspp::Access::optional_get_property((obj), "prop"), "val")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("obj?.nonExistent?.val:"), jspp::Access::optional_get_property(jspp::Access::optional_get_property((obj), "nonExistent"), "val")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("obj?.method?.():"), jspp::Access::optional_call((obj).get_own_property("method"), (obj), {}, "method")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("obj?.nonExistentMethod?.():"), jspp::Access::optional_call((obj).get_own_property("nonExistentMethod"), (obj), {}, "nonExistentMethod")}); })();
  arr = jspp::AnyValue::make_array({jspp::ONE, jspp::AnyValue::make_number(2), jspp::AnyValue::make_number(3)});
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("arr?.[1]:"), jspp::Access::optional_get_element((arr), 1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("arr?.[10]:"), jspp::Access::optional_get_element((arr), 10)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- In Operator ---")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("'prop' in obj:"), jspp::Access::in(jspp::AnyValue::make_string("prop"), (obj))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("'nonExistent' in obj:"), jspp::Access::in(jspp::AnyValue::make_string("nonExistent"), (obj))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("'length' in arr:"), jspp::Access::in(jspp::AnyValue::make_string("length"), (arr))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("'0' in arr:"), jspp::Access::in(jspp::AnyValue::make_string("0"), (arr))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Instanceof ---")}); })();
  *MyClass = jspp::AnyValue::make_class([=](const jspp::AnyValue& __this_val__0, const std::vector<jspp::AnyValue>& args) mutable -> jspp::AnyValue {
                 return jspp::UNDEFINED;
             }, "MyClass");
  myInstance = (*MyClass).construct({}, "MyClass");
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("myInstance instanceof MyClass:"), jspp::Access::instanceof((myInstance), (*MyClass))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("myInstance instanceof Object:"), jspp::Exception::throw_unresolved_reference("Object")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("[] instanceof Array:"), jspp::Exception::throw_unresolved_reference("Array")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("[] instanceof Object:"), jspp::Exception::throw_unresolved_reference("Object")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Unary Plus and Not ---")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("+'5':"), +jspp::AnyValue::make_string("5")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("+true:"), +jspp::TRUE}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("!true:"), !jspp::TRUE}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("!false:"), !jspp::FALSE}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("!0:"), !jspp::ZERO}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("!1:"), !jspp::ONE}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Nullish Coalescing ---")}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("null ?? 'default':"), jspp::nullish_coalesce(jspp::Null, [&](){ return jspp::AnyValue::make_string("default"); })}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("undefined ?? 'default':"), jspp::nullish_coalesce(jspp::UNDEFINED, [&](){ return jspp::AnyValue::make_string("default"); })}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("false ?? 'default':"), jspp::nullish_coalesce(jspp::FALSE, [&](){ return jspp::AnyValue::make_string("default"); })}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("0 ?? 'default':"), jspp::nullish_coalesce(0, [&](){ return jspp::AnyValue::make_string("default"); })}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("--- Delete ---")}); })();
  delObj = ([&]() {
    auto __obj_4 = jspp::AnyValue::make_object({});
    __obj_4.define_data_property("a", jspp::ONE);
    __obj_4.define_data_property("b", jspp::AnyValue::make_number(2));
    return __obj_4;
  })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("delete delObj.a:"), jspp::Access::delete_property(delObj, jspp::AnyValue::make_string("a"))}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("'a' in delObj:"), jspp::Access::in(jspp::AnyValue::make_string("a"), (delObj))}); })();
  delArr = jspp::AnyValue::make_array({jspp::ONE, jspp::AnyValue::make_number(2), jspp::AnyValue::make_number(3)});
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("delete delArr[1]:"), jspp::Access::delete_property(delArr, jspp::ONE)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("delArr[1]:"), (delArr).get_own_property(1)}); })();
  ([&](){ auto __obj = console; return __obj.get_own_property("log").as_function("log")->call(__obj, {jspp::AnyValue::make_string("delArr.length:"), (delArr).get_own_property("length")}); })();
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