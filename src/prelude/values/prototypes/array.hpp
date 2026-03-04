#pragma once

#include "types.hpp"
#include <optional>

namespace jspp
{
    class AnyValue;

    namespace ArrayPrototypes
    {
        AnyValue &get_toString_fn();
        AnyValue &get_iterator_fn();
        AnyValue &get_length_desc();
        AnyValue &get_push_fn();
        AnyValue &get_pop_fn();
        AnyValue &get_shift_fn();
        AnyValue &get_unshift_fn();
        AnyValue &get_join_fn();
        AnyValue &get_forEach_fn();
        AnyValue &get_at_fn();
        AnyValue &get_includes_fn();
        AnyValue &get_indexOf_fn();
        AnyValue &get_lastIndexOf_fn();
        AnyValue &get_find_fn();
        AnyValue &get_findIndex_fn();
        AnyValue &get_findLast_fn();
        AnyValue &get_findLastIndex_fn();
        AnyValue &get_values_fn();
        AnyValue &get_keys_fn();
        AnyValue &get_entries_fn();
        AnyValue &get_map_fn();
        AnyValue &get_filter_fn();
        AnyValue &get_every_fn();
        AnyValue &get_some_fn();
        AnyValue &get_reduce_fn();
        AnyValue &get_reduceRight_fn();
        AnyValue &get_flat_fn();
        AnyValue &get_flatMap_fn();
        AnyValue &get_fill_fn();
        AnyValue &get_reverse_fn();
        AnyValue &get_sort_fn();
        AnyValue &get_splice_fn();
        AnyValue &get_copyWithin_fn();
        AnyValue &get_concat_fn();
        AnyValue &get_slice_fn();
        AnyValue &get_toReversed_fn();
        AnyValue &get_toSorted_fn();
        AnyValue &get_toSpliced_fn();
        AnyValue &get_with_fn();
        AnyValue &get_toLocaleString_fn();

        std::optional<AnyValue> get(const std::string &key);
        std::optional<AnyValue> get(const AnyValue &key);
    }
}
