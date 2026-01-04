#pragma once

#include "types.hpp"
#include "any_value.hpp"
#include "values/object.hpp"
#include "values/function.hpp"

namespace jspp
{
    inline void AnyValue::define_data_property(const std::string &key, const AnyValue &value)
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

    inline void AnyValue::define_data_property(const AnyValue &key, const AnyValue &value)
    {
        if (key.is_symbol())
            define_data_property(key.as_symbol()->key, value);
        else
            define_data_property(key.to_std_string(), value);
    }

    inline void AnyValue::define_data_property(const std::string &key, const AnyValue &value, bool writable, bool enumerable, bool configurable)
    {
        define_data_property(key, AnyValue::make_data_descriptor(value, writable, enumerable, configurable));
    }

    inline void AnyValue::define_getter(const std::string &key, const AnyValue &getter)
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
                    desc->get = [getter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        return getter.call(thisVal, args);
                    };
                }
                else
                {
                    auto getFunc = [getter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        return getter.call(thisVal, args);
                    };
                    obj->storage[offset.value()] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
                }
            }
            else
            {
                auto getFunc = [getter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
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
                desc->get = [getter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    return getter.call(thisVal, args);
                };
            }
            else
            {
                auto getFunc = [getter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    return getter.call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
            }
        }
    }

    inline void AnyValue::define_getter(const AnyValue &key, const AnyValue &getter)
    {
        if (key.is_symbol())
            define_getter(key.as_symbol()->key, getter);
        else
            define_getter(key.to_std_string(), getter);
    }

    inline void AnyValue::define_setter(const std::string &key, const AnyValue &setter)
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
                    desc->set = [setter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        if (args.empty())
                            return Constants::UNDEFINED;
                        return setter.call(thisVal, args);
                    };
                }
                else
                {
                    auto setFunc = [setter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                    {
                        if (args.empty())
                            return Constants::UNDEFINED;
                        return setter.call(thisVal, args);
                    };
                    obj->storage[offset.value()] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
                }
            }
            else
            {
                auto setFunc = [setter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                        return Constants::UNDEFINED;
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
                desc->set = [setter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                        return Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
            }
            else
            {
                auto setFunc = [setter](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                {
                    if (args.empty())
                        return Constants::UNDEFINED;
                    return setter.call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
            }
        }
    }

    inline void AnyValue::define_setter(const AnyValue &key, const AnyValue &setter)
    {
        if (key.is_symbol())
            define_setter(key.as_symbol()->key, setter);
        else
            define_setter(key.to_std_string(), setter);
    }
}