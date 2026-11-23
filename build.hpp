#ifndef BUILD_PP 
#define BUILD_PP

#include <cstdio>
#include <filesystem>
#include <vector>
#include <string_view>
#include <string>
#include <set>
#include <unordered_map>

#include <fstream>
#include <utility>
#include <span>
#include <source_location>
#include <mutex>

#include "json.hpp"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace  fs = std::filesystem;
using namespace std::string_view_literals;

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
            const size_t len   {*posArray + *(posArray + 1)};
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
    constexpr input_t(input_t& other) noexcept 
    : data(other.data) , loc(other.loc) {}
    constexpr input_t(input_t&& other) noexcept 
    : data(other.data) , loc(other.loc) {}
    constexpr input_t(input_t&& val, std::source_location s = std::source_location::current()) noexcept 
    : data(val), loc(s) {}
    constexpr input_t(const input_t& val, std::source_location s = std::source_location::current()) noexcept 
    : data(val), loc(s) {}
    template<typename U>
    constexpr input_t(U&& val, std::source_location s = std::source_location::current()) noexcept 
    : data(val), loc(s) {}
    template<typename U>
    constexpr input_t(const U& val, std::source_location s = std::source_location::current()) noexcept 
    : data(val), loc(s) {}
};

template <typename T>
struct input_t<T, 0> : comp_t<T,0> {
    T data;
    constexpr input_t(input_t& other) noexcept 
    : data(other.data) {}
    constexpr input_t(input_t&& other) noexcept 
    : data(other.data) {}
    template<typename U>
    constexpr input_t(U&& val) noexcept : data(val) {}
};
template <typename T>
struct input_t<T,1> : comp_t<T,1> {
    T data;
    constexpr input_t(input_t& other) noexcept 
    : data(other.data) {}
    constexpr input_t(input_t&& other) noexcept 
    : data(other.data) {}
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
        None ,Log , Warning, Error
    };
    inline constexpr std::string_view error_header   = "\033[1;31mError:\033[1;0m";
    inline constexpr std::string_view warning_header = "\033[0;33mWarning:\033[1;0m";
    inline constexpr std::string_view log_header     = "Log:";
    namespace detail {
        inline void msg_impl(const char header[],int hs,const char msg[],int msgs) noexcept {
            std::fprintf(stderr,"\n%*s %*s\n", 
                hs,header,
                msgs,msg
            );
        }
         
        inline void msg_impl(const char header[],int hs,const char msg[],int msgs,const char input[],int ins) noexcept {
            std::fprintf(stderr,"\n%*s %*s \nWhat: with value [ %*s ]\n", 
                hs,header,
                msgs,msg,
                ins,input
            );
        }
        
        inline void msg_impl(const char header[],int hs,const char msg[],int msgs,const char input[],int ins,const std::source_location& loc) noexcept {
            const unsigned int ll[] {loc.line(),loc.column()};
            std::fprintf(stderr,"\n%*s %s:%u:%u \nWhat: %*s [%*s]\n",
                hs,header,
                loc.file_name(),
                ll[0],ll[1],
                msgs,msg,
                ins,input
            );
        }

        template <size_t L>
        inline consteval std::string_view get_level() noexcept {
            return 
            L < 3 && L > 0 ? warning_header :
            L == 3 ? error_header:
            log_header;
        }
    }
    
    template<typename T,size_t L> requires (std::is_integral_v<T> || std::is_convertible_v<T, std::string_view>)
    void msg(std::string_view msg,const input_t<T,L>& input) {
        if constexpr (L > 0) {
            const std::string_view t {detail::get_level<L>()};
            if constexpr (std::is_integral_v<T>) {
                char value[64];
                char* ptr = &(*std::to_chars(value,value + sizeof(value),input.data).ptr = '\0');
                if constexpr (L == 1) detail::msg_impl(t.data(),t.size(),msg.data(),msg.size(),value,ptr - value);
                else detail::msg_impl(t.data(),t.size(),msg.data(),msg.size(),value,ptr - value,input.loc);
            } else {
                const std::string_view in {input.data};
                if constexpr (L == 1) detail::msg_impl(t.data(),t.size(),msg.data(),msg.size(),in.data(),in.size());
                else detail::msg_impl(t.data(),t.size(),msg.data(),msg.size(),in.data(),in.size(),input.loc);
            }
        }
        if constexpr(L > 2) std::exit(1);
    }

    template<log_type L,typename T> requires (std::is_integral_v<T> || std::is_convertible_v<T, std::string_view>)
    void msg(std::string_view msg,const T& input) {
        if constexpr (L > 0) {
            const std::string_view t {detail::get_level<L>()};
            if constexpr (std::is_integral_v<T>) {
                char value[64];
                char* ptr = &(*std::to_chars(value,value + sizeof(value),input).ptr = '\0');
                detail::msg_impl(t.data(),t.size(),msg.data(),msg.size(),value,ptr - value);
            } else {
                const std::string_view in {input};
                detail::msg_impl(t.data(),t.size(),msg.data(),msg.size(),in.data(),in.size());
            }
        }
        if constexpr(L > 2) std::exit(1);
    }
    template<log_type L>
    void msg(std::string_view msg) {
        if constexpr (L > 0) {
            const std::string_view t {detail::get_level<L>()};
            detail::msg_impl(t.data(),t.size(),msg.data(),msg.size());
        }
        if constexpr(L > 2) std::exit(1);
    }
};

class cmdImpl {
    struct PipeDeleter {
        int* exitStatus = nullptr;

        void operator()(FILE* fp) const {
            if (!fp) return;
            
            int rawStatus = pclose(fp);
            if (exitStatus) {
                #if defined(__unix__) || defined(__APPLE__)
                if (rawStatus != -1 && WIFEXITED(rawStatus)) {
                    *exitStatus = WEXITSTATUS(rawStatus);
                } else {
                    *exitStatus = rawStatus;
                }
                #else
                *exitStatus = rawStatus; // Windows (_pclose returns exit code directly)
                #endif
            }
        }
    };
    using pipe = std::unique_ptr<FILE, PipeDeleter>;
    public:
    int ret;
    
    auto run(std::string inCmd) -> cmdImpl& {
        {
            pipe cmd (popen(inCmd.c_str(),"r"),PipeDeleter{&ret});
            if (!cmd) {
                ret = -1; // popen failed to open
                return *this;
            }
        }
        return *this;
    }
    auto err(const std::string& msg) -> int {
        if (ret != 0) log::msg<log::Error>(msg);
        return ret;
    }
    auto operator <<(std::string_view cmd) -> cmdImpl& { return run({cmd.data(),cmd.size()});}
    auto operator >>(std::string_view cmd) -> int      { return err({cmd.data(),cmd.size()});}
};
inline cmdImpl cmd;

struct outputPath {
    fs::path rootPath   {};
    fs::path buildPath  {};
    fs::path objPath    {};
    fs::path stdPath    {};
    fs::path modulePath {};
    fs::path exePath    {};

