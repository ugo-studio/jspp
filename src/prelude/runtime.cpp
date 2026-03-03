#include "jspp.hpp"

// Include all core implementations here so they are compiled only once into runtime.o
#include "any_value_helpers.hpp"
#include "any_value_access.hpp"
#include "any_value_defines.hpp"
#include "exception_helpers.hpp"

// Include helper implementations that were previously in headers
#include "values/helpers/iterator.hpp"
#include "values/helpers/async_iterator.hpp"

namespace jspp {
    // Explicit template instantiation definitions
    template class JsIterator<AnyValue>;
    template class JsAsyncIterator<AnyValue>;
}
