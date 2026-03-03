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

    // AnyValue methods that depend on Iterator types
    JsIterator<AnyValue>* AnyValue::as_iterator() const noexcept { 
        return static_cast<JsIterator<AnyValue> *>(get_ptr()); 
    }
    
    JsAsyncIterator<AnyValue>* AnyValue::as_async_iterator() const noexcept { 
        return static_cast<JsAsyncIterator<AnyValue> *>(get_ptr()); 
    }
}

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
    // Now that AnyValue is complete, we can instantiate the templates safely.
    template class JsIterator<AnyValue>;
    template class JsAsyncIterator<AnyValue>;
}