    auto setRootPath(std::string_view root) -> outputPath& { rootPath = root; return *this;}
    auto setExePath(std::string_view exe) -> outputPath& {
        if(rootPath.empty()) log::msg<log::Error>("Root path empty");
        if (exe.empty()) {
            exePath = rootPath;
        } else {
            exePath = rootPath / exe;
        }
        return *this;
    }
    auto setBuildfolder(std::string_view folder) -> outputPath& {
        if(rootPath.empty()) log::msg<log::Error>("Root path empty");
        buildPath = rootPath / folder;
        return *this;
    }
    auto setOutpath(std::string_view folder) -> void {
        if(rootPath.empty()) log::msg<log::Error>("Root path empty");
        if(buildPath.empty()) log::msg<log::Error>("Build path empty");
        objPath = buildPath / folder / "obj";
        modulePath = buildPath / folder / "module";
        stdPath = modulePath / folder / "std";
    }
    auto ensurePath() -> void {
        if((exePath != rootPath) && !fs::exists(exePath)) {
            fs::create_directories(exePath);
        }
        if(!fs::exists(buildPath)) {
            fs::create_directories(buildPath);
        }
        if(!fs::exists(modulePath)) {
            fs::create_directories(modulePath);
        }
        if(!fs::exists(objPath)) {
            fs::create_directories(objPath);
        }
        if(!fs::exists(stdPath)) {
            fs::create_directories(stdPath);
        }
    }
};

struct fileUtil
{
    static constexpr std::string_view platform {
        #ifdef _WIN32 
        "windows"
        #elif
        "other"
        #endif
    };
    static constexpr std::string_view executable {
        #ifdef _WIN32 
        ".exe"
        #elif
        ".out"
        #endif
    };
    static constexpr std::string libTool {
        #ifdef _WIN32 
        "ar"
        #elif defined(__unix__) 
        "ar"
        #endif
    };
    static constexpr std::string_view cppSource[]
    {
        ".cpp",
        ".cxx",
        ".cc",
        "c++"
    };
    static constexpr std::string_view cppModule[] 
    {
        ".cppm",
        ".ccm",
        ".cxx",
        ".ixx",
        ".cxxm",
        ".c++m"
        
    };
    static constexpr std::string_view cppHeader[]
    {
        ".h", 
        ".hpp", 
        ".hh", 
        ".hxx", 
        ".H", 
        ".h++"
    };
    
    static constexpr std::string_view objFile      {".o"};
    static constexpr std::string_view libFile      {
        #ifdef _WIN32 
        ".lib"
        #elif
        ".a"
        #endif
    };
    static constexpr std::string_view soFile       {".so"};
    static constexpr std::string_view pcmModule    {".pcm"};
    static constexpr std::string_view cSource      {".c"};
    static constexpr std::string_view cHeader      {".h"};
    static constexpr std::string_view importToken  {"import"};
    static constexpr std::string_view includeToken {"#include"};
    static constexpr std::string_view exportToken  {"export module"};
    static constexpr auto isCpp(std::string_view file) -> bool
    {
        for (const auto& i : cppSource)
        {
            if (i == file) {return true; break;}
        }
        return false;
    }
    static constexpr auto isModule(std::string_view file) -> bool
    {
        for (const auto& i : cppModule)
        {
            if (file == i) {return true; break;}
        }
        return false;
    }
    static constexpr auto isCppHeader(std::string_view file) -> bool
    {
        for (const auto& i : cppHeader)
        {
            if (i == file) {return true; break;}
        }
        return false;
    }
};

namespace util {

    inline auto getRawHeaderName(const std::string_view in) {
        struct out {
            const std::string_view sv;
            const bool startNpos;
            const bool endNpos;
        };
        const char* data = in.data();
        std::size_t startPos {std::string_view::npos};
        std::size_t endPos   {std::string_view::npos};
        char closeChar       {'\0'};
        for (const char& c : in) {
            const bool quote = c == '\"';
            const bool bracket = c == '<';
            if (quote || bracket) {
                closeChar = (bracket) ? '>' : '\"';
                data = &c;
                startPos = &c - in.begin();
                break;
            }
        }

        for (++data; data != in.end(); ++data) {
            if (*data == closeChar) {
                endPos = data - in.begin();
                break;
            }
        }

        const bool startFound = (startPos != std::string_view::npos);
        const bool endFound   = (endPos != std::string_view::npos);

        return out{
            (startFound && endFound && endPos > startPos) 
                ? in.substr(startPos + 1, endPos - startPos - 1) 
                : std::string_view{},
            !startFound,
            !endFound,
        };
    }

    inline auto trim(std::string_view str) -> std::string_view 
    {
        const size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        const size_t last = str.find_last_not_of(" \t\r\n");
        return std::string_view(str.data() + first, (last - first + 1));
    }
    inline auto isSame(char c,const char* ca,size_t count) noexcept -> bool {
        for(const char ch : std::span<const char>{ca,count}) if (c == ch) return true;
        return false;
    };
    
    inline auto containsToken(std::string_view str,const std::string_view target) noexcept -> bool 
    {
        const char* start {nullptr};
        for (const char& c : str) {
            if (c == ' ' || c == str.back()) {
                if (&c != start) {
                    if (std::string_view(start, &c) == target) {
                        return true;
                    }
                }
                start = &c + 1;
            }
        }
        return false;
    }
    inline auto addSep(std::string& to,char sep) {
        if(!to.empty() && to.back() != sep) to += sep;
    }
    inline auto appendWithSep(std::string& to,std::string_view add,char sep) {
        addSep(to,sep);
        to += add;
    }
    inline auto appendWithSep(std::string& to,std::string_view p,std::string_view s,char sep) {
        addSep(to,sep);
        (to += p) += s;
    }
    
    inline auto anyOfchar (const char c,std::string_view sv) -> bool {
        for(const char ch : sv) {
            if (c == ch) return true;
        }
        return false;
    }
    struct BitFlags {
        int data;
        constexpr BitFlags(int mask) : data(mask) {}
        constexpr bool operator==(const int mask) {
            return (data & mask) != 0;
        }
        constexpr bool operator!=(const int mask) {
            return (data & mask) == 0;
        }
        constexpr BitFlags& set(const int mask,bool value) {
            if (value) data |= mask;
            else       data &= ~mask;
            return *this;
        }
    };
    inline auto stripComments(std::string str) -> std::string {
        std::string_view it {str.data(),str.size()};
        enum State {
            None         = 0,
            BlockComment = 1 << 0,
            LineComment  = 1 << 1,
            Quote        = 1 << 2,
            ExtendScope  = 1 << 3,
            Preprocess   = 1 << 4,
            EndStatement = 1 << 5
        };
        BitFlags cnd {0};
        unsigned int scopeDepth = 0;
        char* cursor    = str.data();
        for(const char& c : it) {
            const char* cprev = cursor == str.data() ? str.data() : cursor - 1;
            const char* prev  = &c == str.data() ? str.data() : &c - 1;
            const char* next  = &c == str.data() + str.size() ? &c : &c + 1;
            const bool endl   = c == '\n';
            const bool space  = c == ' ';
            if (c == '#' && cnd != Quote) cnd.set(Preprocess, true);
            if (space && *cprev == ' ') continue;
            if (c == '/' && cnd != Quote) {
                if(*next == '/') { cnd.set(LineComment, true);  continue;}
                if(*next == '*') { cnd.set(BlockComment, true);  continue;}
                if(*prev == '*') { cnd.set(BlockComment, false); continue;}
            }
            if (endl) {
                cnd.set(LineComment, false);
                if(cnd != ExtendScope) cnd.set(Preprocess, false);
                if(*cprev == '\n') continue;
            }
            if (cnd == LineComment || cnd == BlockComment) continue;
            
            if (!scopeDepth && *cprev == '{') ++scopeDepth;
            if (scopeDepth) {
                if(c == '{') ++scopeDepth;
                if(c == '}') --scopeDepth;
                if(scopeDepth > 0) continue;
            }
            
            if (c == '\\') { cnd.set(ExtendScope, true); continue;}
            if (cnd == ExtendScope) {
                if(endl) cnd.set(ExtendScope, false); 
                continue;
            }
            
            const bool qmark {c == '"'};
            if(cnd != Quote && qmark) {*cursor++ = c; cnd.set(Quote, true); continue;}
            if(cnd == Quote && qmark) {*cursor++ = c; cnd.set(Quote, false); continue;}
            // if (c == ';') cnd.set(EndStatement, true);
            // if (*cursor == '\n' && cnd != Preprocess && cnd != EndStatement) {
            //     cnd.set(EndStatement , false);
            //     continue;
            // }
            
            *cursor++ = c;

        }
        str.erase(cursor - str.data());
        return str;
    }

