#ifndef BUILD_PP 
#define BUILD_PP

#include <cstddef>
#include <cstdlib>
#include <concepts>
#include <filesystem>
#include <vector>
#include <string_view>
#include <string>
#include <set>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <utility>
#include <algorithm>
#include <ranges>
#include <source_location>
#include <mutex>
#include <limits>

#include "json.hpp"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace  fs = std::filesystem;
using namespace std::string_view_literals;

namespace Math {
    template<typename T> requires (std::integral<std::decay_t<T>> || std::floating_point<std::decay_t<T>>)
    static consteval auto maxDigits() -> std::size_t {
        using Unqualified = std::remove_cvref_t<T>;
        if (std::integral<Unqualified>) {
            constexpr bool isSigned = std::is_signed_v<Unqualified>;
            return std::numeric_limits<Unqualified>::digits10 + 1 + (isSigned ? 1 : 0);
        } else {
            return 24;
        }
    }

    static constexpr double PI = 3.1415926535;

    constexpr std::array<double, 19> CompactPow10 = []() consteval {
        std::array<double, 19> arr{};
        arr[0] = 1.0;
        for (size_t i = 1; i < arr.size(); ++i) {
            arr[i] = arr[i - 1] * 10.0;
        }
        return arr;
    }();

    
    template <std::floating_point T>
    constexpr size_t digitLength (const T& in,size_t precision = 0) {
        if (in == 0) return 1;

        bool isNeg = in < 0.0;
        double absVal = static_cast<double>(isNeg ? -in : in);

        if (absVal < 9.9) {
            return 1 + precision + (isNeg ? 1 : 0);
        }

        // Fast unrolled binary search over the 19-element LUT
        std::size_t digits = 1;
        if (absVal >= Math::CompactPow10[10]) {
            if (absVal >= Math::CompactPow10[14]) {
                digits = (absVal >= Math::CompactPow10[16]) 
                    ? (absVal >= Math::CompactPow10[17] ? (absVal >= Math::CompactPow10[18] ? 19 : 18) : 17)
                    : (absVal >= Math::CompactPow10[15] ? 16 : 15);
            } else {
                digits = (absVal >= Math::CompactPow10[12]) 
                    ? (absVal >= Math::CompactPow10[13] ? 14 : 13)
                    : (absVal >= Math::CompactPow10[11] ? 12 : 11);
            }
        } else {
            if (absVal >= Math::CompactPow10[5]) {
                digits = (absVal >= Math::CompactPow10[7]) 
                    ? (absVal >= Math::CompactPow10[8] ? (absVal >= Math::CompactPow10[9] ? 10 : 9) : 8)
                    : (absVal >= Math::CompactPow10[6] ? 7 : 6);
            } else {
                digits = (absVal >= Math::CompactPow10[2]) 
                    ? (absVal >= Math::CompactPow10[3] ? (absVal >= Math::CompactPow10[4] ? 5 : 4) : 3)
                    : (absVal >= Math::CompactPow10[1] ? 2 : 1);
            }
        }

        return digits + precision + (isNeg ? 1 : 0);
    }

    template <std::integral T>
    constexpr size_t digitLength (const T& in) {
        using UnsignedT = std::make_unsigned_t<T>;
        bool isNeg = in < 0;
        UnsignedT val = isNeg ? static_cast<UnsignedT>(-in) : static_cast<UnsignedT>(in);

        if (val <= 9) return 1;

        static constexpr unsigned char table[] = {
            1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9,
            10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 19, 20
        };

        size_t bits = sizeof(UnsignedT) * 8 - std::countl_zero(val);
        size_t digits = table[bits - 1];
        digits -= (val < Math::CompactPow10[digits - 1]);

        return digits + isNeg;
    }
};


static_assert(Math::digitLength(900) == 3,"" );
static_assert(Math::digitLength(900.6) == 3,"" );

template <typename T>
concept onlyStrConv = requires(T t) { { std::string_view(t) } -> std::same_as<std::string_view>; };
template <typename T>
concept Formattable = 
    onlyStrConv<T> || 
    std::integral<std::decay_t<T>> ||
    std::floating_point<std::decay_t<T>>;

template <typename... Args>
concept onlyStr = (Formattable<Args> && ...);

struct [[nodiscard]] fmt {
    std::string str;

    enum colors {
        Not_color,
        Black,     Bold_Black,     High_Black,
        Red,       Bold_Red,       High_Red,
        Green,     Bold_Green,     High_Green,
        Yellow,    Bold_Yellow,    High_Yellow,
        Blue,      Bold_Blue,      High_Blue,
        Purple,    Bold_Purple,    High_Purple,
        Cyan,      Bold_Cyan,      High_Cyan,
        White,     Bold_White,     High_White,
    };

    // 2. Format String Constructor: fmt("Value: {}, Status: {}", 42, "OK")
    constexpr fmt(std::string_view fmtStr) : str(fmtStr) {}
    template<onlyStr... Args>
    constexpr fmt(std::string_view fmtStr, Args&&... args) {
        // Check if fmtStr actually contains placeholders
        // if constexpr (sizeof...(Args) > 0) {
        //     if (fmtStr.find('{') != std::string_view::npos) {
        formatInit(fmtStr, std::forward<Args>(args)...);
        // return;
        //     }
        // }
        // varConcat(fmtStr, std::forward<Args>(args)...);
        // return;
        // __builtin_unreachable();
        // Default to concatenation if no placeholders found
    }
    template<onlyStr... Args>
    constexpr auto varConcat(Args&&... args) -> fmt& { 
        size_t preAlloc = (getArgSize(args) + ... + 0);
        str.reserve(preAlloc);
        (appendArg(std::forward<Args>(args)), ...);
        return *this;
    }
    
