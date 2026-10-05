
#include <type_traits>
#include <string_view>
#include <string>
#include <charconv>
#include <iostream>

using size_t = decltype(sizeof(0));
using ptrdiff_t = decltype((char*)(nullptr) - (char*)(nullptr));

template<size_t N>
struct formatString {
    using size_type = size_t;
    static constexpr size_type NPlaceholder {N * 2};
    using char_array_type = const char*;
    using PosArray_type = size_type[NPlaceholder];

    struct posArray {
        PosArray_type data;
    };
    const std::string_view sv;
    // const posArray Array;
    posArray Array;
    constexpr formatString(char_array_type str) noexcept
        : sv{str,strlen(str)} , Array(getPosSize(sv)) {}
    constexpr formatString(std::string_view inSv) noexcept : 
    sv{inSv}, Array(getPosSize(sv)) {}
    constexpr formatString(const std::string& inSv) noexcept: 
    sv{inSv} , Array(getPosSize(sv)) {}

    constexpr formatString(formatString&& str) noexcept : 
    sv{str.sv} , Array(str.Array) {}
    constexpr formatString(const formatString& str) noexcept : 
    sv{str.sv}, Array(str.Array) {}
    
    constexpr auto getArray() noexcept -> PosArray_type& { return Array.data;}
    constexpr auto begin() noexcept -> size_type* { return Array.data;}
    constexpr auto end() noexcept -> size_type* { return Array.data + NPlaceholder;}
    constexpr auto getPosSize(std::string_view str) noexcept -> posArray {
        posArray temp;
        const char* data = str.data();
        const size_type strlen = str.size();
        bool open = false;
        size_type slot = 0,idx = 0, openidx = 0;
        for (;(idx < strlen) && (slot < NPlaceholder);++idx) {
            if (*(data + idx) == '{') {
                openidx = idx;
                open = true;
            } else if (*(data + idx) == '}' && open) {
                temp.data[slot] = openidx;
                temp.data[slot + 1] = (idx + 1) - openidx;
                slot += 2;
                open = false;
            }
        }
        return temp;
    }
    consteval auto strlen(char_array_type str) noexcept -> size_type {
        size_type temp{};
        for(;*str++ != '\0';temp++);
        return temp;
    }
};

struct strColors {
    using view_type = std::string_view;
    using size_type = size_t;
    enum colors {
        Not_color = 0,
        Black     = 1,     Bold_Black = 9,      High_Black = 17,
        Red       = 2,       Bold_Red = 10,       High_Red = 18,
        Green     = 3,     Bold_Green = 11,     High_Green = 19,
        Yellow    = 4,    Bold_Yellow = 12,    High_Yellow = 20,
        Blue      = 5,      Bold_Blue = 13,      High_Blue = 21,
        Purple    = 6,    Bold_Purple = 14,    High_Purple = 22,
        Cyan      = 7,      Bold_Cyan = 15,      High_Cyan = 23,
        White     = 8,     Bold_White = 16,     High_White = 24,
    };
    const view_type current[3];
    const size_type len;
    constexpr strColors(const view_type str,colors c) : 
    current(colorTable[static_cast<size_type>(c)],str,colorTable[0]),
    len(getLen()) {}

    static constexpr view_type colorTable[26] {
        "\033[0m",    //Not_color 
        "\033[0;30m", //Black
        "\033[0;31m", //Red
        "\033[0;32m", //Green
        "\033[0;33m", //Yellow
        "\033[0;34m", //Blue
        "\033[0;35m", //Purple
        "\033[0;36m", //Cyan
        "\033[0;37m", //White
        "\033[1;30m", //Bold_Black
        "\033[1;31m", //Bold_Red
        "\033[1;32m", //Bold_Green
        "\033[1;33m", //Bold_Yellow
        "\033[1;34m", //Bold_Blue
        "\033[1;35m", //Bold_Purple
        "\033[1;36m", //Bold_Cyan
        "\033[1;37m", //Bold_White
        "\033[0;90m", //High_Black
        "\033[0;91m", //High_Red
        "\033[0;92m", //High_Green
        "\033[0;93m", //High_Yellow
        "\033[0;94m", //High_Blue
        "\033[0;95m", //High_Purple
        "\033[0;96m", //High_Cyan
        "\033[0;97m", //High_White
    };
    
    static constexpr auto getColor(colors color) noexcept -> std::string_view {
        return colorTable[static_cast<std::size_t>(color)];
    }
    constexpr auto getLen() noexcept -> size_type {
        return size_type{current[0].size() + current[1].size() + current[2].size()};
    }
};


struct addColors {
    using string_type = std::string;
    using view_type = std::string_view;
    using size_type = size_t;
    strColors coloredstr;
    
    template<size_type N>
    consteval addColors(const char (&in)[N],const strColors::colors c) noexcept
    :coloredstr{{in,N - 1},c} {}
    constexpr addColors(const view_type in,const strColors::colors c) noexcept
    :coloredstr{in,c} {}
    constexpr operator string_type() const noexcept {
        return string();
    }
    constexpr auto string() const noexcept -> string_type {
        auto& [sv,len] = coloredstr;
        std::string temp(len,'\0');
        const std::string_view* idx = sv;
        char* data = temp.data();
        char* dend = temp.data() + len;
        const char* start = idx->begin();
        const char* end = idx->end();
        for (;data < dend;) {
            if(start == end) {idx++;start = idx->begin();end = idx->end();}
            *data++ = *start++;
        }
        return temp;
    }
}; 

template<typename T>
struct formatter {
    // Custom types specialize this struct and implement format(val, ctx)
    static_assert(!sizeof(T*), "Type does not have a custom_formatter specialization!");
};

