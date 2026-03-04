#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace StringPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_iterator_fn();
        AnyValue &get_length_desc();
        AnyValue &get_charAt_fn();
        AnyValue &get_concat_fn();
        AnyValue &get_endsWith_fn();
        AnyValue &get_includes_fn();
        AnyValue &get_indexOf_fn();
        AnyValue &get_lastIndexOf_fn();
        AnyValue &get_padEnd_fn();
        AnyValue &get_padStart_fn();
        AnyValue &get_repeat_fn();
        AnyValue &get_replace_fn();
        AnyValue &get_replaceAll_fn();
        AnyValue &get_slice_fn();
        AnyValue &get_split_fn();
        AnyValue &get_startsWith_fn();
        AnyValue &get_substring_fn();
        AnyValue &get_toLowerCase_fn();
        AnyValue &get_toUpperCase_fn();
        AnyValue &get_trim_fn();
        AnyValue &get_trimEnd_fn();
        AnyValue &get_trimStart_fn();

        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