    // Apply ANSI Color
    constexpr auto color(colors c) -> fmt& { 
        str.reserve(str.size() + 14);
        str.insert(0, this->getColor(c));
        str.append(this->getColor(Not_color));
        return *this;
    }

    // Collapse multiple contiguous spaces into a single space
    constexpr auto clean() -> fmt& { 
        if (str.empty()) return *this;

        size_t writeIdx = 0;
        bool inSpace = false;

        for (size_t readIdx = 0; readIdx < str.size(); ++readIdx) {
            char c = str[readIdx];
            if (c == ' ') {
                if (!inSpace) {
                    str[writeIdx++] = c;
                    inSpace = true;
                }
            } else {
                str[writeIdx++] = c;
                inSpace = false;
            }
        }
        str.resize(writeIdx);
        return *this;
    }

    constexpr auto endl() -> fmt& { str.append("\n"); return *this; }

    constexpr auto sv() const   -> std::string_view { return str; }
    constexpr auto cstr() const -> const char* { return str.c_str(); }

    constexpr operator std::string() const & { return str; }
    constexpr operator std::string_view() const noexcept { return sv(); }
    constexpr explicit operator const char*() const & noexcept { return cstr(); }
    constexpr explicit operator std::string() && { return std::move(str); }
    
    friend auto operator<<(std::ostream& os, const fmt& f) -> std::ostream& {
        return os << f.str;
    }

private:
    template<onlyStr... Args>
    constexpr auto formatInit(std::string_view fmtStr, Args&&... args) -> void {
        size_t preAlloc = fmtStr.size() + (getArgSize(args) + ... + 0);
        str.reserve(preAlloc);

        size_t lastPos = 0;
        (pargs(std::forward<Args>(args),fmtStr,lastPos), ...);
        if (lastPos < fmtStr.size()) {
            str += fmtStr.substr(lastPos);
        }
    }
    template <typename T>
    constexpr auto pargs(T&& arg,std::string_view& fmtStr,std::size_t& lastPos) {
        size_t pBegin = fmtStr.find('{', lastPos);
        if (pBegin != std::string_view::npos) {
            size_t pEnd = fmtStr.find('}', pBegin + 1);
            if (pEnd != std::string_view::npos) {
                str.append(fmtStr.substr(lastPos, pBegin - lastPos));
                appendArg(std::forward<T>(arg));
                lastPos = pEnd + 1;
            }
        }
    }

    template<onlyStr T>
    constexpr auto getArgSize(const T& arg) const -> size_t {
        using Raw = std::remove_cvref_t<T>;
        if constexpr (std::is_convertible_v<Raw, std::string_view>) {
            return std::string_view (arg).size();
        } else if constexpr (std::integral<Raw>) {
            return Math::digitLength(arg);
        } else if constexpr (std::floating_point<Raw>) {
            return Math::digitLength(arg);
        }
        return 0;
    }

    template<onlyStr T>
    constexpr auto appendArg(T&& arg) -> void {
        using Raw = std::remove_cvref_t<T>;
        if constexpr (std::is_convertible_v<Raw, std::string_view>) {
            str += std::string_view(arg);
        } else if constexpr (std::integral<Raw>) {
            char digit[Math::maxDigits<Raw>()];
            char* eDigit = std::to_chars(digit, digit + Math::maxDigits<Raw>(), std::forward<T>(arg)).ptr;
            str += std::string_view(digit, static_cast<std::size_t>(eDigit - digit));
        } else if constexpr (std::floating_point<Raw>) {
            char digit[Math::maxDigits<Raw>()];
            char* eDigit = std::to_chars(digit, digit + Math::maxDigits<Raw>(), std::forward<T>(arg)).ptr;
            str += std::string_view(digit, static_cast<std::size_t>(eDigit - digit));
        } else {
            __builtin_unreachable();
        }
    }

    

    constexpr auto getColor(colors color) const -> std::string_view {
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
            default:           return "";
        }
    }
};
constexpr auto operator""_fmt(const char* str,size_t) -> fmt { return fmt(str);}

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
        if (ret != 0) {
            std::cout << msg << "\n";
            std::exit(1);
        }
        return ret;
    }
    auto operator <<(std::string_view cmd) -> cmdImpl& { return run({cmd.data(),cmd.size()});}
    auto operator >>(std::string_view cmd) -> int      { return err({cmd.data(),cmd.size()});}
};
inline cmdImpl cmd;

struct outputPath {
    private:
    auto err (bool e = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) -> outputPath& {
        if (e) {std::cout << fmt ("{} {}:{}:{} at: {}\n",msg,fn.file_name(),fn.line(),fn.column(),fn.function_name()); std::exit(1);} 
        return *this;
    }
    public:
    fs::path rootPath   {};
    fs::path buildPath  {};
    fs::path objPath    {};
    fs::path stdPath    {};
    fs::path modulePath {};
    fs::path exePath    {};

