#ifndef BUILD_PP 
#define BUILD_PP


#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <fstream>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <source_location>
#include <utility>
#include <set>
#include <functional>
#include <ranges>

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

template<typename... Args> concept onlyStr = ((
    std::convertible_to<Args, std::string_view> || 
    std::convertible_to<Args,const char*>) && ...);

template<typename T>
concept funcPtr = std::is_pointer_v<T> && std::is_function_v<remove_ptr_t<T>>;


struct fmt {
    std::string str;
    
    enum colors {Not_color,
        Black,    Bold_Black,   High_Black,
        Red,      Bold_Red,     High_Red,
        Green,    Bold_Green,   High_Green,
        Yellow,   Bold_Yellow,  High_Yellow,
        Blue,     Bold_Blue,    High_Blue,
        Purple,   Bold_Purple,  High_Purple,
        Cyan,     Bold_Cyan,    High_Cyan,
        White,    Bold_White,   High_White,
    };
    

    constexpr fmt(onlyStr auto&&... args) {
        const size_t totalSize = [](auto&&... args) -> size_t {
            size_t size = 0;
            ((size += std::string_view(args).size()), ...);
            return size;
        }(args...);
        str.reserve(totalSize);
        
        (str.append(std::forward<decltype(args)>(args)), ...);
    };
    constexpr fmt& color(colors color) { 
        str.reserve((color == fmt::Black) ? 13 : 14);
        str.insert(0, this->getColor(color))
        .append(this->getColor(Not_color));
        return *this;
    }
    constexpr std::vector<std::string_view> toArray() {
        std::vector<std::string_view> temp;
        for (auto idx = str.begin();idx != str.end();idx++) {
            
        }
        return temp;
    }
    constexpr fmt& clean() { 
        str.erase(std::unique(str.begin(), str.end(), [](char lhs,char rhs){return (lhs == rhs) && (lhs == ' ');}), str.end());
        return *this;
    }
    constexpr fmt& endl() {this->str.append("\n"); return *this;}
    
    constexpr std::string_view sv() const { return this->str;}
    constexpr const char* cstr() const { return this->str.c_str();}
    constexpr operator std::string_view() const { return this->sv();}
    constexpr operator const char*() const { return this->cstr();}
    constexpr std::ostream& operator <<(std::ostream& os) const { return os << this->str;}

    private:
    constexpr std::string_view getColor(colors color) {
        switch (color) {
            case Not_color:    return "\033[0m"; break;
            case Black:        return "\033[0;0m" ; break;
            case Red:          return "\033[0;31m"; break;
            case Green:        return "\033[0;32m"; break;
            case Yellow:       return "\033[0;33m"; break;
            case Blue:         return "\033[0;34m"; break;
            case Purple:       return "\033[0;35m"; break;
            case Cyan:         return "\033[0;36m"; break;
            case White:        return "\033[0;37m"; break;
            case Bold_Black:   return "\033[1;30m"; break;
            case Bold_Red:     return "\033[1;31m"; break;
            case Bold_Green:   return "\033[1;32m"; break;
            case Bold_Yellow:  return "\033[1;33m"; break;
            case Bold_Blue:    return "\033[1;34m"; break;
            case Bold_Purple:  return "\033[1;35m"; break;
            case Bold_Cyan:    return "\033[1;36m"; break;
            case Bold_White:   return "\033[1;37m"; break;
            case High_Black:   return "\033[0;90m"; break;
            case High_Red:     return "\033[0;91m"; break;
            case High_Green:   return "\033[0;92m"; break;
            case High_Yellow:  return "\033[0;93m"; break;
            case High_Blue:    return "\033[0;94m"; break;
            case High_Purple:  return "\033[0;95m"; break;
            case High_Cyan:    return "\033[0;96m"; break;
            case High_White:   return "\033[0;97m"; break;
        }
    };
};
constexpr fmt operator""_fmt(const char* str,size_t) { return fmt(str);}

inline struct implPrint
{
    constexpr implPrint& operator <<(std::string_view in) {
        std::printf("%s",in.data());
        return *this;
    }
    void operator <<(implPrint& f) {f = *this;}
}print;

class cmdImpl {
    using pipe = std::unique_ptr<FILE, decltype(&pclose)>;
    public:
    int ret;
    
    cmdImpl& run(const char* inCmd) {
        pipe cmd (popen(inCmd,"r"),pclose);
        #ifdef __unix__
        int rawStatus = pclose(cmd.release());
        ret = (raw_status != -1 && WIFEXITED(raw_status)) ? WEXITSTATUS(raw_status) : raw_status;
        #else
        ret = pclose(cmd.release());
        #endif
        return *this;
    }
    int err(const char* msg) {
        if (ret != 0) {
            print << msg << "\n";
            std::exit(1);
        }
        return ret;
    }
    cmdImpl& operator <<(const char* cmd) { return run(cmd);}
    int operator >>(const char* cmd) { return err(cmd);}
};
inline cmdImpl cmd;

struct outputPath {
    private:
    constexpr outputPath& err(bool e = false,std::string_view msg = "",std::string_view fn = std::source_location().current().function_name()) {
        if (e) {print << fmt("error: "_fmt.color(fmt::Red) ,msg , " at ", fn ,"\n"); std::exit(1);} 
        return *this;
    }
    public:
    fs::path rootPath   {};
    fs::path outPath    {};
    fs::path objPath    {};
    fs::path modulePath {};
    fs::path exePath    {};

