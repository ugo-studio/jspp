#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdlib> // malloc/free
#include <new>     // placement new
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <limits>

#include "values/non_values.hpp"
#include "values/object.hpp"
#include "values/array.hpp"
#include "values/function.hpp"

namespace jspp
{

    // keep your JsType
    enum class JsType : uint8_t
    {
        Undefined = 0,
        Null,
        Uninitialized,
        Boolean,
        Number, // used only in fallback TaggedValue and for NaN sentinel in NaN-box mode
        String,
        Object,
        Array,
        Function,
    };

    // A fallback tagged struct (same fields you had previously)
    struct TaggedValue
    {
        JsType type;
        union
        {
            JsUndefined undefined;
            JsNull null;
            JsUninitialized uninitialized;
            bool boolean;
            double number;
            std::string *str;
            JsObject *object;
            JsArray *array;
            JsFunction *function;
        };

        // default-construct as undefined
        TaggedValue() noexcept
        {
            type = JsType::Undefined;
        }
    };

    class AnyValue
    {
    private:
        // NaN-boxing constants
        static constexpr uint64_t QNAN_MASK = 0x7FF8000000000000ULL;    // quiet NaN pattern in exp + top mantissa bit
        static constexpr uint64_t TAG_SHIFT = 48;                       // we use bits [51:48] as a 4-bit type tag
        static constexpr uint64_t TAG_NIBBLE_MASK = 0xFULL;             // 4 bits
        static constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFFULL; // low 48 bits

        // storage: either 64-bit bitpattern (nan-box) OR a full TaggedValue
        union Storage
        {
            uint64_t bits;
            TaggedValue tagged;
            Storage() noexcept { bits = 0; } // default
            ~Storage() noexcept {}           // destruction handled manually if needed
        } storage;

        // Global runtime decision (computed on first use)
        static bool detect_nan_boxing() noexcept
        {
            // Must be on 64-bit pointers
            if (sizeof(void *) < 8)
                return false;

            // Quick runtime check whether pointers' high bits fit in 48-bit payload.
            bool ok = true;
            for (int i = 0; i < 8; ++i)
            {
                void *p = std::malloc(1);
                if (!p)
                {
                    ok = false;
                    break;
                }
                uintptr_t bits = reinterpret_cast<uintptr_t>(p);
                std::free(p);
                if ((bits & ~PAYLOAD_MASK) != 0)
                {
                    ok = false;
                    break;
                }
            }
            return ok;
        }

        static bool nan_boxing_available() noexcept
        {
            static const bool available = detect_nan_boxing();
            return available;
        }

        // helpers for NaN-box form
        static inline uint64_t double_to_bits(double d) noexcept
        {
            uint64_t x;
            std::memcpy(&x, &d, sizeof(double));
            return x;
        }
        static inline double bits_to_double(uint64_t x) noexcept
        {
            double d;
            std::memcpy(&d, &x, sizeof(double));
            return d;
        }

        // internal constructors for setting storage in each mode
        void set_bits(uint64_t b) noexcept { storage.bits = b; }
        void set_tagged(const TaggedValue &t) noexcept
        {
            new (&storage.tagged) TaggedValue(t);
        }

        // extract low 4-bit tag nibble from bits [51:48]
        inline uint8_t get_tag_nibble() const noexcept
        {
            uint64_t top16 = (storage.bits >> TAG_SHIFT) & 0xFFFFULL;
            return static_cast<uint8_t>(top16 & TAG_NIBBLE_MASK);
        }

        // compose a NaN-box with a given 4-bit tag and 48-bit payload
        static inline uint64_t make_nanbox_bits(JsType tag, uint64_t payload = 0) noexcept
        {
            return QNAN_MASK |
                   (((static_cast<uint64_t>(tag) & TAG_NIBBLE_MASK) << TAG_SHIFT)) |
                   (payload & PAYLOAD_MASK);
        }

