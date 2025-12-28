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
            storage.object->props[key] = value;
        }
        else if (is_function())
        {
            storage.function->props[key] = value;
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
            auto &props = storage.object->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->get = [getter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    return getter.as_function()->call(thisVal, args);
                };
            }
            else
            {
                auto getFunc = [getter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    return getter.as_function()->call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(getFunc, std::nullopt, true, true);
            }
        }
        else if (is_function())
        {
            auto &props = storage.function->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->get = [getter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    return getter.as_function()->call(thisVal, args);
                };
            }
            else
            {
                auto getFunc = [getter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    return getter.as_function()->call(thisVal, args);
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
            auto &props = storage.object->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->set = [setter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    if (args.empty())
                        return AnyValue::make_undefined();
                    return setter.as_function()->call(thisVal, args);
                };
            }
            else
            {
                auto setFunc = [setter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    if (args.empty())
                        return AnyValue::make_undefined();
                    return setter.as_function()->call(thisVal, args);
                };
                props[key] = AnyValue::make_accessor_descriptor(std::nullopt, setFunc, true, true);
            }
        }
        else if (is_function())
        {
            auto &props = storage.function->props;
            auto it = props.find(key);
            if (it != props.end() && it->second.is_accessor_descriptor())
            {
                auto desc = it->second.as_accessor_descriptor();
                desc->set = [setter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    if (args.empty())
                        return AnyValue::make_undefined();
                    return setter.as_function()->call(thisVal, args);
                };
            }
            else
            {
                auto setFunc = [setter](const AnyValue &thisVal, const std::vector<AnyValue> &args) -> AnyValue
                {
                    if (args.empty())
                        return AnyValue::make_undefined();
                    return setter.as_function()->call(thisVal, args);
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