    [[nodiscard]] inline std::string fileTostring(input_t<std::string,3> path) {
        std::ifstream file({path.data.data(),path.data.size()}, std::ios::ate | std::ios::binary); // open file and immediately seek to the end
        // opening file as binary allows us to skip pointless newline re-encoding
        if (!file.good()) log::msg("Could not open file ",path);
        const auto file_size = file.tellg(); // returns cursor pos, which is the end of file
        file.seekg(std::ios::beg);           // seek to the beginning
        path.data.resize(file_size);
        
        file.read(path.data.data(), file_size);  // read into the string
        file.close();
        return path.data;
    }
    // [[nodiscard]] inline std::string fileTostring(input_t<const std::string&,3> path) {
    //     return fileTostring(path);
    // }
    inline constexpr auto viewLine(const char* ptr,std::string_view& it,char sep) noexcept -> std::string_view {
        const bool atSep = *ptr == sep; 
        const bool atBack = ptr == &it.back(); 
        if(atSep || atBack) {
            // const bool atFront = ptr == it.data(); 
            std::string_view line {it.data(),ptr - (atSep && ptr != it.data())};
            it = {ptr + !atBack , it.end()};
            return line;
        }
        return "";
    };
}

class compileCommand {
    utl::json::node jsonNode;
    mutable std::mutex mtx;

    public:
    auto addCompilecmd(std::string_view path,std::string_view arg,std::string_view file,std::string_view out) -> compileCommand& 
    {
        utl::json::node innode;
        innode["directory"] = path;
        innode["command"] = arg ;
        innode["file"] = file ;
        innode["output"] = out ;
        {
            std::lock_guard<std::mutex> lock(mtx);
            jsonNode.push_back(std::move(innode));
        }
        return *this;
    }
    auto write(fs::path to) const -> void
    {
        std::lock_guard<std::mutex> lock(mtx);
        jsonNode.to_file(to.string());
    }


};


struct File {
    using string_type = std::string;
    using view_type = std::string_view;

    mutable bool compiled = false;
    mutable bool haveHeaderUnit = false;
    mutable bool onArchive = false;
    mutable bool isPartition = false;
    mutable bool isMainPartition = false;
    enum file_types : char {
        Source,
        Module, 
        ModuleImpl, 
        SystemHeader, 
        HeaderUnit, 
        none
    } ;
    mutable file_types fileType = none;

    string_type FileName    {};
    string_type Name        {};
    string_type Path        {};
    string_type Flags       {};
    string_type ldFlags     {};
    string_type objectPath  {};
    string_type rawData     {};
    std::vector<string_type> dependencies {};

    [[nodiscard]] auto getModuleOutput(const fs::path* mPath) const -> string_type {
        if (Name.empty()) log::msg<log::Error>("File Name Empty");
        if ((fileType != Module) && (fileType != SystemHeader) && (fileType != HeaderUnit)) log::msg<log::Error>("File Not a Module");
        return sformat("{}{}",(*mPath / getName()).string(), fileUtil::pcmModule);
    }
    auto setObjOutputName(const fs::path& oPath) -> void {
        if (Name.empty()) log::msg<log::Error>("File Name Empty");
        objectPath = sformat("{}{}{}",(oPath / getName()).string(),fileType == ModuleImpl ? "-impl" : "", fileUtil::objFile);
    }
    auto getName() const -> string_type {
        if (Name.empty()) log::msg<log::Error>("File Name Empty");
        if (isPartition) {
            string_type temp = Name;
            temp.replace(temp.find(':'),1,"-");
            return temp;
        }
        return Name;
    }
};

struct HeaderFile {
    using string_type = std::string;
    string_type Name    {};
    string_type Path    {};
    string_type flags   {};
    string_type rawData {};
};

class FileManager {
    using File_type       = File;
    using MapPair         = std::pair<const std::string, File_type>;
    using HeaderContainer = std::vector<HeaderFile>;
    using index_t         = input_t<size_t,3>;
    using key_t           = input_t<std::string_view,3>;
    struct StringHash {
        using is_transparent = void;

        std::size_t operator()(std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }
        std::size_t operator()(const key_t& sv) const {
            return std::hash<std::string_view>{}(sv.data);
        }
        std::size_t operator()(const std::string& s) const {
            return std::hash<std::string>{}(s);
        }
        std::size_t operator()(const char* str) const {
            return std::hash<std::string_view>{}(str);
        }
    };
    using HeaderMap     = std::unordered_map<std::string, HeaderContainer,StringHash,std::equal_to<>>;
    using SourceFileMap = std::unordered_map<std::string, File_type ,StringHash,std::equal_to<>>;
    
    HeaderMap Header {};
    SourceFileMap Files {};

    public:
    File_type* Main = nullptr;
    auto addFile(std::string_view name,File&& file) -> MapPair& {
        auto [it,s] = Files.try_emplace({name.data(),name.size()}, std::move(file));
        return *it;
    }
    auto addFile(std::string_view name,const File& file) -> MapPair& {
        auto [it,s] = Files.try_emplace({name.data(),name.size()}, std::move(file));
        return *it;
    }
    auto addFile(std::string_view name) -> MapPair& {
        return addFile(name,File{.FileName={name.data(),name.size()}});
    }
    
    auto setHeaderPath(std::string_view path) -> HeaderContainer& {
        return Header.try_emplace({path.data(),path.size()},HeaderContainer{}).first->second;
    }

    auto addHeader(key_t path,std::string_view name) -> HeaderFile* {
        auto it = Header.find(path.data);
        if (it != Header.end()) {
            bool found = false;
            for (const auto& F : it->second) {
                if (name == F.Name) found = true;
            }
            if (!found) {
                std::string temp {path.data.data(),path.data.size()};
                return &it->second.emplace_back(HeaderFile{
                    .Name = {name.data(),name.size()},
                    .Path = temp,
                });
            }
        } else {
            log::msg("Path is not registered",path);
        }
        return nullptr;
    }
    auto moveHeaderFrom(const std::string& Path,HeaderFile& H) -> void {
        auto it = Header[Path];
        // const bool found = [&](std::span<HeaderFile> from)-> bool{ 
        // }(it);
        for (const auto& F : it) {
            if (H.Name == F.Name) it.push_back(std::move(H));;
        }
    }
    
