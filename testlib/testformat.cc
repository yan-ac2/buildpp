#include <cstddef>
#include <type_traits>
#include <string_view>
#include <string>
#include <charconv>
#include <iostream>

template<size_t N>
struct formatString {
    static constexpr size_t NPlaceholder {N * 2};
    std::string_view sv;
    size_t posArray[NPlaceholder];
    template<size_t I>
    consteval formatString(const char (&str)[I]) noexcept
        : sv{str,I - 1} {getPosSize(sv);}
    constexpr formatString(const char* inSv) noexcept
        : sv{inSv} { getPosSize(sv); }
    constexpr formatString(std::string_view inSv) noexcept
        : sv{inSv} { getPosSize(sv); }
    constexpr formatString(const std::string& inSv) noexcept
        : sv{inSv} { getPosSize(sv); }

    constexpr formatString(formatString&& str) noexcept
        : sv{str.sv}
    {
        size_t idx {0}; 
        for (size_t s : str.posArray) posArray[idx++] = s; 
    }
    constexpr formatString(const formatString& str) noexcept
        : sv{str.sv}
    {
        size_t idx {0}; 
        for (size_t s : str.posArray) posArray[idx++] = s; 
    }


    constexpr size_t getPosSize(std::string_view str) noexcept {
        bool open = false;
        size_t slot = 0,idx = 0, openidx = 0;
        for (char c : str) {
            if (slot > NPlaceholder) break;
            if (c == '{') {
                openidx = idx;
                open = true;
            } else if (c == '}' && open) {
                posArray[slot] = openidx;
                posArray[slot + 1] = (idx + 1) - openidx;
                slot += 2;
                open = false;
            }
            ++idx;
        }
        return str.size();
    }
};



struct strColors {
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
    static constexpr std::string_view colorTable[] {
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
};


struct addColors {
    static constexpr std::string_view notcolor = strColors::getColor(strColors::Not_color);
    std::string_view str[3];
    constexpr addColors(std::string_view str,const strColors::colors c) noexcept
    :str{strColors::getColor(c),str,notcolor} {}
    std::string_view* begin() noexcept {return str;}
    std::string_view* end()   noexcept {return str + 3;}
};

// template<typename T> 
// struct converter {
//     converter(T&&) {}

// };
// template<> 
// struct converter<const char*> {
//     std::string_view sv;
//     converter(const char* str) : sv(str) {}
//     converter(const char* str,size_t count) : sv(str,count) {}
//     template<size_t N>
//     converter(const char (&str)[N]) : sv(str,N) {}
//     constexpr auto appendArg(std::string& str,std::size_t offset,const std::size_t& Pos) -> std::size_t {
//         const size_t start = Pos;
//         const size_t len = *(&Pos + 1);
//         const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(start) + offset);
//         str.replace(actualStart, len, sv);
//         return static_cast<std::ptrdiff_t>(sv.size()) - static_cast<std::ptrdiff_t>(len);
//     } 
// };
// template<> 
// struct converter<std::size_t> {
//     char digit[64];
//     size_t len;
//     converter(std::size_t arg) : len(std::to_chars(digit, digit + sizeof(digit), arg).ptr - digit) {}
//     constexpr auto appendArg(std::string& str,std::size_t offset,const std::size_t& Pos) -> std::size_t {
//         const size_t start = Pos;
//         const size_t len = *(&Pos + 1);
//         const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(start) + offset);
//         std::string_view sv {digit, len};
//         str.replace(actualStart, len, sv);
//         return static_cast<std::ptrdiff_t>(sv.size()) - static_cast<std::ptrdiff_t>(len);
//     } 
// };
template <typename T>
constexpr auto appendArg(T&& arg,std::string& str, std::ptrdiff_t& offset, const size_t& pos) noexcept -> void {
    using Raw = std::remove_cvref_t<T>;
    const size_t start = pos;
    const size_t len = (&pos)[1];
    // 1. Calculate shifted start position
    const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(start) + offset);

    if constexpr (std::is_same_v<Raw, addColors>) {
        std::string temp;
        for (std::string_view sv : arg) {
            temp += sv;
        }
        str.replace(actualStart, len, temp);
        offset += static_cast<std::ptrdiff_t>(temp.size()) - static_cast<std::ptrdiff_t>(len);
    } else {
        std::string_view argStr;
        if constexpr (std::is_convertible_v<Raw, std::string_view>) {
            argStr = std::string_view(arg);
            // 2. Replace only the placeholder length (Pos.len)
            str.replace(actualStart, len, argStr);
        } else if constexpr (std::integral<Raw> || std::floating_point<Raw>) {
            char digit[64];
            auto [ptr, ec] = std::to_chars(digit, digit + sizeof(digit), arg);
            argStr = std::string_view(digit, static_cast<size_t>(ptr - digit));
            str.replace(actualStart, len, argStr);
        }
        // 3. Accumulate delta into offset for the next replacement
        offset += static_cast<std::ptrdiff_t>(argStr.size()) - static_cast<std::ptrdiff_t>(len);

    }

};

template<typename... Args>
constexpr auto sformat(formatString<sizeof...(Args)> fmtStr,Args&&... args) noexcept -> std::string {
    std::string str {fmtStr.sv};
    if constexpr (sizeof...(Args) > 0) {
        size_t* posArray = fmtStr.posArray;
        size_t lastPos = 0;
        std::ptrdiff_t offset = 0;
        ((appendArg(std::forward<Args>(args),str,offset,posArray[lastPos]),lastPos += 2), ...);
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

    std::cout << makeOptions;
    std::cout << makeOptions2;
    std::cout << makeOptions3;
    std::cout << makeOptions4 << "\n";
    std::cout << makeOptions5;
    std::cout << makeOptions6;
    std::cout << makeOptions7;
    std::cout << makeOptions8;
    std::cout << makeOptions9;
    
    return 0; 
}
