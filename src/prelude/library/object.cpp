#include "jspp.hpp"
#include "library/object.hpp"

namespace jspp {
    jspp::AnyValue Object = jspp::AnyValue::make_class(
        std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
    {
        if (args.empty() || args[0].is_undefined() || args[0].is_null()) {
            return jspp::AnyValue::make_object({});
        }
        if (args[0].is_object() || args[0].is_array() || args[0].is_function() || args[0].is_promise() || args[0].is_iterator()) {
            return args[0];
        }
        return jspp::AnyValue::make_object({});
    }), "Object");

    struct ObjectInit
    {
        ObjectInit()
        {
            Object.define_data_property("keys", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Object.keys called on non-object", "TypeError");
                auto obj = args[0];
                if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.keys called on null or undefined", "TypeError");
                
                auto keys = jspp::Access::get_object_keys(obj);
                std::vector<jspp::AnyValue> keyValues;
                for(const auto& k : keys) {
                    keyValues.push_back(k);
                }
                return jspp::AnyValue::make_array(std::move(keyValues));
            }), "keys"));

            Object.define_data_property("values", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Object.values called on non-object", "TypeError");
                auto obj = args[0];
                if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.values called on null or undefined", "TypeError");
                
                auto keys = jspp::Access::get_object_keys(obj);
                std::vector<jspp::AnyValue> values;
                for(const auto& k : keys) {
                    values.push_back(obj.get_own_property(k));
                }
                return jspp::AnyValue::make_array(std::move(values));
            }), "values"));

            Object.define_data_property("entries", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Object.entries called on non-object", "TypeError");
                auto obj = args[0];
                if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.entries called on null or undefined", "TypeError");
                
                auto keys = jspp::Access::get_object_keys(obj);
                std::vector<jspp::AnyValue> entries;
                for(const auto& k : keys) {
                    std::vector<jspp::AnyValue> entry;
                    entry.push_back(k);
                    entry.push_back(obj.get_own_property(k));
                    entries.push_back(jspp::AnyValue::make_array(std::move(entry)));
                }
                return jspp::AnyValue::make_array(std::move(entries));
            }), "entries"));

            Object.define_data_property("assign", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Cannot convert undefined or null to object", "TypeError");
                auto target = args[0];
                if (target.is_null() || target.is_undefined()) throw jspp::Exception::make_exception("Cannot convert undefined or null to object", "TypeError");

                for (size_t i = 1; i < args.size(); ++i) {
                    auto source = args[i];
                    if (source.is_null() || source.is_undefined()) continue;
                    
                    auto keys = jspp::Access::get_object_keys(source, true);
                    for(const auto& k : keys) {
                         auto val = source.get_own_property(k);
                         target.set_own_property(k, val);
                    }
                }
                return target;
            }), "assign"));

            Object.define_data_property("getOwnPropertySymbols", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Object.getOwnPropertySymbols called on non-object", "TypeError");
                auto obj = args[0];
                if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.getOwnPropertySymbols called on null or undefined", "TypeError");
                
                auto keys = jspp::Access::get_object_keys(obj, true);
                std::vector<jspp::AnyValue> symbolValues;
                for(const auto& k : keys) {
                    if (k.is_symbol()) symbolValues.push_back(k);
                }
                return jspp::AnyValue::make_array(std::move(symbolValues));
            }), "getOwnPropertySymbols"));

            Object.define_data_property("is", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                jspp::AnyValue v1 = args.size() > 0 ? args[0] : jspp::Constants::UNDEFINED;
                jspp::AnyValue v2 = args.size() > 1 ? args[1] : jspp::Constants::UNDEFINED;
                
                if (v1.is_number() && v2.is_number()) {
                    double d1 = v1.as_double();
                    double d2 = v2.as_double();
                    if (std::isnan(d1) && std::isnan(d2)) return jspp::Constants::TRUE;
                    if (d1 == 0 && d2 == 0) {
                        return jspp::AnyValue::make_boolean(std::signbit(d1) == std::signbit(d2));
                    }
                    return jspp::AnyValue::make_boolean(d1 == d2);
                }
                
                return jspp::is_strictly_equal_to(v1, v2);
            }), "is"));

            Object.define_data_property("getPrototypeOf", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                 if (args.empty()) throw jspp::Exception::make_exception("Object.getPrototypeOf called on non-object", "TypeError");
                 auto obj = args[0];
                 
                 if (obj.is_object()) {
                     return obj.as_object()->proto;
                 }
                 if (obj.is_array()) {
                     return obj.as_array()->proto;
                 }
                 if (obj.is_function()) {
                     return obj.as_function()->proto;
                 }
                 
                 return jspp::Constants::Null;
            }), "getPrototypeOf"));

            Object.define_data_property("setPrototypeOf", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                 if (args.size() < 2) throw jspp::Exception::make_exception("Object.setPrototypeOf requires at least 2 arguments", "TypeError");
                 auto obj = args[0];
                 auto proto = args[1];
                 
                 if (!proto.is_object() && !proto.is_null()) {
                     throw jspp::Exception::make_exception("Object prototype may only be an Object or null", "TypeError");
                 }
                 
                 if (obj.is_object()) {
                     obj.as_object()->proto = proto;
                 } else if (obj.is_array()) {
                     obj.as_array()->proto = proto;
                 } else if (obj.is_function()) {
                     obj.as_function()->proto = proto;
                 }
                 
                 return obj;
            }), "setPrototypeOf"));

            Object.define_data_property("create", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                 if (args.empty()) throw jspp::Exception::make_exception("Object prototype may only be an Object or null", "TypeError");
                 auto proto = args[0];
                 if (!proto.is_object() && !proto.is_null()) {
                     throw jspp::Exception::make_exception("Object prototype may only be an Object or null", "TypeError");
                 }
                 
                 auto newObj = jspp::AnyValue::make_object({}).set_prototype(proto);
                 return newObj;
            }), "create"));

            auto getDescHelper = [](jspp::AnyValue descVal) -> jspp::AnyValue
            {
                if (descVal.is_undefined())
                    return jspp::Constants::UNDEFINED;

                auto result = jspp::AnyValue::make_object({});

                if (descVal.is_data_descriptor())
                {
                    auto d = descVal.as_data_descriptor();
                    result.set_own_property("value", d->value);
                    result.set_own_property("writable", jspp::AnyValue::make_boolean(d->writable));
                    result.set_own_property("enumerable", jspp::AnyValue::make_boolean(d->enumerable));
                    result.set_own_property("configurable", jspp::AnyValue::make_boolean(d->configurable));
                }
                else if (descVal.is_accessor_descriptor())
                {
                    auto a = descVal.as_accessor_descriptor();
                    if (a->get.has_value())
                    {
                        result.set_own_property("get", jspp::AnyValue::make_function(
                            std::function<AnyValue(AnyValue, std::span<const AnyValue>)>(a->get.value()), std::nullopt));
                    }
                    else
                    {
                        result.set_own_property("get", jspp::Constants::UNDEFINED);
                    }

                    if (a->set.has_value())
                    {
                        result.set_own_property("set", jspp::AnyValue::make_function(
                            std::function<AnyValue(AnyValue, std::span<const AnyValue>)>(a->set.value()), std::nullopt));
                    }
                    else
                    {
                        result.set_own_property("set", jspp::Constants::UNDEFINED);
                    }
                    result.set_own_property("enumerable", jspp::AnyValue::make_boolean(a->enumerable));
                    result.set_own_property("configurable", jspp::AnyValue::make_boolean(a->configurable));
                }
                return result;
            };

            Object.define_data_property("getOwnPropertyDescriptor", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>( [getDescHelper](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Object.getOwnPropertyDescriptor called on non-object", "TypeError");
                auto obj = args[0];
                if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.getOwnPropertyDescriptor called on null or undefined", "TypeError");
                jspp::AnyValue prop = args.size() > 1 ? args[1] : jspp::Constants::UNDEFINED;
                return getDescHelper(obj.get_own_property_descriptor(prop));
            }), "getOwnPropertyDescriptor"));

            Object.define_data_property("defineProperty", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                 if (args.size() < 3) throw jspp::Exception::make_exception("Object.defineProperty requires 3 arguments", "TypeError");
                 auto obj = args[0];
                 if (!obj.is_object() && !obj.is_array() && !obj.is_function()) throw jspp::Exception::make_exception("Object.defineProperty called on non-object", "TypeError");
                 
                 std::string prop = args[1].to_std_string();
                 auto descObj = args[2];
                 
                 bool enumerable = false;
                 bool configurable = false;
                 bool writable = false;
                 
                 if (descObj.has_property("enumerable")) enumerable = jspp::is_truthy(descObj.get_own_property("enumerable"));
                 if (descObj.has_property("configurable")) configurable = jspp::is_truthy(descObj.get_own_property("configurable"));
                 if (descObj.has_property("writable")) writable = jspp::is_truthy(descObj.get_own_property("writable"));
                 
                 bool hasValue = descObj.has_property("value");
                 bool hasGet = descObj.has_property("get");
                 bool hasSet = descObj.has_property("set");
                 
                 if (hasValue && (hasGet || hasSet)) {
                     throw jspp::Exception::make_exception("Invalid property descriptor. Cannot both specify accessors and a value or writable attribute", "TypeError");
                 }
                 
                 if (hasValue) {
                     auto value = descObj.get_own_property("value");
                     obj.define_data_property(prop, value, writable, enumerable, configurable);
                 } else {
                     jspp::AnyValue getter = jspp::Constants::UNDEFINED;
                     jspp::AnyValue setter = jspp::Constants::UNDEFINED;
                     
                     if (hasGet) getter = descObj.get_own_property("get");
                     if (hasSet) setter = descObj.get_own_property("set");
                     
                     if (!getter.is_undefined() && !getter.is_function()) throw jspp::Exception::make_exception("Getter must be a function", "TypeError");
                     if (!setter.is_undefined() && !setter.is_function()) throw jspp::Exception::make_exception("Setter must be a function", "TypeError");

                     if (obj.is_object()) {
                        auto o_ptr = obj.as_object();
                        std::optional<std::function<jspp::AnyValue(jspp::AnyValue, std::span<const jspp::AnyValue>)>> getFunc;
                        std::optional<std::function<jspp::AnyValue(jspp::AnyValue, std::span<const jspp::AnyValue>)>> setFunc;
                        
                        if (getter.is_function()) {
                            getFunc = [getter](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue {
                                return getter.call(thisVal, args);
                            };
                        }
                        if (setter.is_function()) {
                            setFunc = [setter](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue {
                                return setter.call(thisVal, args);
                            };
                        }
                        
                        auto desc = jspp::AnyValue::make_accessor_descriptor(getFunc, setFunc, enumerable, configurable);
                        auto offset = o_ptr->shape->get_offset(prop);
                        if (offset.has_value()) {
                            o_ptr->storage[offset.value()] = desc;
                        } else {
                            o_ptr->shape = o_ptr->shape->transition(prop);
                            o_ptr->storage.push_back(desc);
                        }
                     }
                 }
                 return obj;
            }), "defineProperty"));

            Object.define_data_property("hasOwn", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty()) throw jspp::Exception::make_exception("Object.hasOwn called on non-object", "TypeError");
                auto obj = args[0];
                if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.hasOwn called on null or undefined", "TypeError");
                std::string prop = args.size() > 1 ? args[1].to_std_string() : "undefined";
                
                if (obj.is_object()) return jspp::AnyValue::make_boolean(obj.as_object()->shape->get_offset(prop).has_value());
                if (obj.is_function()) return jspp::AnyValue::make_boolean(obj.as_function()->props.count(prop));
                if (obj.is_array()) {
                    if (prop == "length") return jspp::Constants::TRUE;
                    if (jspp::JsArray::is_array_index(prop)) {
                        uint32_t idx = static_cast<uint32_t>(std::stoull(prop));
                        auto arr = obj.as_array();
                        if (idx < arr->dense.size() && !(arr->dense[idx].is_uninitialized())) return jspp::Constants::TRUE;
                        if (arr->sparse.count(idx)) return jspp::Constants::TRUE;
                        return jspp::Constants::FALSE;
                    }
                    return jspp::AnyValue::make_boolean(obj.as_array()->props.count(prop));
                }
                return jspp::Constants::FALSE;
            }), "hasOwn"));

            auto proto = Object.get_own_property("prototype");
            proto.define_data_property("hasOwnProperty", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                 std::string prop = args.size() > 0 ? args[0].to_std_string() : "undefined";
                 if (thisVal.is_object()) return jspp::AnyValue::make_boolean(thisVal.as_object()->shape->get_offset(prop).has_value());
                 if (thisVal.is_function()) return jspp::AnyValue::make_boolean(thisVal.as_function()->props.count(prop));
                 if (thisVal.is_array()) {
                     if (prop == "length") return jspp::Constants::TRUE;
                     if (jspp::JsArray::is_array_index(prop)) {
                         uint32_t idx = static_cast<uint32_t>(std::stoull(prop));
                         auto arr = thisVal.as_array();
                         if (idx < arr->dense.size() && !(arr->dense[idx].is_uninitialized())) return jspp::Constants::TRUE;
                         if (arr->sparse.count(idx)) return jspp::Constants::TRUE;
                         return jspp::Constants::FALSE;
                     }
                     return jspp::AnyValue::make_boolean(thisVal.as_array()->props.count(prop));
                 }
                 return jspp::Constants::FALSE;
            }), "hasOwnProperty"), true, false, true);

            auto toStringFn = jspp::ObjectPrototypes::get("toString");
            if (toStringFn.has_value()) {
                proto.define_data_property("toString", toStringFn.value(), true, false, true);
            }

            proto.define_data_property("valueOf", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            { return thisVal; }), "valueOf"), true, false, true);

            proto.define_data_property("isPrototypeOf", jspp::AnyValue::make_function(
                std::function<AnyValue(AnyValue, std::span<const AnyValue>)>([](jspp::AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
            {
                if (args.empty() || !args[0].is_object()) return jspp::Constants::FALSE;
                auto target = args[0];
                auto current = target.get_own_property("__proto__");
                if (current.is_undefined()) {
                     if (target.is_object()) current = target.as_object()->proto;
                     else if (target.is_array()) current = target.as_array()->proto;
                     else if (target.is_function()) current = target.as_function()->proto;
                }
                
                while (!current.is_null()) {
                    if (jspp::is_strictly_equal_to_primitive(current, thisVal)) return jspp::Constants::TRUE;
                    if (current.is_object()) current = current.as_object()->proto;
                    else if (current.is_array()) current = current.as_array()->proto;
                    else if (current.is_function()) current = current.as_function()->proto;
                    else break;
                }
                return jspp::Constants::FALSE;
            }), "isPrototypeOf"), true, false, true);
        }
    };
    static ObjectInit objectInit;
}
