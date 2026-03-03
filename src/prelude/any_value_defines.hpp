#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "values/object.hpp"
#include "values/function.hpp"

namespace jspp
{
    void AnyValue::define_data_property(const std::string &key, AnyValue value)
    {
        if (is_object())
        {
            auto obj = as_object();
            auto offset = obj->shape->get_offset(key);
            if (offset.has_value())
            {
                obj->storage[offset.value()] = value;
            }
            else
            {
                obj->shape = obj->shape->transition(key);
                obj->storage.push_back(value);
            }
        }
        else if (is_function())
        {
            as_function()->props[key] = value;
        }
    }

    void AnyValue::define_data_property(const AnyValue &key, AnyValue value)
    {
        if (key.is_symbol())
        {
            if (is_object())
            {
                as_object()->symbol_props[key] = value;
            }
            else if (is_function())
            {
                as_function()->symbol_props[key] = value;
            }
        }
        else
            define_data_property(key.to_std_string(), value);
    }

    void AnyValue::define_data_property(const std::string &key, AnyValue value, bool writable, bool enumerable, bool configurable)
    {
        define_data_property(key, AnyValue::make_data_descriptor(value, writable, enumerable, configurable));
    }

    void AnyValue::define_getter(const std::string &key, AnyValue getter)
    {
        if (is_object())
        {
            auto obj = as_object();
            auto offset = obj->shape->get_offset(key);

            if (offset.has_value())
            {
                auto &val = obj->storage[offset.value()];
                if (val.is_accessor_descriptor())
                {
                    auto desc = val.as_accessor_descriptor();
                    desc->get = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        return getter.call(thisVal, args);
                    };
                }
                else
                {
                    auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        return getter.call(thisVal, args);
                    };
                    obj->storage[offset.value()] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
                }
            }
            else
            {
                auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    return getter.call(thisVal, args);
                };
                obj->shape = obj->shape->transition(key);
                obj->storage.push_back(AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true));
            }
        }
        else if (is_function())
        {
            auto &props = as_function()->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->get = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    return getter.call(thisVal, args);
                };
            }
            else
            {
                auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    return getter.call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
            }
        }
    }

    void AnyValue::define_data_property(const AnyValue &key, AnyValue value, bool writable, bool enumerable, bool configurable)
    {
        if (key.is_symbol())
        {
            auto desc = AnyValue::make_data_descriptor(value, writable, enumerable, configurable);
            if (is_object())
            {
                as_object()->symbol_props[key] = desc;
            }
            else if (is_function())
            {
                as_function()->symbol_props[key] = desc;
            }
        }
        else
            define_data_property(key.to_std_string(), value, writable, enumerable, configurable);
    }

    void AnyValue::define_getter(const AnyValue &key, AnyValue getter)
    {
        if (key.is_symbol())
        {
            auto getFunc = [getter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
            {
                return getter.call(thisVal, args);
            };
            auto desc = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
            if (is_object())
            {
                as_object()->symbol_props[key] = desc;
            }
            else if (is_function())
            {
                as_function()->symbol_props[key] = desc;
            }
        }
        else
            define_getter(key.to_std_string(), getter);
    }

    void AnyValue::define_setter(const std::string &key, AnyValue setter)
    {
        if (is_object())
        {
            auto obj = as_object();
            auto offset = obj->shape->get_offset(key);

            if (offset.has_value())
            {
                auto &val = obj->storage[offset.value()];
                if (val.is_accessor_descriptor())
                {
                    auto desc = val.as_accessor_descriptor();
                    desc->set = [setter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        if (args.empty())
                            return Constants::UNDEFINED;
                        return setter.call(thisVal, args);
                    };
                }
                else
                {
                    auto setFunc = [setter](AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                    {
                        if (args.empty())
                            return jspp::Constants::UNDEFINED;
                        return setter.call(thisVal, args);
                    };
                    obj->storage[offset.value()] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
                }
            }
            else
            {
                auto setFunc = [setter](AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                {
                    if (args.empty())
                        return jspp::Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
                obj->shape = obj->shape->transition(key);
                obj->storage.push_back(AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true));
            }
        }
        else if (is_function())
        {
            auto &props = as_function()->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->set = [setter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                        return Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
            }
            else
            {
                auto setFunc = [setter](AnyValue thisVal, std::span<const jspp::AnyValue> args) -> jspp::AnyValue
                {
                    if (args.empty())
                        return jspp::Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
            }
        }
    }

        void AnyValue::define_setter(const AnyValue &key, AnyValue setter)
        {
            if (key.is_symbol())
            {
                auto setFunc = [setter](AnyValue thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                        return Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
                auto desc = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
                if (is_object())
                {
                    as_object()->symbol_props[key] = desc;
                }
                else if (is_function())
                {
                    as_function()->symbol_props[key] = desc;
                }
            }
            else
                define_setter(key.to_std_string(), setter);
        }
    }
    