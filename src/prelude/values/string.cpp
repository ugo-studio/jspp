#include "jspp.hpp"
#include "values/string.hpp"
#include "values/prototypes/string.hpp"

namespace jspp {

// --- JsString Implementation ---

std::string JsString::to_std_string() const
{
    return value;
}

AnyValue JsString::get_property(const std::string &key, const AnyValue &thisVal)
{
    auto proto_fn = StringPrototypes::get(key);
    if (proto_fn.has_value())
    {
        return AnyValue::resolve_property_for_read(proto_fn.value(), thisVal, key);
    }
    if (JsArray::is_array_index(key))
    {
        uint32_t idx = static_cast<uint32_t>(std::stoull(key));
        return get_property(idx);
    }
    return Constants::UNDEFINED;
}

AnyValue JsString::get_property(uint32_t idx)
{
    if (idx < value.length())
    {
        return AnyValue::make_string(std::string(1, value[idx]));
    }
    return Constants::UNDEFINED;
}

// --- StringPrototypes Implementation ---

namespace StringPrototypes {

AnyValue &get_toString_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 { return AnyValue::make_string(thisVal.as_string()->value); },
                                                 "toString");
    return fn;
}

AnyValue &get_iterator_fn()
{
    static AnyValue fn = AnyValue::make_generator([](AnyValue thisVal, std::vector<AnyValue> _) -> jspp::JsIterator<jspp::AnyValue>
                                                  {
                                                      auto self = thisVal.as_string();
                                                      const std::string &value = self->value;
                                                      for (size_t i = 0; i < value.length();)
                                                      {
                                                          unsigned char c = static_cast<unsigned char>(value[i]);
                                                          size_t len = 1;
                                                          if ((c & 0x80) == 0)
                                                              len = 1;
                                                          else if ((c & 0xE0) == 0xC0)
                                                              len = 2;
                                                          else if ((c & 0xF0) == 0xE0)
                                                              len = 3;
                                                          else if ((c & 0xF8) == 0xF0)
                                                              len = 4;

                                                          if (i + len > value.length())
                                                              len = value.length() - i;

                                                          co_yield AnyValue::make_string(value.substr(i, len));
                                                          i += len;
                                                      }
                                                      co_return AnyValue::make_undefined(); },
                                                  "Symbol.iterator");
    return fn;
}

AnyValue &get_length_desc()
{
    static auto getter = [](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
    { return AnyValue::make_number(thisVal.as_string()->value.length()); };
    static auto setter = [](const AnyValue &thisVal, std::span<const AnyValue>) -> AnyValue
    { return Constants::UNDEFINED; };
    static AnyValue desc = AnyValue::make_accessor_descriptor(getter, setter, false, false);
    return desc;
}

AnyValue &get_charAt_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     double pos = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                     int index = static_cast<int>(pos);
                                                     if (index < 0 || index >= self->value.length())
                                                     {
                                                         return AnyValue::make_string("");
                                                     }
                                                     return AnyValue::make_string(std::string(1, self->value[index])); },
                                                 "charAt");
    return fn;
}

AnyValue &get_concat_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     std::string result = thisVal.as_string()->value;
                                                     for (const auto &arg : args)
                                                     {
                                                         result += arg.to_std_string();
                                                     }
                                                     return AnyValue::make_string(result); },
                                                 "concat");
    return fn;
}

AnyValue &get_endsWith_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.empty())
                                                         return Constants::FALSE;
                                                     std::string search = args[0].to_std_string();
                                                     size_t end_pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : self->value.length();

                                                     if (end_pos > self->value.length())
                                                         end_pos = self->value.length();
                                                     if (search.length() > end_pos)
                                                         return Constants::FALSE;

                                                     return AnyValue::make_boolean(self->value.substr(end_pos - search.length(), search.length()) == search); },
                                                 "endsWith");
    return fn;
}

AnyValue &get_includes_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.empty())
                                                         return Constants::FALSE;
                                                     std::string search = args[0].to_std_string();
                                                     size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;

                                                     return AnyValue::make_boolean(self->value.find(search, pos) != std::string::npos); },
                                                 "includes");
    return fn;
}

AnyValue &get_indexOf_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.empty())
                                                         return AnyValue::make_number(-1);
                                                     std::string search = args[0].to_std_string();
                                                     size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                                                     size_t result = self->value.find(search, pos);
                                                     return result == std::string::npos ? AnyValue::make_number(-1) : AnyValue::make_number(result); },
                                                 "indexOf");
    return fn;
}

