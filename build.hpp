#ifndef BUILD_PP 
#define BUILD_PP

#include <cstdio>
#include <filesystem>
#include <vector>
#include <string_view>
#include <string>
#include <set>
#include <unordered_map>
// #include <iostream>
#include <fstream>
#include <utility>
#include <ranges>
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
            std::fprintf(stderr,"\n%*s %*s \nWhat: with value [%*s]", 
                hs,header,
                msgs,msg,
                ins,input
            );
        }
        
        inline void msg_impl(const char header[],int hs,const char msg[],int msgs,const char input[],int ins,const std::source_location& loc) noexcept {
            const unsigned int ll[] {loc.line(),loc.column()};
            std::fprintf(stderr,"\n%*s %s:%u:%u \nWhat: %*s [%*s]",
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
// template <typename T>
// constexpr T iabs(T num) {
//     if constexpr (std::is_signed_v<T>) {
        // return num = num | 
        // int t = ~(-22 << sizeof(int) * 8);
//     }
//     return num;
// };
// constexpr char test {iabs<char>(-22)} ;
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
    // using IDx = std::size_t;
    // auto err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) const -> const File& {
    //     if (cnd) {
    //         const std::string ret = sformat("{} {}:{}:{} at: {}\n",msg,fn.file_name(),fn.line(),fn.column(),fn.function_name());
    //         std::fprintf(stderr,"%*s",static_cast<int>(ret.size()),ret.c_str()); 
    //         std::exit(1);
    //     } 
    //     return *this;
    // }

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
    string_type Name  {};
    string_type Path  {};
    string_type flags {};
};

class FileManager {
    using File_type = File;
    using MapPair = std::pair<const std::string, File_type>;
    using HeaderContainer = std::vector<HeaderFile>;
    using index_t = input_t<size_t,3>;
    using key_t = input_t<std::string_view,3>;
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
    using HeaderMap = std::unordered_map<std::string, HeaderContainer,StringHash,std::equal_to<>>;
    using SourceFileMap = std::unordered_map<std::string, File_type ,StringHash,std::equal_to<>>;
    
    HeaderMap Header {};
    SourceFileMap Files {};

    public:
    File_type* Main = nullptr;
    auto addFile(std::string_view name) -> MapPair& {
        auto ref = Files.try_emplace({name.data(),name.size()}, File{.FileName={name.data(),name.size()}}).first;
        
        return *ref;
    }
    
    auto setHeaderPath(std::string_view path) -> HeaderContainer& {
        return Header.try_emplace({path.data(),path.size()},HeaderContainer{}).first->second;
    }

    auto addHeader(key_t path,std::string_view name) -> void {
        auto it = Header.find(path.data);
        auto found = [&](std::span<HeaderFile> from){ 
            for (const auto& F : from) {
                if (name == F.Name) return true;
            } return false;
        };
        if (it != Header.end()) {
            if (!found(it->second)) it->second.push_back(HeaderFile{.Name = {name.data(),name.size()},.Path={path.data.data(),path.data.size()}});
        } else {
            log::msg("Path is not registered",path);
        }
    }
    auto moveHeaderFrom(const std::string& Path,HeaderFile& H) -> void {
        auto it = Header[Path];
        const bool found = [&](std::span<HeaderFile> from)-> bool{ 
            for (const auto& F : from) {
                if (H.Name == F.Name) return true;
            } return false;
        }(it);
        if (!found) {it.push_back(std::move(H));}
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
        for (auto& I : Files) {
            if (I.second.Name == id.data) {
                return &I.second;
            }
        }
        log::msg("File with Name doesn't exists",id);
        return nullptr;
    }
    auto operator [](key_t id) -> File* {
        auto it = Files.find(id.data); 
        if (it != Files.end()) {
            return &it->second;
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

    auto getFile(std::string_view id) -> File& {
        return *(*this)[id];
    }
    auto setMain(std::string_view id) -> void {
        // std::cout << "Set Main: " << id << " " << (*this)[id]->Name << "\n";
        Main = (*this)[id];
    }
    auto getMain() -> File& {
        return *Main;
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

    constexpr auto setMain        (std::string_view main) -> Project& {ProjectFile.setMain(main); return *this;}
    constexpr auto setCompiler    (std::string_view comp) -> Project& {Compiler = comp; return *this;}
    constexpr auto addOptions     (std::string_view opt)  -> Project& {
        if(!Options.empty()) Options+= " ";
        Options += opt; 
        return *this;
    }
    constexpr auto addOptions     (std::span<const std::string_view> opt)  -> Project& {
        for (auto& sv : opt) {
            if(!Options.empty()) Options+= " ";
            Options += sv; 
        }
        return *this;
    }
    constexpr auto addLdOptions   (std::span<const std::string_view> opt)  -> Project&  {
        for (auto& sv : opt) {
            if(!Options.empty()) LdOptions+= " ";
            LdOptions += sv; 
        }
        return *this;
    }
    constexpr auto addLdOptions   (std::string_view opt)  -> Project&  {
        if(!LdOptions.empty()) LdOptions+= " ";
        LdOptions += opt; 
        return *this;
    }
    constexpr auto setProjectPath (std::string_view in) -> Project&   {Path = in; return *this;}
    constexpr auto setResourcePath(std::string_view in) -> Project&   {ResPath = in; return *this;}
    auto addSourcePath  (std::string_view in) -> Project&   {
        const fs::path temp {Path / in};
        if (!fs::exists(temp)) log::msg<log::Error>("Source path does not exist",temp.string());
        SourcePath.insert(temp.string()); 
        return *this;
    }
    constexpr auto addSourcePathList  (std::vector<std::string_view> ListPath) -> Project&   {
        for (const auto& P : ListPath) {
            addSourcePath(P);
        } 
        return *this;
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
    auto addIncludePathList (std::initializer_list<const std::string_view> ListPath) -> Project& {
        return addIncludePathList({ListPath.begin(),ListPath.end()});
    }
    
    auto getMainPath () const -> const std::string&    {return *SourcePath.begin();}
    auto getCompileCommand () const -> compileCommand* {return cmdJson;}

    auto addCompileCommand(compileCommand* cmd) -> Project&
    {
        cmdJson = cmd;
        return *this;
    }
    
    auto addSource(std::string_view from,std::string_view file) -> Project& {
        const fs::path fromPath {Path / from};
        if (!fs::is_directory(fromPath)) log::msg<log::Error>("Is not a directory",fromPath.string());
        
        if (file == "*") {
            const fs::directory_iterator iterator(fromPath);
            for (const auto& entry : iterator) {
                const bool isModule = fileUtil::isModule(entry.path().extension().string());
                const bool isSource = fileUtil::isCpp(entry.path().extension().string());
                if (entry.is_regular_file() && ( isModule || isSource)) {
                    const auto& path = entry.path();
                    // std::cout << fmt("add project file " , entry.path().filename().string() , " " , entry.path().string()).endl();
                    auto& f = ProjectFile.addFile(path.filename().string());
                    f.second.FileName = path.filename().string();
                    f.second.Name = path.stem().string();
                    f.second.Path = path.string();
                    f.second.fileType = isModule ? File::Module : File::Source;
                    // f.second.onArchive = (outFile == Project::staticLib ? true : false);
                }
            }
        } else {
            const fs::path sourcePath {fromPath / file};
            if (!fs::exists(sourcePath)) log::msg<log::Error>("Source path does not exist",sourcePath.string());
            const bool isModule = fileUtil::isModule(sourcePath.extension().string());
            // const bool isSource = fileUtil::isCpp(sourcePath.extension().string());
            auto & P = ProjectFile.addFile(sourcePath.filename().string());

            P.second.FileName = sourcePath.filename().string();
            P.second.Name = sourcePath.stem().string();
            P.second.Path = sourcePath.string();
            P.second.fileType = isModule ? File::Module : File::Source;
            // P.second.onArchive = (outFile == Project::staticLib ? true : false);
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
                LdOptions += sformat(" -L{} -l{}",other.OutPath->exePath.string(),other.outputName);
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
            File.ldFlags += " -l";
            File.ldFlags +=  dep;
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
            File.Flags += " ";
            File.Flags += dep;
        }
        return *this;
    }
    auto addCompileFlags(const std::string_view inFile,std::initializer_list<const std::string_view> ListFlags) -> Project&
    {
        return addCompileFlags(inFile,{ListFlags.begin(),ListFlags.end()});
    }
    
    auto getRawHeaderName(const std::string_view in) {
        struct out {
            const std::string_view sv;
            const bool startNpos;
            const bool endNpos;
        };
        const char* data = in.data();
        std::size_t startPos {std::string_view::npos};
        std::size_t endPos   {std::string_view::npos};
        char closeChar       {'\0'};
        // ; data != in.end(); ++data
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
    
    auto solveHeaderDependencies(HeaderFile& Header) -> void 
    {
        if (Header.Path.empty()) log::msg<log::Error>("Project path is Empty",Header.Path); 
        const std::string HeaderPath = (fs::path{Header.Path} / Header.Name).generic_string(); 
        std::ifstream files(HeaderPath);
        if (!files.is_open()) log::msg<log::Error>("Unable to open file",HeaderPath);
        std::string line;
        bool inBlockComment = false; 
        while (std::getline(files, line)) {
            if(blockedComment(&inBlockComment, &line)) {continue;}
            const size_t ipos = line.find(fileUtil::includeToken);
            
            if(singleLineComment(line, &ipos)) {continue;}
            if (ipos != std::string::npos) { 
                const size_t searchStart = ipos + fileUtil::includeToken.length();
                const auto [headerName,startNpos,endNpos] = getRawHeaderName(std::string_view{line}.substr(searchStart));
                if (startNpos || endNpos) { continue; }
                
                for (const auto& [K,V] : ProjectFile.headerContainer()) {
                    if (K.empty()) continue;
                    // const bool foundMatch = std::ranges::find(V,headerName,&HeaderFile::Name) != V.end();
                    // std::cout << fmt("is header path "_fmt.color(fmt::Bold_Blue) , headerName , " in " , Header.Name ).endl();
                    for (const auto& HN : V) {
                        if (HN.Name == headerName) {
                            std::string flags {sformat("-I{}",K)};
                            const bool alreadyAdded = containsToken(Header.flags, flags);
                            const bool notSamePath = (Header.Path != K);
                            if (!alreadyAdded && notSamePath) {
                                std::printf("%s",sformat("{} {} to {}\n",addColors("Add dependencies",strColors::Bold_Blue) , K , Header.Name ).c_str());
                                if (!Header.flags.empty()) {
                                    Header.flags += " ";
                                }
                                Header.flags += flags;
                            }
                        }
                    }
                }
            }
        }  
        
    }

    auto getHeaderFile() -> Project& 
    {
        for (const auto& K : ProjectFile.headerContainer()) {
            const fs::directory_iterator it(K.first);
            // std::cout << "Scan File: "_fmt.color(fmt::Bold_Green) << K;
            for (const auto& entry : it) {
                if (entry.is_regular_file() && fileUtil::isCppHeader(entry.path().extension().string()) ) {
                    // std::cout << fmt("Add include " , entry.path().filename().string() , " From: " , K.first).endl();
                    ProjectFile.addHeader(K.first, entry.path().filename().string());
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
                bool isModule = fileUtil::isModule(entry.path().extension().string());
                bool isSource = fileUtil::isCpp(entry.path().extension().string());
                if (entry.is_regular_file() && ( isModule || isSource)) {
                    const auto& path = entry.path();
                    // std::cout << fmt("add project file " , entry.path().filename().string() , " " , entry.path().string()).endl();
                    auto& f = ProjectFile.addFile(path.filename().string());
                    f.second.FileName = path.filename().string();
                    f.second.Name = path.stem().string();
                    f.second.Path = path.string();
                    f.second.fileType = File::Source;
                    f.second.onArchive = outFile == Project::staticLib ? true : false;
                }
            }
        }
    }

    
    auto trim(std::string_view str) -> std::string_view 
    {
        const size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        const size_t last = str.find_last_not_of(" \t\r\n");
        return std::string_view(str.data() + first, (last - first + 1));
    }

    auto singleLineComment (std::string_view line, const std::size_t* ipos) -> bool 
    {
        // 1. Safety check for null pointers
        if (!line.data() || !ipos) return false;
        const size_t inlineComment = line.find("//");
        // 2. If there is no comment on this line, we definitely don't skip based on comments
        if (inlineComment == std::string::npos) {
            return false;
        }
        // 3. If the import token wasn't found, but a comment WAS found, skip the line
        if (*ipos == std::string::npos) {
            return true;
        }
        // 4. Skip only if the comment physically appears BEFORE the import token
        return inlineComment < *ipos;
    };

    auto blockedComment (bool* inBlockComment,const std::string* line) -> bool 
    {
        if (!inBlockComment || !line) return false;
        if (*inBlockComment) {
            const size_t endComment = line->find("*/");
            if (endComment != std::string::npos) {
                *inBlockComment = false; // Block ended, but skip this line anyway to be safe
                return true;
            }
            return true; // Skip processing this line
        }
        // 2. Check if a multi-line comment block starts on this line
        const size_t startBlock = line->find("/*");
        if (startBlock != std::string::npos) {
            const size_t endBlock = line->find("*/", startBlock + 2);
            // If it doesn't close on the same line, flag it for subsequent lines
            if (endBlock == std::string::npos) {
                *inBlockComment = true;
            }
            return true; // This line contains a block start, skip processing it
        }
        return false;
    };

   auto containsToken(std::string_view str,const std::string_view target) noexcept -> bool 
    {
        size_t startidx{0};
        // size_t endidx = 0; endidx <= str.size(); ++endidx
        for (const char& c : str) {
            if (c == str.back() || c == ' ') {
                const size_t idx = &c - str.data();
                const size_t token_len = idx - startidx;
                if (token_len > 0) {
                    if (std::string_view(str.data() + startidx, token_len) == target) {
                        return true;
                    }
                }
                startidx = idx + 1;
            }
        }
        return false;
    }

    auto scanHeader() -> Project& 
    {
        for (auto& [K, V] : ProjectFile) {
            if (V.Path.empty()) log::msg<log::Error>("Project path is Empty",V.Path);
            
            std::ifstream files(V.Path.data());
            if (!files.is_open()) log::msg<log::Error>("Unable to open file",V.Path);

            std::string line;
            bool inBlockComment = false;
            bool exportModuleFound = false;
            bool moduleFound = false;

            while (std::getline(files, line)) {
                if (blockedComment(&inBlockComment, &line)) continue;
                std::string_view trimmedLine = trim(line);

                // -------------------------------------------------------------------
                // 1. Detect Module Interface: "export module <name>;"
                // -------------------------------------------------------------------
                const size_t epos = line.find(fileUtil::exportToken); // e.g., "export module"
                if (singleLineComment(line, &epos)) continue;

                if (!exportModuleFound && epos != std::string::npos) {
                    std::string_view moduleName = std::string_view{line}.substr(epos + fileUtil::exportToken.length() + 1);
                    moduleName = moduleName.substr(0, moduleName.find(';'));
                    moduleName = trim(moduleName);

                    if (moduleName.find(':') != std::string::npos) {
                        V.isPartition = true;
                    }

                    V.Name = moduleName;
                    V.fileType = File::Module; // Interface module
                    V.setObjOutputName(OutPath->objPath);

                    std::string moPath = V.getModuleOutput(&OutPath->modulePath);
                    V.compiled = recompile ? false : (fs::exists(V.objectPath) && fs::exists(moPath) 
                        ? fs::last_write_time(V.Path) < fs::last_write_time(V.objectPath) 
                        : false);

                    exportModuleFound = true;
                    moduleFound = true;
                    continue; // Skip implementation check for this line
                }

                // -------------------------------------------------------------------
                // 2. Detect Module Implementation: "module <name>;"
                // -------------------------------------------------------------------
                if (!moduleFound && trimmedLine.rfind("module ", 0) == 0) {
                    const size_t mpos = line.find("module");
                    if (!singleLineComment(line, &mpos)) {
                        std::string_view moduleName = std::string_view{line}.substr(mpos + 6); // 6 == length of "module"
                        moduleName = moduleName.substr(0, moduleName.find(';'));
                        moduleName = trim(moduleName);

                        if (!moduleName.empty()) {
                            if (moduleName.find(':') != std::string::npos) {
                                V.isPartition = true;
                            }

                            V.Name = moduleName;
                            V.fileType = File::ModuleImpl;
                            V.setObjOutputName(OutPath->objPath);
                            auto minterface = ProjectFile.getByName(moduleName);
                            V.dependencies.push_back(minterface->Name);
                            // std::string moPath = V.getModuleOutput(&OutPath->modulePath);
                            V.compiled = recompile ? false : (fs::exists(V.objectPath)
                                ? fs::last_write_time(V.Path) < fs::last_write_time(V.objectPath) 
                                : false);

                            moduleFound = true;
                        }
                    }
                }

                // -------------------------------------------------------------------
                // 3. Detect #include or import dependencies
                // -------------------------------------------------------------------
                const size_t ipos = line.find(fileUtil::includeToken);
                if (ipos != std::string::npos) {
                    const size_t searchStart = ipos + fileUtil::includeToken.size();
                    auto [headerName,start,end] = getRawHeaderName(std::string_view{line}.substr(searchStart));
                    if (start || end) { continue; }
                        
                    for (const auto&  [HP,HF] : ProjectFile.headerContainer()) {
                        auto findInclude = &*HF.end();
                        for (auto& s : HF) if(s.Name == headerName) {findInclude = &s; break;}
                        
                        if (findInclude != &*HF.end()) {
                            const std::string i {sformat("-I{}",HP)};
                            if (!containsToken(V.Flags, i)) {
                                V.Flags.append(" ") += i; 
                                if(!findInclude->flags.empty()) {
                                    const char* beg = &*findInclude->flags.begin();
                                    for (auto& c : findInclude->flags) {
                                        if (c == ' ') {
                                            std::string_view temp {beg,&c};
                                            if(!containsToken(V.Flags,temp)) V.Flags.append(" ") += temp;
                                            beg = &c;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            files.close();
        }
        return *this;
    }

    

    auto scanModule() -> Project& 
    {
        for (const auto& [K,V] : ProjectFile) {
            if (V.fileType == File::SystemHeader) { continue;}
            std::printf("%s",sformat("Scan module {}\n" , V.Path ).c_str());
            if (V.Path.empty()) log::msg<log::Error>("Project path is Empty",V.Path);
            
            const std::string temp = (fs::path(V.Path) / V.Name).generic_string();
            std::ifstream files;
            if (V.fileType == File::HeaderUnit) {files.open(temp);}
            else {files.open(V.Path);}
            if (!files.is_open()) log::msg<log::Error>("Unable to open file",temp);

            std::string line;
            bool inBlockComment = false;

            while (std::getline(files, line)) {
                auto& ModuleFile = *ProjectFile[V.FileName]; 

                const size_t ipos = line.find(fileUtil::importToken);
                const size_t epos = line.find(';');
                if(blockedComment(&inBlockComment, &line) || singleLineComment(line, &ipos)) {continue;}
            
                std::string_view moduleName;
                if (ipos != std::string::npos && epos != std::string::npos) {
                    const size_t searchStart = ipos + fileUtil::importToken.length();
        
                    const size_t startPos = line.find_first_not_of(" \t\r", searchStart);
                    if (startPos == std::string::npos || startPos >= epos) {continue;}
                    
                    // Find the end of the module name string before trailing whitespaces or semicolon
                    const size_t endPos = line.find_last_not_of(" \t\r", epos - 1);
                    if (endPos == std::string::npos || endPos < startPos) {continue;}

                    moduleName = std::string_view{line}.substr(startPos, (endPos - startPos) + 1);
                    std::printf("%s",sformat("{} {} found in {}\n",addColors("import module",strColors::Bold_Blue), moduleName , V.Path).c_str());
                    
                    // -------------------------------------------------------------------
                    // 3. Detect System Module Unit
                    // -------------------------------------------------------------------
                    if (moduleName.front() == '<' || moduleName.front() == '"') {
                        std::printf("%s",sformat("{} From: {} Name: {}\n",addColors("Header Unit module",strColors::Bold_Blue), V.Name , moduleName).c_str());
                        const auto [rawHeader,start,end] = getRawHeaderName(moduleName);
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
                            // auto findHeader = std::ranges::find( it,moduleName,&HeaderFile::Name);
                            auto findHeader = &*it.end()->second.end();
                            for(const auto& [key,value] : it) {
                                for (const auto& file : value) if (file.Name == moduleName) {findHeader = &file; break;}
                            }
                            if (findHeader != &*it.end()->second.end()) {
                                auto& h = findHeader;
                                std::printf("%s",sformat("{} From: {} to: {} and {}\n",addColors("add Header into Unit module",strColors::Bold_Green) , h->Path , moduleName , V.Name).c_str());
                                std::string i {sformat("-I{}",h->Path)};
                                if (!containsToken(headerUnitFlags,i)) headerUnitFlags.append(" ") += i;
                                if (!containsToken(ModuleFile.Flags,i)) ModuleFile.Flags.append(" ") += i;

                                if (!h->flags.empty()) {
                                    const char* beg = &*h->flags.begin();
                                    for (auto& c : h->flags) {
                                        if (c == ' ') {
                                            std::string_view temp {beg,&c};
                                            if(!containsToken(ModuleFile.Flags,temp)) ModuleFile.Flags.append(" ") += temp;
                                            if(!containsToken(headerUnitFlags,temp)) headerUnitFlags.append(" ") += temp;
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
                        auto Added = [&ModuleFile,&MV]{
                            for(auto& dep : ModuleFile.dependencies) if (MV.FileName == dep) return true;
                            return false;
                        };
                        if (sameName && !Added()) ModuleFile.dependencies.emplace_back(MV.FileName);
                    }
                }
            }
            files.close();
        }
        return *this;
    }

    auto configureModuleFlags() -> Project& {
        std::string temp {};
        for (auto& File : ProjectFile | std::views::values) {
            auto& inFile = File;
            const bool isSystemHeader = (File.fileType == File::SystemHeader);
            if(isSystemHeader) continue;
            for (const auto& I : File.dependencies) {
                const auto& depFile = *ProjectFile[I];
                const auto& mPath = (isSystemHeader ? OutPath->stdPath : OutPath->modulePath);
                const bool depisSystemHeader = depFile.fileType == File::SystemHeader;
                const bool depisHeaderUnit = depFile.fileType == File::HeaderUnit;
                if(!temp.empty() && !temp.ends_with(' ')) temp+= ' ';
                temp += (
                    depisSystemHeader ? sformat("-fmodule-file={}",depFile.objectPath) :
                    depisHeaderUnit ? sformat("-fmodule-file={}{}",(mPath / depFile.getName()).string(), fileUtil::pcmModule) :
                    sformat("-fmodule-file={}={}{}",depFile.Name,(mPath / depFile.getName()).string(),fileUtil::pcmModule)
                );
            }
            if (!inFile.Flags.ends_with(' ')) inFile.Flags += ' ';
            inFile.Flags += temp;
            temp.clear();
        }
        return *this;
    }

    auto compilePCH(std::string_view PCHfile) -> bool {
        const std::string headerFile = (fs::path(ProjectFile.getHeaderPath(PCHfile)) / PCHfile).string();
        const auto& oPath = OutPath->buildPath;
        const std::string pchOut {sformat("{}.pch",(oPath / fs::path(PCHfile).stem()).string())};
        const std::string f_cmd  {sformat("{} {} -x c++-header {} -o {}",Compiler, Options,headerFile,pchOut)};
        Options += sformat(" -include-pch {} ",pchOut);
        if (fs::exists(pchOut)) {
            if (fs::last_write_time(headerFile) > fs::last_write_time(pchOut)) {
                fs::rename(pchOut,sformat("{}.old",pchOut));
            } else {
                return 1;
            }
        }
        std::printf("%s\n",sformat("{} {}",addColors("Compiling PCH",strColors::Bold_Green) , f_cmd).c_str());
        return cmd << f_cmd >> sformat("{}",addColors("Error compiling ",strColors::Bold_Red));
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
                    // std::cout << dep.FileName << " is not compiled \n";
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
        const std::string_view compileFormat = (HasHeaderUnit ? "{} {} -Wno-experimental-header-units {} {} -o {}": "{} {} {} {} -o {}");
        const std::string f_cmd {
            isSystemHeader ? sformat("{} {} {}",Compiler, Options,f_srcInput) : 
            isHeaderUnit ? sformat("{} {} {} {}",Compiler, Options,inFile.Flags,f_srcInput) :
            sformat(compileFormat , Compiler, Options ,f_srcInput ,inFile.Flags,fObjOutput)
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
                const std::string old {sformat("{}.old",fModule)}; 
                if (fs::exists(old)){fs::remove(old);}
                fs::rename(fModule,old);
            }
            #endif
            // fs::copy(old,fModule);
            std::printf("%s",sformat("{} {}\n",addColors("compiling module",strColors::Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> sformat("{}",addColors("recompile error",strColors::Red));
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
            ret = cmd << f_cmd >> sformat("{}",addColors("Error compiling",strColors::Bold_Red));
            
        } else if (fs::last_write_time(inFile.Path) > fs::last_write_time(objOutput))
        {
            std::printf("%s",sformat("{} {}\n",addColors("Updating",strColors::Bold_Green) , f_cmd).c_str());
            ret = cmd << f_cmd >> sformat("{}",addColors("Error compiling",strColors::Bold_Red));
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
                if(!I.ldFlags.empty() && !isStaticLib) {temp +=I.ldFlags;}
                if(!temp.empty() && *temp.end() != ' ') {temp += " ";}
                temp += I.objectPath;
            }
            return (hasDependencies && !isStaticLib ?
            sformat( "{} -fprebuilt-module-path={}",temp,(OutPath->modulePath / ".").string()) :
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
        cmd << f_cmd >> sformat("{}",addColors("linking error",strColors::Bold_Red));
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

// int compileModule(std::string_view in_path) {

    //     bool headerUnits = false;
    //     const bool f_isSystemHeader = in_path.front() == '<';
    //     if(f_isSystemHeader) in_path = in_path.substr(1,in_path.length() - 2);
        
    //     const fs::path fPath = in_path;
    //     const bool f_isUserHeader = file.isCppHeader(fPath.extension().string());
    //     const auto& mPath = OutPath->modulePath;
    //     const auto& oPath = OutPath->objPath;
        
    //     const std::string f_module    = fmt((mPath / (f_isUserHeader ? fPath.filename() : fPath.stem())).string(), file.pcmModule).str;

    //     const std::string f_objOutput = fmt((oPath / fPath.stem()).string(), file.objFile).str;
        
    //     const auto l_rewrite = [&f_isUserHeader,&f_module,&f_isSystemHeader] -> void {
    //         if (!f_isSystemHeader || !f_isUserHeader) {
    //             const std::string old = fmt(f_module,".old").str; 
    //             if(fs::exists(f_module)) {
    //                 if (fs::exists(old)){
    //                 fs::remove(old);}
    //                 fs::rename(f_module,old);
    //             }
    //         }
    //     };
    //     const std::string compileInclude = [this,&fPath](){
    //         std::string temp {};
    //         for (const auto& i : IncludeMap)
    //         {
    //             for (const auto& m : i.second) {
    //                 if (m == fPath.string()) {
    //                     temp.append(fmt(" -I",i.first.first));
    //                 }
    //             }
    //         }
    //         return temp;
    //     }();

    //     const bool f_inObject = [&f_objOutput,this]() {
    //         for (const auto& obj : Object) {
    //             if (obj == f_objOutput) {
    //                 return true;
    //                 break;
    //             }
    //         } return false;
    //     }();
        
    //     const std::string f_srcInput = 
    //     f_isUserHeader ? fmt("-Wno-pragma-system-header-outside-header -fmodule-header=user --precompile ",fPath.string()," -o ",f_module).str :
    //     f_isSystemHeader ? fmt("-Wno-pragma-system-header-outside-header -x c++-system-header --precompile ",in_path," -o ",f_module).str :
    //     fmt("-c ",(Path / fPath).string()," -fno-modules-reduced-bmi -fmodule-output=",f_module," -fprebuilt-module-path=",(mPath).string()).str;
        
    //     // if (f_isSystemHeader) {
    //     //     f_srcInput = fmt("-Wno-pragma-system-header-outside-header -x c++-system-header --precompile ", in_path, " -o ", f_module).str;
    //     // } 
    //     // else if (f_isUserHeader) { // If it skips this, f_isUserHeader is definitely false
    //     //     f_srcInput = fmt("-Wno-pragma-system-header-outside-header -x c++-header --precompile ", fPath.string(), " -o ", f_module).str;
    //     // } 
    //     // else {
    //     //     f_srcInput = fmt("-c ", (Path / fPath).string(), " -fno-modules-reduced-bmi -fmodule-output=", f_module, " -fprebuilt-module-path=", (mPath).string()).str;
    //     // }

    //     for (const auto& [mod , dep] : ModuleMap)
    //     {
    //         if (fs::path(mod).filename() == fPath.filename()) {
    //             for(const auto& d : dep)
    //             {
    //                 headerUnits = d.front() == '<'; 
    //                 ModuleDeps[fPath.filename().string()].append(
    //                     fmt(headerUnits ? fmt(" -fmodule-file=",(mPath / d.substr(1,d.length() - 2)).string()) : fmt(" -fmodule-file=",d,"=",(mPath / d).string()),file.pcmModule," "));
    //             }
    //         }
    //     }
        
    //     if (!f_inObject && !f_isSystemHeader && !f_isUserHeader) {Object.emplace_back(f_objOutput);}

    //     const std::string f_cmd = f_isSystemHeader ? fmt(Compiler, Options,f_srcInput\)().str : 
    //     f_isUserHeader ? fmt(Compiler, Options,f_srcInput\)().str :
    //     fmt(Compiler, Options,headerUnits ? "-Wno-experimental-header-units ": "" ,f_srcInput ,compileInclude,ModuleDeps[fPath.filename().string()]," -o ",f_objOutput\)().str;
        
    //     if(cmdJson != nullptr && !f_isSystemHeader) { cmdJson->addCompilecmd((Path / fPath).parent_path().string(),f_cmd,(Path / fPath).string(),f_objOutput);}
        
    //     int ret {};
    //     if (recompile && !f_isSystemHeader) {
    //         std::cout << fmt("recompiling "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         l_rewrite();
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } else if (f_isSystemHeader) {
    //         std::cout << fmt("System Header "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         if(fs::exists(f_module)) {return 0;}
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } else if (!fs::exists(f_module)) {
    //         std::cout << fmt("compiling "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } else if (fs::last_write_time(in_path) > fs::last_write_time(f_module)) {
    //         std::cout << fmt("updated "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         l_rewrite();
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } 
    //     ProjectFile[in_path].compiled = (ret == 0 ? true : false);
    //     return ret; 
    // };
#endif