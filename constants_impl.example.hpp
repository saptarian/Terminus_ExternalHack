// Just in case the user wants to use the entire document in a constexpr
// context, it can be included safely
#ifndef Entries_COMPILED_JSON_IMPL
#define Entries_COMPILED_JSON_IMPL
#include <json2cpp/json2cpp.hpp>

namespace compiled_json::Entries::impl {

using json = json2cpp::basic_json<char>;
using data_t = json2cpp::data_variant<char>;
using string_view = std::basic_string_view<char>;
using array_t = json2cpp::basic_array_t<char>;
using object_t = json2cpp::basic_object_t<char>;
using value_pair_t = json2cpp::basic_value_pair_t<char>;

inline constexpr std::array<value_pair_t, 5> object_data_45 = {
    value_pair_t{R"string(adr_alias)string",
                 {string_view{R"string(example_address_alias)string"}}},
    value_pair_t{
        R"string(disable)string",
        {string_view{
            R"string(00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF)string"}}},
    value_pair_t{
        R"string(enable)string",
        {string_view{
            R"string(00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF)string"}}},
    value_pair_t{R"string(offset)string", {std::uint64_t{0}}},
    value_pair_t{
        R"string(scan)string",
        {string_view{
            R"string(00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF)string"}}},
};
inline constexpr std::array<json, 1> object_data_44 = {{
    {object_t{object_data_45}},
}};
inline constexpr std::array<value_pair_t, 3> object_data_41 = {
    value_pair_t{R"string(id)string", {std::uint64_t{5}}},
    value_pair_t{
        R"string(name)string",
        {string_view{R"string(Maxed out conditions next turn)string"}}},
    value_pair_t{R"string(records)string", {array_t{object_data_44}}},
};

inline constexpr std::array<json, 18> object_data_0 = {{
    {object_t{object_data_41}},
}};

inline constexpr auto document = json{{array_t{object_data_0}}};

}  // namespace compiled_json::Entries::impl

#endif