AnyValue &get_lastIndexOf_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.empty())
                                                         return AnyValue::make_number(-1);
                                                     std::string search = args[0].to_std_string();
                                                     size_t pos = (args.size() > 1 && !args[1].is_undefined()) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : std::string::npos;
                                                     size_t result = self->value.rfind(search, pos);
                                                     return result == std::string::npos ? AnyValue::make_number(-1) : AnyValue::make_number(result); },
                                                 "lastIndexOf");
    return fn;
}

AnyValue &get_padEnd_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     size_t target_length = args.empty() ? 0 : static_cast<size_t>(Operators_Private::ToNumber(args[0]));
                                                     if (self->value.length() >= target_length)
                                                         return AnyValue::make_string(self->value);
                                                     std::string pad_string = (args.size() > 1 && !args[1].is_undefined() && !args[1].to_std_string().empty()) ? args[1].to_std_string() : " ";
                                                     std::string result = self->value;
                                                     while (result.length() < target_length)
                                                     {
                                                         result += pad_string;
                                                     }
                                                     return AnyValue::make_string(result.substr(0, target_length)); },
                                                 "padEnd");
    return fn;
}

AnyValue &get_padStart_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     size_t target_length = args.empty() ? 0 : static_cast<size_t>(Operators_Private::ToNumber(args[0]));
                                                     if (self->value.length() >= target_length)
                                                         return AnyValue::make_string(self->value);
                                                     std::string pad_string = (args.size() > 1 && !args[1].is_undefined() && !args[1].to_std_string().empty()) ? args[1].to_std_string() : " ";
                                                     std::string padding;
                                                     while (padding.length() < target_length - self->value.length())
                                                     {
                                                         padding += pad_string;
                                                     }
                                                     return AnyValue::make_string(padding.substr(0, target_length - self->value.length()) + self->value); },
                                                 "padStart");
    return fn;
}

AnyValue &get_repeat_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     double count = args.empty() ? 0 : Operators_Private::ToNumber(args[0]);
                                                     if (count < 0)
                                                     {
                                                         // In a real implementation, this should throw a RangeError.
                                                         return AnyValue::make_string("");
                                                     }
                                                     std::string result = "";
                                                     for (int i = 0; i < count; ++i)
                                                     {
                                                         result += self->value;
                                                     }
                                                     return AnyValue::make_string(result); },
                                                 "repeat");
    return fn;
}

AnyValue &get_replace_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.size() < 2)
                                                         return AnyValue::make_string(self->value);
                                                     std::string search = args[0].to_std_string();
                                                     std::string replacement = args[1].to_std_string();
                                                     std::string result = self->value;
                                                     size_t pos = result.find(search);
                                                     if (pos != std::string::npos)
                                                     {
                                                         result.replace(pos, search.length(), replacement);
                                                     }
                                                     return AnyValue::make_string(result); },
                                                 "replace");
    return fn;
}

AnyValue &get_replaceAll_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.size() < 2)
                                                         return AnyValue::make_string(self->value);
                                                     std::string search = args[0].to_std_string();
                                                     if (search.empty())
                                                         return AnyValue::make_string(self->value);
                                                     std::string replacement = args[1].to_std_string();
                                                     std::string result = self->value;
                                                     size_t pos = result.find(search);
                                                     while (pos != std::string::npos)
                                                     {
                                                         result.replace(pos, search.length(), replacement);
                                                         pos = result.find(search, pos + replacement.length());
                                                     }
                                                     return AnyValue::make_string(result); },
                                                 "replaceAll");
    return fn;
}

AnyValue &get_slice_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     int len = self->value.length();
                                                     int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                                                     int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                                                     if (start < 0)
                                                         start += len;
                                                     if (end < 0)
                                                         end += len;

                                                     start = std::max(0, std::min(len, start));
                                                     end = std::max(0, std::min(len, end));

                                                     if (start >= end)
                                                         return AnyValue::make_string("");
                                                     return AnyValue::make_string(self->value.substr(start, end - start)); },
                                                 "slice");
    return fn;
}

AnyValue &get_split_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     std::string separator = (args.empty() || args[0].is_undefined()) ? "" : args[0].to_std_string();
                                                     std::vector<jspp::AnyValue> result_vec;

                                                     if (separator.empty())
                                                     {
                                                         for (char c : (self->value))
                                                         {
                                                             result_vec.push_back(AnyValue::make_string(std::string(1, c)));
                                                         }
                                                     }
                                                     else
                                                     {
                                                         std::string temp = (self->value);
                                                         size_t pos = 0;
                                                         while ((pos = temp.find(separator)) != std::string::npos)
                                                         {
                                                             result_vec.push_back(AnyValue::make_string(temp.substr(0, pos)));
                                                             temp.erase(0, pos + separator.length());
                                                         }
                                                         result_vec.push_back(AnyValue::make_string(temp));
                                                     }
                                                     return AnyValue::make_array(std::move(result_vec)); },
                                                 "split");
    return fn;
}