    auto getHeaderPath(key_t name) -> std::string_view {
        for (auto& [I , IN] : Header) {
            for(const auto& N : IN)
            if (N.Name == name.data) {
                return I;
            }
        }
        log::msg("Key doesn't exists",name);
        return "";
    }
    auto copyFile(std::string& name,const File& other) -> File_type& {
        auto ref = Files.try_emplace(name,File{
        .compiled = other.compiled,
        .haveHeaderUnit = other.haveHeaderUnit,
        .onArchive = other.onArchive,
        .fileType = other.fileType,
        .Name = other.Name,
        .Path = name,
        .Flags = other.Flags,
        .ldFlags = other.ldFlags,
        .objectPath = other.objectPath
        }).first;
        return ref->second;
    }
    
    
    auto getByName(key_t id) -> File* {
        for (auto& [K,F] : Files) {
            if (F.Name == id.data) {
                return &F;
            }
        }
        log::msg("File with Name doesn't exists",id);
        return nullptr;
    }
    auto getByFileName(key_t id) -> File* {
        for (auto& [K,F] : Files) {
            if (F.FileName == id.data) {
                return &F;
            }
        }
        log::msg("File with Name doesn't exists",id);
        return nullptr;
    }
    auto getPair (key_t id) -> MapPair* {
        auto it = Files.find(id.data);
        if (it != Files.end()) {
            return &*it; 
        }
        log::msg("File with Name doesn't exists",id);
        return nullptr; 
    }

    auto setMain(key_t id) -> void {
        Main = getByFileName(id);
    }
    auto getMain() -> File& {
        return *Main;
    }

    auto operator [](key_t id) -> File* {
        return getByFileName(id);
    }
    
    auto getFileContainer() -> SourceFileMap& {
        return Files;
    }
    auto empty () const -> bool {return Files.empty();}
    auto headerContainer() -> HeaderMap& {
        return Header;
    }
    auto begin() { return Files.begin(); }
    auto end()   { return Files.end(); }
    
    auto begin() const { return Files.begin(); }
    auto end()   const { return Files.end(); }
};


class Project
{
    using str_input = input_t<std::string_view,3>;
    outputPath* OutPath;
    compileCommand* cmdJson;
    std::string ProjectName     {};
    std::string Options         {};
    std::string LdOptions       {};
    std::string Compiler        {};
    std::string outputName      {};
    
    bool recompile;
    
    public:
    enum projectType : char {
        exe,
        staticLib,
        dynamicLib
    } outFile;
    
    fs::path Path       {};
    fs::path ResPath    {};
    std::set<std::string> SourcePath  {};
    FileManager ProjectFile        {};

    Project(std::string_view name,outputPath& path,projectType exe,bool recomp = false) {
        ProjectName = name,
        OutPath = &path,
        outFile = exe,
        cmdJson = nullptr,
        recompile = recomp;
        std::printf("%s",sformat("{} {}\n",addColors("Project initialized at",strColors::Green), this->OutPath->rootPath.string()).c_str());
    };

    auto getMainPath       () const -> const std::string&    {return *SourcePath.begin();}
    auto getCompileCommand () const -> compileCommand* {return cmdJson;}

    constexpr auto setMain        (std::string_view main) -> Project& {ProjectFile.setMain(main); return *this;}
    constexpr auto setCompiler    (std::string_view comp) -> Project& {Compiler = comp; return *this;}
    
    constexpr auto addOptions     (std::string_view opt)  -> Project& {
        util::appendWithSep(Options,opt,' ');
        return *this;
    }
    constexpr auto addOptions     (std::span<const std::string_view> list_opt)  -> Project& {
        for (auto& sv : list_opt) addLdOptions(sv);
        return *this;
    }
    constexpr auto addOptions     (std::initializer_list<std::string_view> list_opt)  -> Project& {
        return addOptions({list_opt.begin(),list_opt.end()});
    }
    
    constexpr auto addLdOptions (std::string_view opt)  -> Project&  {
        util::appendWithSep(LdOptions,opt,' ');
        return *this;
    }
    constexpr auto addLdOptions (std::span<const std::string_view> list_opt)  -> Project&  {
        for (auto& sv : list_opt) addLdOptions(sv);
        return *this;
    }
    constexpr auto addLdOptions (std::initializer_list<const std::string_view> list_opt)  -> Project&  {
        return addLdOptions({list_opt.begin(),list_opt.end()});
    }

    constexpr auto setProjectPath (std::string_view in) -> Project&   {Path = in; return *this;}
    constexpr auto setResourcePath(std::string_view in) -> Project&   {ResPath = in; return *this;}

    auto addSourcePath  (std::string_view in) -> Project&   {
        const fs::path temp {Path / in};
        if (!fs::exists(temp)) log::msg<log::Error>("Source path does not exist",temp.string());
        SourcePath.insert(temp.string()); 
        return *this;
    }
    constexpr auto addSourcePathList  (std::span<const std::string_view> ListPath) -> Project&   {
        for (const auto& P : ListPath) addSourcePath(P);
        return *this;
    }
    constexpr auto addSourcePathList  (std::initializer_list<std::string_view> ListPath) -> Project&   {
        return addSourcePathList({ListPath.begin(),ListPath.end()});
    }

    auto addIncludePath (std::string_view IncludePath) -> Project& {
        const fs::path temp {Path / IncludePath};
        if (!fs::exists(temp)) log::msg<log::Error>("Include path does not exist",temp.string());
        ProjectFile.setHeaderPath(temp.string()); 
        return *this;
    }
    auto addIncludePathList (std::span<const std::string_view> ListPath) -> Project& {
        if(ListPath.empty()) return *this;
        for (auto& in : ListPath) {
            addIncludePath(in);
        }
        return *this;
    }
    auto addIncludePathList (std::initializer_list<std::string_view> ListPath) -> Project& {
        return addIncludePathList({ListPath.begin(),ListPath.end()});
    }

    auto addCompileCommand(compileCommand* cmd) -> Project&
    {
        cmdJson = cmd;
        return *this;
    }

    auto addSource(const fs::path& file) -> void {
        const bool isModule = fileUtil::isModule(file.extension().string());
        const bool isSource = fileUtil::isCpp(file.extension().string());
        if (fs::is_regular_file(file) && ( isModule || isSource)) {
            ProjectFile.addFile(file.filename().string(),File{
                .onArchive = (outFile == Project::staticLib ? true : false),
                .fileType = isModule ? File::Module : File::Source,
                .FileName = file.filename().string(),
                .Name = file.stem().string(),
                .Path = file.string(),
                .rawData = util::stripComments(util::fileTostring(file.string())),
            });
        }
    };
    
