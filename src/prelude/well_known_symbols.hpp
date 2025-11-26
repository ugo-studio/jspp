#pragma once

#include "values/symbol.hpp"
#include <memory>

namespace jspp
{
    namespace WellKnownSymbols
    {
        // We use a specific prefix "@@" for well-known symbols to distinguish them from user symbols
        inline std::shared_ptr<JsSymbol> iterator = std::make_shared<JsSymbol>("Symbol.iterator", "@@iterator");
        inline std::shared_ptr<JsSymbol> toString = std::make_shared<JsSymbol>("Symbol.toString", "@@toString");
    }
}