// Specialization: String Views / C-Strings
template<>
struct formatter<std::string_view> {
    static constexpr auto format(std::string_view val) -> std::string {
        return std::string{val};
    }
};

template<size_t N>
struct formatter<char[N]> {
    static constexpr auto format(const char* val) -> std::string {
        return std::string{val,N - 1};
    }
};

template<>
struct formatter<const char*> {
    static constexpr auto format(const char* val) -> std::string {
        return std::string{val};
    }
};

// Specialization: Integral Types
template<typename T> requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
struct formatter<T> {
    static constexpr auto format(T val) -> std::string {
        char buffer[64];
        auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), val);
        return std::string{buffer, static_cast<size_t>(ptr - buffer)};
    }
};

template <typename T>
constexpr auto appendArg(T&& arg,std::string& str, std::size_t* offset, const size_t* pos) noexcept -> void {
    using Raw = std::remove_cvref_t<T>;
    const size_t start = *pos;
    const size_t len = *(pos + 1);
    // 1. Calculate shifted start position
    const size_t actualStart = start + *offset;

    if constexpr (std::is_same_v<Raw, addColors>) {
        const size_t current = str.size() - 2;
        str.replace(actualStart, len, arg);
        const size_t diff = str.size() - current;
        *offset += diff - len;
    } else {
        std::string_view argStr;
        if constexpr (std::is_convertible_v<Raw, std::string_view>) {
            argStr = std::string_view(arg);
            // 2. Replace only the placeholder length (Pos.len)
            str.replace(actualStart, len, argStr);
        } else if constexpr (std::integral<Raw> || std::floating_point<Raw>) {
            char digit[64];
            auto [ptr, ec] = std::to_chars(digit, digit + sizeof(digit),arg);
            argStr = std::string_view(digit, static_cast<size_t>(ptr - digit));
            str.replace(actualStart, len, argStr);
        } else if constexpr (std::is_pointer_v<Raw>) {
            // Option 1: Formatting the pointee value (dereferencing)
            using Pointee = std::remove_pointer_t<Raw>;
            if constexpr (std::integral<Pointee> || std::floating_point<Pointee>) {
                char digit[64];
                auto [ptr, ec] = std::to_chars(digit, digit + sizeof(digit), *arg);
                if (ec == std::errc{}) {
                    argStr = std::string_view(digit, static_cast<size_t>(ptr - digit));
                    str.replace(actualStart, len, argStr);
                }
            }
        }
        // 3. Accumulate delta into offset for the next replacement
        *offset += argStr.size() - len;

    }

};

template<typename... Args>
constexpr auto sformat(formatString<sizeof...(Args)> fmtStr,Args&&... args) noexcept -> std::string {
    std::string str {fmtStr.sv};
    if constexpr (sizeof...(Args) > 0) {
        auto posArray = fmtStr.begin();
        size_t offset = 0;
        ((appendArg(std::forward<Args>(args),str,&offset,(posArray)),(posArray) += 2), ...);
    }
    return str;
}
template<typename... Args>
constexpr auto snformat(formatString<sizeof...(Args)> fmtStr,Args&&... args) noexcept -> std::string {
    std::string str {};
    if constexpr (sizeof...(Args) > 0) {
        auto data = fmtStr.sv.data();
        auto posArray = fmtStr.begin();
        auto format_arg = [&]<typename T>(const T& arg) {
            using Decayed = std::decay_t<T>;
            if constexpr (std::is_convertible_v<Decayed, std::string_view>) {
                return formatter<std::string_view>::format(std::string_view(arg));
            } else {
                return formatter<Decayed>::format(arg);
            }
        };
        std::string fargs[sizeof...(Args)] {(format_arg(args),...)};
        size_t begin = 0;
        for(size_t idx{0};idx < sizeof...(Args);++idx) {
            str += std::string_view(data + begin,*posArray);
            str += fargs[idx];
            begin = *(posArray + 1);
            posArray += 2;
        };
    }
    return str;
}

int main()
{
    struct test {
        const char* str;
        size_t num;
    };
    constexpr size_t num1 {1};
    constexpr size_t num2 {10};
    constexpr size_t num3 {11};
    const std::string makeOptions {sformat("{{{}}} {} {}\n", num1,num2,num3)};
    const std::string makeOptions9 {sformat("{{{}}} {} {}\n", num1,num2,num3)};
    const std::string makeOptions2 = sformat("{} {}\n",addColors("hello world",strColors::Red),12);
    const std::string makeOptions3 = sformat("hello {}\n",2);
    std::string makeOptions4;
    for (auto s : {test{"hello",5},test{"world",15},test{"num",65}}) {
        makeOptions4 += sformat("{} {} ",s.str,s.num);
    }
    std::string makeOptions5;
    for (auto s : {"{}","{}","{}","\n"}) {
        makeOptions5 += s;
    }
    constexpr formatString<2> fstr {"{} {}\n"};
    const std::string makeOptions6 = sformat(makeOptions5,1,2,3);
    const std::string makeOptions7 = sformat(fstr,1,2);
    const std::string makeOptions8 = sformat(fstr,1.213,435.f);
    const std::string makeOptions10 = snformat("using snformat {} {}",1.213,435.f);

    std::cout << makeOptions;
    std::cout << makeOptions2;
    std::cout << makeOptions3;
    std::cout << makeOptions4 << "\n";
    std::cout << makeOptions5;
    std::cout << makeOptions6;
    std::cout << makeOptions7;
    std::cout << makeOptions8;
    std::cout << makeOptions9;
    std::cout << makeOptions10;
    
    return 0; 
}
