module;
#include <vector>
#include <string>
#include <variant>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <functional>
#include <format>
export module lib:stdlib;
export namespace std
{
    using std::vector;
    using std::format;
    using string = std::string;
    using string_view = std::string_view;
    using std::printf;
    using std::to_string;
    using std::basic_string;
    using std::char_traits;
    using std::variant;
    using std::visit;
    using std::monostate;
    using std::bind;  
    using std::is_same_v;    
    using std::int8_t;    
    using std::uint8_t;    
    using std::int32_t;    
    using std::uint32_t;    
    using std::int64_t;
    using std::uint64_t;
};