    auto setRootPath(fs::path root) -> outputPath& { rootPath = root; return *this;}
    auto setExePath(fs::path exe) -> outputPath& {
        if(rootPath.empty()) {return err(true,"root path is empty");}
        if (!fs::exists(exe)) 
        {
            if (fs::create_directory(exe)) {
                // std::cout << fmt("Directory created: {}\n" , exe.string());
            } else {
                err(true,fmt("Failed to create directory: {}\n",exe.string()));
            }        
        } else {
            // std::cout << fmt("{} {}\n","Directory already exists:"_fmt.color(fmt::Bold_Yellow) , exe.string());
        } 
        exePath = exe;
        return err();
    }
    auto setBuildfolder(fs::path folder) -> outputPath& {
    if(rootPath.empty()) {return err(true,"root path is empty");} 
        if (!fs::exists(folder)) 
        {
            if (fs::create_directory(folder)) {
                // std::cout << fmt("Directory created: {}\n" , folder.string());
            } else {
                err(true,fmt("Failed to create directory: {}\n",folder.string()));
            }        
        } else {
            // std::cout << fmt("{} {}\n","Directory already exists:"_fmt.color(fmt::Bold_Yellow) , folder.string());
        }
        return *this;
    }
    auto setOutpath(fs::path out) -> void {
        err(rootPath.empty(),"root path is empty");
        buildPath = out;
        objPath = buildPath / "obj";
        modulePath = buildPath / "module";
        stdPath = modulePath / "std";
        for (const auto& lm_dir : {buildPath,objPath,modulePath,stdPath})
        {
            if (!fs::exists(lm_dir)) 
            {
                if (!lm_dir.has_parent_path()) {
                    err(true,fmt("Error: Path has no parent path: {}" ,lm_dir.string()));
                }
                if (fs::create_directory(lm_dir)) {
                    // std::cout << fmt("Directory created: {}\n" , lm_dir.string());
                } else {
                    err(true,fmt("Failed to create directory: {}\n",lm_dir.string()));
                }        
            }
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
    auto err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) const -> const File& {
        if (cnd) {
            std::cout << fmt ("{} {}:{}:{} at: {}\n",msg,fn.file_name(),fn.line(),fn.column(),fn.function_name()); 
            std::exit(1);
        } 
        return *this;
    }

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
        err(Name.empty(), "File Name Empty");
        err((fileType != Module) && (fileType != SystemHeader) && (fileType != HeaderUnit), "File Not a Module");
        return fmt("{}{}",(*mPath / getName()).string(), fileUtil::pcmModule);
    }
    auto setObjOutputName(const fs::path& oPath) -> void {
        err(Name.empty(),"File Name Empty");
        objectPath = fmt("{}{}{}",(oPath / getName()).string(),fileType == ModuleImpl ? "-impl" : "", fileUtil::objFile);
    }
    auto getName() const -> string_type {
        err(Name.empty(),"File Name Empty");
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
    struct StringHash {
        using is_transparent = void;

        std::size_t operator()(std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }
        std::size_t operator()(const std::string& s) const {
            return std::hash<std::string>{}(s);
        }
        std::size_t operator()(const char* str) const {
            return std::hash<std::string_view>{}(str);
        }
    };
    using File_type = File;
    using MapPair = std::pair<const std::string, File_type>;
    using HeaderContainer = std::vector<HeaderFile>;
    using HeaderMap = std::unordered_map<std::string, HeaderContainer,StringHash,std::equal_to<>>;
    using SourceFileMap = std::unordered_map<std::string, File_type ,StringHash,std::equal_to<>>;

    auto err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) -> FileManager& {
        if (cnd) {
            std::cout << fmt ("{} {}:{}:{} at: {}\n",msg,fn.file_name(),fn.line(),fn.column(),fn.function_name()); 
            // std::cout << fmt ("At: ",fn.file_name(), " ",fn.function_name(), " col: ",std::to_string(fn.column())," line: ",std::to_string(fn.line()), "\n" , msg ,"\n"); 
            std::exit(1);
        } 
        return *this;
    }
    HeaderMap Header {};
    SourceFileMap Files {};

    public:
    File_type* Main = nullptr;
    auto addFile(std::string_view name) -> MapPair& {
        auto it = Files.find(name);
        if (it != Files.end()) {
            return *it;
        }
        auto ref = Files.try_emplace(std::string(name), File{.FileName={name.data(),name.size()}}).first;
        
        return *ref;
    }
    
    auto setHeaderPath(std::string_view path) -> HeaderContainer& {
        return Header.try_emplace({path.data(),path.size()},HeaderContainer{}).first->second;
    }

    auto addHeader(std::string_view path,std::string_view name) -> void {
        auto it = Header.find(path);
        if (it != Header.end()) {
            it->second.push_back(HeaderFile{.Name = {name.data(),name.size()},.Path={path.data(),path.size()}});
        }
        return;
    }
    auto addHeader(HeaderFile H) -> void {
        auto it = Header.find(H.Path);
        if (it == Header.end()) {
            auto ref = &Header.try_emplace(H.Path.data()).first->second;
            ref->push_back(HeaderFile{
                .Name = {std::move(H.Name)},
                .Path=std::move(H.Path),
                .flags=std::move(H.flags)
            });
        } else if (it != Header.end()) {
            it->second.push_back(HeaderFile{
                .Name = {std::move(H.Name)},
                .Path=std::move(H.Path),
                .flags=std::move(H.flags)
            });
        }
        return;
    }
    
