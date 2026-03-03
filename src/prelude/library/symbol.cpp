#include "jspp.hpp"
#include "library/symbol.hpp"

namespace jspp {
    void init_symbol() {
        static SymbolInit symbolInit;
    }
}
