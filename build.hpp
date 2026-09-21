#ifndef BUILD_PP 
#define BUILD_PP

#include <cstddef>
#include <cstdlib>
// #include <cstdio>
#include <filesystem>
// #include <initializer_list>
#include <vector>
#include <string_view>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <utility>
#include <algorithm>
#include <ranges>
#include <source_location>
#include <functional>
#include <mutex>

#include "json.hpp"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace  fs = std::filesystem;
#undef size_t
using size_t = __SIZE_TYPE__;
using namespace std::string_view_literals;

template<typename T> struct remove_ptr {using type = T;};
template<typename T> using remove_ptr_t = remove_ptr<T>::type;

template<typename T>
concept funcPtr = std::is_pointer_v<T> && std::is_function_v<remove_ptr_t<T>>;


template<typename T> requires (std::integral<std::decay_t<T>> || std::floating_point<std::decay_t<T>>)
consteval std::size_t maxDigits() {
    using Unqualified = std::remove_cvref_t<T>;
    if (std::integral<Unqualified>) {
        constexpr bool isSigned = std::is_signed_v<Unqualified>;
        return std::numeric_limits<Unqualified>::digits10 + 1 + (isSigned ? 1 : 0);
    } else {
        return 24;
    }
}

struct Math {
    static constexpr double PI = 3.1415926535;

    static constexpr std::array<double, maxDigits<double>() - 1> Pow10LUT = []() consteval {
        std::array<double, maxDigits<double>() - 1> arr{};
        arr[0] = 1.0;
        for (size_t i = 1; i < arr.size(); ++i) {
            arr[i] = arr[i - 1] * 10.0;
        }
        return arr;
    }();

    
    template <typename T> requires (std::integral<std::decay_t<T>> || std::floating_point<std::decay_t<T>>)
    static constexpr size_t digitLength (const T& in) {
        bool isNeg = in < 0;
        // Find digit count (or highest power index) safely
        for (size_t i = 0; i < Pow10LUT.size(); ++i) {
            if ((isNeg ? -in :in) < Pow10LUT[i]) {
                return i + (isNeg ? 1 : 0);
            }
        }
        return Pow10LUT.size();
    }
};
// static constexpr pow10 test;
// static constexpr int test2 = 900;
// static_assert(test[test2] == 3,"" );

template <typename T>
concept Formattable = 
    requires(T t) { { std::string_view(t) } -> std::same_as<std::string_view>; } || 
    std::integral<std::decay_t<T>> ||
    std::floating_point<std::decay_t<T>>;

template <typename... Args>
concept onlyStr = (Formattable<Args> && ...);

struct fmt {
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

    template<onlyStr... Args>
    constexpr fmt& varConcat(Args&&... args) { 
        size_t preAlloc = (getArgSize(args) + ... + 0);
        str.reserve(preAlloc);
        (appendArg(std::forward<Args>(args)), ...);
        return *this;
    }
    // 2. Format String Constructor: fmt("Value: {}, Status: {}", 42, "OK")
    template<onlyStr... Args>
    fmt(std::string_view fmtStr, Args&&... args) {
        // Check if fmtStr actually contains placeholders
        if constexpr (sizeof...(Args) > 0) {
            if (fmtStr.find('{') != std::string_view::npos) {
                formatInit(fmtStr, std::forward<Args>(args)...);
                return;
            }
        }
        
        // Default to concatenation if no placeholders found
        varConcat(fmtStr, std::forward<Args>(args)...);
    }

    // Apply ANSI Color
    constexpr fmt& color(colors c) { 
        str.reserve(str.size() + 14);
        str.insert(0, this->getColor(c));
        str.append(this->getColor(Not_color));
        return *this;
    }