    public:
        // default ctor (Undefined)
        AnyValue() noexcept
        {
            if (nan_boxing_available())
            {
                set_bits(make_nanbox_bits(JsType::Undefined));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Undefined;
                new (&storage.tagged) TaggedValue(tv);
            }
        }

        // factories -------------------------------------------------------
        static AnyValue make_number(double d) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                if (std::isnan(d))
                {
                    // Use a dedicated NaN-box sentinel tagged as "Number"
                    v.set_bits(make_nanbox_bits(JsType::Number));
                }
                else
                {
                    // store raw double bits directly
                    v.set_bits(double_to_bits(d));
                }
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Number;
                tv.number = d;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_uninitialized() noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                v.set_bits(make_nanbox_bits(JsType::Uninitialized));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Uninitialized;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_undefined() noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                v.set_bits(make_nanbox_bits(JsType::Undefined));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Undefined;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_null() noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                v.set_bits(make_nanbox_bits(JsType::Null));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Null;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_boolean(bool b) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uint64_t payload = b ? 1ULL : 0ULL;
                v.set_bits(make_nanbox_bits(JsType::Boolean, payload));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Boolean;
                tv.boolean = b;
                v.set_tagged(tv);
            }
            return v;
        }

        // pointer boxing (strings/objects/arrays/functions)
        static AnyValue make_string(std::string *s) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(s);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                v.set_bits(make_nanbox_bits(JsType::String, static_cast<uint64_t>(ptr_bits)));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::String;
                tv.str = s;
                v.set_tagged(tv);
            }
            return v;
        }
        static AnyValue make_string(std::string raw_s) noexcept
        {
            // FIX: avoid dangling pointer from temporary shared_ptr
            auto *s = new std::string(std::move(raw_s));
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(s);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                v.set_bits(make_nanbox_bits(JsType::String, static_cast<uint64_t>(ptr_bits)));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::String;
                tv.str = s;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_object(JsObject *o) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(o);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                v.set_bits(make_nanbox_bits(JsType::Object, static_cast<uint64_t>(ptr_bits)));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Object;
                tv.object = o;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_array(JsArray *a) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(a);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                v.set_bits(make_nanbox_bits(JsType::Array, static_cast<uint64_t>(ptr_bits)));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Array;
                tv.array = a;
                v.set_tagged(tv);
            }
            return v;
        }

        static AnyValue make_function(JsFunction *f) noexcept
        {
            AnyValue v;
            if (nan_boxing_available())
            {
                uintptr_t ptr_bits = reinterpret_cast<uintptr_t>(f);
                assert((ptr_bits & ~PAYLOAD_MASK) == 0 && "pointer doesn't fit in NaN-box payload");
                v.set_bits(make_nanbox_bits(JsType::Function, static_cast<uint64_t>(ptr_bits)));
            }
            else
            {
                TaggedValue tv;
                tv.type = JsType::Function;
                tv.function = f;
                v.set_tagged(tv);
            }
            return v;
        }

        // predicates -----------------------------------------------------
        bool is_number() const noexcept
        {
            if (nan_boxing_available())
            {
                // Numbers are either:
                // - any non-NaN double (i.e., pattern does NOT match QNAN_MASK),
                // - or the special NaN-boxed sentinel tagged as Number.
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return true;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Number);
            }
            else
            {
                return storage.tagged.type == JsType::Number;
            }
        }

        bool is_string() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::String);
            }
            else
            {
                return storage.tagged.type == JsType::String;
            }
        }

        bool is_object() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Object);
            }
            else
            {
                return storage.tagged.type == JsType::Object;
            }
        }

        bool is_array() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Array);
            }
            else
            {
                return storage.tagged.type == JsType::Array;
            }
        }

        bool is_function() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Function);
            }
            else
            {
                return storage.tagged.type == JsType::Function;
            }
        }

        bool is_boolean() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Boolean);
            }
            else
            {
                return storage.tagged.type == JsType::Boolean;
            }
        }

        bool is_null() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Null);
            }
            else
            {
                return storage.tagged.type == JsType::Null;
            }
        }

        bool is_undefined() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Undefined);
            }
            else
            {
                return storage.tagged.type == JsType::Undefined;
            }
        }

        bool is_uninitialized() const noexcept
        {
            if (nan_boxing_available())
            {
                if ((storage.bits & QNAN_MASK) != QNAN_MASK)
                    return false;
                return get_tag_nibble() == static_cast<uint8_t>(JsType::Uninitialized);
            }
            else
            {
                return storage.tagged.type == JsType::Uninitialized;
            }
        }

        // extractors ----------------------------------------------------
        double as_double() const noexcept
        {
            if (nan_boxing_available())
            {
                // If this is the special number-NaN sentinel, return a NaN
                if ((storage.bits & QNAN_MASK) == QNAN_MASK &&
                    get_tag_nibble() == static_cast<uint8_t>(JsType::Number))
                {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                assert(is_number());
                return bits_to_double(storage.bits);
            }
            else
            {
                return storage.tagged.number;
            }
        }

        bool as_boolean() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_boolean());
                return (storage.bits & 1ULL) != 0;
            }
            else
            {
                return storage.tagged.boolean;
            }
        }

        std::string *as_string() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_string());
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<std::string *>(payload);
            }
            else
            {
                return storage.tagged.str;
            }
        }

        JsObject *as_object() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_object());
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsObject *>(payload);
            }
            else
            {
                return storage.tagged.object;
            }
        }

        JsArray *as_array() const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_array());
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsArray *>(payload);
            }
            else
            {
                return storage.tagged.array;
            }
        }

        JsFunction *as_function(const std::string &name = "") const noexcept
        {
            if (nan_boxing_available())
            {
                assert(is_function());
                uintptr_t payload = storage.bits & PAYLOAD_MASK;
                return reinterpret_cast<JsFunction *>(payload);
            }
            else
            {
                return storage.tagged.function;
            }
        }

        // small utility: tell whether runtime chose NaN-boxing
        static bool runtime_using_nan_boxing() noexcept
        {
            return nan_boxing_available();
        }

        // convert value to raw string
        std::string convert_to_raw_string() const noexcept
        {
            if (is_undefined())
                return "undefined";
            if (is_null())
                return "null";
            if (is_uninitialized())
                return "<uninitialized>";
            if (is_boolean())
                return as_boolean() ? "true" : "false";
            if (is_number())
            {
                auto value = as_double();
                if (std::isnan(value))
                {
                    return "NaN";
                }
                if (std::abs(value) >= 1e21 || (std::abs(value) > 0 && std::abs(value) < 1e-6))
                {
                    std::ostringstream oss;
                    oss << std::scientific << std::setprecision(4) << value;
                    return oss.str();
                }
                else
                {
                    std::ostringstream oss;
                    oss << std::setprecision(6) << std::fixed << value;
                    std::string s = oss.str();
                    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                    if (!s.empty() && s.back() == '.')
                    {
                        s.pop_back();
                    }
                    return s;
                }
            }
            if (is_string())
                return *as_string();
            if (is_object())
                return as_object()->to_raw_string();
            if (is_array())
                return as_array()->to_raw_string();
            if (is_function())
                return as_function()->to_raw_string();
            // should not be reached
            return "";
        }

        AnyValue &operator[](const std::string &key)
        {
            std::cout << key << std::endl;
            if (is_object())
                return (*as_object())[key];
            if (is_array())
                return (*as_array())[key];
            if (is_function())
                return (*as_function())[key];
            static AnyValue empty{};
            return empty;
        }
        AnyValue &operator[](uint32_t idx)
        {
            if (is_array())
                return (*as_array())[idx];
            static AnyValue empty{};
            return empty;
        }
        AnyValue &operator[](const AnyValue &key)
        {
            return (*this)[key.convert_to_raw_string()];
        }
    };

} // namespace jspp