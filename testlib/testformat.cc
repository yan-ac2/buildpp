
#include <windows.h>

#include <type_traits>
#include <string_view>
#include <string>
#include <source_location>
#include <charconv>
#include <cassert>
#include <cstdlib>

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
    constexpr auto strlen(char_array_type str) noexcept -> size_type {
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


struct addColors_impl {
    using string_type = std::string;
    using view_type = std::string_view;
    using size_type = size_t;
    strColors coloredstr;
    
    template<size_type N>
    consteval addColors_impl(const char (&in)[N],const strColors::colors c) noexcept
    :coloredstr{{in,N - 1},c} {}
    constexpr addColors_impl(const view_type in,const strColors::colors c) noexcept
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

constexpr auto addColors(std::string_view sv,strColors::colors c) noexcept -> std::string {
    return addColors_impl(sv,c).string();
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
template<>
struct formatter<addColors_impl> {
    static constexpr auto format(addColors_impl& val) -> std::string {
        return val.string();
    }
};

template<size_t N>
struct formatter<char[N]> {
    static constexpr auto format(const char* val) -> std::string {
        return std::string(val,N - 1);
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
constexpr auto format_arg (T& arg) noexcept -> std::string {
    using Decayed = std::decay_t<T>;
    if constexpr (std::is_convertible_v<Decayed, std::string_view>) {
        return formatter<std::string_view>::format(std::string_view(arg));
    } else {
        return formatter<Decayed>::format(arg);
    }
};

constexpr auto merge_args(std::string& str,const std::string fargs[],std::string_view sv,size_t posArray[],size_t count) noexcept -> void {
    const char* data  = sv.data();
    const size_t size = sv.size();
    size_t begin {0};
    const char* start = data + *posArray;
    for (const char& c : sv) {
        if (&c == start) {
            const size_t idx   {static_cast<size_t>(&c - data)};
            const size_t len = *posArray + *(posArray + 1);
            str.reserve((str.size() + (idx - begin)) + fargs->size());
            (str += {data + begin, idx - begin}) += *fargs;
            begin = len;
            if (--count > 0) {
                posArray += 2; start = data + *posArray;
                ++fargs;
            }
        }
    }
    str += std::string_view{data + begin,size - begin};
}

template<typename... Args>
constexpr auto sformat(formatString<sizeof...(Args)> fmtStr,Args&&... args) noexcept -> std::string {
    std::string str {};
    if constexpr (sizeof...(Args) > 0) {
        auto* posArray = fmtStr.begin();
        const std::string fargs[sizeof...(Args)] {format_arg(args)...};
        merge_args(str,fargs,fmtStr.sv,posArray,sizeof...(Args));
    } else {
        str = fmtStr.sv;
    }
    return str; 
}

template <typename T,size_t level = 0>
struct comp_t;

template <typename T, size_t L = 0>
struct input_t : comp_t<T,L> {
    T data;
    const std::source_location loc;
    template<typename U>
    constexpr input_t(U&& val, std::source_location s = std::source_location::current()) noexcept 
    : data(val), loc(s) {}
};

template <typename T>
struct input_t<T, 0> : comp_t<T,0> {
    T data;
    
    template<typename U>
    constexpr input_t(U&& val) noexcept : data(val) {}
};
template <typename T>
struct input_t<T,1> : comp_t<T,1> {
    T data;
    template<typename U>
    constexpr input_t(U&& val) noexcept 
    : data(val) {}
};

template <typename T,size_t level>
struct comp_t {
    using type = input_t<T,level>;

    template <typename R>
    friend constexpr bool operator <(type& s,R&& rhs) noexcept {
        return s.data < rhs;
    }
    template <typename R>
    friend constexpr bool operator <=(type& s,R&& rhs)noexcept {
        return s.data <= rhs;
    }
    template<typename R>
    friend constexpr bool operator >(type& s,R&& rhs) noexcept {
        return s.data > rhs;
    }
    template<typename R>
    friend constexpr bool operator >=(type& s,R&& rhs) noexcept {
        return s.data >= rhs;
    }
    template<typename R>
    friend constexpr bool operator ==(type& s,R&& rhs) noexcept {
        return s.data == rhs;
    }
    template<typename R>
    friend constexpr bool operator !=(type& s,R&& rhs) noexcept {
        return s.data != rhs;
    }
};

namespace log {
    
    enum log_type {
        Log , Warning, Error
    };
    static constexpr std::string_view error_header   = "\033[1;31mError:\033[1;0m";
    static constexpr std::string_view warning_header = "\033[0;33mWarning:\033[1;0m";
    static constexpr std::string_view log_header     = "Log:";
    namespace {
        void errmsg_impl(const char header[],int hs,const char msg[],int msgs,const char input[],int ins) noexcept {
            std::fprintf(stderr,"\n%*s %*s \nWhat: with value [%*s]", 
                hs,header,
                msgs,msg,
                ins,input
            );
        }
        void errmsg_impl(const char header[],int hs,const char msg[],int msgs,const char input[],int ins,const std::source_location& loc) noexcept {
    
            const unsigned int ll[] {loc.line(),loc.column()};
            std::fprintf(stderr,"\n%*s %s:%u:%u \nWhat: %*s [%*s]",
                hs,header,
                loc.file_name(),
                ll[0],ll[1],
                msgs,msg,
                ins,input
            );
        }
    }
    template <size_t L>
    consteval std::string_view get_level() noexcept {
        return 
        L < 3 && L > 0 ? warning_header :
        L == 3 ? error_header:
        log_header;
    }
    
    template<typename T,size_t L> requires (std::is_integral_v<T> || std::is_convertible_v<T, std::string_view>)
    void errmsg(std::string_view msg,const input_t<T,L>& input) {
        if constexpr (L > 0) {
            const std::string_view t {get_level<L>()};
            if constexpr (std::is_integral_v<T>) {
                char value[64];
                char* ptr = &(*std::to_chars(value,value + sizeof(value),input.data).ptr = '\0');
                if constexpr (L == 1) errmsg_impl(t.data(),t.size(),msg.data(),msg.size(),value,ptr - value);
                else errmsg_impl(t.data(),t.size(),msg.data(),msg.size(),value,ptr - value,input.loc);
            } else {
                const std::string_view in {input.data};
                if constexpr (L == 1) errmsg_impl(t.data(),t.size(),msg.data(),msg.size(),in.data(),in.size());
                else errmsg_impl(t.data(),t.size(),msg.data(),msg.size(),in.data(),in.size(),input.loc);
            }
        }
        if constexpr(L > 2) std::exit(1);
    }
};

struct testArray {
    char data[24];
    
    constexpr auto size() const -> size_t {
        return sizeof(data) - 1; 
    }
    using index_type = input_t<size_t,2>;
    constexpr char& operator[](index_type idx) {
        if (idx > size()) log::errmsg("index error", idx);
        return  data[idx.data];
    };

    constexpr char* begin() {
        return data;
    }
    constexpr char* end() {
        return &data[size()];
    }
};


struct runtime {
    UINT originalCP = GetConsoleOutputCP();
    runtime() {
        SetConsoleOutputCP(CP_UTF8);
    }

    ~runtime(){
        SetConsoleOutputCP(originalCP);
    }
};


int main()
{
    // runtime app_rt;
    // struct test {
    //     const char* str;
    //     size_t num;
    // };
    testArray d;
    for (auto& c : d) {
            c = 'a';
    }
    d[2] = 'c';
    d[22] = 'c';
    for (auto i : {1,2,3,4,5,7,8,9,15,24}) {
        // std::cout << d[i] << " ";
        d[i];
    }
    d[22] = 'c';
    std::printf("%s","\ndone\n");
    
    // constexpr size_t num1 {1};
    // size_t num2 {11};
    // size_t num3 {11};
    // constexpr std::string makeOptions {sformat("{{{}}} {} {} {}\n", 123,"hello",num1,12345)};
    // const std::string makeOptions9 {sformat("{{{}}} {} {}\n", num1,num2,num3)};
    // const std::string makeOptions2 = sformat("{} {}\n",addColors("hello world",strColors::Red),12);
    // const std::string makeOptions3 = sformat("hello {}\n",2);
    // std::string makeOptions4;
    // for (auto s : {test{"hello",5},test{"world",15},test{"num",65}}) {
    //     makeOptions4 += sformat("{} {} ",s.str,s.num);
    // }
    // std::string makeOptions5;
    // for (auto s : {"{}","{}","{}","\n"}) {
    //     makeOptions5 += s;
    // }
    // constexpr formatString<2> fstr {"{} {}\n"};
    // const std::string makeOptions6 = sformat(makeOptions5,1,2,3);
    // const std::string makeOptions7 = sformat(fstr,1,2);
    // const std::string makeOptions8 = sformat(fstr,1.213,435.f);
    // const std::string makeOptions10 = sformat("using snformat 2 {} {} {}","hello",2.23f,3);
    // std::cout << makeOptions;
    // std::cout << makeOptions2;
    // std::cout << makeOptions3;
    // std::cout << makeOptions4 << "\n";
    // std::cout << makeOptions5;
    // std::cout << makeOptions6;
    // std::cout << makeOptions7;
    // std::cout << makeOptions8;
    // std::cout << makeOptions9;
    // std::cout << makeOptions10;

    return 0; 
}

