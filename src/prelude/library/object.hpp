#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "utils/access.hpp"
#include "exception.hpp"

// Define Object constructor
inline auto Object = jspp::AnyValue::make_class([](const jspp::AnyValue& thisVal, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
    if (args.empty() || args[0].is_undefined() || args[0].is_null()) {
        return jspp::AnyValue::make_object({});
    }
    // Return argument if it is an object
    if (args[0].is_object() || args[0].is_array() || args[0].is_function() || args[0].is_promise() || args[0].is_iterator()) {
        return args[0];
    }
    // TODO: Wrapper objects for primitives
    return jspp::AnyValue::make_object({}); 
}, "Object");

struct ObjectInit {
    ObjectInit() {
        // Object.keys(obj)
        Object.define_data_property("keys", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
            if (args.empty()) throw jspp::Exception::make_exception("Object.keys called on non-object", "TypeError");
            auto obj = args[0];
            if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.keys called on null or undefined", "TypeError");
            
            auto keys = jspp::Access::get_object_keys(obj);
            std::vector<std::optional<jspp::AnyValue>> keyValues;
            for(const auto& k : keys) {
                keyValues.push_back(jspp::AnyValue::make_string(k));
            }
            return jspp::AnyValue::make_array(keyValues);
        }, "keys"));

        // Object.values(obj)
        Object.define_data_property("values", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
            if (args.empty()) throw jspp::Exception::make_exception("Object.values called on non-object", "TypeError");
            auto obj = args[0];
            if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.values called on null or undefined", "TypeError");
            
            auto keys = jspp::Access::get_object_keys(obj);
            std::vector<std::optional<jspp::AnyValue>> values;
            for(const auto& k : keys) {
                values.push_back(obj.get_property_with_receiver(k, obj));
            }
            return jspp::AnyValue::make_array(values);
        }, "values"));

        // Object.entries(obj)
        Object.define_data_property("entries", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
            if (args.empty()) throw jspp::Exception::make_exception("Object.entries called on non-object", "TypeError");
            auto obj = args[0];
            if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.entries called on null or undefined", "TypeError");
            
            auto keys = jspp::Access::get_object_keys(obj);
            std::vector<std::optional<jspp::AnyValue>> entries;
            for(const auto& k : keys) {
                std::vector<std::optional<jspp::AnyValue>> entry;
                entry.push_back(jspp::AnyValue::make_string(k));
                entry.push_back(obj.get_property_with_receiver(k, obj));
                entries.push_back(jspp::AnyValue::make_array(entry));
            }
            return jspp::AnyValue::make_array(entries);
        }, "entries"));

        // Object.assign(target, ...sources)
        Object.define_data_property("assign", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
            if (args.empty()) throw jspp::Exception::make_exception("Cannot convert undefined or null to object", "TypeError");
            auto target = args[0];
            if (target.is_null() || target.is_undefined()) throw jspp::Exception::make_exception("Cannot convert undefined or null to object", "TypeError");

            // In JS, Object.assign modifies target in place if it's an object. 
            // If it's a primitive, it wraps it (but our primitives are tricky, let's assume objects for now).
            
            for (size_t i = 1; i < args.size(); ++i) {
                auto source = args[i];
                if (source.is_null() || source.is_undefined()) continue;
                
                auto keys = jspp::Access::get_object_keys(source);
                for(const auto& k : keys) {
                     auto val = source.get_property_with_receiver(k, source);
                     target.set_own_property(k, val);
                }
            }
            return target;
        }, "assign"));
        
        // Object.is(value1, value2)
        Object.define_data_property("is", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
            jspp::AnyValue v1 = args.size() > 0 ? args[0] : jspp::AnyValue::make_undefined();
            jspp::AnyValue v2 = args.size() > 1 ? args[1] : jspp::AnyValue::make_undefined();
            
            if (v1.is_number() && v2.is_number()) {
                double d1 = v1.as_double();
                double d2 = v2.as_double();
                if (std::isnan(d1) && std::isnan(d2)) return jspp::TRUE;
                if (d1 == 0 && d2 == 0) {
                    // check signs
                    return jspp::AnyValue::make_boolean(std::signbit(d1) == std::signbit(d2));
                }
                return jspp::AnyValue::make_boolean(d1 == d2);
            }
            
            return jspp::is_strictly_equal_to(v1, v2);
        }, "is"));
        
        // Object.getPrototypeOf(obj)
        Object.define_data_property("getPrototypeOf", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
             if (args.empty()) throw jspp::Exception::make_exception("Object.getPrototypeOf called on non-object", "TypeError");
             auto obj = args[0];
             // In ES6+, primitives are coerced to objects.
             // We'll focus on Objects/Arrays/Functions for now.
             
             if (obj.is_object()) {
                 auto p = obj.as_object()->proto;
                 return p ? *p : jspp::AnyValue::make_null();
             }
             if (obj.is_array()) {
                 auto p = obj.as_array()->proto;
                 return p ? *p : jspp::AnyValue::make_null();
             }
             if (obj.is_function()) {
                 auto p = obj.as_function()->proto;
                 return p ? *p : jspp::AnyValue::make_null();
             }
             
             // For primitives, they use their prototype from the global constructors usually
             // e.g. Number.prototype. For now return null or implement if needed.
             return jspp::AnyValue::make_null();
        }, "getPrototypeOf"));
        
         // Object.setPrototypeOf(obj, proto)
        Object.define_data_property("setPrototypeOf", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
             if (args.size() < 2) throw jspp::Exception::make_exception("Object.setPrototypeOf requires at least 2 arguments", "TypeError");
             auto obj = args[0];
             auto proto = args[1];
             
             if (!proto.is_object() && !proto.is_null()) {
                 throw jspp::Exception::make_exception("Object prototype may only be an Object or null", "TypeError");
             }
             
             if (obj.is_object()) {
                 obj.as_object()->proto = std::make_shared<jspp::AnyValue>(proto);
             } else if (obj.is_array()) {
                 obj.as_array()->proto = std::make_shared<jspp::AnyValue>(proto);
             } else if (obj.is_function()) {
                 obj.as_function()->proto = std::make_shared<jspp::AnyValue>(proto);
             }
             
             return obj;
        }, "setPrototypeOf"));

        // Object.create(proto, [propertiesObject])
        Object.define_data_property("create", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
             if (args.empty()) throw jspp::Exception::make_exception("Object prototype may only be an Object or null", "TypeError");
             auto proto = args[0];
             if (!proto.is_object() && !proto.is_null()) {
                 throw jspp::Exception::make_exception("Object prototype may only be an Object or null", "TypeError");
             }
             
             auto newObj = jspp::AnyValue::make_object_with_proto({}, proto);
             
             if (args.size() > 1 && !args[1].is_undefined()) {
                 // Object.defineProperties(newObj, propertiesObject)
                 // TODO: implement defineProperties logic if needed.
             }
             
             return newObj;
        }, "create"));
        
        // Object.defineProperty(obj, prop, descriptor)
        Object.define_data_property("defineProperty", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
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
                 jspp::AnyValue getter = jspp::AnyValue::make_undefined();
                 jspp::AnyValue setter = jspp::AnyValue::make_undefined();
                 
                 if (hasGet) getter = descObj.get_own_property("get");
                 if (hasSet) setter = descObj.get_own_property("set");
                 
                 if (!getter.is_undefined() && !getter.is_function()) throw jspp::Exception::make_exception("Getter must be a function: " + getter.to_std_string(), "TypeError");
                 if (!setter.is_undefined() && !setter.is_function()) throw jspp::Exception::make_exception("Setter must be a function", "TypeError");

                 if (obj.is_object()) {
                    auto& props = obj.as_object()->props;
                    std::optional<std::function<jspp::AnyValue(const jspp::AnyValue &, const std::vector<jspp::AnyValue> &)>> getFunc;
                    std::optional<std::function<jspp::AnyValue(const jspp::AnyValue &, const std::vector<jspp::AnyValue> &)>> setFunc;
                    
                    if (getter.is_function()) {
                        getFunc = [getter](const jspp::AnyValue &thisVal, const std::vector<jspp::AnyValue> &args) -> jspp::AnyValue {
                            return getter.as_function()->call(thisVal, args);
                        };
                    }
                    if (setter.is_function()) {
                        setFunc = [setter](const jspp::AnyValue &thisVal, const std::vector<jspp::AnyValue> &args) -> jspp::AnyValue {
                            return setter.as_function()->call(thisVal, args);
                        };
                    }
                    
                    props[prop] = jspp::AnyValue::make_accessor_descriptor(getFunc, setFunc, enumerable, configurable);
                 }
                 // TODO: Handle Array/Function/others
             }
             
             return obj;
        }, "defineProperty"));

        // Object.hasOwn(obj, prop)
        Object.define_data_property("hasOwn", jspp::AnyValue::make_function([](const jspp::AnyValue&, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
            if (args.empty()) throw jspp::Exception::make_exception("Object.hasOwn called on non-object", "TypeError");
            auto obj = args[0];
            if (obj.is_null() || obj.is_undefined()) throw jspp::Exception::make_exception("Object.hasOwn called on null or undefined", "TypeError");
            std::string prop = args.size() > 1 ? args[1].to_std_string() : "undefined";
            
            if (obj.is_object()) return jspp::AnyValue::make_boolean(obj.as_object()->props.count(prop));
            if (obj.is_function()) return jspp::AnyValue::make_boolean(obj.as_function()->props.count(prop));
            if (obj.is_array()) {
                if (prop == "length") return jspp::TRUE;
                if (jspp::JsArray::is_array_index(prop)) {
                    uint32_t idx = static_cast<uint32_t>(std::stoull(prop));
                    auto arr = obj.as_array();
                    if (idx < arr->dense.size() && arr->dense[idx].has_value()) return jspp::TRUE;
                    if (arr->sparse.count(idx)) return jspp::TRUE;
                    return jspp::FALSE;
                }
                return jspp::AnyValue::make_boolean(obj.as_array()->props.count(prop));
            }
            
            return jspp::FALSE;
        }, "hasOwn"));

        // Object.prototype.hasOwnProperty
        auto proto = Object.get_own_property("prototype");
        proto.define_data_property("hasOwnProperty", jspp::AnyValue::make_function([](const jspp::AnyValue& thisVal, const std::vector<jspp::AnyValue>& args) -> jspp::AnyValue {
             std::string prop = args.size() > 0 ? args[0].to_std_string() : "undefined";
             if (thisVal.is_object()) return jspp::AnyValue::make_boolean(thisVal.as_object()->props.count(prop));
             if (thisVal.is_function()) return jspp::AnyValue::make_boolean(thisVal.as_function()->props.count(prop));
             if (thisVal.is_array()) {
                 if (prop == "length") return jspp::TRUE;
                 if (jspp::JsArray::is_array_index(prop)) {
                     uint32_t idx = static_cast<uint32_t>(std::stoull(prop));
                     auto arr = thisVal.as_array();
                     if (idx < arr->dense.size() && arr->dense[idx].has_value()) return jspp::TRUE;
                     if (arr->sparse.count(idx)) return jspp::TRUE;
                     return jspp::FALSE;
                 }
                 return jspp::AnyValue::make_boolean(thisVal.as_array()->props.count(prop));
             }
             return jspp::FALSE;
        }, "hasOwnProperty"));
    }
} objectInit;
