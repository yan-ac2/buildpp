#include <cstddef>
#include <type_traits>
#include <string_view>
#include <string>
#include <charconv>
#include <iostream>


template<size_t N>
struct formatString {
    static constexpr size_t NPlaceholder {N * 2};
    const char* sv;
    const size_t count;
    size_t posArray[NPlaceholder];
    
    template<size_t I>
    consteval formatString(const char (&str)[I]) noexcept
        : sv{str}, count(I - 1) { getPosSize(str, I - 1); }
    consteval formatString(formatString&& str) noexcept
        : sv{str.sv}, count(str.count) 
        { 
            size_t idx {0};
            for (size_t s : str.posArray) posArray[idx++] = s;    
        }

    constexpr formatString(std::string_view inSv) noexcept
        : sv(inSv.data()), count(getPosSize(sv, inSv.size())) {}

    constexpr auto getPosSize(const char* str, const size_t strLen) noexcept -> size_t {
        size_t slot = 0;
        bool openFound = false;
        for (size_t idx = 0; idx < strLen && slot < N * 2; ++idx) {
            if (str[idx] == '{') {
                posArray[slot]     = idx;
                openFound = true;
            } 
            if (str[idx] == '}' && openFound) {
                posArray[slot + 1] = (idx + 1) - posArray[slot];
                slot += 2;
                openFound = false;
            }
        }
        return strLen;
    }
};

template<typename T> 
struct converter {
    converter(T&&) {}

};
template<> 
struct converter<const char*> {
    std::string_view sv;
    converter(const char* str) : sv(str) {}
    converter(const char* str,size_t count) : sv(str,count) {}
    template<size_t N>
    converter(const char (&str)[N]) : sv(str,N) {}
    constexpr auto appendArg(std::string& str,std::size_t offset,const std::size_t& Pos) -> std::size_t {
        const size_t start = Pos;
        const size_t len = *(&Pos + 1);
        const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(start) + offset);
        str.replace(actualStart, len, sv);
        return static_cast<std::ptrdiff_t>(sv.size()) - static_cast<std::ptrdiff_t>(len);
    } 
};
template<> 
struct converter<std::size_t> {
    char digit[64];
    size_t len;
    converter(std::size_t arg) : len(std::to_chars(digit, digit + sizeof(digit), arg).ptr - digit) {}
    constexpr auto appendArg(std::string& str,std::size_t offset,const std::size_t& Pos) -> std::size_t {
        const size_t start = Pos;
        const size_t len = *(&Pos + 1);
        const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(start) + offset);
        std::string_view sv {digit, len};
        str.replace(actualStart, len, sv);
        return static_cast<std::ptrdiff_t>(sv.size()) - static_cast<std::ptrdiff_t>(len);
    } 
};
template <typename T>
constexpr auto appendArg(T&& arg,std::string& str, std::ptrdiff_t& offset, const size_t& pos) noexcept -> void {
    using Raw = std::remove_cvref_t<T>;
    const size_t start = pos;
    const size_t len = (&pos)[1];
    std::string_view argStr;
    // 1. Calculate shifted start position
    const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(start) + offset);

    if constexpr (std::is_convertible_v<Raw, std::string_view>) {
        argStr = std::string_view(arg);
        // 2. Replace only the placeholder length (Pos.len)
        str.replace(actualStart, len, argStr);
        // 3. Accumulate delta into offset for the next replacement
        offset += static_cast<std::ptrdiff_t>(argStr.size()) - static_cast<std::ptrdiff_t>(len);
    } else if constexpr (std::integral<Raw> || std::floating_point<Raw>) {
        char digit[64];
        auto [ptr, ec] = std::to_chars(digit, digit + sizeof(digit), arg);
        argStr = std::string_view(digit, static_cast<size_t>(ptr - digit));
        str.replace(actualStart, len, argStr);
        offset += static_cast<std::ptrdiff_t>(argStr.size()) - static_cast<std::ptrdiff_t>(len);
    }

};

