#pragma once

#include "types.hpp"

namespace jspp {
    namespace Prototype
    {
        using PrototypeMap = std::map<std::string, std::variant<DataDescriptor, AccessorDescriptor, JsValue>>;

        inline std::function<JsValue(const std::vector<JsValue> &)> to_handler(std::function<JsValue()> fn)
        {
            return [fn = std::move(fn)](const std::vector<JsValue> &)
            {
                return fn();
            };
        }

        inline std::function<JsValue(const std::vector<JsValue> &)> to_handler(std::function<JsValue(JsValue)> fn)
        {
            return [fn = std::move(fn)](const std::vector<JsValue> &args)
            {
                return fn(args.empty() ? undefined : args[0]);
            };
        }

        inline void set_data_property(
            PrototypeMap &prototype,
            const std::string &name,
            const JsValue &value,
            bool writable = true,
            bool enumerable = false,
            bool configurable = true)
        {
            prototype[name] = DataDescriptor{value, writable, enumerable, configurable};
        }

        inline void set_accessor_property(
            PrototypeMap &prototype,
            const std::string &name,
            const std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> &getter,
            const std::variant<std::function<JsValue(const std::vector<JsValue> &)>, Undefined> &setter,
            bool enumerable = false,
            bool configurable = true)
        {
            prototype[name] = AccessorDescriptor{getter, setter, enumerable, configurable};
        }
    }
}
