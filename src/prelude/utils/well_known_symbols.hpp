#pragma once

#include "values/symbol.hpp"
#include <memory>

namespace jspp
{
    namespace WellKnownSymbols
    {
        // We use a specific prefix "@@" for well-known symbols to distinguish them from user symbols
        inline std::shared_ptr<JsSymbol> iterator = std::make_shared<JsSymbol>("Symbol.iterator", "@@iterator");
        inline std::shared_ptr<JsSymbol> asyncIterator = std::make_shared<JsSymbol>("Symbol.asyncIterator", "@@asyncIterator");
        inline std::shared_ptr<JsSymbol> hasInstance = std::make_shared<JsSymbol>("Symbol.hasInstance", "@@hasInstance");
        inline std::shared_ptr<JsSymbol> isConcatSpreadable = std::make_shared<JsSymbol>("Symbol.isConcatSpreadable", "@@isConcatSpreadable");
        inline std::shared_ptr<JsSymbol> match = std::make_shared<JsSymbol>("Symbol.match", "@@match");
        inline std::shared_ptr<JsSymbol> matchAll = std::make_shared<JsSymbol>("Symbol.matchAll", "@@matchAll");
        inline std::shared_ptr<JsSymbol> replace = std::make_shared<JsSymbol>("Symbol.replace", "@@replace");
        inline std::shared_ptr<JsSymbol> search = std::make_shared<JsSymbol>("Symbol.search", "@@search");
        inline std::shared_ptr<JsSymbol> species = std::make_shared<JsSymbol>("Symbol.species", "@@species");
        inline std::shared_ptr<JsSymbol> split = std::make_shared<JsSymbol>("Symbol.split", "@@split");
        inline std::shared_ptr<JsSymbol> toPrimitive = std::make_shared<JsSymbol>("Symbol.toPrimitive", "@@toPrimitive");
        inline std::shared_ptr<JsSymbol> toStringTag = std::make_shared<JsSymbol>("Symbol.toStringTag", "@@toStringTag");
        inline std::shared_ptr<JsSymbol> unscopables = std::make_shared<JsSymbol>("Symbol.unscopables", "@@unscopables");
    }
}