    auto addSource(std::string_view from,std::string_view file) -> Project& {
        const fs::path fromPath {Path / from};
        if (!fs::is_directory(fromPath)) log::msg<log::Error>("Is not a directory",fromPath.string());
        
        if (file == "*") {
            const fs::directory_iterator iterator(fromPath);
            for (const auto& entry : iterator) {
                addSource(entry.path());
            }
        } else {
            const fs::path sourcePath {fromPath / file};
            if (!fs::exists(sourcePath)) log::msg<log::Error>("Source path does not exist",sourcePath.string());
            addSource(sourcePath);
        }
    
        return *this;
    }
    constexpr auto addSource(const std::string_view from,std::span<const std::string_view> ListFiles) -> Project& {
        if (ListFiles.empty()) log::msg<log::Error>("add atleast one file to compile");
        for (const auto& i : ListFiles) {
            addSource(from,i);
        }
        return *this;
    }
    constexpr auto addSource(const std::string_view from,std::initializer_list<const std::string_view> ListFiles) -> Project& {
        return addSource(from,{ListFiles.begin(),ListFiles.end()});
    }

    auto getLib(Project&& other) -> Project&
    {
        if (other.outFile == Project::staticLib) {
            for (auto& [Path,V] : other.ProjectFile.headerContainer()) {
                for (auto& HeaderFile : V) {
                    ProjectFile.moveHeaderFrom(Path,HeaderFile);
                }
            }
            if(!other.outputName.empty()) {
                util::appendWithSep(LdOptions,sformat("-L{} -l{}",other.OutPath->exePath.string(),other.outputName),' ');
            }
            other.~Project();
        }
        return *this;
    }

    auto LinkLibrary(const std::string_view inFile, std::span<const std::string_view> ListDeps) -> Project&
    {
        if(ListDeps.empty()) return *this;
        auto& File = *ProjectFile[inFile];
        for (auto&& d : ListDeps) {
            std::string_view dep {d};
            if(dep.empty()) continue;
            while (!dep.empty() && dep.front() == ' ') dep.remove_prefix(1);
            while (!dep.empty() && dep.back() == ' ') dep.remove_suffix(1);
            util::appendWithSep(File.ldFlags,"-l",dep,' ');
        }
        return *this;
    }
    auto LinkLibrary(const std::string_view inFile, std::initializer_list<const std::string_view> ListDeps) -> Project&
    {
        return LinkLibrary(inFile,{ListDeps.begin(),ListDeps.end()});
    }

    auto addCompileFlags(const std::string_view inFile,std::span<const std::string_view> ListFlags) -> Project&
    {
        auto& File = *ProjectFile[inFile];
        for (auto&& dep : ListFlags) {
            if(dep.empty()) continue;
            util::appendWithSep(File.Flags,dep,' ');
        }
        return *this;
    }
    auto addCompileFlags(const std::string_view inFile,std::initializer_list<const std::string_view> ListFlags) -> Project&
    {
        return addCompileFlags(inFile,{ListFlags.begin(),ListFlags.end()});
    }
    
    auto tryAddHeader(HeaderFile& to,const std::string& path) {
        std::string flags {"-I" + path};
        const bool alreadyAdded = util::containsToken(to.flags, flags);
        const bool notSamePath = (to.Path != path);
        if (!alreadyAdded && notSamePath) {
            std::printf("%s",sformat("{} {} to {}\n",addColors("Add dependencies",strColors::Bold_Blue) , path , to.Name ).c_str());
            util::appendWithSep(to.flags,flags,' ');
        }
    }
    
    auto solveHeaderDependencies(HeaderFile& Header) -> void 
    {
        if (Header.Path.empty()) log::msg<log::Error>("Project path is Empty",Header.Path); 
        const std::string HeaderPath = (fs::path{Header.Path} / Header.Name).generic_string(); 
        std::string_view files(Header.rawData);
        for (const char& c : files) {
            std::string_view line {util::viewLine(&c, files, '\n')};
            if(line.empty()) continue;
            const size_t ipos = line.find(fileUtil::includeToken);
            if (ipos != std::string::npos) { 
                const size_t searchStart = ipos + fileUtil::includeToken.length();
                const auto [headerName,startNpos,endNpos] = util::getRawHeaderName(std::string_view{line}.substr(searchStart));
                if (startNpos || endNpos) { continue; }
                
                for (const auto& [P,V] : ProjectFile.headerContainer()) {
                    if (P.empty()) continue;
                    for (const auto& HN : V) {
                        HN.Name == headerName ? tryAddHeader(Header,P) : void();
                    }
                }
            }
        }  
        
    }

    auto getHeaderFile() -> Project& 
    {
        for (const auto& K : ProjectFile.headerContainer()) {
            const fs::directory_iterator it(K.first);
            for (const auto& entry : it) {
                if (entry.is_regular_file() && fileUtil::isCppHeader(entry.path().extension().string()) ) {
                    ProjectFile.addHeader(K.first, entry.path().filename().string())->rawData = util::stripComments(util::fileTostring(entry.path().string()));
                }
            }
        }
        
        for (auto& [Path,Header]: ProjectFile.headerContainer()) {
            for (auto& H : Header) solveHeaderDependencies(H);
        }
        return *this;
    } 
    

    auto getCppFile() -> void 
    {
        for (const auto& p : SourcePath)
        {
            if(!fs::exists(p) || !fs::is_directory(p)) log::msg<log::Error>("Directory does not exist",p);
            fs::directory_iterator iterator(p);
            
            for (const auto& entry : iterator) {
                addSource(entry.path());
            }
        }
    }

    auto addScannedHeader(File& file,const std::string& path,const HeaderFile& header) {
        const std::string i {"-I" + path};
        if (util::containsToken(file.Flags, i)) return;
        util::appendWithSep(file.Flags,i,' '); 
        if(!header.flags.empty()) {
            const char* beg = header.flags.data();
            for (auto& c : header.flags) {
                if (c == ' ') {
                    std::string_view temp {beg,&c};
                    if(!util::containsToken(file.Flags,temp)) util::appendWithSep(file.Flags,temp,' ');
                    beg = &c;
                }
            }
        }
    }