    // Collapse multiple contiguous spaces into a single space
    constexpr fmt& clean() { 
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

    constexpr fmt& endl() { str.append("\n"); return *this; }

    constexpr std::string_view sv() const { return str; }
    constexpr const char* cstr() const { return str.c_str(); }
    constexpr explicit operator const char*() const & noexcept { return cstr(); }
    constexpr operator std::string_view() const noexcept { return sv(); }
    constexpr operator std::string() const & { return str; }
    constexpr explicit operator std::string() && { return std::move(str); }
    
    friend std::ostream& operator<<(std::ostream& os, const fmt& f) {
        return os << f.str;
    }

private:
    template<onlyStr... Args>
    void formatInit(std::string_view fmtStr, Args&&... args) {
        size_t preAlloc = fmtStr.size() + (getArgSize(args) + ... + 0);
        str.reserve(preAlloc);

        size_t lastPos = 0;
        auto processArg = [this, &fmtStr, &lastPos](auto&& arg) {
            size_t pBegin = fmtStr.find('{', lastPos);
            if (pBegin != std::string_view::npos) {
                size_t pEnd = fmtStr.find('}', pBegin + 1);
                if (pEnd != std::string_view::npos) {
                    str.append(fmtStr.substr(lastPos, pBegin - lastPos));
                    appendArg(std::forward<decltype(arg)>(arg));
                    lastPos = pEnd + 1;
                }
            }
        };

        (processArg(std::forward<Args>(args)), ...);
        if (lastPos < fmtStr.size()) {
            str.append(fmtStr.substr(lastPos));
        }
    }

    template<onlyStr T>
    constexpr size_t getArgSize(const T& arg) const {
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
    constexpr void appendArg(T&& arg) {
        using Raw = std::remove_cvref_t<T>;
        if constexpr (std::is_convertible_v<Raw, std::string_view>) {
            str += std::string_view(arg);
        } else if constexpr (std::integral<Raw>) {
            char digit[maxDigits<Raw>()];
            auto [eDigit, ec] = std::to_chars(digit, digit + maxDigits<Raw>(), std::forward<T>(arg));
            str.append(digit, static_cast<std::size_t>(eDigit - digit));
        } else if constexpr (std::floating_point<Raw>) {
            char digit[maxDigits<Raw>()];
            auto [eDigit, ec] = std::to_chars(digit, digit + maxDigits<Raw>(), std::forward<T>(arg));
            str.append(digit, static_cast<std::size_t>(eDigit - digit));
        } else {
            __builtin_unreachable();
        }
    }

    

    constexpr std::string_view getColor(colors color) const {
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
constexpr fmt operator""_fmt(const char* str,size_t) { return fmt(str);}

// struct implstd::cout
// {
//     constexpr implstd::cout& operator <<(std::string in) {
//         std::cout << in ;
//         return *this;
//     }
//     constexpr implstd::cout& operator <<(std::string_view in) {
//         std::cout << in ;
//         return *this;
//     }
//     void operator <<(implstd::cout& f) {f = *this;}
//     ~implstd::cout() {std::cout << std::flush;}
// };
// inline implstd::cout std::cout;

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
    
    cmdImpl& run(std::string inCmd) {
        {
            pipe cmd (popen(inCmd.c_str(),"r"),PipeDeleter{&ret});
            if (!cmd) {
                ret = -1; // popen failed to open
                return *this;
            }
        }
        return *this;
    }
    int err(const std::string& msg) {
        if (ret != 0) {
            std::cout << msg << "\n";
            std::exit(1);
        }
        return ret;
    }
    cmdImpl& operator <<(std::string_view cmd) { return run({cmd.data(),cmd.size()});}
    int operator >>(std::string_view cmd) { return err({cmd.data(),cmd.size()});}
};
inline cmdImpl cmd;

struct outputPath {
    private:
    constexpr outputPath& err(bool e = false,std::string_view msg = "",std::string_view fn = std::source_location().current().function_name()) {
        if (e) {std::cout << fmt("error: "_fmt.color(fmt::Red) ,msg , " at ", fn ,"\n"); std::exit(1);} 
        return *this;
    }
    public:
    fs::path rootPath   {};
    fs::path outPath    {};
    fs::path objPath    {};
    fs::path stdPath    {};
    fs::path modulePath {};
    fs::path exePath    {};

    outputPath& setRootPath(fs::path root) { rootPath = root; return *this;}
    outputPath& setExePath(fs::path exe) {
        if(rootPath.empty()) {return err(true,"root path is empty");}
        if (!fs::exists(exe)) 
        {
            if (fs::create_directory(exe)) {
                std::cout << fmt("Directory created: " , exe.string() , "\n");
            } else {
                err(true,fmt("Failed to create directory: ",exe.string()));
            }        
        } else {
            std::cout << fmt("Directory already exists: "_fmt.color(fmt::Bold_Yellow) , exe.string(),"\n");
        } 
        exePath = exe;
        return err();
    }
    outputPath& setOutfolder(fs::path folder) {
    if(rootPath.empty()) {return err(true,"root path is empty");} 
        if (!fs::exists(folder)) 
        {
            if (fs::create_directory(folder)) {
                std::cout << fmt("Directory created: " , folder.string() , "\n");
            } else {
                err(true,fmt("Failed to create directory: "));
            }        
        } else {
            std::cout << fmt("Directory already exists: "_fmt.color(fmt::Bold_Yellow) , folder.string(),"\n");
        }
        return *this;
    }
    void setOutpath(fs::path out) {
        if(rootPath.empty()) { err(true,"root path is empty"sv);}
        outPath = out;
        objPath = outPath / "obj";
        modulePath = outPath / "module";
        stdPath = modulePath / "std";
        for (const auto& lm_dir : {outPath,objPath,modulePath,stdPath})
        {
            if (!lm_dir.has_parent_path()) {
                err(true,fmt("Error: Path has no parent path: " ,lm_dir.string()));
            }
    
            if (!fs::exists(lm_dir)) 
            {
                if (fs::create_directory(lm_dir)) {
                    std::cout << fmt("Directory created: " , lm_dir.string() , "\n");
                } else {
                    err(true,fmt("Failed to create directory: "));
                }        
            } else {
                std::cout << fmt("Directory already exists: "_fmt.color(fmt::Bold_Yellow) , lm_dir.string(),"\n");
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
    static constexpr bool isCpp(std::string_view file)
    {
        for (const auto& i : cppSource)
        {
            if (i == file) {return true; break;}
        }
        return false;
    }
    static constexpr bool isModule(std::string_view file)
    {
        for (const auto& i : cppModule)
        {
            if (file == i) {return true; break;}
        }
        return false;
    }
    static constexpr bool isCppHeader(std::string_view file)
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
    compileCommand& addCompilecmd(std::string_view path,std::string_view arg,std::string_view file,std::string_view out) {
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
    void write(fs::path to)
    {
        std::lock_guard<std::mutex> lock(mtx);
        jsonNode.to_file(to.string());
    }


};


struct File {
    using fStr = std::string;
    using fStrView = std::string_view;
    // using IDx = std::size_t;
    const File& err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) const {
        if (cnd) {
            std::cout << fmt ("At: ",fn.file_name(), " ",fn.function_name(), " col: ",std::to_string(fn.column())," line: ",std::to_string(fn.line()), "\n" , msg ,"\n"); 
            std::exit(1);
        } 
        return *this;
    }

    mutable bool compiled = false;
    mutable bool haveHeaderUnit = false;
    mutable bool onArchive = false;
    mutable bool isPartition = false;
    mutable bool isMainPartition = false;
    enum fTypes : char {
        Source,
        Module, 
        ModuleImpl, 
        SystemHeader, 
        HeaderUnit, 
        none
    } ;
    mutable fTypes fileType = none;

    // IDx ID           {};
    fStr FileName    {};
    fStr Name        {};
    fStr Path        {};
    fStr Flags       {};
    fStr ldFlags     {};
    fStr objectPath  {};
    std::vector<fStr> dependencies {};
    [[nodiscard]] fStr getModuleOutput(const fs::path* mPath) {
        err(Name.empty(), "File Name Empty");
        err((fileType != Module) && (fileType != SystemHeader) && (fileType != HeaderUnit), "File Not a Module");
        return fmt((*mPath / getName()).string(), fileUtil::pcmModule).clean().str;
    }
    void setObjOutputName(const fs::path* oPath) {
        err(Name.empty(),"File Name Empty");
        objectPath = fmt((*oPath / getName()).string(),fileType == ModuleImpl ? "-impl" : "", fileUtil::objFile).clean();
    }
    fStr getName() const {
        err(Name.empty(),"File Name Empty");
        if (isPartition) {
            fStr temp = Name;
            temp.replace(temp.find(':'),1,"-");
            return temp;
        }
        return Name;
    }
};

struct HeaderFile {
    using fStr = std::string;
    using fStrView = std::string_view;

    fStr Name  {};
    fStr Path  {};
    fStr flags {};
};

class FileManager {
    constexpr FileManager& err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) {
        if (cnd) {
            std::cout << fmt ("{} {}:{}:{} at: {}\n",msg,fn.file_name(),fn.line(),fn.column(),fn.function_name()); 
            // std::cout << fmt ("At: ",fn.file_name(), " ",fn.function_name(), " col: ",std::to_string(fn.column())," line: ",std::to_string(fn.line()), "\n" , msg ,"\n"); 
            std::exit(1);
        } 
        return *this;
    }
    template<typename T>
    struct itUtl {
        T* container;
        itUtl(T* other) : container(other) {}
        auto begin() {return container->begin();}
        auto end() {return container->end();}
        auto begin() const {return container->begin();}
        auto end() const {return container->end();}
    };
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
    using MapPair = std::pair<const std::string, File>;

    std::unordered_map<std::string, std::vector<HeaderFile>,StringHash,std::equal_to<>> Header {};
    std::unordered_map<std::string, File,StringHash,std::equal_to<>> Files {};
    // std::vector<File*> IDMap {};
    // std::size_t NextID = 0;
    public:
    File* Main = nullptr;
    MapPair& addFile(std::string_view name) {
        auto it = Files.find(name);
        if (it != Files.end()) {
            return *it;
        }
        auto ref = Files.try_emplace(std::string(name), File{.FileName={name.data(),name.size()}}).first;
        
        return *ref;
    }
    
    auto setHeaderPath(std::string_view path) -> std::vector<HeaderFile>& {
        return Header.try_emplace(path.data(),std::vector<HeaderFile>{}).first->second;
    }

    void addHeader(std::string_view path,std::string_view name) {
        auto it = Header.find(path);
        if (it != Header.end()) {
            it->second.push_back(HeaderFile{.Name = {name.data(),name.size()},.Path={path.data(),path.size()}});
        }
        return;
    }
    void addHeader(HeaderFile H) {
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
    
    std::string_view getHeaderPath(std::string_view name,std::source_location loc = std::source_location::current()) {
        for (auto& [I , IN] : Header) {
            for(const auto& N : IN)
            if (N.Name == name) {
                // std::string temp = (fs::path(I.first) / name).string();
                return I;
            }
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Key ",name," doesn't exists"),loc);
        return "";
    }
    File& copyFile(std::string_view name,const File& other) {
        // NextID = Files.size();
        
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
        // IDMap.emplace_back(&ref->second);
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
    File* getByName(std::string_view id) {
        for (auto& I : Files) {
            if (I.second.Name == id) {
                return &I.second;
            }
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Name " ,id, " doesn't exists"));
        return nullptr;
    }
    File* operator [](std::string_view id,std::source_location fn = std::source_location::current()) {
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
    
    MapPair* getPair (std::string_view id) {
        auto it = Files.find(id);
        if (it != Files.end()) {
            return &*it; 
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Key doesn't exists"));
        return nullptr; 
    }

    File& getFile(std::string_view id) {
        return *(*this)[id];
    }
    void setMain(std::string_view id) {
        // std::cout << "Set Main: " << id << " " << (*this)[id]->Name << "\n";
        Main = (*this)[id];
    }
    File& getMain() {
        return *Main;
    }
    decltype(Files)& getFileContainer() {
        return Files;
    }
    bool empty () const {return Files.empty();}

    auto begin() { return Files.begin(); }
    auto end()   { return Files.end(); }
    auto hIter() {
        return itUtl(&Header);
    }
    auto& headerContainer() {
        return Header;
    }
    // auto VIter() {
    //     return itUtl(&IDMap);
    // }
    auto begin() const { return Files.begin(); }
    auto end()   const { return Files.end(); }
};

class cProject{
    inline static fileUtil file;
    
    std::string Options         {};
    std::string LdOptions       {};
    std::string Compiler        {};
    std::string compileInclude  {};
    
    constexpr cProject& err(bool isError = false,std::string_view msg = "",std::string_view f = std::source_location().current().function_name()) {
        if (isError) { std::cout << "ERROR "_fmt.color(fmt::Bold_Red) << fmt(msg , " at " , f); std::exit(1); } 
        return *this;
    }
    public:
    bool recompile,singleFile;
    enum projectType : int {
        exe,
        staticLib,
        dynamicLib
    } outFile;
    outputPath* projectOutPath;
    compileCommand* cmdJson;
    
    fs::path Path       {};
    std::string ProjectName     {};
    std::string outputName      {};
    std::vector<std::string> SourcePath {};
    FileManager ProjectFile;

    cProject(outputPath* path, projectType exe,bool recomp = false) {
        projectOutPath = path;
        outFile = exe;
        cmdJson = nullptr;
        recompile = recomp;
    };
    
    cProject& setMain(std::string_view main) {
        ProjectFile.setMain(main); 
        return *this;
    }
    cProject& setCompiler(std::string_view comp)     {Compiler = fmt(comp, " ").str;return *this;}
    cProject& setOptions (std::string_view opt)      {Options = fmt(opt, " ").str;return *this;}
    cProject& setProjectPath(fs::path in)            {
        Path = in;
        // std::cout << fmt("Project initialized at "_fmt.color(fmt::Bold_Green) , in.string(),"\n");
        ;return *this;
    }
    cProject& setSourcePath(fs::path in)             {SourcePath.emplace_back(fs::path(in).lexically_proximate(Path).string()); return *this;}
    cProject& setSource(std::initializer_list<std::string_view> in) {
        for (const auto& i : in) {
            // project.push_back((sourcePath / i).string());
            auto& ptr = ProjectFile.addFile(i);
            ptr.second.Name = std::string_view(ptr.first.data(),i.find_last_of("."));
            ptr.second.fileType = File::Source;
        }
        return *this;
    }
    std::string& getMainSource() {
        return SourcePath[0];
    }
    cProject& addCompileCommand(compileCommand* cmd)
    {
        cmdJson = cmd;
        return *this;
    }

    cProject& addDependency(std::string_view inFile, std::initializer_list<std::string_view> inDeps){
        // std::cout << fmt("add dependency for " , inFile , " with deps: ");
        // for (const auto& d : inDeps) {
        //     std::cout << d << " ";
        // }        std::cout << "\n";

        std::string f_file;
        std::string f_deps; 

        size_t f_totalSize = 0;
        for (const auto& d : inDeps) {
            f_totalSize += d.size() + 6; // +1 for space
        }
        
        f_deps.reserve(f_totalSize);
        for (const auto& d : inDeps) {
            f_deps += fmt(" -l" , d , " ");
        }

        for (const auto& [k,mod] : ProjectFile) {
            // std::string filename = fs::path(k).filename().string();
            //std::cout << fmt("check file " , filename , " against " , inFile , "\n");
            if (k == inFile) {
                // std::cout << fmt("dependency found for " , inFile , " in " , k , " imp filename " , filename , "\n");
                f_file = k;
                break;
            }
        }
        ProjectFile[f_file]->ldFlags = f_deps;
        return *this;
    }

    cProject& addIncludefile(std::string_view inPath) {
        err(!fs::exists(inPath),fmt("Include path: ",inPath, " does not exist ").color(fmt::Bold_Red));
        
        ProjectFile.setHeaderPath(inPath);
        return *this;
    }

    cProject& getCFile() {
        for (const auto& F : SourcePath) {
            err((!fs::exists(Path / F) || !fs::is_directory(Path / F)), fmt("Directory: ",F," does not exist.\n"));
            fs::directory_iterator iterator(Path/F);
            for (const auto& entry : iterator) {
                if (entry.is_regular_file() && (entry.path().extension() == file.cSource)) {
                    // std::cout << "add project file " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    // project.emplace_back(entry.path().string());
                    auto& ptr = ProjectFile.addFile(entry.path().filename().string());
                    ptr.second.Name = std::string_view(ptr.first.data(),entry.path().filename().string().length());
                    ptr.second.Path = entry.path().string();
                    ptr.second.fileType = File::Source;
                    ptr.second.onArchive = outFile == cProject::staticLib ? true : false;
                }
            }
        }
            
        for (const auto& [K,V] : ProjectFile.hIter()) {
            fs::recursive_directory_iterator it(K);
            // std::cout << "Scan File: "_fmt.color(fmt::Bold_Green) << K;
            for (const auto& entry : it) {
                if (entry.is_regular_file() && file.isCppHeader(entry.path().extension().string()) ) {
                    // std::cout << fmt("Add include " , entry.path().filename().string() , " From: " , K).endl();
                    
                    ProjectFile.addHeader(K, entry.path().string());
                }
            }
        }
        return *this;
    }

    cProject& scanInclude() {
        
        for (const auto& [K,V] : ProjectFile) {
            auto& modFile = *ProjectFile[V.FileName];
            err(V.Path.empty(),"Error: Empty project path"_fmt.color(fmt::Bold_Red));
            
            std::ifstream files(V.Path);
            err(V.Path.empty(),fmt("Error: Unable to open file " , K , "\n").color(fmt::Bold_Red));
            
            std::string line;
            std::string includeFound;

            while (std::getline(files, line)) {
                size_t pos = line.find(file.includeToken);
                if (pos != std::string::npos) {
                    includeFound = line.substr(pos + file.includeToken.length());
                    std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });
                    // std::cout << fmt("Header: "_fmt.color(fmt::Bold_Purple),includeFound).endl();

                    for(const auto& [KI,VI] : ProjectFile.hIter()) {
                        for(const auto& I : VI) {
                            if (includeFound == I.Name) {
                                // std::cout << fmt("Include dependency Found: "_fmt.color(fmt::Blue), includeFound, " " , I , " in " , K).endl();
                                modFile.Flags += fmt(" -I",KI);
                                break;
                            }
                        }
                    }
                }
            }
            files.close();
        }
        return *this;
    }

    cProject& compileC(File& target)
    {   
        const std::string f_obj {fmt((projectOutPath->objPath / target.Name).string(), file.objFile)};

        const std::string f_cmd {fmt(Compiler,Options, target.Flags ,fmt(" -c ", target.Path, " -o ", f_obj)).clean()};

        if(cmdJson != nullptr) cmdJson->addCompilecmd(fs::path(target.Path).parent_path().string(), f_cmd, target.Path, f_obj);
        
        target.objectPath = f_obj;
        int ret {};
        if (recompile)
        {
            std::cout << fmt("recompile "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
        }else if (!fs::exists(f_obj))
        {
            std::cout << fmt("compile "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "compile error "_fmt.color(fmt::Red);
        } else if (fs::last_write_time(target.Path) >= fs::last_write_time(f_obj))
        {
            std::cout << fmt("updating "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
        }
        target.compiled = (ret == 0 ? true : false);
        return *this;
    }

    cProject& link(File& target) {
        std::cout << "Linking"_fmt.color(fmt::Bold_Green) << "\n";
        // const std::string f_exe      {fmt(" -o ", fmt((projectOutPath->exePath / target.Name).string(), file.platform), " ")};
        const std::string f_Outfile  { fmt((projectOutPath->objPath / target.Name).string(), (outFile == cProject::staticLib ? file.libFile : file.objFile))};
        std::string f_object   {};

        for(const auto& [O,F] : ProjectFile) {
            if (!F.ldFlags.empty() && outFile != cProject::staticLib) {
                f_object += fmt(" ",F.ldFlags);
            }
            f_object += fmt(" ",F.objectPath);
        }
        outputName = f_Outfile;
        const std::string f_cmd {fmt((outFile == cProject::staticLib ? fmt(file.libTool," rcs ",f_Outfile,f_object) : fmt(Compiler,Options,LdOptions,target.ldFlags,f_object,f_Outfile)))};
        std::cout << f_cmd << "\n";
        cmd << f_cmd.c_str() >> "Linking error:"_fmt.color(fmt::Bold_Red); 
        return *this;      
    }

    cProject& dumpProject() {
        
        std::cout << "Dump project"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V]: ProjectFile) {
            std::cout << " Name: "<< V.Name << " File: " << K << " Type: "
            << (V.fileType == File::Source ? "Source" : V.fileType == File::Module ? "Module" : V.fileType == File::SystemHeader ? "SystemHeader" : "ETC") << "\n";
        }

        std::cout << "\nDump Include"_fmt.color(fmt::Yellow).endl();
        // ProjectFile.testHeader();
        for (const auto& [K,V]: ProjectFile.hIter()) {
            std::cout << "Include Dir: " << K << " Header File: ";
            for (const auto& N : V) {
                std::cout << N.Name << " ";
            }
            std::cout << "\n";
        }

        std::cout << "\nDump Module"_fmt.color(fmt::Yellow).endl();
        for (const auto&  [K,V]: ProjectFile) {
            if (V.fileType != File::Module) {continue;}
            std::cout << "Module: " << K << " Name: " << V.Name << "\n";
        }

        std::cout << "\nDump Dependencies"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.dependencies.empty()){continue;}

            std::cout << "File: " << K << " Depends on: ";
            for (const auto& I : V.dependencies) {
                std::cout << " ID: " << I << " " << ProjectFile[I]->Name << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\nDump File Flags"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.Flags.empty()){continue;}

            std::cout << "File: " << K << " With Flags: " << V.Flags << "\n";
        }
        std::cout << "\n";
        return *this;
    }

};


class Project
{
    std::string ProjectName     {};
    std::string Options         {};
    std::string LdOptions       {};
    std::string Compiler        {};
    std::string outputName      {};
    
    constexpr Project& err(bool cnd = false,std::string_view msg = "",std::source_location fn = std::source_location::current()) {
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
    std::vector<std::string> SourcePath  {};
    FileManager ProjectFile        {};

    constexpr Project& setMain        (std::string_view main) {ProjectFile.setMain(main); return *this;}
    constexpr Project& setCompiler    (std::string_view comp) {Compiler = comp; return *this;}
    constexpr Project& setOptions     (std::string_view opt)  {Options = fmt(" ",opt," ").clean().str; return *this;}
    constexpr Project& setLdOptions   (std::string_view opt)  {LdOptions = fmt(" ",opt," ").clean().str; return *this;}
    constexpr Project& setProjectPath (std::string_view in)   {Path = in; return *this;}
    constexpr Project& setResourcePath(std::string_view in)   {ResPath = in; return *this;}
    constexpr Project& addSourcePath  (std::string_view in)   {
        err(!fs::exists(in),fmt("Source path: ",in, " does not exist ").color(fmt::Bold_Red));
        SourcePath.emplace_back(in); return *this;
    }
    constexpr Project& addIncludePath (std::string_view in)   {
        err(!fs::exists(in),fmt("Include path: ",in, " does not exist ").color(fmt::Bold_Red));
        ProjectFile.setHeaderPath(in); return *this;
    }
    
    constexpr std::string& getMainPath () {return SourcePath[0];}
    
    constexpr Project& addSource(std::initializer_list<fs::path> in) {
        for (const auto& i : in) {
            
            err (!fs::exists(i),fmt("source file "_fmt.color(fmt::Red) , i.string() , " does not exist" ));
            auto & P = ProjectFile.addFile(i.filename().string());
            
            P.second.Name = std::string_view(P.first.data(),i.stem().string().length());
            P.second.Path = i.string();
            P.second.fileType = File::Source;
        }
        return *this;
    }

    Project(const char* name,outputPath* path,projectType exe,bool recomp = false) {
        ProjectName = name,
        OutPath = path,
        outFile = exe,
        cmdJson = nullptr,
        recompile = recomp;
        std::cout << fmt("Project initialized at "_fmt.color(fmt::Green) , this->Path.string(),"\n" );
    };

    Project& addCompileCommand(compileCommand* cmd)
    {
        cmdJson = cmd;
        return *this;
    }


    Project& getLib(cProject* cProj)
    {
        if (cProj->outFile == cProject::staticLib) {
             for (const auto& [K,V] : cProj->ProjectFile.hIter()) {
                for (const auto& I : V) {
                    ProjectFile.addHeader(I);
                }
            }
            if(!cProj->outputName.empty()) {
                LdOptions += fmt(" -L",cProj->projectOutPath->objPath.string()," -l",cProj->ProjectFile.getMain().Name);
            }
        }
        return *this;
    }

    Project& getLib(Project* other)
    {
        if (other->outFile == Project::staticLib) {
            // for (const auto& [K,V] : other->ProjectFile) {
            //     ProjectFile.copyFile(K,V);
            // }
            for (const auto& [K,V] : other->ProjectFile.hIter()) {
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

    Project& addLinkLibrary(std::string_view inFile, std::string_view inDeps){
        auto Deps = inDeps | std::views::split(','); 
        auto rangeFile = ProjectFile | std::views::keys ;
        auto finds = std::ranges::find(rangeFile,inFile);
        
        if (finds != rangeFile.end()) {
            std::string_view f_file = *finds;
            auto& File = *ProjectFile[f_file]; 
            File.ldFlags.reserve(inDeps.size());
            for (auto&& d : Deps) {
                std::string_view dep {d.data(),d.size()};
                if(dep.empty()) continue;
                while (!dep.empty() && dep.front() == ' ') dep.remove_prefix(1);
                while (!dep.empty() && dep.back() == ' ') dep.remove_suffix(1);
                File.ldFlags += " -l";
                File.ldFlags +=  dep;
            }
        }
        return *this;
    }

    Project& addCompileFlags(std::string_view inFile, std::string_view inDeps){
        auto Deps = inDeps | std::views::split(','); 
        auto rangeFile = ProjectFile | std::views::keys;
        auto finds = std::ranges::find(rangeFile,inFile);

        if (finds != rangeFile.end()) {
            std::string_view f_file {*finds};
            auto& File = *ProjectFile[f_file]; 
            File.ldFlags.reserve(inDeps.size());
            for (auto&& d : Deps) {
                 std::string_view dep {d.data(),d.size()};
                if(dep.empty()) continue;
                File.Flags += " ";
                File.Flags += dep;
            }
        }
        
        return *this;
    }

    auto trim(std::string_view str) -> std::string_view {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return std::string_view(str.data() + first, (last - first + 1));
    }

    auto singleLineComment (std::string_view line, const std::size_t* ipos) -> bool {
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
    auto blockedComment (bool* inBlockComment,const std::string* line) -> bool {
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

    bool containsToken(std::string_view str,std::string_view target) {
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
    
    void solveHeaderDependencies() {
        for (auto& Header: ProjectFile.hIter() | std::views::values | std::views::join ) {
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
                    
                    for (const auto& [K,V] : ProjectFile.hIter()) {
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
    

    void getCppFile() {
        for (const auto& K : ProjectFile.hIter()) {
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

    Project& scanHeader() {
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

                // Trim leading/trailing whitespace for reliable prefix checking
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
                    V.setObjOutputName(&OutPath->objPath);

                    std::string moPath = V.getModuleOutput(&OutPath->modulePath);
                    V.compiled = fs::exists(V.objectPath) && fs::exists(moPath) 
                        ? fs::last_write_time(V.Path) < fs::last_write_time(V.objectPath) 
                        : false;

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

                        // Ignore global module fragment "module;" (where moduleName becomes empty)
                        if (!moduleName.empty()) {
                            if (moduleName.find(':') != std::string::npos) {
                                V.isPartition = true;
                            }

                            V.Name = moduleName;
                            V.fileType = File::ModuleImpl;
                            V.setObjOutputName(&OutPath->objPath);
                            auto minterface = ProjectFile.getByName(moduleName);
                            V.dependencies.push_back(minterface->Name);
                            // std::string moPath = V.getModuleOutput(&OutPath->modulePath);
                            V.compiled = fs::exists(V.objectPath)
                                ? fs::last_write_time(V.Path) < fs::last_write_time(V.objectPath) 
                                : false;

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
                        
                    for (auto&  [HP,HF] : ProjectFile.hIter()) {
                        auto findInclude = std::ranges::find_if(HF,[&](auto& s){
                            return s.Name == headerName;
                        });
                        auto splitFlags = findInclude->flags 
                        | std::views::split(' ') 
                        | std::views::filter([](const auto& sv) {
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
                            std::string i {fmt("-I",HP)};
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

    

    Project& scanModule() {
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
                        auto [rawHeader,start,end] = getRawHeaderName(moduleName);
                        const size_t extension = rawHeader.find_last_of('.');
                        const bool isHeaderUnit = (extension == std::string_view::npos ? false : fileUtil::isCppHeader(rawHeader.substr(extension)));
                        auto& [headerUnit,headerUnitFile] = ProjectFile.addFile( isHeaderUnit ? rawHeader : moduleName);
                        
                        headerUnitFile.Name = rawHeader;
                        headerUnitFile.fileType = isHeaderUnit ? File::HeaderUnit : File::SystemHeader;
                        if (isHeaderUnit) {
                            auto& headerUnitFlags = headerUnitFile.Flags; 
                            headerUnitFile.Path = ProjectFile.getHeaderPath(rawHeader);
                            moduleName = rawHeader;
                            auto it = ProjectFile.hIter() | std::views::values | std::views::join;
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
                    | std::views::filter([&](const auto& Pair){
                        const auto& [Module, MV] = Pair;

                        const bool sameName = ((MV.fileType != File::Module) ? Module 
                        : MV.isPartition ? MV.Name.substr(MV.Name.find(':')) 
                        : MV.Name) == moduleName;
                        
                        const bool notAdded = std::ranges::find(ModuleFile.dependencies, MV.FileName) == ModuleFile.dependencies.end();
                        
                        return notAdded && sameName;
                    })
                    | std::views::transform([](const auto& Pair){const auto& [_,MV] = Pair; return std::string_view{MV.FileName.data(),MV.FileName.size()};});
             
                    for (auto FileName : filter) {
                        ModuleFile.dependencies.emplace_back(FileName);
                    }
                }
            }
            files.close();
        }
        return *this;
    }

    int compilePCH(std::string_view PCHfile) {
        const std::string headerFile = (fs::path(ProjectFile.getHeaderPath(PCHfile)) / PCHfile).string();
        const auto& oPath = OutPath->outPath;
        const std::string pchOut = fmt((oPath / fs::path(PCHfile).stem()).string(),".pch").str;
        const std::string f_cmd {fmt("{} {} -x c++-header {} -o {}",Compiler, Options,headerFile,pchOut).clean()};
        Options += fmt(" -include-pch {} ",pchOut).str;
        if (fs::exists(pchOut)) {
            if (fs::last_write_time(headerFile) > fs::last_write_time(pchOut)) {
                fs::rename(pchOut,fmt(pchOut,".old").str);
            } else {
                return 1;
            }
        }
        std::cout << fmt("Compiling PCH "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
        return cmd << f_cmd >> "Error compiling "_fmt.color(fmt::Bold_Red);
    };

    Project& configureModuleFlags() {
        std::string temp {};
        for (auto& [Key,File] : ProjectFile) {
            auto& inFile = File;
            const bool isSystemHeader = (File.fileType == File::SystemHeader);
            if(isSystemHeader) continue;
            for (const auto& I : File.dependencies) {
                const auto& depFile = *ProjectFile[I];
                const bool depisSystemHeader = depFile.fileType == File::SystemHeader;
                const bool depisHeaderUnit = depFile.fileType == File::HeaderUnit;
                const auto& mPath = isSystemHeader ? OutPath->stdPath : OutPath->modulePath;
                temp += ((
                    depisSystemHeader ? fmt(" -fmodule-file={}",depFile.objectPath) :
                    depisHeaderUnit ? fmt(" -fmodule-file={}{}",(mPath / depFile.getName()).string(), fileUtil::pcmModule) :
                    fmt(" -fmodule-file={}={}{}",depFile.Name,(mPath / depFile.getName()).string(),fileUtil::pcmModule)
                ));
            }
            inFile.Flags += temp;
            temp.clear();
        }
        return *this;
    }

    int compileModule(File& inFile) {
        const bool isSystemHeader = (inFile.fileType == File::SystemHeader);
        const bool isModuleImpl = (inFile.fileType == File::ModuleImpl);
        const bool isHeaderUnit = (inFile.fileType == File::HeaderUnit);
        if (isSystemHeader || isModuleImpl) {
            return 1;
        }
        if(!inFile.dependencies.empty()) {
            for (const auto& I : inFile.dependencies) {
                auto& dep = *ProjectFile[I];
                // std::cout << inFile.Path <<" Is Compiled: "_fmt.color(fmt::Bold_Yellow) << ProjectFile[I].Name << (ProjectFile[I].compiled ? " Yes" : " No") << "\n"; 
                
                if(!dep.compiled) {return -1;}
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
        
        const std::string fModule    = inFile.getModuleOutput(&mPath);
        
        const std::string fObjOutput {fmt((oPath / inFile.getName()).string(), fileUtil::objFile)};
        
        const std::string f_srcInput {
            isHeaderUnit ? fmt("-Wno-pragma-system-header-outside-header -fmodule-header=user --precompile {} -o {}",
                (fs::path(inFile.Path)/inFile.Name).string(),fModule) :
                isSystemHeader ? fmt("-Wno-pragma-system-header-outside-header -Wno-gnu-anonymous-struct -Wno-nullability-extension -Wno-gcc-compat -Wno-user-defined-literals -x c++-system-header --precompile {} -o {}",inFile.Name,fModule) :
                fmt("-c {} -fmodules-reduced-bmi -fmodule-output={} -fprebuilt-module-path={} ",inFile.Path,fModule,(mPath).string())
            };
        if (inFile.objectPath.empty()) {inFile.objectPath = fObjOutput;}
        
        const std::string f_cmd {
            isSystemHeader ? fmt(Compiler, Options,f_srcInput).clean() : 
            isHeaderUnit ? fmt("{} {} {} {}",Compiler, Options,inFile.Flags,f_srcInput).clean() :
            fmt("{} {} {} {} {} -o {}",Compiler, Options,(inFile.haveHeaderUnit ? "-Wno-experimental-header-units ": "") ,f_srcInput ,inFile.Flags,fObjOutput).clean()};
            
            if(cmdJson != nullptr && !isSystemHeader) { 
                cmdJson->addCompilecmd((Path / inFile.Path).parent_path().string(),f_cmd,(Path / inFile.Path).string(),fObjOutput);
        }
        
        if (inFile.compiled && !recompile) {
            inFile.compiled = true;
            return 1;
        }
        int ret {};
        // module use reduced bmi        
        if (isSystemHeader) {
            std::cout << fmt("compiling module "_fmt.color(fmt::Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } else {
            const std::string old = fmt(fModule,".old"); 
            if(fs::exists(fModule)) {
                if (fs::exists(old)){fs::remove(old);}
                fs::rename(fModule,old);
            }
            std::cout << fmt("compiling module "_fmt.color(fmt::Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        }
        inFile.compiled = (ret == 0 ? true : false);
        return ret; 
    };
    
    int compileCpp(File& inFile)
    {
        const bool isModule = (inFile.fileType == File::Module || inFile.fileType == File::SystemHeader || inFile.fileType == File::HeaderUnit);
        const bool noDependencies = inFile.dependencies.empty();
        if (isModule || inFile.compiled) {return 1;}
        
        const auto& oPath = OutPath->objPath;
        const auto& mPath = OutPath->modulePath;

        const std::string objOutput { inFile.fileType == File::ModuleImpl ? inFile.objectPath : fmt((oPath / inFile.getName()).string(), fileUtil::objFile)};

        const std::string filein    { isModule ? fmt((mPath / inFile.getName()).string(),fileUtil::pcmModule ) : inFile.Path};

        const std::string cppOutput { 
            fmt(noDependencies ? "{} {}" : "{} {} -fprebuilt-module-path={}",(isModule ? "":"-c"),filein,(mPath).string()),
        };

        // for (const auto& I : inFile.dependencies) { 
        //     const auto& depFile = *ProjectFile[I];
        //     auto addDeps = depFile.dependencies | std::views::filter([&](const auto& s) {
        //         return std::ranges::find(inFile.dependencies,s) == inFile.dependencies.end();
        //     });
        //     for(const auto& IDF : addDeps){
        //         inFile.dependencies.emplace_back(IDF);
        //     }
        // }
           
        const std::string f_cmd {fmt("{} {} {} {} {} {} {}",
            Compiler, Options,
            inFile.haveHeaderUnit ? "-Wno-experimental-header-units": "" ,
            cppOutput,
            inFile.Flags,
            isModule?"-c -o":"-o", 
            objOutput).clean()};
        
        if(cmdJson != nullptr && !isModule) { 
            cmdJson->addCompilecmd(
                (Path / inFile.Path).parent_path().string(),
                f_cmd,
                (Path / inFile.Path).string(),
                objOutput
            );
        }
        
        if (inFile.objectPath.empty())
        {
            inFile.objectPath = objOutput;
        } 

        int ret {};
        if (recompile) {
            std::cout << fmt("recompiling "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd.c_str() >> "Error compiling "_fmt.color(fmt::Bold_Red);
            
        } else if (!fs::exists(objOutput))
        {
            std::cout << fmt("compiling "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd.c_str() >> "Error compiling "_fmt.color(fmt::Bold_Red);
            
        } else if (fs::last_write_time(inFile.Path) > fs::last_write_time(objOutput))
        {
            std::cout << fmt("updated "_fmt.color(fmt::Bold_Green) , f_cmd) << "\n" ;
            ret = cmd << f_cmd.c_str() >> "Error compiling "_fmt.color(fmt::Bold_Red);
        }
        inFile.compiled = (ret == 0 ? true : false);
        return ret; 
    }
    
    
    void link(File& inPath) {
        const bool isStaticLib {outFile == Project::staticLib};
        const bool isExecutable {outFile == Project::exe};
        const bool hasDependencies {inPath.dependencies.empty()};
        const std::string f_targetOut {fmt((OutPath->exePath / inPath.Name).string(), outFile == staticLib ? fileUtil::libFile : fileUtil::executable)};
        const std::string f_Output    {fmt(outFile == staticLib ? " " : " -o ", f_targetOut)};
        
        const std::string makeFlags = [&]{ 
            std::string temp;
             for (const auto& [K,I] : ProjectFile) {
                if(I.onArchive == true) {continue;}
                if(I.fileType == File::SystemHeader) {continue;}
                if(!I.ldFlags.empty()) {temp.append(I.ldFlags);}
                temp += " ";
                temp += I.objectPath;
            }
            return temp;
        }();
        const std::string f_Object {
            fmt(hasDependencies ? "{}" : "{} -fprebuilt-module-path={}",makeFlags,(hasDependencies ? "" :(OutPath->modulePath / ".").string()))
        };

        if(isStaticLib) {
            outputName = f_targetOut;
        } else if (isExecutable) {
            if (fs::exists(f_targetOut)) {
                fs::rename(f_targetOut, fmt(f_targetOut, ".old").sv());
            }
        }

        const std::string f_cmd {fmt((outFile == Project::staticLib ? fmt(fileUtil::libTool," /out:") : fmt(Compiler,Options,LdOptions)),f_Object,f_Output).clean()};

        if (!ResPath.empty() && fs::exists(getMainPath() / ResPath)) {
            if (!fs::exists(OutPath->exePath/ResPath)) {
                std::cout << fmt("Copying from: ", (getMainPath() / ResPath).string(), " to: ",(OutPath->exePath/ResPath).string()) << "\n"; 
                fs::copy(getMainPath() / ResPath,OutPath->exePath/ResPath,fs::copy_options::recursive | fs::copy_options::skip_existing);
            }
        }
        std::cout << "Linking "_fmt.color(fmt::Bold_Green) << f_cmd << "\n";
        cmd << f_cmd.c_str() >> "linking error"_fmt.color(fmt::Bold_Red);
    }

    Project& dumpProject() {
        
        std::cout << "Dump project"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V]: ProjectFile) {
            bool isSource = V.fileType == File::Source;
            bool isModule = V.fileType == File::Module;
            bool isSystemHeader = V.fileType == File::SystemHeader;
            bool isModuleImpl = V.fileType == File::ModuleImpl;
            std::cout << " File: "<< K << " Name: "<< V.Name << " Path: " << V.Path << " Type: "
            << (isSource ? "Source" : isModule ? "Module" : isSystemHeader ? "SystemHeader" : isModuleImpl ? "ModuleImpl" : "ETC") 
            << " OutPath: " << V.objectPath <<"\n";
        }

        std::cout << "\nDump Include"_fmt.color(fmt::Yellow).endl();
        // ProjectFile.testHeader();
        for (const auto& [K,V]: ProjectFile.hIter()) {
            std::cout << "Include Dir: " << K;
            for (const auto& Header : V) {
                const bool flagsEmpty = Header.flags.empty();
                std::cout << "\nFile: " << Header.Name 
                << " Path: " << Header.Path 
                << (flagsEmpty ? "" : "\ndependency ")
                << (flagsEmpty ? "" : Header.flags);
            }
            std::cout << "\n";
        }

        std::cout << "\nDump Module"_fmt.color(fmt::Yellow).endl();
        for (const auto&  [K,V]: ProjectFile) {
            if (V.fileType != File::Module) {continue;}
            std::cout << "Module: " << K << " Name: " << V.Name << "\n";
        }

        std::cout << "\nDump Dependencies"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.dependencies.empty()){continue;}

            std::cout << "File: " << K << " Depends on: ";
            for (const auto& I : V.dependencies) {
                std::cout << fmt(" File: " , I , " ");
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

    //     const std::string f_cmd = f_isSystemHeader ? fmt(Compiler, Options,f_srcInput).clean().str : 
    //     f_isUserHeader ? fmt(Compiler, Options,f_srcInput).clean().str :
    //     fmt(Compiler, Options,headerUnits ? "-Wno-experimental-header-units ": "" ,f_srcInput ,compileInclude,ModuleDeps[fPath.filename().string()]," -o ",f_objOutput).clean().str;
        
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