    auto getHeaderPath(std::string_view name,std::source_location loc = std::source_location::current()) -> std::string_view {
        for (auto& [I , IN] : Header) {
            for(const auto& N : IN)
            if (N.Name == name) {
                return I;
            }
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Key ",name," doesn't exists"),loc);
        return "";
    }
    auto copyFile(std::string_view name,const File& other) -> File_type& {
        auto ref = Files.try_emplace(std::string(name),File()).first;
        ref->second = {
        .compiled = other.compiled,
        .haveHeaderUnit = other.haveHeaderUnit,
        .onArchive = other.onArchive,
        .fileType = other.fileType,
        // .ID = NextID,
        .Name = other.Name,
        .Path = ref->first,
        .Flags = other.Flags,
        .ldFlags = other.ldFlags,
        .objectPath = other.objectPath
        };
        return ref->second;
    }
    // int getID(std::string_view str) {
    //     const auto it = Files.find(str);
    //     if (it != Files.end()) {
    //         return it->second.ID; 
    //     }
        
    //     std::size_t newId = Files.size();
    //     auto ref = Files.try_emplace(std::string(str), File{.ID = newId}).first;
    //     ref->second.Path = ref->first;
    //     IDMap.emplace_back(&ref->second);
    //     return ref->second.ID;
    // }
    auto getByName(std::string_view id) -> File* {
        for (auto& I : Files) {
            if (I.second.Name == id) {
                return &I.second;
            }
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Name " ,id, " doesn't exists"));
        return nullptr;
    }
    auto operator [](std::string_view id,std::source_location fn = std::source_location::current()) -> File* {
        auto it = Files.find(id); 
        if (it != Files.end()) {
            return &it->second;
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Key " ,id, " doesn't exists"),fn);
        return nullptr;
    }
    // File& operator [](std::size_t id) {
    //     err(id > IDMap.size(),fmt("Error: "_fmt.color(fmt::Red),"ID out of bounds in IDMap"));
    //     return *IDMap[id]; 
    // }
    
    auto getPair (std::string_view id) -> MapPair* {
        auto it = Files.find(id);
        if (it != Files.end()) {
            return &*it; 
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Key doesn't exists"));
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

    auto begin() { return Files.begin(); }
    auto end()   { return Files.end(); }
    
    auto headerContainer() -> HeaderMap& {
        return Header;
    }
    auto begin() const { return Files.begin(); }
    auto end()   const { return Files.end(); }
};

class Project
{
    std::string ProjectName     {};
    std::string Options         {};
    std::string LdOptions       {};
    std::string Compiler        {};
    std::string outputName      {};
    
    constexpr auto err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) -> Project& {
        if (cnd) {std::cout << fmt ("{} {}:{}:{} at: {}\n",msg,fn.file_name(),fn.line(),fn.column(),fn.function_name()); std::exit(1);} 
        return *this;
    }
    
    bool recompile;
    
    public:
    enum projectType : char {
        exe,
        staticLib,
        dynamicLib
    } outFile;
    
    outputPath* OutPath;
    compileCommand* cmdJson;
   

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
        std::cout << fmt("{} {}\n","Project initialized at "_fmt.color(fmt::Green) , this->OutPath->rootPath.string());
    };

    constexpr auto setMain        (std::string_view main) -> Project& {ProjectFile.setMain(main); return *this;}
    constexpr auto setCompiler    (std::string_view comp) -> Project& {Compiler = comp; return *this;}
    constexpr auto addOptions     (std::string_view opt)  -> Project& {
        if(!Options.empty()) Options+= " ";
        Options += opt; 
        return *this;
    }
    constexpr auto addLdOptions   (std::string_view opt)  -> Project&  {
        if(!LdOptions.empty()) LdOptions+= " ";
        LdOptions += opt; 
        return *this;
    }
    constexpr auto setProjectPath (fs::path in)         -> Project&   {Path = in; return *this;}
    constexpr auto setResourcePath(std::string_view in) -> Project&   {ResPath = in; return *this;}
    constexpr auto addSourcePath  (std::string_view in) -> Project&   {
        const fs::path temp {Path / in};
        err(!fs::exists(temp),fmt("{} Source path: {} does not exist","Error:"_fmt.color(fmt::Bold_Red),temp.string()));
        SourcePath.insert(temp.string()); 
        return *this;
    }
    constexpr auto addSourcePathList  (std::vector<std::string_view> ListPath) -> Project&   {
        for (const auto& P : ListPath) {
            addSourcePath(P);
        } 
        return *this;
    }
    constexpr auto addIncludePath (std::string_view IncludePath) -> Project& {
        const fs::path temp {Path / IncludePath};
        err(!fs::exists(temp),fmt("Include path: ",temp.string(), " does not exist ").color(fmt::Bold_Red));
        ProjectFile.setHeaderPath(temp.string()); 
        return *this;
    }
    constexpr auto addIncludePathList (std::initializer_list<std::string_view> ListPath) -> Project& {
        for (auto& in : ListPath) {
            const fs::path temp {Path / in};
            err(!fs::exists(temp),fmt("Include path: ",temp.string(), " does not exist ").color(fmt::Bold_Red));
            ProjectFile.setHeaderPath(temp.string()); 
        }
        return *this;
    }
    
    constexpr auto getMainPath () const -> const std::string& {return *SourcePath.begin();}
    
    constexpr auto addSource(std::string_view from,std::string_view file) -> Project& {
        const fs::path fromPath {Path / from};
        err (!fs::is_directory(fromPath),fmt("{} {} is not a directory","Error"_fmt.color(fmt::Red) , fromPath.string()));
        
        if (file == "*") {
            fs::directory_iterator iterator(fromPath);
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
                    f.second.fileType = File::Source;
                    // f.second.onArchive = (outFile == Project::staticLib ? true : false);
                }
            }
        } else {
            const fs::path sourcePath {fromPath / file};
            err (!fs::exists(sourcePath),fmt("{} Source Path {} does not exist","Error:"_fmt.color(fmt::Red) , sourcePath.string()));
            auto & P = ProjectFile.addFile(sourcePath.filename().string());

            P.second.FileName = sourcePath.filename().string();
            P.second.Name = sourcePath.stem().string();
            P.second.Path = sourcePath.string();
            P.second.fileType = File::Source;
            // P.second.onArchive = (outFile == Project::staticLib ? true : false);
        }
    
        return *this;
    }
    constexpr auto addSource(std::string_view from,std::initializer_list<std::string_view> ListFiles) -> Project& {
        for (const auto& i : ListFiles) {
            addSource(from,i);
        }
        return *this;
    }
    constexpr auto addSource(std::string_view from,std::span<std::string_view> ListFiles) -> Project& {
        for (const auto& i : ListFiles) {
            addSource(from,i);
        }
        return *this;
    }

    auto addCompileCommand(compileCommand* cmd) -> Project&
    {
        cmdJson = cmd;
        return *this;
    }


    // auto getLib(cProject* cProj) -> Project&
    // {
    //     if (cProj->outFile == cProject::staticLib) {
    //          for (const auto& [K,V] : cProj->ProjectFile.headerContainer()) {
    //             for (const auto& I : V) {
    //                 ProjectFile.addHeader(I);
    //             }
    //         }
    //         if(!cProj->outputName.empty()) {
    //             LdOptions += fmt(" -L",cProj->projectOutPath->objPath.string()," -l",cProj->ProjectFile.getMain().Name);
    //         }
    //     }
    //     return *this;
    // }

    auto getLib(Project* other) -> Project&
    {
        if (other->outFile == Project::staticLib) {
            for (const auto& [K,V] : other->ProjectFile.headerContainer()) {
                for (const auto& I : V) {
                    ProjectFile.addHeader(I);
                }
            }
            if(!other->outputName.empty()) {
                LdOptions += fmt(" -L",other->OutPath->exePath.string()," -l",other->outputName);
            }
            other->~Project();
        }
        return *this;
    }

    auto addLinkLibrary(std::string_view inFile, std::initializer_list<std::string_view> ListDeps) -> Project&
    {
        // auto Deps = inDeps | std::views::split(','); 
        const auto rangeFile = ProjectFile | std::views::keys ;
        const auto finds = std::ranges::find(rangeFile,inFile);
        
        if (finds != rangeFile.end()) {
            std::string_view f_file = *finds;
            auto& File = *ProjectFile[f_file]; 
            for (auto&& d : ListDeps) {
                std::string_view dep {d};
                if(dep.empty()) continue;
                while (!dep.empty() && dep.front() == ' ') dep.remove_prefix(1);
                while (!dep.empty() && dep.back() == ' ') dep.remove_suffix(1);
                File.ldFlags += " -l";
                File.ldFlags +=  dep;
            }
        }
        return *this;
    }

    auto addCompileFlags(std::string_view inFile, std::initializer_list<std::string_view> ListFlags) -> Project&
    {
        // auto Deps = inFlags | std::views::split(','); 
        const auto rangeFile = ProjectFile | std::views::keys;
        const auto finds = std::ranges::find(rangeFile,inFile);

        if (finds != rangeFile.end()) {
            std::string_view f_file {*finds};
            auto& File = *ProjectFile[f_file]; 
            for (auto&& dep : ListFlags) {
                if(dep.empty()) continue;
                File.Flags += " ";
                File.Flags += dep;
            }
        }
        
        return *this;
    }

    auto trim(std::string_view str) -> std::string_view 
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return std::string_view(str.data() + first, (last - first + 1));
    }