    auto scanHeader(File& file) -> void
    {
        if (file.Path.empty()) log::msg<log::Error>("Project path is Empty",file.Path);

        // const std::string files = util::stripComments(util::fileTostring(file.Path));
        std::string_view files {file.rawData};
        bool exportModuleFound = false;
        bool moduleFound = false;
        std::printf("%.*s\n",static_cast<int>(files.size()),files.data());
        for (const char& c : file.rawData) {
            std::string_view line {util::viewLine(&c, files, '\n')};
            if(line.empty()) continue;
            // -------------------------------------------------------------------
            // 1. Detect Module Interface: "export module <name>;"
            // -------------------------------------------------------------------
            const size_t epos = line.find(fileUtil::exportToken); // e.g., "export module"

            if (!exportModuleFound && epos != std::string::npos) {
                std::string_view moduleName = std::string_view{line}.substr(epos + fileUtil::exportToken.length() + 1);
                moduleName = moduleName.substr(0, moduleName.find(';'));
                moduleName = util::trim(moduleName);

                if (moduleName.find(':') != std::string::npos) {
                    file.isPartition = true;
                }

                file.Name = moduleName;
                file.fileType = File::Module; // Interface module
                file.setObjOutputName(OutPath->objPath);

                std::string moPath = file.getModuleOutput(&OutPath->modulePath);
                file.compiled = recompile ? false : (fs::exists(file.objectPath) && fs::exists(moPath) 
                    ? fs::last_write_time(file.Path) < fs::last_write_time(file.objectPath) 
                    : false);

                exportModuleFound = true;
                moduleFound = true;
                continue; // Skip implementation check for this line
            }

            // -------------------------------------------------------------------
            // 2. Detect Module Implementation: "module <name>;"
            // -------------------------------------------------------------------
            if (!moduleFound && line.rfind("module ", 0) == 0) {
                const size_t mpos = line.find("module");
                std::string_view moduleName = std::string_view{line.data() + mpos + 6,line.data() + line.find(';') }; // 6 == length of "module"
                moduleName = util::trim(moduleName);

                if (!moduleName.empty()) {
                    file.isPartition = moduleName.find(':') != std::string::npos;

                    file.Name = moduleName;
                    file.fileType = File::ModuleImpl;
                    file.setObjOutputName(OutPath->objPath);
                    auto minterface = ProjectFile.getByName(moduleName);
                    file.dependencies.push_back(minterface->Name);
                    file.compiled = recompile ? false : (fs::exists(file.objectPath)
                        ? fs::last_write_time(file.Path) < fs::last_write_time(file.objectPath) 
                        : false);
                    moduleFound = true;
                }
            }

            // -------------------------------------------------------------------
            // 3. Detect #include or import dependencies
            // -------------------------------------------------------------------
            const size_t ipos = line.find(fileUtil::includeToken);
            if (ipos != std::string::npos) {
                const size_t searchStart = ipos + fileUtil::includeToken.size();
                auto [headerName,start,end] = util::getRawHeaderName(std::string_view{line}.substr(searchStart));
                if (start || end) { continue; }
                    
                for (const auto&  [HP,HF] : ProjectFile.headerContainer()) {
                    auto findInclude = &*HF.end();
                    for (auto& s : HF) if(s.Name == headerName) {findInclude = &s; break;}
                    if (findInclude != &*HF.end()) addScannedHeader(file,HP,*findInclude);
                }
            }
        }
    }
    auto scanHeaders() -> Project& 
    {
        for (auto& [K, V] : ProjectFile) {
            scanHeader(V);
        }
        return *this;
    }

    auto scanModule(File& ModuleFile) -> void
    {
        if (ModuleFile.Path.empty()) log::msg<log::Error>("Project path is Empty",ModuleFile.Path);
        
        // const std::string files {
        //     ModuleFile.fileType == File::HeaderUnit ?
        //     util::stripComments(util::fileTostring((fs::path(ModuleFile.Path) / ModuleFile.Name).generic_string())) :
        //     util::stripComments(util::fileTostring(ModuleFile.Path))
        // };
        std::string_view files {ModuleFile.rawData};
        for (const char& c : files) {
            std::string_view line {util::viewLine(&c, files, '\n')};
            if(line.empty()) continue;

            const size_t ipos = line.find(fileUtil::importToken);
            const size_t epos = line.find(';');
        
            std::string_view moduleName;
            if (ipos != std::string::npos && epos != std::string::npos) {
                const size_t searchStart = ipos + fileUtil::importToken.length();
    
                const size_t startPos = line.find_first_not_of(" \t\r", searchStart);
                if (startPos == std::string::npos || startPos >= epos) {continue;}
                
                // Find the end of the module name string before trailing whitespaces or semicolon
                const size_t endPos = line.find_last_not_of(" \t\r", epos - 1);
                if (endPos == std::string::npos || endPos < startPos) {continue;}

                moduleName = std::string_view{line}.substr(startPos, (endPos - startPos) + 1);
                
                // -------------------------------------------------------------------
                // Detect System Module Unit
                // -------------------------------------------------------------------
                if (moduleName.front() == '<' || moduleName.front() == '"') {
                    // std::printf("%s",sformat("{} From: {} Name: {}\n",addColors("Header Unit module",strColors::Bold_Blue), V.Name , moduleName).c_str());
                    const auto [rawHeader,start,end] = util::getRawHeaderName(moduleName);
                    const size_t extension = rawHeader.find_last_of('.');
                    const bool isHeaderUnit = (extension == std::string_view::npos ? false : fileUtil::isCppHeader(rawHeader.substr(extension)));
                    auto& [headerUnit,headerUnitFile] = ProjectFile.addFile( isHeaderUnit ? rawHeader : moduleName);
                    
                    headerUnitFile.Name = rawHeader;
                    headerUnitFile.fileType = isHeaderUnit ? File::HeaderUnit : File::SystemHeader;
                    if (isHeaderUnit) {
                        auto& headerUnitFlags = headerUnitFile.Flags; 
                        headerUnitFile.Path = ProjectFile.getHeaderPath(rawHeader);
                        moduleName = rawHeader;
                        const auto& it = ProjectFile.headerContainer();
                        auto findHeader = &*it.end()->second.end();
                        for(const auto& [key,value] : it) {
                            for (const auto& file : value) if (file.Name == moduleName) {findHeader = &file; break;}
                        }
                        if (findHeader != &*it.end()->second.end()) {
                            auto& h = findHeader;
                            // std::printf("%s",sformat("{} From: {} to: {} and {}\n",addColors("add Header into Unit module",strColors::Bold_Green) , h->Path , moduleName , V.Name).c_str());
                            std::string i {"-I" + h->Path};
                            if (!util::containsToken(headerUnitFlags,i)) util::appendWithSep(headerUnitFlags,i, ' ');
                            if (!util::containsToken(ModuleFile.Flags,i)) util::appendWithSep(ModuleFile.Flags,i, ' ');

                            if (!h->flags.empty()) {
                                const char* beg = h->flags.data();
                                for (auto& c : h->flags) {
                                    if (c == ' ') {
                                        std::string_view temp {beg,&c};
                                        if(!util::containsToken(headerUnitFlags,temp)) util::appendWithSep(headerUnitFlags, temp, ' ');
                                        if(!util::containsToken(ModuleFile.Flags,temp)) util::appendWithSep(ModuleFile.Flags, temp, ' ');
                                        beg = &c;
                                    }
                                }
                            }
                        }
                    } 
                    headerUnitFile.objectPath = isHeaderUnit ? 
                    sformat("{}{}",(OutPath->modulePath / rawHeader).string(),fileUtil::pcmModule) : 
                    headerUnitFile.getModuleOutput(&OutPath->stdPath);
                    headerUnitFile.compiled = fs::exists(headerUnitFile.objectPath);
                    ModuleFile.haveHeaderUnit = true;
                }
            }
            if (!moduleName.empty()) {
                for (auto& [Module,MV] : ProjectFile) {
                    const bool sameName = ((MV.fileType != File::Module) ? Module 
                    : MV.isPartition ? MV.Name.substr(MV.Name.find(':')) 
                    : MV.Name) == moduleName;
                    auto Added = [](const File& ModuleFile,const File& MV){
                        for(auto& dep : ModuleFile.dependencies) if (MV.FileName == dep) return true;
                        return false;
                    };
                    if (sameName && !Added(ModuleFile,MV)) ModuleFile.dependencies.emplace_back(MV.FileName);
                }
            }
        }
    }