AnyValue &get_startsWith_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     if (args.empty())
                                                         return Constants::FALSE;
                                                     std::string search = args[0].to_std_string();
                                                     size_t pos = (args.size() > 1) ? static_cast<size_t>(Operators_Private::ToNumber(args[1])) : 0;
                                                     if (pos > self->value.length())
                                                         pos = self->value.length();

                                                     return AnyValue::make_boolean(self->value.rfind(search, pos) == pos); },
                                                 "startsWith");
    return fn;
}

AnyValue &get_substring_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     auto self = thisVal.as_string();
                                                     int len = self->value.length();
                                                     int start = args.empty() ? 0 : Operators_Private::ToInt32(args[0]);
                                                     int end = (args.size() < 2 || args[1].is_undefined()) ? len : Operators_Private::ToInt32(args[1]);

                                                     start = std::max(0, start);
                                                     end = std::max(0, end);

                                                     if (start > end)
                                                         std::swap(start, end);

                                                     start = std::min(len, start);
                                                     end = std::min(len, end);

                                                     return AnyValue::make_string(self->value.substr(start, end - start)); },
                                                 "substring");
    return fn;
}

AnyValue &get_toLowerCase_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     std::string result = thisVal.as_string()->value;
                                                     std::transform(result.begin(), result.end(), result.begin(),
                                                                    [](unsigned char c)
                                                                    { return std::tolower(c); });
                                                     return AnyValue::make_string(result); },
                                                 "toLowerCase");
    return fn;
}

AnyValue &get_toUpperCase_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     std::string result = thisVal.as_string()->value;
                                                     std::transform(result.begin(), result.end(), result.begin(),
                                                                    [](unsigned char c)
                                                                    { return std::toupper(c); });
                                                     return AnyValue::make_string(result); },
                                                 "toUpperCase");
    return fn;
}

AnyValue &get_trim_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     const char *whitespace = " \t\n\r\f\v";
                                                     std::string result = thisVal.as_string()->value;
                                                     result.erase(0, result.find_first_not_of(whitespace));
                                                     result.erase(result.find_last_not_of(whitespace) + 1);
                                                     return AnyValue::make_string(result); },
                                                 "trim");
    return fn;
}

AnyValue &get_trimEnd_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     const char *whitespace = " \t\n\r\f\v";
                                                     std::string result = thisVal.as_string()->value;
                                                     result.erase(result.find_last_not_of(whitespace) + 1);
                                                     return AnyValue::make_string(result); },
                                                 "trimEnd");
    return fn;
}

AnyValue &get_trimStart_fn()
{
    static AnyValue fn = AnyValue::make_function([](const AnyValue &thisVal, std::span<const AnyValue> args) -> AnyValue
                                                 {
                                                     const char *whitespace = " \t\n\r\f\v";
                                                     std::string result = thisVal.as_string()->value;
                                                     result.erase(0, result.find_first_not_of(whitespace));
                                                     return AnyValue::make_string(result); },
                                                 "trimStart");
    return fn;
}

std::optional<AnyValue> get(const std::string &key)
{
    if (key == "toString" || key == "valueOf") return get_toString_fn();
    if (key == "length") return get_length_desc();
    if (key == "charAt") return get_charAt_fn();
    if (key == "concat") return get_concat_fn();
    if (key == "endsWith") return get_endsWith_fn();
    if (key == "includes") return get_includes_fn();
    if (key == "indexOf") return get_indexOf_fn();
    if (key == "lastIndexOf") return get_lastIndexOf_fn();
    if (key == "padEnd") return get_padEnd_fn();
    if (key == "padStart") return get_padStart_fn();
    if (key == "repeat") return get_repeat_fn();
    if (key == "replace") return get_replace_fn();
    if (key == "replaceAll") return get_replaceAll_fn();
    if (key == "slice") return get_slice_fn();
    if (key == "split") return get_split_fn();
    if (key == "startsWith") return get_startsWith_fn();
    if (key == "substring") return get_substring_fn();
    if (key == "toLowerCase") return get_toLowerCase_fn();
    if (key == "toUpperCase") return get_toUpperCase_fn();
    if (key == "trim") return get_trim_fn();
    if (key == "trimEnd") return get_trimEnd_fn();
    if (key == "trimStart") return get_trimStart_fn();
    return std::nullopt;
}

std::optional<AnyValue> get(const AnyValue &key)
{
    if (key.is_string())
        return get(key.as_string()->value);

    if (key == AnyValue::from_symbol(WellKnownSymbols::toStringTag)) return get_toString_fn();
    if (key == AnyValue::from_symbol(WellKnownSymbols::iterator)) return get_iterator_fn();

    return std::nullopt;
}

} // namespace StringPrototypes

} // namespace jspp