template<typename... Args>
constexpr auto sformat(formatString<sizeof...(Args)> fmtStr,Args&&... args) noexcept -> std::string {
    std::string str {fmtStr.sv,fmtStr.count};
    if constexpr (sizeof...(Args) > 0) {
        size_t* posArray = fmtStr.posArray;
        size_t lastPos = 0;
        std::ptrdiff_t offset = 0;
        ((appendArg(std::forward<Args>(args),str,offset,posArray[lastPos]),lastPos += 2), ...);
    }

    return str;
}


struct strColors {
    enum colors {
        Not_color = 0,
        Black,     Bold_Black,     High_Black,
        Red,       Bold_Red,       High_Red,
        Green,     Bold_Green,     High_Green,
        Yellow,    Bold_Yellow,    High_Yellow,
        Blue,      Bold_Blue,      High_Blue,
        Purple,    Bold_Purple,    High_Purple,
        Cyan,      Bold_Cyan,      High_Cyan,
        White,     Bold_White,     High_White,
    };
    
    static constexpr auto getColor(colors color) noexcept -> std::string_view {
        switch (color) {
            case Not_color:    return "\033[0m";
            case Black:        return "\033[0;30m";
            case Red:          return "\033[0;31m";
            case Green:        return "\033[0;32m";
            case Yellow:       return "\033[0;33m";
            case Blue:         return "\033[0;34m";
            case Purple:       return "\033[0;35m";
            case Cyan:         return "\033[0;36m";
            case White:        return "\033[0;37m";
            case Bold_Black:   return "\033[1;30m";
            case Bold_Red:     return "\033[1;31m";
            case Bold_Green:   return "\033[1;32m";
            case Bold_Yellow:  return "\033[1;33m";
            case Bold_Blue:    return "\033[1;34m";
            case Bold_Purple:  return "\033[1;35m";
            case Bold_Cyan:    return "\033[1;36m";
            case Bold_White:   return "\033[1;37m";
            case High_Black:   return "\033[0;90m";
            case High_Red:     return "\033[0;91m";
            case High_Green:   return "\033[0;92m";
            case High_Yellow:  return "\033[0;93m";
            case High_Blue:    return "\033[0;94m";
            case High_Purple:  return "\033[0;95m";
            case High_Cyan:    return "\033[0;96m";
            case High_White:   return "\033[0;97m";
            default:           __builtin_unreachable();
        }
    }
};

constexpr auto addColors(std::string_view str,strColors::colors c) noexcept -> std::string {
    constexpr std::string_view notcolor = strColors::getColor(strColors::Not_color);
    const std::string_view color = strColors::getColor(c);
    const std::size_t len = str.size() + notcolor.size() + color.size();
    std::string temp;
    temp.reserve(len);
    for (auto& c : {color,str,notcolor}) {
        temp += c;
    }
    return temp;
}

int main()
{
    struct test {
        const char* str;
        size_t num;
    };
    std::string makeOptions  = sformat("{{{}}} {} {}\n", 2,3,4);
    std::string makeOptions2 = sformat("{} {}\n",addColors("hello world",strColors::Red),12);
    std::string makeOptions3 = sformat("hello {}\n",2);
    std::string makeOptions4;
    for (auto s : {test{"hello",5},test{"world",15},test{"num",65}}) {
        makeOptions4 += sformat("{} {} ",s.str,s.num);
    }
    std::string makeOptions5;
    for (auto s : {"{}","{}","{}","\n"}) {
        makeOptions5 += s;
    }
    std::string makeOptions6 = sformat(std::string_view(makeOptions5),1,2,3);

    std::cout << makeOptions;
    std::cout << makeOptions2;
    std::cout << makeOptions3;
    std::cout << makeOptions4 << "\n";
    std::cout << makeOptions5;
    std::cout << makeOptions6;
    
    return 0; 
}