    auto scanModules() -> Project& 
    {
        for (auto& [K,V] : ProjectFile) {
            if (V.fileType == File::SystemHeader) { continue;}
            scanModule(V);
        }
        return *this;
    }
    

    auto configureModuleFlags() -> Project& {
        std::string temp {};
        for (auto& [K,File] : ProjectFile) {
            auto& inFile = File;
            const bool isSystemHeader = (File.fileType == File::SystemHeader);
            if(isSystemHeader) continue;
            for (const auto& I : File.dependencies) {
                const auto& depFile = *ProjectFile[I];
                const auto& mPath = (isSystemHeader ? OutPath->stdPath : OutPath->modulePath);
                const bool depisSystemHeader = depFile.fileType == File::SystemHeader;
                const bool depisHeaderUnit = depFile.fileType == File::HeaderUnit;
                util::appendWithSep(temp,(
                    depisSystemHeader ? sformat("-fmodule-file={}",depFile.objectPath) :
                    depisHeaderUnit ? sformat("-fmodule-file={}{}",(mPath / depFile.getName()).string(), fileUtil::pcmModule) :
                    sformat("-fmodule-file={}={}{}",depFile.Name,(mPath / depFile.getName()).string(),fileUtil::pcmModule)
                ),' ');
            }
            util::appendWithSep(inFile.Flags ,temp,' ');
            temp.clear();
        }
        return *this;
    }

    auto compilePCH(std::string_view PCHfile) -> bool {
        const std::string headerFile = (fs::path(ProjectFile.getHeaderPath(PCHfile)) / PCHfile).string();
        const auto& oPath = OutPath->buildPath;
        const std::string pchOut {sformat("{}.pch",(oPath / fs::path(PCHfile).stem()).string())};
        const std::string f_cmd  {sformat("{} {} -x c++-header {} -o {}",Compiler, Options,headerFile,pchOut)};
        util::appendWithSep(Options,"-include-pch ",pchOut,' ');
        if (fs::exists(pchOut)) {
            if (fs::last_write_time(headerFile) > fs::last_write_time(pchOut)) {
                fs::rename(pchOut,pchOut + ".old");
            } else {
                return 1;
            }
        }
        std::printf("%s\n",sformat("{} {}",addColors("Compiling PCH",strColors::Bold_Green) , f_cmd).c_str());
        return cmd << f_cmd >> addColors("Error compiling ",strColors::Bold_Red);
    };