    outputPath& setRootPath(fs::path root) { rootPath = root; return *this;}
    outputPath& setExePath(fs::path exe) {
        if(rootPath.empty()) {return err(true,"root path is empty");}
        if (!fs::exists(exe)) 
        {
            if (fs::create_directory(exe)) {
                print << fmt("Directory created: " , exe.string() , "\n").sv();
            } else {
                err(true,fmt("Failed to create directory: "));
            }        
        } else {
            print << fmt("Directory already exists: "_fmt.color(fmt::Bold_Yellow) , exe.string(),"\n").sv();
        } 
        exePath = exe;
        return err();
    }
    outputPath& setOutfolder(fs::path folder) {
    if(rootPath.empty()) {return err(true,"root path is empty");} 
        if (!fs::exists(folder)) 
        {
            if (fs::create_directory(folder)) {
                print << fmt("Directory created: " , folder.string() , "\n");
            } else {
                err(true,fmt("Failed to create directory: "));
            }        
        } else {
            print << fmt("Directory already exists: "_fmt.color(fmt::Bold_Yellow) , folder.string(),"\n");
        }
        return *this;
    }
    void setOutpath(fs::path out) {
        if(rootPath.empty()) { err(true,"root path is empty"sv);}
        outPath = out;
        objPath = outPath / "obj";
        modulePath = outPath / "module";
        for (const auto& lm_dir : {outPath,objPath,modulePath})
        {
            if (!lm_dir.has_parent_path()) {
                err(true,fmt("Error: Path has no parent path: " ,lm_dir.string()).sv());
            }
    
            if (!fs::exists(lm_dir)) 
            {
                if (fs::create_directory(lm_dir)) {
                    print << fmt("Directory created: " , lm_dir.string() , "\n");
                } else {
                    err(true,fmt("Failed to create directory: "));
                }        
            } else {
                print << fmt("Directory already exists: "_fmt.color(fmt::Bold_Yellow) , lm_dir.string(),"\n");
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
        "lib"
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
    constexpr bool isCpp(std::string_view file) const
    {
        for (const auto& i : cppSource)
        {
            if (i == file) {return true; break;}
        }
        return false;
    }
    constexpr bool isModule(std::string_view file) const
    {
        for (const auto& i : cppModule)
        {
            if (file == i) {return true; break;}
        }
        return false;
    }
    constexpr bool isCppHeader(std::string_view file) const
    {
        for (const auto& i : cppHeader)
        {
            if (i == file) {return true; break;}
        }
        return false;
    }
    // static constexpr bool isSystemHeader(std::string_view file) {
    //     for(const auto& i : sysHeader) {
    //         if (i.first.front() == file.front()) {
    //             for (auto fl : *i.second) {
    //                 const std::string_view f = *i.second;
    //                 if (file.compare(f)) {
    //                     return true;
    //                 }
    //             }
    //             break;
    //         }
    //     }
    //     return false;
    // }
};

class compileCommand {
    utl::json::node jsonNode;
    public:
    compileCommand& addCompilecmd(std::string_view path,std::string_view arg,std::string_view file,std::string_view out) {
        utl::json::node innode;
        innode["directory"] = path;
        innode["command"] = arg ;
        innode["file"] = file ;
        innode["output"] = out ;

        jsonNode.push_back(innode);
        return *this;
    }
    // compileCommand& addCompilecmd(std::string arg) {
    //     utl::json::node innode = arg;

    //     jsonNode.push_back(innode);
    //     return *this;
    // }
    void write(fs::path to)
    {
        jsonNode.to_file(to.string());
    }


};


struct File {
    mutable bool compiled = false;
    mutable enum fTypes : char {
        Source,
        Module, 
        SystemHeader, 
        none
    } fileType = none;
    std::size_t ID          {};
    std::string Name        {};
    std::string Flags       {};
    std::string ldFlags     {};
    std::string objectPath  {};
    std::vector<std::size_t> dependencies {};
};

class FileManager {
    constexpr FileManager& err(bool cnd = false,std::string_view msg = "",std::string fn = std::source_location::current().file_name()) {
        if (cnd) {print << fmt (msg , "\n"); std::exit(1);} 
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
    std::unordered_map<std::string, std::vector<std::string>,StringHash,std::equal_to<>> Header {};
    std::unordered_map<std::string, File,StringHash,std::equal_to<>> Files {};
    std::vector<MapPair*> IDMap {};
    std::size_t NextID = 0;
    public:
    MapPair* Main = nullptr;
    File& addFile(std::string_view name) {
        auto it = Files.find(name);
        if (it != Files.end()) {
            return it->second;
        }
        NextID = Files.size();
        
        auto ref = Files.emplace(std::string(name), File{.ID = NextID}).first;
        IDMap.emplace_back(&(*ref));
        
        return ref->second;
    }
    
    std::vector<std::string>& setHeaderPath(std::string_view name) {
        return Header.try_emplace(name.data(),std::vector<std::string>{}).first->second;
    }
    void addHeader(std::string_view name,std::string_view H) {
        auto it = Header.find(name);
        std::string n = fs::path(H).lexically_relative(name).generic_string();
        if (it != Header.end()) {
            it->second.push_back(n);
        }
        return;
    }
    File& copyFile(std::string_view name,const File& other) {
        NextID = Files.size();
        
        auto ref = Files.try_emplace(std::string(name), other).first;
        IDMap.emplace_back(&(*ref));
        return IDMap[NextID]->second;
    }
    int getID(std::string_view str) {
        const auto it = Files.find(str);
        if (it != Files.end()) {
            return it->second.ID; 
        }
        
        std::size_t newId = Files.size();
        auto ref = Files.try_emplace(std::string(str), File{.ID = newId}).first;
        IDMap.emplace_back(&(*ref));
        return ref->second.ID;
    }
    // File& operator [](std::size_t id) {
    //     return Files[id];
    // }
    File* operator [](std::string_view id) {
        auto it = Files.find(id); 
        if (it != Files.end()) {
            return &it->second;
        }
        err(true,fmt("Error: "_fmt.color(fmt::Red),"Key doesn't exists")); 
        return nullptr;
    }
    File& operator [](std::size_t id) {
        err(id > IDMap.size(),fmt("Error: "_fmt.color(fmt::Red),"ID out of bounds in IDMap"));
        return IDMap[id]->second; 
    }

    MapPair& getPair (std::string_view id) {
        return *IDMap[(*this)[id]->ID]; 
    }

    File* getFile(std::string_view id) {
        return (*this)[id];
    }
    void setMain(std::string_view id) {
        print << "Set Main: " << (*this)[id]->Name;
        Main = IDMap[(*this)[id]->ID];
    }
    File& getMain() {
        return Main->second;
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
    auto VIter() {
        return itUtl(&IDMap);
    }
    auto begin() const { return Files.begin(); }
    auto end()   const { return Files.end(); }
};

class cProject{
    inline static fileUtil file;
    outputPath* projectOutPath;
    
    std::string Main            {};
    std::string Options         {};
    std::string Compiler        {};
    std::string compileInclude  {};
    
    constexpr cProject& err(bool isError = false,std::string_view msg = "",std::string_view f = std::source_location().current().function_name()) {
        if (isError) { print << fmt(msg , " at " , f); std::exit(1); } 
        return *this;
    }
    public:
    bool recompile,singleFile;
    enum projectType : int {
        exe,
        staticLib,
        dynamicLib
    } outFile;
    compileCommand* cmdJson;
    
    std::string ProjectName     {};
    std::string outputName      {};
    fs::path Path       {};
    std::vector<std::string> SourcePath {};
    FileManager ProjectFile;
    std::vector<std::string> includePath {};
    std::vector<std::string> project     {};
    std::vector<std::string> object      {};
    std::vector<std::string> include     {};
    std::vector<std::pair<std::string, std::string>> dependency {};
    std::unordered_map<std::string,std::vector<std::string_view>>   includeMap {};

    cProject(outputPath* path, projectType exe,bool recomp = false) {
        projectOutPath = path;
        outFile = exe;
        cmdJson = nullptr;
        recompile = recomp;
        print << fmt("Project initialized at "_fmt.color(fmt::Bold_Green) , this->Path.string());
    };

    cProject& setMain(std::string_view main)         {Main = main;ProjectFile.setMain(main); return *this;}
    cProject& setCompiler(std::string_view comp)     {Compiler = fmt(comp, " ").str;return *this;}
    cProject& setOptions (std::string_view opt)      {Options = fmt(opt, " ").str;return *this;}
    cProject& setProjectPath(fs::path in)            {Path = in;return *this;}
    cProject& setSourcePath(fs::path in)             {SourcePath.emplace_back(fs::path(in).lexically_proximate(Path).string()); return *this;}
    cProject& setSource(std::initializer_list<std::string_view> in) {
        for (const auto& i : in) {
            // project.push_back((sourcePath / i).string());
            auto& ptr = ProjectFile.addFile((fs::path(getMainSource()) / i).string());
            ptr.Name = i;
            ptr.fileType = File::Source;
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
        print << fmt("add dependency for " , inFile , " with deps: ");
        for (const auto& d : inDeps) {
            print << d << " ";
        }        print << "\n";
        std::string f_file;
        std::string f_deps; 

        size_t f_totalSize = 0;
        for (const auto& d : inDeps) {
            f_totalSize += d.size() + 1; // +1 for space
        }
        
        f_deps.reserve(f_totalSize);
        for (const auto& d : inDeps) {
            f_deps.append(fmt(" -l" , d , " "));
        }

        for (const auto& [k,mod] : ProjectFile) {
            std::string filename = fs::path(k).filename().string();
            //print << fmt("check file " , filename , " against " , inFile , "\n");
            if (filename == inFile) {
                print << fmt("dependency found for " , inFile , " in " , k , " imp filename " , filename , "\n");
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
            err((!fs::exists(F) || !fs::is_directory(F)), "Directory does not exist.\n");
            fs::directory_iterator iterator(F);
            for (const auto& entry : iterator) {
                if (entry.is_regular_file() && (entry.path().extension() == file.cSource)) {
                    // std::cout << "add project file " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    // project.emplace_back(entry.path().string());
                    auto& ptr = ProjectFile.addFile(entry.path().string());
                    ptr.Name = entry.path().stem().string();
                    ptr.fileType = File::Source;
                }
            }
        }
            
        for (const auto& [K,V] : ProjectFile.hIter()) {
            fs::recursive_directory_iterator it(K);
            print << "Scan File: "_fmt.color(fmt::Bold_Green) << K;
            for (const auto& entry : it) {
                if (entry.is_regular_file() && file.isCppHeader(entry.path().extension().string()) ) {
                    print << fmt("Add include " , entry.path().filename().string() , " From: " , K).endl();
                    ProjectFile.addHeader(K, entry.path().string());
                }
            }
        }
        return *this;
    }

    cProject& scanInclude() {
        
        for (const auto& [K,V] : ProjectFile) {
            auto& modFile = ProjectFile[V.ID];
            err(K.empty(),"Error: Empty project path"_fmt.color(fmt::Bold_Red));
            
            std::ifstream files(K);
            err(K.empty(),fmt("Error: Unable to open file " , K , "\n").color(fmt::Bold_Red));
            
            std::string line;
            std::string includeFound;
            
            // while (std::getline(files, line)) {
            //     size_t pos = line.find(file.includeToken);
            //     if (pos == std::string::npos) {
            //         //include.emplace_back(includeFile);
            //         continue;
            //     }
            //     includeFound = {fs::path{line.substr(pos + file.includeToken.length())}.filename().string()};
            //     std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });
            //     for (const auto& map : includeMap) {
            //         if (map.first == includeFound) {
            //             print << fmt("include found " , includeFound , " in " , p , "\n");
            //             includeMap[includeFound] = {p};
            //             include.emplace_back(includeFound);
            //         }
            //     }
            // }

            while (std::getline(files, line)) {
                size_t pos = line.find(file.includeToken);
                if (pos != std::string::npos) {
                    includeFound = line.substr(pos + file.includeToken.length());
                    std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });
                    print << fmt("Header: "_fmt.color(fmt::Bold_Purple),includeFound).endl();

                    for(const auto& [KI,VI] : ProjectFile.hIter()) {
                        for(const auto& I : VI) {
                            if (includeFound == I) {
                                print << fmt("Include dependency Found: "_fmt.color(fmt::Blue), includeFound, " " , I , " in " , K).endl();
                                modFile.Flags.append(fmt(" -I",KI));
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

    cProject& compileC(std::pair<const std::string,File>& inPath)
    {
        auto& [K,V] = inPath;
        
        const std::string f_obj {fmt((projectOutPath->objPath / V.Name).string(), file.objFile)};

        const std::string f_cmd {fmt(Compiler,Options, V.Flags ,fmt(" -c ", K, " -o ", f_obj)).clean()};

        if(cmdJson != nullptr) cmdJson->addCompilecmd(fs::path(K).parent_path().string(), f_cmd, K, f_obj);
        object.push_back(f_obj);
        int ret {};
        if (recompile)
        {
            print << fmt("recompile "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
        }else if (!fs::exists(f_obj))
        {
            print << fmt("compile "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "compile error "_fmt.color(fmt::Red);
        } else if (fs::last_write_time(K) >= fs::last_write_time(f_obj))
        {
            print << fmt("updating "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
        }
        V.compiled = (ret == 0 ? true : false);
        return *this;
    }

    cProject& link(std::pair<const std::string,File>& target) {
        auto& [K,V] = target;
        const std::string f_exe = fmt(" -o ", fmt((projectOutPath->exePath / V.Name).string(), file.platform), " ").str;
        const std::string f_object = fmt((projectOutPath->objPath / V.Name).string(), file.objFile).str;
        
        const std::string f_cmd = fmt(Compiler,Options,V.ldFlags,f_object,f_exe).str;
        print << f_cmd << "\n";
        cmd << f_cmd.c_str(); 
        return *this;      
    }

    cProject& dumpProject() {
        
        print << "Dump project"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V]: ProjectFile) {
            print << "ID: " << std::to_string(V.ID) << " Name: "<< V.Name << " File: " << K << " Type: "
            << (V.fileType == File::Source ? "Source" : V.fileType == File::Module ? "Module" : V.fileType == File::SystemHeader ? "SystemHeader" : "ETC") << "\n";
        }

        print << "\nDump Include"_fmt.color(fmt::Yellow).endl();
        // ProjectFile.testHeader();
        for (const auto& [K,V]: ProjectFile.hIter()) {
            print << "Include Dir: " << K << " Header File: ";
            for (const auto& N : V) {
                print << N << " ";
            }
            print << "\n";
        }

        print << "\nDump Module"_fmt.color(fmt::Yellow).endl();
        for (const auto&  [K,V]: ProjectFile) {
            if (V.fileType != File::Module) {continue;}
            print << "Module: " << K << " Name: " << V.Name << "\n";
        }

        print << "\nDump Dependencies"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.dependencies.empty()){continue;}

            print << "File: " << K << " Depends on: ";
            for (const auto& I : V.dependencies) {
                print << " ID: " << std::to_string(I) << " " << ProjectFile[I].Name << " ";
            }
            print << "\n";
        }
        print << "\nDump File Flags"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.Flags.empty()){continue;}

            print << "File: " << K << " With Flags: " << V.Flags << "\n";
        }
        print << "\n";
        return *this;
    }

};


class Project
{
    inline static fileUtil file;
    std::string ProjectName     {};
    std::string Main            {};
    std::string Options         {};
    std::string LdOptions       {};
    std::string Compiler        {};
    std::string StaticLib       {};
    std::string outputName      {};
    
    constexpr Project& err(bool cnd = false,std::string_view msg = "",std::string fn = std::source_location::current().file_name()) {
        if (cnd) {print << fmt (msg , "\n"); std::exit(1);} 
        return *this;
    }
    bool recompile;
    public:
    enum projectType : char {
        exe,
        staticLib,
        dynamicLib
    } outFile;
    enum includeas : char {
        normal,
        system,
        module
    } includeas;
    
    outputPath* OutPath;
    compileCommand* cmdJson;

   

    fs::path Path       {};
    fs::path ResPath    {};
    std::vector<std::string> SourcePath  {};
    std::vector<std::string> IncludePath {};
    FileManager ProjectFile        {};
    // std::vector<std::string> Object      {};
    // std::vector<std::pair<std::string, std::string>> Dependency {};
    // std::vector<std::pair<std::string, std::string>> Modules    {};
    // std::unordered_map<std::string,std::string>      ModuleDeps {};
    // std::unordered_map<std::string_view,std::vector<std::string>> ModuleMap  {};
    // std::unordered_map<std::string,std::vector<std::string>>      Include    {};
    // std::map<std::pair<std::string,std::string>,std::set<std::string_view>>   IncludeMap {};

    constexpr Project& setMain        (std::string_view main) {Main = main;ProjectFile.setMain((fs::path(getMainPath()) / main).string()); return *this;}
    constexpr Project& setCompiler    (std::string_view comp) {Compiler = comp; return *this;}
    constexpr Project& setOptions     (std::string_view opt)  {Options = fmt(" ",opt," ").clean().str; return *this;}
    constexpr Project& setLdOptions   (std::string_view opt)  {LdOptions = fmt(" ",opt," ").clean().str; return *this;}
    constexpr Project& setProjectPath (std::string_view in)   {Path = in; return *this;}
    constexpr Project& setResourcePath(std::string_view in)   {ResPath = in; return *this;}
    constexpr Project& addSourcePath  (std::string_view in)   {SourcePath.emplace_back(fs::path(in).lexically_proximate(Path).string()); return *this;}
    constexpr Project& addIncludePath (std::string_view in)   {IncludePath.emplace_back(in); ProjectFile.setHeaderPath(in); return *this;}
    constexpr Project& setincludeas   (enum includeas opt)    {includeas = opt; return *this;}
    
    constexpr std::string& getMainPath () {return SourcePath[0];}
    
    constexpr Project& addSource(std::initializer_list<std::string_view> in) {
        for (const auto& i : in) {
            auto filePath = (Path / i);
            err (!fs::exists(filePath),fmt("source file "_fmt.color(fmt::Red) , i , " does not exist" ));
            auto & P = ProjectFile.addFile(filePath.string());
            P.Name = filePath.stem().string();
            P.fileType = File::Source;
        }
        return *this;
    }

    constexpr Project(const char* name,outputPath* path,projectType exe,bool recomp = false) {
        ProjectName = name,
        OutPath = path,
        outFile = exe,
        cmdJson = nullptr,
        recompile = recomp;
        print << fmt("Project initialized at "_fmt.color(fmt::Green) , this->Path.string(),"\n" );
    };

    Project& addCompileCommand(compileCommand* cmd)
    {
        cmdJson = cmd;
        return *this;
    }


    Project& getLib(cProject* cProj)
    {
        if (cProj->outFile == cProject::staticLib) {
            for (const auto& [K,V] : cProj->ProjectFile) {
                this->ProjectFile.copyFile(K, V);
            }
             for (const auto& [K,V] : cProj->ProjectFile.hIter()) {
                for (const auto& I : V) {
                    ProjectFile.addHeader(K, I);
                }
            }
        }
        StaticLib.append(fmt(" -L",cProj->outputName," -l",cProj->ProjectName,".a"));
        return *this;
    }

    Project& getLib(Project* other)
    {
        if (other->outFile == Project::staticLib) {
            for (const auto& [K,V] : other->ProjectFile) {
                ProjectFile.copyFile(K,V);
            }
            for (const auto& [K,V] : other->ProjectFile.hIter()) {
                for (const auto& I : V) {
                    ProjectFile.addHeader(K, I);
                }
            }
            StaticLib.append(fmt(" -L",other->outputName," -l",other->ProjectName,".a"));
            other->~Project();
        }
        return *this;
    }

    Project& addDependency(std::string_view inFile, std::initializer_list<std::string_view> inDeps){
        print << fmt("add dependency for " , inFile , " with deps: ");
        for (const auto& d : inDeps) {
            print << d << " ";
        }        print << "\n";
        std::string f_file;
        std::string f_deps; 

        size_t f_totalSize = 0;
        for (const auto& d : inDeps) {
            f_totalSize += d.size() + 1; // +1 for space
        }
        
        f_deps.reserve(f_totalSize);
        for (const auto& d : inDeps) {
            f_deps.append(fmt(" -l" , d , " "));
        }

        for (const auto& [k,mod] : ProjectFile) {
            std::string filename = fs::path(k).filename().string();
            //print << fmt("check file " , filename , " against " , inFile , "\n");
            if (filename == inFile) {
                print << fmt("dependency found for " , inFile , " in " , k , " imp filename " , filename , "\n");
                f_file = k;
                break;
            }
        }
        ProjectFile[f_file]->ldFlags = f_deps;
        return *this;
    }

    Project& addIncludefile(std::string_view inPath) {
        err(!fs::exists(inPath),fmt("Include path: ",inPath, " does not exist ").color(fmt::Bold_Red));
        
        ProjectFile.setHeaderPath(inPath);
        return *this;
    }

    void getCppFile() {
       
        for (const auto& p : SourcePath)
        {
            err ((!fs::exists(p) || !fs::is_directory(p)), fmt("Directory does not exist. "_fmt.color(fmt::Bold_Red),p));
            fs::directory_iterator iterator(p);
            if (ProjectFile.empty())
            {
                for (const auto& entry : iterator) {
                    bool isModule = file.isModule(entry.path().extension().string());
                    bool isSource = file.isCpp(entry.path().extension().string());
                    if (entry.is_regular_file() && ( isModule || isSource)) {
                        
                        print << fmt("add project file " , entry.path().filename().string() , " " , entry.path().string()).endl();
                        auto& f = ProjectFile.addFile(entry.path().string());
                        f.Name = entry.path().stem().string();
                        f.fileType = File::Source;
                    }
                }
                
            }

            for (const auto& [K,V] : ProjectFile.hIter()) {
                fs::recursive_directory_iterator it(K);
                print << "Scan File: "_fmt.color(fmt::Bold_Green) << K;
                for (const auto& entry : it) {
                    if (entry.is_regular_file() && file.isCppHeader(entry.path().extension().string()) ) {
                        print << fmt("Add include " , entry.path().filename().string() , " From: " , K).endl();
                        ProjectFile.addHeader(K, entry.path().string());
                    }
                }
            }
        }
    }
    

    Project& scanHeader() {
        print << "Scanning Files"_fmt.color(fmt::Bold_Green).endl();
        for (const auto& [K, V] : ProjectFile) {
            std::string_view shName = K;
            auto& modFile = ProjectFile[V.ID];
            print << "scan " << shName << "\n";
            if (shName.empty()) {err(true,"Error: Empty project path"_fmt.color(fmt::Bold_Red));}
            
            std::ifstream files(shName.data());
            if (!files.is_open()) {err (true,fmt("Error: Unable to open file "_fmt.color(fmt::Bold_Red) , shName));}

            std::string line;
            std::string includeFound;
            std::string moduleName;

            while (std::getline(files,line)) {
                size_t epos = line.find(file.exportToken);
                if (epos != std::string::npos) {
                        moduleName = line.substr(epos + file.exportToken.length() + 1);
                        moduleName.erase(moduleName.find(';'));
                        print << fmt("export module "_fmt.color(fmt::Yellow), moduleName ," found in " , shName ).endl();
                        
                        modFile.Name = moduleName;
                        modFile.fileType = File::Module;
                        break; // only one export module per file is allowed 
                    }
            }
            files.clear();
            files.seekg(0);
            
            while (std::getline(files, line)) {
                size_t pos = line.find(file.includeToken);
                if (pos != std::string::npos) {
                    includeFound = line.substr(pos + file.includeToken.length());
                    std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });
                    print << fmt("Header: "_fmt.color(fmt::Bold_Purple),includeFound).endl();

                    for(const auto& [KI,VI] : ProjectFile.hIter()) {
                        for(const auto& I : VI) {
                            if (includeFound == I) {
                                print << fmt("Include dependency Found: "_fmt.color(fmt::Blue), includeFound, " " , I , " in " , shName).endl();
                                modFile.Flags.append(fmt(" -I",KI));
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

    Project& scanModule() {
        for (const auto& [K,V] : ProjectFile) {
           
            std::string_view smName = K;

            print << fmt("Scan module " , smName ).endl();

            err(smName.empty() ,"Error: Empty project path"_fmt.color(fmt::Bold_Red)); 

            std::ifstream files(smName.data());
            err(!files.is_open(),"Error: Unable to open file "_fmt.color(fmt::Bold_Red));

            std::string line;
            bool inBlockComment = false;

            while (std::getline(files, line)) {
                if (inBlockComment) {
                size_t endComment = line.find("*/");
                if (endComment != std::string::npos) {
                    inBlockComment = false; // Block ended, but skip this line anyway to be safe
                }
                continue; // Skip processing this line
                }

                // 2. Check if a multi-line comment block starts on this line
                size_t startBlock = line.find("/*");
                size_t endBlock = line.find("*/");
                
                if (startBlock != std::string::npos) {
                    // If it doesn't close on the same line, flag it for subsequent lines
                    if (endBlock == std::string::npos || endBlock < startBlock) {
                        inBlockComment = true;
                    }
                    continue; // Skip processing the current line completely
                }
                
                 
                size_t ipos = line.find(file.importToken);
                size_t epos = line.find(';');

                // 4. Check for single-line comments (//)
                size_t inlineComment = line.find("//");
                if (inlineComment != std::string::npos) {
                    // If the comment is at the beginning, or appears BEFORE the import token, skip the line
                    if (ipos == std::string::npos || inlineComment < ipos) {
                        continue; 
                    }
                }

                if (ipos != std::string::npos && epos != std::string::npos) {
                    size_t startPos = ipos + file.importToken.length() + 1; 
                    if (startPos >= epos) continue;
                    std::string moduleName = line.substr(startPos,epos - startPos);
                    
                    print << fmt("import module "_fmt.color(
                    fmt::Bold_Blue) , moduleName , " found in " , smName).endl();
                    
                    if (moduleName.front() == '<' || moduleName.front() == '"') {
                        std::string rawHeader = moduleName.substr(1, moduleName.size() - 2);
                        auto& F = ProjectFile.addFile((moduleName.front() == '"') ? (Path / getMainPath() / rawHeader).string() : moduleName);
                        F.Name = rawHeader;
                        F.objectPath = fmt((OutPath->modulePath / rawHeader).string(),file.pcmModule);
                        F.fileType = File::SystemHeader;
                        // Modules.push_back({(moduleName.front() == '"') ? (Path / getMainPath() / rawHeader).string() : moduleName,rawHeader});
                        // ModuleMap[smName].push_back(moduleName);
                    }
                    for (const auto& [M,MV] : ProjectFile) {
                        if (((MV.fileType != File::Module) ? M : MV.Name) == moduleName) {
                            ProjectFile[V.ID].dependencies.emplace_back(MV.ID);
                        }
                    }
                }
            }
            files.close();
        }
        return *this;
    }
    
    // bool isModuleExist(std::string_view infile)
    // {
    //     bool isHeader = (infile.front() == '<' || infile.front() == '"');
    //     if (!infile.empty() && isHeader) {
    //         return true; 
    //     }

    //     const fs::path f_path = infile;
    //     const auto& mPath = OutPath->modulePath;
    //     const auto modMap = ModuleMap.find(infile);
    //     if (modMap == ModuleMap.end()) {
    //         return true; // Or true, depending on your fallback logic if not tracked
    //     }
    //     for (const auto& m : modMap->second)
    //     {
    //         if (m.empty()){continue;}
    //         bool isWrapped = (m.front() == '<' || m.front() == '"');
    //         auto moduleFile = fmt((mPath / (isWrapped ? m.substr(1, m.length() - 2) : m)).string(), file.pcmModule).clean().str;
    //         print << moduleFile << " " << (fs::exists(moduleFile) ? "exist" : "not exist") << "\n";
    //         if (!fs::exists(moduleFile)) {
    //             return false;
    //         }
    //         if(!isWrapped && (fs::last_write_time(f_path) > fs::last_write_time(moduleFile) && recompile))
    //         {
    //             return false;
    //         }
        
    //     }
    //     return true;
    // }

    int compileModule(std::pair<const std::string, File>& inFile) {
        auto& [K,V] = inFile;
        if(!V.dependencies.empty()) {
            for (const auto& I : V.dependencies) {
                print << "Is Compiled: "_fmt.color(fmt::Bold_Yellow) << ProjectFile[I].Name << (ProjectFile[I].compiled ? " Yes" : " No") << "\n"; 
                if(!ProjectFile[I].compiled) {return -1;}
            }
        }
        bool headerUnits = false;
        const bool isSystemHeader = (V.fileType == File::SystemHeader);
        const auto& mPath = OutPath->modulePath;
        const auto& oPath = OutPath->objPath;
        
        // const bool f_isUserHeader = file.isCppHeader(fPath.extension().string());
        // const fs::path fPath = in_path;
        
        const std::string fModule    = fmt((mPath / V.Name).string(), file.pcmModule).str;

        const std::string fObjOutput = fmt((oPath / V.Name).string(), file.objFile).str;
        
        const auto l_rewrite = [&isSystemHeader,&fModule] -> void {
            if (!isSystemHeader) {
                const std::string old = fmt(fModule,".old").str; 
                if(fs::exists(fModule)) {
                    if (fs::exists(old)){
                    fs::remove(old);}
                    fs::rename(fModule,old);
                }
            }
        };

        const std::string f_srcInput = 
        // f_isUserHeader ? fmt("-Wno-pragma-system-header-outside-header -fmodule-header=user --precompile ",fPath.string()," -o ",f_module).str :
        isSystemHeader ? fmt("-Wno-pragma-system-header-outside-header -x c++-system-header --precompile ",V.Name," -o ",fModule).str :
        fmt("-c ",(Path / K).string()," -fno-modules-reduced-bmi -fmodule-output=",fModule," -fprebuilt-module-path=",(mPath).string()).str;
        
        for (const auto& I : V.dependencies) {
            ProjectFile[V.ID].Flags.append(
                ProjectFile[I].fileType == File::SystemHeader ? fmt(" -fmodule-file=",fmt((mPath / ProjectFile[I].Name).string(),file.pcmModule)) :
                fmt(" -fmodule-file=",ProjectFile[I].Name,"=",fmt((mPath / ProjectFile[I].Name).string(),file.pcmModule))
            );
        }
        
        if (V.objectPath.empty()) {ProjectFile[V.ID].objectPath = fObjOutput;}

        const std::string f_cmd = 
        isSystemHeader ? fmt(Compiler, Options,f_srcInput).clean().str : 
        // f_isUserHeader ? fmt(Compiler, Options,f_srcInput).clean().str :
        fmt(Compiler, Options,headerUnits ? "-Wno-experimental-header-units ": "" ,f_srcInput ,V.Flags," -o ",fObjOutput).clean().str;
        
        if(cmdJson != nullptr && !isSystemHeader) { cmdJson->addCompilecmd((Path / K).parent_path().string(),f_cmd,(Path / K).string(),fObjOutput);}
        
        int ret {};
        if (recompile && !isSystemHeader) {
            print << fmt("recompiling "_fmt.color(fmt::Green) , f_cmd , "\n");
            l_rewrite();
            ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } else if (isSystemHeader) {
            print << fmt("System Header "_fmt.color(fmt::Green) , f_cmd , "\n");
            if(fs::exists(fModule)) {ProjectFile[V.ID].compiled = (ret == 0 ? true : false); return 0;}
            ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } else if (!fs::exists(fModule)) {
            print << fmt("compiling "_fmt.color(fmt::Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } else if (fs::last_write_time(K) > fs::last_write_time(fModule)) {
            print << fmt("updated "_fmt.color(fmt::Green) , f_cmd , "\n");
            l_rewrite();
            ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } 
        print << "Status: "_fmt.color(fmt::Bold_Yellow) << std::to_string(ret) << "\n";
        ProjectFile[V.ID].compiled = (ret == 0 ? true : false);
        return ret; 
    };
    

    int compileCpp(std::pair<const std::string, File>& inPath)
    {
        auto& [K,V] = inPath;
        if(V.compiled) {return false;}
        const bool f_isModule = (V.fileType == File::Module || V.fileType == File::SystemHeader);
        if (f_isModule) return false;
        
        const auto& oPath = OutPath->objPath;
        const auto& mPath = OutPath->modulePath;
        
        const std::string f_objOutput = fmt((oPath / V.Name).string(), file.objFile).str;

        const std::string f_filein    = f_isModule ? fmt((mPath / V.Name).string(),file.pcmModule ).str : K;

        const std::string f_cppOutput = 
        fmt(f_isModule ?"" : "-c ",f_filein, 
            V.dependencies.empty() ? "" : 
            fmt(" -fprebuilt-module-path=", (mPath).string()," ")
        ).str;
            
        for (const auto& I : V.dependencies) {
            ProjectFile[V.ID].Flags.append(
                ProjectFile[I].fileType == File::SystemHeader ? fmt(" -fmodule-file=",fmt((mPath / ProjectFile[I].Name).string(),file.pcmModule)) :
                fmt(" -fmodule-file=",ProjectFile[I].Name,"=",fmt((mPath / ProjectFile[I].Name).string(),file.pcmModule))
            );
        }

        const std::string f_cmd = fmt(Compiler, Options ,f_cppOutput,V.Flags,f_isModule?" -c -o ":" -o ", f_objOutput).clean().str;
        
        if(cmdJson != nullptr && !f_isModule) { cmdJson->addCompilecmd((Path / K).parent_path().string(),f_cmd,(Path / K).string(),f_objOutput);}
        
        if (!V.objectPath.empty())
        {
            V.objectPath = f_objOutput;
        } 

        int ret {};
        if (recompile) {
            print << fmt("recompiling "_fmt.color(fmt::Bold_Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "error compiling "_fmt.color(fmt::Bold_Red);
            
        } else if (!fs::exists(f_objOutput))
        {
            print << fmt("compiling "_fmt.color(fmt::Bold_Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "error compiling "_fmt.color(fmt::Bold_Red);
            
        } else if (fs::last_write_time(K) > fs::last_write_time(f_objOutput))
        {
            print << fmt("updated "_fmt.color(fmt::Bold_Green) , f_cmd , "\n");
            ret = cmd << f_cmd.c_str() >> "error compiling "_fmt.color(fmt::Bold_Red);
        }
        V.compiled = (ret == 0 ? true : false);
        return ret; 
    }
    
    
    void link(File& inPath) {
        
        print << fmt("linking"_fmt.color(fmt::Bold_Green).endl());
        const std::string f_targetOut = fmt((OutPath->exePath / inPath.Name).string(), outFile == staticLib ? file.libFile : file.executable).str;
        const std::string f_Output    = fmt(outFile == staticLib ? " " : " -o ", f_targetOut).str;
        
        std::string f_Object;

        if(outFile == Project::staticLib) {
            f_Object = fmt(inPath.dependencies.empty() ? "" : fmt(" -fprebuilt-module-path=", (OutPath->modulePath / ".").string()));
            outputName = f_targetOut;
        } else if (outFile == Project::exe) {
            f_Object = fmt(inPath.dependencies.empty() ? "" : fmt(" -fprebuilt-module-path=", (OutPath->modulePath / ".").string()));
            if (fs::exists(f_targetOut)) {
                print << f_targetOut << "\n";
                fs::rename(f_targetOut, fmt(f_targetOut, ".old").str);
            }
        }
        print << "Is ProjectFile empty: " << (ProjectFile.hIter().container->empty() ? "Yes" : "NO") << "\n";
        for (const auto* I : ProjectFile.VIter()) {
            auto [N,F] = *I;
            if(!F.ldFlags.empty()) {f_Object.append(F.ldFlags);}
            f_Object.append(fmt(" ",F.objectPath));

        }
        const std::string f_cmd = fmt(outFile == Project::staticLib ? fmt(file.libTool," /out:") : Compiler,Options,LdOptions,StaticLib,f_Object,f_Output).clean().str;
        print << "Target Out:" << f_targetOut << "\n" << f_cmd << "\n";

        if (!ResPath.empty() && fs::exists(getMainPath() / ResPath)) {
            if (!fs::exists(OutPath->exePath/ResPath)) {
                print << fmt("Copying from: ", (getMainPath() / ResPath).string(), " to: ",(OutPath->exePath/ResPath).string()) << "\n"; 
                fs::copy(getMainPath() / ResPath,OutPath->exePath/ResPath,fs::copy_options::recursive | fs::copy_options::skip_existing);
            }
        }
        cmd << f_cmd.c_str() >> "linking error"_fmt.color(fmt::Bold_Red);
    }

    Project& dumpProject() {
        
        print << "Dump project"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V]: ProjectFile) {
            print << "ID: " << std::to_string(V.ID) << " Name: "<< V.Name << " File: " << K << " Type: "
            << (V.fileType == File::Source ? "Source" : V.fileType == File::Module ? "Module" : V.fileType == File::SystemHeader ? "SystemHeader" : "ETC") << "\n";
        }

        print << "\nDump Include"_fmt.color(fmt::Yellow).endl();
        // ProjectFile.testHeader();
        for (const auto& [K,V]: ProjectFile.hIter()) {
            print << "Include Dir: " << K << " Header File: ";
            for (const auto& N : V) {
                print << N << " ";
            }
            print << "\n";
        }

        print << "\nDump Module"_fmt.color(fmt::Yellow).endl();
        for (const auto&  [K,V]: ProjectFile) {
            if (V.fileType != File::Module) {continue;}
            print << "Module: " << K << " Name: " << V.Name << "\n";
        }

        print << "\nDump Dependencies"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.dependencies.empty()){continue;}

            print << "File: " << K << " Depends on: ";
            for (const auto& I : V.dependencies) {
                print << " ID: " << std::to_string(I) << " " << ProjectFile[I].Name << " ";
            }
            print << "\n";
        }
        print << "\nDump File Flags"_fmt.color(fmt::Yellow).endl();
        for (const auto& [K,V] : ProjectFile) {
            if (V.Flags.empty()){continue;}

            print << "File: " << K << " With Flags: " << V.Flags << "\n";
        }
        print << "\n";
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
    //         print << fmt("recompiling "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         l_rewrite();
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } else if (f_isSystemHeader) {
    //         print << fmt("System Header "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         if(fs::exists(f_module)) {return 0;}
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } else if (!fs::exists(f_module)) {
    //         print << fmt("compiling "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } else if (fs::last_write_time(in_path) > fs::last_write_time(f_module)) {
    //         print << fmt("updated "_fmt.color(fmt::Green) , f_cmd , "\n");
    //         l_rewrite();
    //         ret = cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
    //     } 
    //     ProjectFile[in_path].compiled = (ret == 0 ? true : false);
    //     return ret; 
    // };
#endif