    auto singleLineComment (std::string_view line, const std::size_t* ipos) -> bool 
    {
        // 1. Safety check for null pointers
        if (!line.data() || !ipos) return false;

        size_t inlineComment = line.find("//");
        
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
            size_t endComment = line->find("*/");
            if (endComment != std::string::npos) {
                *inBlockComment = false; // Block ended, but skip this line anyway to be safe
                return true;
            }
            return true; // Skip processing this line
        }

        // 2. Check if a multi-line comment block starts on this line
        size_t startBlock = line->find("/*");
        if (startBlock != std::string::npos) {
            size_t endBlock = line->find("*/", startBlock + 2);
            
            // If it doesn't close on the same line, flag it for subsequent lines
            if (endBlock == std::string::npos) {
                *inBlockComment = true;
            }
            return true; // This line contains a block start, skip processing it
        }

        return false;
    };

    auto containsToken(std::string_view str,std::string_view target) -> bool 
    {
        auto filter = str | std::views::split(' ')
        | std::views::filter([](auto&& f) { return !f.empty();})
        | std::views::transform([](auto&& f) { 
            return std::string_view(f.data(),f.size());
        });

        return std::ranges::contains(filter,target);
    }

    auto getRawHeaderName(std::string_view in) {
        struct out {
            std::string_view str;
            bool startNpos;
            bool endNpos;
        };
        const auto startPos = in.find_first_of("\"<");
        // Determine the required closing delimiter based on the opening one
        const char openChar = in[startPos];
        const char closeChar = (openChar == '<') ? '>' : '"';
        // Search forward from the opening delimiter for its matching pair
        const auto endPos = in.find(closeChar, startPos + 1);
        return out{
            in.substr(startPos + 1, endPos - (startPos + 1)),
            startPos == std::string_view::npos,
            endPos == std::string_view::npos,
        };
    };
    
    auto solveHeaderDependencies() -> void 
    {
        for (auto& Header: ProjectFile.headerContainer() | std::views::values | std::views::join ) {
            err(Header.Path.empty() ,"Error: Empty project path"_fmt.color(fmt::Bold_Red)); 
            const std::string HeaderPath = (fs::path{Header.Path} / Header.Name).generic_string(); 
            std::ifstream files(HeaderPath);
            err(!files.is_open(),fmt("Error: Unable to open file "_fmt.color(fmt::Bold_Red)," File: ",HeaderPath));
            std::string line;
            bool inBlockComment = false; 
            while (std::getline(files, line)) {
                if(blockedComment(&inBlockComment, &line)) {continue;}
                const size_t ipos = line.find(fileUtil::includeToken);
                
                if(singleLineComment(line, &ipos)) {continue;}
                if (ipos != std::string::npos) { 
                    const size_t searchStart = ipos + fileUtil::includeToken.length();
                    const auto [headerName,start,end] = getRawHeaderName(std::string_view{line}.substr(searchStart));
                    if (start || end) { continue; }
                    
                    for (const auto& [K,V] : ProjectFile.headerContainer()) {
                        if (K.empty()) continue;
                        const bool foundMatch = std::ranges::find(V,headerName,&HeaderFile::Name) != V.end();
                        // std::cout << fmt("is header path "_fmt.color(fmt::Bold_Blue) , headerName , " in " , Header.Name ).endl();
                        if (foundMatch) {
                            std::string flags {fmt("-I",K)};
                            const bool alreadyAdded = containsToken(Header.flags, flags);
                            const bool notSamePath = (Header.Path != K);
                            if (!alreadyAdded && notSamePath) {
                                std::cout << fmt("add dependencies "_fmt.color(fmt::Bold_Blue) , K , " to " , Header.Name ).endl();
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
        solveHeaderDependencies();
        return *this;
    } 
    

    auto getCppFile() -> void 
    {
        for (const auto& p : SourcePath)
        {
            err ((!fs::exists(p) || !fs::is_directory(p)), fmt("Directory does not exist. "_fmt.color(fmt::Bold_Red),p));
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

    auto scanHeader() -> Project& 
    {
        for (auto& [K, V] : ProjectFile) {
            if (V.Path.empty()) { err(true, "Error: Empty project path"_fmt.color(fmt::Bold_Red)); }
            
            std::ifstream files(V.Path.data());
            if (!files.is_open()) { err(true, fmt("Error: Unable to open file "_fmt.color(fmt::Bold_Red), V.Path)); }

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
                        auto findInclude = std::ranges::find_if(HF,[&](auto& s){
                            return s.Name == headerName;
                        });
                        auto splitFlags = findInclude->flags 
                        | std::views::split(' ') 
                        | std::views::filter([](const auto& sv) -> bool {
                            return !sv.empty(); 
                        })
                        | std::views::transform([&](const auto& s){
                            std::string_view temp(s.data(),s.size());
                            struct ret {
                                const std::string_view sv {}; 
                                const bool hasToken;
                            };
                            return ret{temp,containsToken(V.Flags,temp)};
                        });
                        if (findInclude != HF.end()) {
                            std::string i {fmt("-I{}",HP)};
                            if (!containsToken(V.Flags, i)) {
                                V.Flags.append(" ") += i; 
                                if(!findInclude->flags.empty()) 
                                for (const auto [sv,hasTokenModule] : splitFlags) {
                                    if (!hasTokenModule) { V.Flags.append(" ") += sv;}
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
            std::cout << fmt("Scan module " , V.Path ).endl();

            err(V.Path.empty() ,"Error: Empty project path"_fmt.color(fmt::Bold_Red)); 
            
            const std::string temp = (fs::path(V.Path) / V.Name).generic_string();
            std::ifstream files;
            if (V.fileType == File::HeaderUnit) {
                files.open(temp);
            } else {
                files.open(V.Path);
            }
            err(!files.is_open(),fmt("Error:"_fmt.color(fmt::Bold_Red)," Unable to open file "," File: ",temp));

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
        
                    const size_t startPos = line.find_first_not_of(" \t", searchStart);
                    if (startPos == std::string::npos || startPos >= epos) {continue;}
                    
                    // Find the end of the module name string before trailing whitespaces or semicolon
                    const size_t endPos = line.find_last_not_of(" \t", epos - 1);
                    if (endPos == std::string::npos || endPos < startPos) {continue;}

                    moduleName = std::string_view{line}.substr(startPos, (endPos - startPos) + 1);
                    std::cout << fmt("import module "_fmt.color(fmt::Bold_Blue) , moduleName , " found in " , V.Path).endl();
                    
                    // -------------------------------------------------------------------
                    // 3. Detect System Module Unit
                    // -------------------------------------------------------------------
                    if (moduleName.front() == '<' || moduleName.front() == '"') {
                        std::cout << fmt("Header Unit module "_fmt.color(fmt::Bold_Blue) , " From: " , V.Name , " Name: " , moduleName).endl();
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
                            auto it = ProjectFile.headerContainer() | std::views::values | std::views::join;
                            auto findHeader = std::ranges::find( it,moduleName,&HeaderFile::Name);
                            if (findHeader != it.end()) {
                                auto& h = findHeader;
                                std::cout << fmt("add Header into Unit module "_fmt.color(fmt::Bold_Green) , " From: " , h->Path , " to: " , moduleName , " and " , V.Name).endl();
                                std::string i {fmt("-I",h->Path)};
                                if (!containsToken(headerUnitFlags,i)) headerUnitFlags.append(" ") += i;
                                
                                if (!containsToken(ModuleFile.Flags,i)) ModuleFile.Flags.append(" ") += i;
                                if (!h->flags.empty()) {
                                    auto splitFlags = h->flags 
                                    | std::views::split(' ') 
                                    | std::views::filter([](const auto& sv) {
                                        return !sv.empty(); 
                                    })
                                    | std::views::transform([&](const auto& sv){
                                        std::string_view temp(sv.data(),sv.size());
                                        struct ret {
                                            const std::string_view sv {}; 
                                            const bool modulehasToken;
                                            const bool headerUnithasToken;
                                        };
                                        return ret{temp,containsToken(ModuleFile.Flags,temp),containsToken(headerUnitFlags,temp)};
                                    });
                                    for (const auto [sv,hasTokenModule,hasTokenHeaderUnit] : splitFlags){
                                        if (!hasTokenModule) {
                                            ModuleFile.Flags.append(" ") += sv;
                                        }
                                        if (!hasTokenHeaderUnit) {
                                            headerUnitFlags.append(" ") += sv;
                                        }
                                    } 
                                }
                            }
                        } 
                        headerUnitFile.objectPath = isHeaderUnit ? 
                        fmt((OutPath->modulePath / rawHeader).string(),fileUtil::pcmModule).str : 
                        headerUnitFile.getModuleOutput(&OutPath->stdPath);
                        headerUnitFile.compiled = fs::exists(headerUnitFile.objectPath);
                        ModuleFile.haveHeaderUnit = true;
                    }
                }
                if (!moduleName.empty()) {
                    auto filter = ProjectFile 
                    | std::views::filter([&](const auto& Pair) -> bool {
                        const auto& [Module, MV] = Pair;

                        const bool sameName = ((MV.fileType != File::Module) ? Module 
                        : MV.isPartition ? MV.Name.substr(MV.Name.find(':')) 
                        : MV.Name) == moduleName;
                        
                        const bool notAdded = std::ranges::find(ModuleFile.dependencies, MV.FileName) == ModuleFile.dependencies.end();
                        
                        return notAdded && sameName;
                    })
                    | std::views::transform([](const auto& Pair) -> std::string_view {
                        const auto& MV = Pair.second; return std::string_view{MV.FileName.data(),MV.FileName.size()};
                    });
             
                    for (auto FileName : filter) {
                        ModuleFile.dependencies.emplace_back(FileName);
                    }
                }
            }
            files.close();
        }
        return *this;
    }

    auto compilePCH(std::string_view PCHfile) -> bool {
        const std::string headerFile = (fs::path(ProjectFile.getHeaderPath(PCHfile)) / PCHfile).string();
        const auto& oPath = OutPath->buildPath;
        const std::string pchOut {fmt("{}.pch",(oPath / fs::path(PCHfile).stem()).string())};
        const std::string f_cmd  {fmt("{} {} -x c++-header {} -o {}",Compiler, Options,headerFile,pchOut)};
        Options += fmt(" -include-pch {} ",pchOut);
        if (fs::exists(pchOut)) {
            if (fs::last_write_time(headerFile) > fs::last_write_time(pchOut)) {
                fs::rename(pchOut,fmt("{}.old",pchOut).str);
            } else {
                return 1;
            }
        }
        std::cout << fmt("Compiling PCH "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
        return cmd << f_cmd >> "Error compiling "_fmt.color(fmt::Bold_Red);
    };

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
                temp += (
                    depisSystemHeader ? fmt(" -fmodule-file={}",depFile.objectPath) :
                    depisHeaderUnit ? fmt(" -fmodule-file={}{}",(mPath / depFile.getName()).string(), fileUtil::pcmModule) :
                    fmt(" -fmodule-file={}={}{}",depFile.Name,(mPath / depFile.getName()).string(),fileUtil::pcmModule)
                );
            }
            inFile.Flags += temp;
            temp.clear();
        }
        return *this;
    }

    auto compileModule(File& inFile) -> bool 
    {
        const bool isSystemHeader = (inFile.fileType == File::SystemHeader);
        const bool isModuleImpl = (inFile.fileType == File::ModuleImpl);
        const bool isHeaderUnit = (inFile.fileType == File::HeaderUnit);
        if (isModuleImpl || (isSystemHeader && inFile.compiled)) { return true; }
        if(!inFile.dependencies.empty()) {
            for (const auto& I : inFile.dependencies) {
                auto& dep = *ProjectFile[I];
                if(!dep.compiled) {return false;}
                if(!inFile.haveHeaderUnit && (isSystemHeader || isHeaderUnit)) {
                    inFile.haveHeaderUnit = true;
                }
            }
        }
        if(inFile.isPartition) {
            ProjectFile.getByName(inFile.Name.substr(0,inFile.Name.find(':')))->isMainPartition = true;
        }
        
        const auto& mPath = isSystemHeader ? OutPath->stdPath : OutPath->modulePath;
        const auto& oPath = OutPath->objPath;
        
        const std::string fModule    { inFile.getModuleOutput(&mPath) };
        
        const std::string fObjOutput { fmt((oPath / inFile.getName()).string(), fileUtil::objFile) };
        
        const std::string f_srcInput {
                isHeaderUnit ? fmt("-Wno-pragma-system-header-outside-header -fmodule-header=user --precompile {} -o {}",
                (fs::path(inFile.Path)/inFile.Name).string(),fModule) :
                isSystemHeader ? fmt("-Wno-pragma-system-header-outside-header -Wno-gnu-anonymous-struct -Wno-nullability-extension -Wno-gcc-compat -Wno-user-defined-literals -x c++-system-header --precompile {} -o {}",inFile.Name,fModule) :
                fmt("-c {} -fmodules-reduced-bmi -fmodule-output={} -fprebuilt-module-path={} ",inFile.Path,fModule,(mPath).string())
            };
        if (inFile.objectPath.empty()) {inFile.objectPath = fObjOutput;}
        
        const std::string f_cmd {
            isSystemHeader ? fmt("{} {} {}",Compiler, Options,f_srcInput) : 
            isHeaderUnit ? fmt("{} {} {} {}",Compiler, Options,inFile.Flags,f_srcInput) :
            fmt("{} {} {} {} {} -o {}",Compiler, Options,(inFile.haveHeaderUnit ? "-Wno-experimental-header-units ": "") ,
            f_srcInput ,inFile.Flags,fObjOutput)
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
            std::cout << fmt("{} {}","compiling module "_fmt.color(fmt::Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd >> "recompile error"_fmt.color(fmt::Red);
        } else {
            #ifdef __WIN32
            if(fs::exists(fModule)) {
                const std::string old {fmt("{}.old",fModule)}; 
                if (fs::exists(old)){fs::remove(old);}
                fs::rename(fModule,old);
            }
            #endif
            // fs::copy(old,fModule);
            std::cout << fmt("{} {}","compiling module "_fmt.color(fmt::Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd >> "recompile error"_fmt.color(fmt::Red);
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
            fmt("{}{}",(oPath / inFile.getName()).string(), fileUtil::objFile)
        )};

        const std::string filein    { isModule ? 
            fmt("{}{}",(mPath / inFile.getName()).string(),fileUtil::pcmModule ) : 
            inFile.Path
        };

        const std::string cppOutput { 
            noDependencies ? 
            fmt( "{}{}",(isModule ? "":"-c "),filein) :
            fmt( "{}{} -fprebuilt-module-path={}",(isModule ? "":"-c "),filein,(mPath).string())
        };
           
        const std::string f_cmd {fmt("{} {} {} {} {} {} {}",
            Compiler, Options,
            inFile.haveHeaderUnit ? "-Wno-experimental-header-units": "" ,
            cppOutput,
            inFile.Flags,
            isModule? "-c -o":"-o", 
            objOutput).clean()
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
            std::cout << fmt("{} {}","recompiling"_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd >> "Error compiling "_fmt.color(fmt::Bold_Red);
            
        } else if (!fs::exists(objOutput))
        {
            std::cout << fmt("{} {}","compiling "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd >> "Error compiling "_fmt.color(fmt::Bold_Red);
            
        } else if (fs::last_write_time(inFile.Path) > fs::last_write_time(objOutput))
        {
            std::cout << fmt("{} {}","updated "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd >> "Error compiling "_fmt.color(fmt::Bold_Red);
        }
        inFile.compiled = (ret == 0 ? true : false);
        return ret; 
    }
    
    
    auto link(File& inPath) -> void {
        const bool isStaticLib {outFile == Project::staticLib};
        const bool isExecutable {outFile == Project::exe};
        const bool hasDependencies {!inPath.dependencies.empty()};
        const std::string f_targetOut {
            fmt("{}{}", (isStaticLib ? OutPath->buildPath.string() : (OutPath->exePath / inPath.Name).string()), (isStaticLib ? fileUtil::libFile : fileUtil::executable))
        };
        const std::string f_Output    { isStaticLib ?
            f_targetOut :
            fmt("-o {}", f_targetOut) 
        };
        
        const std::string makeFlags = [&]{ 
            std::string temp;
            for (const auto& [K,I] : ProjectFile) {
                if(I.onArchive == true) {continue;}
                if(isStaticLib) {I.onArchive = true;}
                if(I.fileType == File::SystemHeader) {continue;}
                if(!I.ldFlags.empty() && !isStaticLib) {temp +=I.ldFlags;}
                if(*I.ldFlags.end() != ' ') {temp += " ";}
                temp += I.objectPath;
            }
            return (hasDependencies && !isStaticLib ?
            fmt( "{} -fprebuilt-module-path={}",temp,(OutPath->modulePath / ".").string()) :
            temp);
        }();

        if(isStaticLib) {
            outputName = f_targetOut;
        } else if (isExecutable) {
            if (fs::exists(f_targetOut)) {
                fs::rename(f_targetOut, fmt("{}{}",f_targetOut, ".old").sv());
            }
        }
        const std::string typeCmd {isStaticLib ? 
            fmt("{} {}",fileUtil::libTool,"rcs") : 
            fmt("{} {} {}",Compiler,Options,LdOptions)
        };
        const std::string f_cmd { isStaticLib ?
            fmt("{} {} {}",typeCmd,f_Output,makeFlags) :
            fmt("{} {} {}",typeCmd,makeFlags,f_Output) 
        };

        if (!ResPath.empty() && fs::exists(getMainPath() / ResPath)) {
            if (!fs::exists(OutPath->exePath/ResPath)) {
                std::cout << fmt("Copying from: {} to: {}", (getMainPath() / ResPath).string(),(OutPath->exePath/ResPath).string()) << "\n"; 
                fs::copy(getMainPath() / ResPath,OutPath->exePath/ResPath,fs::copy_options::recursive | fs::copy_options::skip_existing);
            }
        }
        std::cout << "Linking "_fmt.color(fmt::Bold_Green) << f_cmd << "\n";
        cmd << f_cmd >> "linking error"_fmt.color(fmt::Bold_Red);
    }

    auto dumpProject() -> Project& {
        
        std::cout << "Dump project"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V]: ProjectFile) {
            const bool isSource = V.fileType == File::Source;
            const bool isModule = V.fileType == File::Module;
            const bool isSystemHeader = V.fileType == File::SystemHeader;
            const bool isModuleImpl = V.fileType == File::ModuleImpl;
            std::cout << fmt("File: {}\n Name: {}\n Path: {}\n Type: {}\n OutPath: {}\n",
            K , 
            V.Name , 
            V.Path ,
            (isSource ? "Source" : isModule ? "Module" : isSystemHeader ? "SystemHeader" : isModuleImpl ? "ModuleImpl" : "ETC") ,
            V.objectPath);
        }

        std::cout << "\nDump Include"_fmt.color(fmt::Yellow).endl();
        // ProjectFile.testHeader();
        for (const auto& [K,V]: ProjectFile.headerContainer()) {
            std::cout << "Include Dir: " << K;
            for (const auto& Header : V) {
                const bool flagsEmpty = Header.flags.empty();
                std::cout << fmt(
                    flagsEmpty ? "\nFile: {} Path: {}" :
                    "\nFile: {} Path: {} {}: {}"
                    ,Header.Name 
                    ,Header.Path 
                    ,(flagsEmpty ? "" : "\ndependency ")
                    ,(flagsEmpty ? "" : Header.flags));
            }
            std::cout << "\n";
        }

        std::cout << "\nDump Module"_fmt.color(fmt::Yellow).endl(); 
        for (const auto&  [K,V]: ProjectFile) {
            if (V.fileType != File::Module) {continue;}
            std::cout << fmt("Module: {} Name: {}\n",K,V.Name);
        }
        

        std::cout << "\nDump Dependencies"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.dependencies.empty()){continue;}

            std::cout << "File: " << K << " Depends on: ";
            for (const auto& I : V.dependencies) {
                std::cout << " File: " << I << " ";
            }
            // std::cout << "\nFile: " << K << " Header dependencies: ";
            // for (const auto& I : V.headerDeps) {
            //     std::cout << " Header: " << I << " ";
            // }
            std::cout << "\n";
        }
        std::cout << "\nDump File Flags"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.Flags.empty() && V.ldFlags.empty()){continue;}

            std::cout << "File: " << K << "\n";
            if (!V.Flags.empty()) std::cout << "cFlags: " << V.Flags << "\n";
            if (!V.ldFlags.empty()) std::cout << "LdFlags: " << V.ldFlags << "\n";
        }
        std::cout << "\n";
        
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