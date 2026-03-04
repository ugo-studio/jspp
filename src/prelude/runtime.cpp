#include "jspp.hpp"

// Include core headers
#include "values/iterator.hpp"
#include "values/async_iterator.hpp"
#include "values/string.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"
#include "values/promise.hpp"
#include "values/symbol.hpp"
#include "values/descriptors.hpp"
#include "exception.hpp"

namespace jspp {
    // Explicit template instantiation definitions
    template class JsIterator<AnyValue>;
    template class JsAsyncIterator<AnyValue>;
}
