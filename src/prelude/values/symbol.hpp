#pragma once

#include "types.hpp"
#include <atomic>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    struct JsSymbol : HeapObject
    {
        std::string description;
        std::string key; // Internal unique key used for AnyValue property maps

        JsType get_heap_type() const override { return JsType::Symbol; }

        // --- Registries ---

        // 1. Global Symbol Registry (for Symbol.for/keyFor)
        static std::unordered_map<std::string, JsSymbol*> &registry()
        {
            static std::unordered_map<std::string, JsSymbol*> reg;
            return reg;
        }

        // 2. All Internal Keys Registry (for robust Object.keys filtering)
        static std::unordered_set<std::string> &internal_keys_registry()
        {
            static std::unordered_set<std::string> keys;
            return keys;
        }

        static bool is_internal_key(const std::string &k)
        {
            return internal_keys_registry().count(k) > 0;
        }

        // --- Constructors ---

        // Standard Constructor (creates unique symbol)
        JsSymbol(const std::string &desc) : description(desc)
        {
            static std::atomic<uint64_t> id_counter{0};
            // Ensure unique internal key for property storage
            key = "__Sym" + std::to_string(id_counter++) + "_" + desc;

            // Register this key as a valid symbol key
            internal_keys_registry().insert(key);
        }

        // Constructor for Well-Known Symbols (fixed keys)
        JsSymbol(const std::string &desc, const std::string &fixed_key)
            : description(desc), key(fixed_key)
        {
            // Register this key as a valid symbol key
            internal_keys_registry().insert(key);
        }

        // --- Global Registry API ---

        // Implements Symbol.for(key)
        static JsSymbol* for_global(const std::string &registryKey)
        {
            auto &reg = registry();
            auto it = reg.find(registryKey);
            if (it != reg.end())
            {
                return it->second;
            }

            // Create new symbol with description = registryKey
            auto newSym = new JsSymbol(registryKey);
            newSym->ref(); // Keep it alive in registry
            reg[registryKey] = newSym;
            return newSym;
        }

        // Implements Symbol.keyFor(sym)
        static std::optional<std::string> key_for(const JsSymbol *sym)
        {
            auto &reg = registry();
            for (const auto &pair : reg)
            {
                if (pair.second == sym)
                {
                    return pair.first;
                }
            }
            return std::nullopt;
        }

        // --- Methods ---
        std::string to_std_string() const;
        AnyValue get_property(const std::string &key, const AnyValue &thisVal);
    };
}