    auto compileModule(File& inFile) -> bool 
    {
        const bool isSystemHeader = (inFile.fileType == File::SystemHeader);
        const bool isModuleImpl = (inFile.fileType == File::ModuleImpl);
        const bool isHeaderUnit = (inFile.fileType == File::HeaderUnit);
        if (isModuleImpl || (isSystemHeader && inFile.compiled)) { return true; }
        if(!inFile.dependencies.empty()) {
            for (const auto& I : inFile.dependencies) {
                auto& dep = *ProjectFile[I];
                if(!dep.compiled) {
                    return false;
                }
            }
            if(!inFile.haveHeaderUnit && (isSystemHeader || isHeaderUnit)) {
                inFile.haveHeaderUnit = true;
            }
        }
        const bool HasHeaderUnit = (inFile.haveHeaderUnit);
        if(inFile.isPartition) {
            ProjectFile.getByName(inFile.Name.substr(0,inFile.Name.find(':')))->isMainPartition = true;
        }
        
        const auto& mPath = isSystemHeader ? OutPath->stdPath : OutPath->modulePath;
        const auto& oPath = OutPath->objPath;
        
        const std::string fModule    { inFile.getModuleOutput(&mPath) };
        
        const std::string fObjOutput { sformat("{}{}",(oPath / inFile.getName()).string(), fileUtil::objFile) };
        
        const std::string f_srcInput {
            isHeaderUnit ? sformat("-Wno-pragma-system-header-outside-header -fmodule-header=user --precompile {} -o {}",
            (fs::path(inFile.Path)/inFile.Name).string(),fModule) :
            isSystemHeader ? sformat("-Wno-pragma-system-header-outside-header -Wno-gnu-anonymous-struct -Wno-nullability-extension -Wno-gcc-compat -Wno-user-defined-literals -x c++-system-header --precompile {} -o {}",inFile.Name,fModule) :
            sformat("-c {} -fmodules-reduced-bmi -fmodule-output={} -fprebuilt-module-path={}",inFile.Path,fModule,(mPath).string())
        };

        if (inFile.objectPath.empty()) {inFile.objectPath = fObjOutput;}
        
        const std::string f_cmd {
            isSystemHeader ? sformat("{} {} {}",Compiler, Options,f_srcInput) : 
            isHeaderUnit ? sformat("{} {} {} {}",Compiler, Options,inFile.Flags,f_srcInput) :
            HasHeaderUnit ? sformat("{} {} -Wno-experimental-header-units {} {} -o {}" , Compiler, Options ,f_srcInput ,inFile.Flags,fObjOutput) :
            inFile.Flags.empty() ? sformat("{} {} {} -o {}" , Compiler, Options ,f_srcInput ,fObjOutput) :
            sformat("{} {} {} {} -o {}" , Compiler, Options ,f_srcInput ,inFile.Flags,fObjOutput)
        };
            
        if(cmdJson != nullptr && !isSystemHeader) { 
            cmdJson->addCompilecmd((Path / inFile.Path).parent_path().string(),f_cmd,(Path / inFile.Path).string(),fObjOutput);
        }
        
        if (inFile.compiled && !recompile) {
            inFile.compiled = true;
            return true;
        }
        int ret {};
        // module use reduced bmi        
        if (isSystemHeader) {
            std::printf("%s\n",sformat("{} {}",addColors("compiling module",strColors::Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> sformat("{}",addColors("recompile error",strColors::Red));
        } else {
            #ifdef __WIN32
            if(fs::exists(fModule)) {
                const std::string old {fModule + ".old"}; 
                if (fs::exists(old)){fs::remove(old);}
                fs::rename(fModule,old);
            }
            #endif
            // fs::copy(old,fModule);
            std::printf("%s",sformat("{} {}\n",addColors("compiling module",strColors::Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> addColors("recompile error",strColors::Red);
        }
        inFile.compiled = (ret == 0 ? true : false);
        return true; 
    };
    
    auto compileCpp(File& inFile) -> bool
    {
        const bool isModule = (inFile.fileType == File::Module || inFile.fileType == File::SystemHeader || inFile.fileType == File::HeaderUnit);
        const bool isModuleImpl = inFile.fileType == File::ModuleImpl;
        const bool noDependencies = inFile.dependencies.empty();
        if (isModule) {return 1;}
        
        const auto& oPath = OutPath->objPath;
        const auto& mPath = OutPath->modulePath;

        const std::string objOutput {(
            isModuleImpl ? inFile.objectPath : 
            sformat("{}{}",(oPath / inFile.getName()).string(), fileUtil::objFile)
        )};

        const std::string filein    { isModule ? 
            sformat("{}{}",(mPath / inFile.getName()).string(),fileUtil::pcmModule ) : 
            inFile.Path
        };

        const std::string cppOutput { 
            noDependencies ? sformat( "-c {}",filein) :
            sformat( "-c {} -fprebuilt-module-path={}",filein,(mPath).string())
        };
           
        const std::string f_cmd {
            inFile.Flags.empty() ? sformat("{} {} {} -o {}",Compiler, Options,cppOutput,objOutput)
            : sformat("{} {} {} {} -o {}",Compiler, Options,cppOutput,inFile.Flags,objOutput)
        };
        
        if(cmdJson != nullptr && !isModule) { 
            cmdJson->addCompilecmd(
                (Path / inFile.Path).parent_path().string(),
                f_cmd,
                (Path / inFile.Path).string(),
                objOutput
            );
        }
        
        if (inFile.objectPath.empty()) { inFile.objectPath = objOutput; } 

        if (inFile.compiled && !recompile) {
            inFile.compiled = true;
            return 1;
        }
        
        int ret {};
        if (recompile) {
            std::printf("%s",sformat("{} {}\n",addColors("recompiling",strColors::Bold_Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> sformat("{}",addColors("Error compiling",strColors::Bold_Red));
            
        } else if (!fs::exists(objOutput))
        {
            std::printf("%s",sformat("{} {}\n",addColors("compiling",strColors::Bold_Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> addColors("Error compiling",strColors::Bold_Red);
            
        } else if (fs::last_write_time(inFile.Path) > fs::last_write_time(objOutput))
        {
            std::printf("%s",sformat("{} {}\n",addColors("Updating",strColors::Bold_Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> addColors("Error compiling",strColors::Bold_Red);
        }
        inFile.compiled = (ret == 0 ? true : false);
        return ret; 
    }
    
    
    auto link(File& inPath) -> void {
        const bool isStaticLib {outFile == Project::staticLib};
        const bool isExecutable {outFile == Project::exe};
        const bool hasDependencies {!inPath.dependencies.empty()};
        const std::string f_targetOut {
            sformat("{}{}", (isStaticLib ? OutPath->buildPath.string() : (OutPath->exePath / inPath.Name).string()), (isStaticLib ? fileUtil::libFile : fileUtil::executable))
        };
        const std::string f_Output    { isStaticLib ?
            f_targetOut :
            sformat("-o {}", f_targetOut) 
        };
        
        const std::string makeFlags = [&]{ 
            std::string temp;
            for (const auto& [K,I] : ProjectFile) {
                if(I.onArchive == true) {continue;}
                if(isStaticLib) {I.onArchive = true;}
                if(I.fileType == File::SystemHeader) {continue;}
                if(!I.ldFlags.empty() && !isStaticLib) { util::appendWithSep(temp,I.ldFlags,' ');}
                util::appendWithSep(temp,I.objectPath,' ');
            }
            return (hasDependencies && !isStaticLib ?
            sformat("{} -fprebuilt-module-path={}",temp,(OutPath->modulePath / ".").string()) :
            temp);
        }();

        if(isStaticLib) {
            outputName = f_targetOut;
        } else if (isExecutable) {
            if (fs::exists(f_targetOut)) {
                fs::rename(f_targetOut, sformat("{}.old",f_targetOut));
            }
        }
        const std::string typeCmd {isStaticLib ? 
            sformat("{} {}",fileUtil::libTool,"rcs") : 
            LdOptions.empty() ? sformat("{} {}",Compiler,Options) : sformat("{} {} {}",Compiler,Options,LdOptions)
        };
        const std::string f_cmd { isStaticLib ?
            sformat("{} {} {}",typeCmd,f_Output,makeFlags) :
            sformat("{} {} {}",typeCmd,makeFlags,f_Output) 
        };

        if (!ResPath.empty() && fs::exists(getMainPath() / ResPath)) {
            if (!fs::exists(OutPath->exePath/ResPath)) {
                std::printf("%s",sformat("Copying from: {} to: {}\n", (getMainPath() / ResPath).string(),(OutPath->exePath/ResPath).string()).c_str()); 
                fs::copy(getMainPath() / ResPath,OutPath->exePath/ResPath,fs::copy_options::recursive | fs::copy_options::skip_existing);
            }
        }
        std::printf("%s",sformat("{} {}\n", addColors("Linking",strColors::Bold_Green), f_cmd).c_str());
        cmd << f_cmd >> addColors("linking error",strColors::Bold_Red);
    }

    auto dumpProject() -> Project& {
        
        std::printf("%s",sformat("{}\n",addColors("Dump project",strColors::Yellow)).c_str());
        for (const auto& [K,V]: ProjectFile) {
            const bool isSystemHeader = V.fileType == File::SystemHeader;
            if(isSystemHeader) continue;
            const bool isSource = V.fileType == File::Source;
            const bool isModule = V.fileType == File::Module;
            const bool isModuleImpl = V.fileType == File::ModuleImpl;
            std::printf("%s",sformat("File: {} Name: {}\nPath: {} Type: {}\n\n",
            K , 
            V.Name , 
            V.Path ,
            (isSource ? "Source" : isModule ? "Module" : isSystemHeader ? "SystemHeader" : isModuleImpl ? "ModuleImpl" : "ETC")
            // V.objectPath
            ).c_str());
        }

        std::printf("%s",sformat("\n{}\n",addColors("Dump Include",strColors::Yellow)).c_str());
        // ProjectFile.testHeader();
        for (const auto& [K,V]: ProjectFile.headerContainer()) {
            std::printf("Include Dir: %s",K.c_str());
            for (const auto& Header : V) {
                const bool flagsEmpty = Header.flags.empty();
                std::printf("%s", (flagsEmpty ? sformat("\nFile: {} Path: {}",Header.Name,Header.Path) 
                : sformat("\nFile: {} Path: {} \ndependency: {}",Header.Name,Header.Path,Header.flags)).c_str());
            }
            std::printf("\n");
        }

        std::printf("%s",sformat("\n{}\n",addColors("Dump Module",strColors::Yellow)).c_str()); 
        for (const auto&  [K,V]: ProjectFile) {
            if (V.fileType != File::Module) {continue;}
            std::printf("%s",sformat("Module: {} Name: {}\n",K,V.Name).c_str());
        }
        

        std::printf("%s",sformat("\n{}\n",addColors("Dump Dependencies",strColors::Yellow)).c_str());
        for (const auto& [K,V] : ProjectFile) {
            if (V.fileType == File::SystemHeader){continue;}

            std::printf("File: %s Depends on: ", K.c_str());
            for (const auto& I : V.dependencies) {
                std::printf(" File: %s ", I.c_str() );
            }
            // std::cout << "\nFile: " << K << " Header dependencies: ";
            // for (const auto& I : V.headerDeps) {
            //     std::cout << " Header: " << I << " ";
            // }
            std::printf("\n");
        }
        std::printf("%s",sformat("\n{}\n",addColors("Dump File Flags",strColors::Yellow)).c_str());
        for (const auto& [K,V] : ProjectFile) {
            if (V.Flags.empty() && V.ldFlags.empty()) continue;

            std::printf("File: %s\n", K.c_str() );
            if (!V.Flags.empty())   std::printf("cFlags : %s\n" , V.Flags.c_str());
            if (!V.ldFlags.empty()) std::printf("LdFlags: %s\n" , V.ldFlags.c_str());
        }
        std::printf("\n");
        
        return *this;
    }
};

#endif