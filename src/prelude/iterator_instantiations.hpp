#pragma once
#include "values/iterator.hpp"
#include "values/async_iterator.hpp"

namespace jspp {
    // Explicit template instantiation declarations
    // This tells the compiler that the implementation is in libjspp.a
    extern template class JsIterator<AnyValue>;
    extern template class JsAsyncIterator<AnyValue>;
}
