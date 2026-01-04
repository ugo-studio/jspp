#pragma once

#include "values/symbol.hpp"

namespace jspp
{
    namespace WellKnownSymbols
    {
        // We use a specific prefix "@@" for well-known symbols to distinguish them from user symbols
        inline JsSymbol* iterator = new JsSymbol("Symbol.iterator", "@@iterator");
        inline JsSymbol* asyncIterator = new JsSymbol("Symbol.asyncIterator", "@@asyncIterator");
        inline JsSymbol* hasInstance = new JsSymbol("Symbol.hasInstance", "@@hasInstance");
        inline JsSymbol* isConcatSpreadable = new JsSymbol("Symbol.isConcatSpreadable", "@@isConcatSpreadable");
        inline JsSymbol* match = new JsSymbol("Symbol.match", "@@match");
        inline JsSymbol* matchAll = new JsSymbol("Symbol.matchAll", "@@matchAll");
        inline JsSymbol* replace = new JsSymbol("Symbol.replace", "@@replace");
        inline JsSymbol* search = new JsSymbol("Symbol.search", "@@search");
        inline JsSymbol* species = new JsSymbol("Symbol.species", "@@species");
        inline JsSymbol* split = new JsSymbol("Symbol.split", "@@split");
        inline JsSymbol* toPrimitive = new JsSymbol("Symbol.toPrimitive", "@@toPrimitive");
        inline JsSymbol* toStringTag = new JsSymbol("Symbol.toStringTag", "@@toStringTag");
        inline JsSymbol* unscopables = new JsSymbol("Symbol.unscopables", "@@unscopables");
    }
}
