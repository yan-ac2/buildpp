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
#include <queue>
#include <string_view>
#include <unordered_map>
#include <source_location>

#include "json.hpp"

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

inline class cmdImpl {
    public:
    int ret;
    cmdImpl& run(const char* cmd) {
        ret = std::system(cmd);
        return *this;
    }
    cmdImpl& err(const char* msg) {
        if (ret) {
            print << msg << "\n";
            std::exit(1);
        }
        return *this;
    }
    cmdImpl& operator <<(const char* cmd) { return run(cmd);}
    cmdImpl& operator >>(const char* cmd) { return err(cmd);}
}cmd;

template<typename F> 
class defer
{
    F _fn;
    public:
    //template<typename... Args>
    defer(F&& f) : _fn(std::forward<F>(f)) {}
    ~defer() {
        _fn(); 
    }
};

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

struct fileExtension
{
    static constexpr std::string platform {
        #ifdef _WIN32 
        ".exe"
        #elif defined(__unix__) 
        ".out"
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
    static constexpr std::string_view libFile      {".a"};
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


class cProject{
    outputPath* projectOutPath;
    
    std::string Main            {};
    std::string Options         {};
    std::string Compiler        {};
    std::string compileInclude  {};

    constexpr cProject& err(bool isError = false,std::string_view msg = "",std::string_view f = std::source_location().current().function_name()) {
        if (isError) { print << fmt(msg , " at " , f); std::exit(1); } 
        return *this;
    }
    static fileExtension file;
    public:
    bool recompile,singleFile;
    enum projectType : int {
        exe,
        staticLib,
        dynamicLib
    } outFile;
    compileCommand* cmdJson;
    
    fs::path path       {};
    fs::path sourcePath {};
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
        print << fmt("Project initialized at "_fmt.color(fmt::Bold_Green) , this->path.string());
    };

    cProject& setMain(std::string_view main)         {Main = main; return *this;}
    cProject& setCompiler(std::string_view comp)     {Compiler = fmt(comp, " ").sv();return *this;}
    cProject& setOptions (std::string_view opt)      {Options = fmt(opt, " ").sv();return *this;}
    cProject& setProjectPath(fs::path in)            {path = in;return *this;}
    cProject& setSourcePath(fs::path in)             {sourcePath = path / in;return *this;}
    cProject& setSource(std::initializer_list<std::string_view> in) {
        project.reserve(in.size());
        for (const auto& i : in) {
            project.push_back((sourcePath / i).string());
        }
        return *this;
    }
    cProject& addCompileCommand(compileCommand* cmd)
    {
        cmdJson = cmd;
        return *this;
    }

    cProject& addDependency(std::string_view inFile, std::initializer_list<std::string_view> inDeps){
        print << fmt("add dependency for " , inFile , " with deps: ");
        for (const auto& d : inDeps) { print << fmt(d , " "); }
        print << "\n";
        std::string f_file;
        std::string f_deps; 

        size_t totalSize = 0;
        for (const auto& d : inDeps) {
            totalSize += d.size() + 1; // +1 for space
        }
        
        f_deps.reserve(totalSize);
        for (const auto& d : inDeps) {
            f_deps.append(fmt(" -l" , d , " "));
        }

        for (const auto& mod : project) {
            const std::string filename = fs::path(mod).filename().string();
            if (filename == inFile) {
                print << fmt("check file " , filename , " against " , inFile , "\n");
                f_file = mod;
                break;
            }
        }
        dependency.emplace_back(f_file, f_deps);
        return *this;
    }

    cProject& addInclude(fs::path IncludePath) {
        if (IncludePath.empty()) {err(true,"compileIncludePath cannot be empty");} 

        if (!fs::exists(IncludePath)) {err(true,fmt("Include path ",IncludePath.string(), "does not exist").cstr());}

        includePath.push_back(IncludePath.string());
        compileInclude.append(fmt("-I ",IncludePath.string(), " ").sv());
        return *this;
    }


    cProject& scanInclude() {
        
        for (const auto& p : project) {
            if (p.empty()) {
                print << "Error: Empty project path \n";
                continue;
            }
            
            std::ifstream files(p);
            if (!files.is_open()) {
                print << fmt("Error: Unable to open file " , p , "\n");
                continue;
            }

            std::string line;
            std::string includeFound;
            
            while (std::getline(files, line)) {
                size_t pos = line.find(file.includeToken);
                if (pos == std::string::npos) {
                    //include.emplace_back(includeFile);
                    continue;
                }
                includeFound = {fs::path{line.substr(pos + file.includeToken.length())}.filename().string()};
                std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });
                for (const auto& map : includeMap) {
                    if (map.first == includeFound) {
                        print << fmt("include found " , includeFound , " in " , p , "\n");
                        includeMap[includeFound] = {p};
                        include.emplace_back(includeFound);
                    }
                }
            }
            files.close();
        }
        
        std::erase_if(includeMap, [&](const auto& c) {return std::find(include.begin(), include.end(), c.first) == include.end(); });
        return *this;
    }

    cProject& getCFile() {
        if (!fs::exists(sourcePath) || !fs::is_directory(sourcePath)) {err(true, "Directory does not exist.\n");}
        fs::directory_iterator iterator(sourcePath);
        for (const auto& entry : iterator) {
            if (entry.is_regular_file() && (entry.path().extension() == file.cSource)) {
                // std::cout << "add project file " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                project.emplace_back(entry.path().string());
            }
        }

        for (const auto& includeScan : includePath) {
            fs::recursive_directory_iterator iterator2(includeScan);
            for (const auto& entry : iterator2) {
                if (entry.is_regular_file() && (entry.path().extension() == file.cHeader)) {
                    //std::cout << "add include " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    includeMap[entry.path().filename().string()].emplace_back(entry.path().string());
                }
            }
        }
        return *this;
    }

    cProject& compileC(std::string_view inPath)
    {
        const fs::path f_path = inPath;
        
        const std::string f_obj {fmt((projectOutPath->objPath / f_path.stem()).string(), file.objFile)};

        // auto f_found = std::find_if(includeMap.begin(), includeMap.end(), [&f_path](const auto& p) 
        // {return p.second[0] == f_path.string();}) != includeMap.end();
        const bool f_found = [this,&f_path](){
                    for (const auto& i : includeMap)
                    {
                        for (const auto& m : i.second) {
                            if (m == f_path.string()) return true;
                        }
                    }
                    return false;
                }();

        const std::string f_cmd {fmt(Compiler, f_found ? compileInclude : "" ,fmt(" -c ", inPath, " -o ", f_obj)).clean()};
        if(cmdJson != nullptr) cmdJson->addCompilecmd(f_path.parent_path().string(), f_cmd, f_path.string(), f_obj);
        object.push_back(f_obj);
        if (recompile)
        {
            print << fmt("recompile "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
            return *this;
        }else if (!fs::exists(f_obj))
        {
            print << fmt("compile "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "compile error "_fmt.color(fmt::Red);
            return *this;
        } else if (fs::last_write_time(f_path) >= fs::last_write_time(f_obj))
        {
            print << fmt("updating "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
            return *this;
        }
        return *this;
    }

    cProject& link(std::string_view target) {
        const std::string f_exe {fmt(" -o ", fmt((projectOutPath->exePath / target).string(), file.platform), " ")};
        std::string f_object     {fmt(" -fprebuilt-module-path=", (projectOutPath->modulePath / ".").string(), " " ,
            (projectOutPath->objPath / target).string(), file.objFile)};
        
        for (auto& p : object) {
            for (const auto& deps : dependency)
            {
                if (fs::path(deps.first).stem().string() == fs::path(p).stem().string())
                {
                    f_object.append(deps.second);   
                }
            }
            f_object.append(fmt(" ",p));
        }
        
        const std::string f_cmd {fmt(Compiler,f_object,f_exe)};
        print << f_cmd << "\n";
        cmd << f_cmd.c_str(); 
        return *this;      
    }

    void dumpProject() {
        print << " dump project \n";
        for (const auto& p : project) {
            print << p << "\n";
        }
    }

    void dumpIncludeMap() {
        print << " dump include \n";
        for (const auto& [key, value] : includeMap) {
            print << fmt("include " , key , " ");
            for (const auto& v : value) {
                print << v << " "; 
            }
            print << "\n";
        }
    }
    void dumpDependencies()
    {
        print << " dump dependencies \n" ;
        for (const auto& dep : dependency)
        {
            print << fmt("file " , dep.first , " depends on " , dep.second , " \n");
        }

    }

    void dumpObjects() {
        print << " dump Objects\n";
        for (const auto& p : object) {
            print << p << "\n";
        }
    }
    
};


class Project
{
    std::string Main            {};
    std::string Options         {};
    std::string Compiler        {};
    std::string compileInclude  {};
    
    constexpr Project& err(bool e = false,std::string_view msg = "",std::string fn = std::source_location::current().file_name()) {
        if (e) {print << fmt (msg , " at " , fn , "\n"); std::exit(1);} 
        return *this;
    }
    bool recompile;
    const fileExtension file;
    public:
    enum projectType : char {
        exe,
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
    std::vector<fs::path> SourcePath     {};
    std::vector<fs::path> IncludePath    {};
    std::vector<std::string> ProjectFile {};
    std::vector<std::string> Object      {};
    std::vector<std::string> Include     {};
    std::vector<std::pair<std::string, std::string>> Dependency {};
    std::vector<std::pair<std::string, std::string>> Modules    {};
    std::unordered_map<std::string_view,std::vector<std::string_view>>   IncludeMap {};
    std::unordered_map<std::string_view,std::vector<std::string_view>>   ModuleMap  {};
    std::unordered_map<std::string,std::string> ModuleDeps;

    constexpr Project& setMain        (std::string_view main) {Main = main; return *this;}
    constexpr Project& setCompiler    (std::string_view comp) {Compiler = comp; return *this;}
    constexpr Project& setOptions     (std::string_view opt)  {Options = fmt(" ",opt," ").sv(); return *this;}
    constexpr Project& setProjectPath (fs::path in)           {Path = in; return *this;}
    constexpr Project& addSourcePath  (fs::path in)           {SourcePath.emplace_back(in); return *this;}
    constexpr Project& addIncludePath (fs::path in)           {IncludePath.emplace_back(in); return *this;}
    constexpr Project& setincludeas   (enum includeas opt)    {includeas = opt; return *this;}
    
    constexpr Project& addSource(std::initializer_list<std::string_view> in) {
        ProjectFile.reserve(in.size());
        for (const auto& i : in) {
            if (!fs::exists(Path / i)) { err (true,fmt("source file "_fmt.color(fmt::Red) , i , " does not exist" ));}
            ProjectFile.emplace_back((Path / i).string()); 
        }
        return *this;
    }

    constexpr Project(outputPath* path,bool recomp = false, projectType exe = Project::exe) {
        OutPath = path;
        outFile = exe;
        cmdJson = nullptr;
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
            if (cProj->object.size() > 1) {
                this->Object.reserve(cProj->object.size());
            }
            if (cProj->dependency.size() > 1) {
                this->Dependency.reserve(cProj->dependency.size());
            }
            for (const auto& obj : cProj->object) {
                this->Object.emplace_back(obj);
            }
            for (const auto& dep : cProj->dependency) {
                this->Dependency.emplace_back(dep);
            }
        }
        return *this;
    }

    Project& addDependency(const std::string& inFile, std::initializer_list<std::string_view> inDeps){
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

        for (const auto& mod : ProjectFile) {
            std::string filename = fs::path(mod).filename().string();
            //print << fmt("check file " , filename , " against " , inFile , "\n");
            if (filename == inFile) {
                print << fmt("dependency found for " , inFile , " in " , mod , " imp filename " , filename , "\n");
                f_file = mod;
                break;
            }
        }
        Dependency.emplace_back(f_file, f_deps);
        return *this;
    }

    Project& addIncludefile(fs::path inPath) {
        // if (IncludePath.empty())         { err(true,"IncludePath cannot be empty"_fmt.color(fmt::Bold_Red));} 

        if (!fs::exists(inPath)) { err(true,fmt("Include path ",inPath.string(), "does not exist ").color(fmt::Bold_Red));}
        
        IncludePath.push_back(inPath);
        compileInclude.append(fmt("-I",inPath.string()," "));
        return *this;
    }

    void getCppFile() {
        if (recompile) {
            for(const auto& d : fs::directory_iterator(OutPath->modulePath))
            fs::remove_all(d.path());
        }
        for (const auto& p : SourcePath)
        {
            if (!fs::exists(p) || !fs::is_directory(p)) {err (true, fmt("Directory does not exist. "_fmt.color(fmt::Bold_Red),p.string()));}
    
            fs::directory_iterator iterator(p);
            if (ProjectFile.empty())
            {
                for (const auto& entry : iterator) {
                    if (entry.is_regular_file() && (file.isModule(entry.path().extension().string()) || file.isCpp(entry.path().extension().string()))) {
                        
                        print << fmt("add project file " , entry.path().filename().string() , " " , entry.path().string()).endl();
                        ProjectFile.emplace_back(entry.path().string());
                    }
                }
                // if (ProjectFile.size() > 1) {
                //     Object.reserve(ProjectFile.size());
                //     Modules.reserve(ProjectFile.size());
                // }
            }
            for (const auto& includeScan : IncludePath) {
                fs::directory_iterator iterator2(includeScan);
                for (const auto& entry : iterator2) {
                    if (entry.is_regular_file() && file.isCppHeader(entry.path().extension().string()) ) {
    
                        print << fmt("add include " , entry.path().filename().string() , " " , entry.path().string()).endl();
                        Include.push_back(entry.path().filename().string());
                        
                    }
                }
            }
        }
    }
    

    Project& scanHeader() {
        print << "Scanning Files"_fmt.color(fmt::Bold_Green).endl();
        for (const auto& p : ProjectFile) {
            print << "scan " << p << "\n";
            if (p.empty()) {err(true,"Error: Empty project path"_fmt.color(fmt::Bold_Red));}
            
            std::ifstream files(p);
            if (!files.is_open()) {err (true,fmt("Error: Unable to open file "_fmt.color(fmt::Bold_Red) , p));}

            std::string line;
            std::string includeFound;
            std::string moduleName;

            while (std::getline(files,line)) {
                size_t epos = line.find(file.exportToken);
                if (epos != std::string::npos) {
                    moduleName = line.substr(epos + file.exportToken.length() + 1);
                    moduleName.erase(moduleName.find(';'));
                    print << fmt("export module "_fmt.color(fmt::Yellow), moduleName ," found in " , p ).endl();
                    ModuleMap[p];
                    Modules.push_back ({p, moduleName});
                    break; // only one export module per file is allowed 
                }
            }
            
            files.clear();
            files.seekg(0);

            while (std::getline(files, line)) {
                size_t pos = line.find(file.includeToken);
                if (pos != std::string::npos) {
                    
                    includeFound = {fs::path{line.substr(pos + file.includeToken.length())}.filename().string()};
                    std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });

                    // if (file.isSystemHeader(includeFound)) {
                    //     SystemHeader.push_back(includeFound);
                    //     Modules.push_back ({includeFound,includeFound});
                    // }
                    
                    for (const auto& map : Include) {
                        if (map == includeFound) {
                            print << fmt("include found "_fmt.color(fmt::Blue) , map , " in " , p);
                            IncludeMap[map] = {p};
                        }
                    }
                }
                
            }
            files.close();
        }

        return *this;
    }

    Project& scanModule() {
        for (const auto& p : ProjectFile) {
            const auto& name = fs::path(p).filename().stem().string();
            print << fmt("Scan module " , p ).endl();
            if (p.empty())        {err(true ,"Error: Empty project path"); }

            std::ifstream files(p);
            if (!files.is_open()) {err(true,"Error: Unable to open file "_fmt.color(fmt::Bold_Red) , p);}

            std::string line;
            while (std::getline(files, line)) {
                size_t ipos = line.find(file.importToken);
                if (ipos != std::string::npos && line.find(';') != std::string::npos) {
                    std::string moduleName = line.substr(ipos + file.importToken.length() + 1);
                    moduleName.erase(moduleName.find(';'));
                    print << fmt("import module "_fmt.color(fmt::Bold_Blue) , moduleName , " found in " , p).endl();
                    
                    // for (const auto& i : SystemHeader) {
                    //     // print << fmt(moduleName.substr(1,moduleName.find('>')-1)).endl(); 
                    //     if (moduleName.substr(1,moduleName.find('>')-1) == i)
                    //     ModuleMap[p].push_back(i);
                    // }

                    for (const auto& i : Modules)
                    {
                        if (i.second == moduleName)
                        {
                            print << fmt("compare module "_fmt.color(fmt::Bold_Cyan) , i.second , " found in " , std::string_view(p)).endl();
                            ModuleMap[p].push_back(i.second);
                        }
                    }
                }
            }
            files.close();
        }
        return *this;
    }
    
    bool isModuleExist(std::string_view infile)
    {
        const auto& mPath = OutPath->modulePath;
        const fs::path f_path {infile};
        bool exist = [&mPath,&infile,&f_path,this] -> bool{
            
            const auto modMap = ModuleMap.find(infile)->second;
            if (!ModuleMap.empty())
            {
                for (const auto& m : modMap)
                {
                    // print << fmt("check module "_fmt.color(fmt::Bold_Green) , f_path.string() , " is exist "_fmt.color(fmt::Blue) , fmt((mPath / fs::path(m).stem()).string(), file.pcmModule) 
                    //).endl().cstr(); 
                    if (!fs::exists(fmt((mPath / m).string(), file.pcmModule).sv())) {
                        return false;
                    }
                    if(fs::last_write_time(f_path) > fs::last_write_time(fmt((mPath / m).string(),file.pcmModule).sv()) && recompile)
                    {
                        return false;
                    }
                }
            }
            return true;
        }();
        return exist;
    }

    void compileModule(fs::path f_path) {
        
        // const bool f_inModule = std::find_if(modules.begin(), modules.end(), [&f_path](const auto& p) 
        // {return p.first == f_path.string();}) != modules.end();
        // const bool f_found = std::find_if(_includeMap.begin(), _includeMap.end(), [&f_path](const auto& p) 
        // {return p.second[0] == f_path.string();}) != _includeMap.end();
        const auto* mPath = &OutPath->modulePath;
        const auto* oPath = &OutPath->objPath;
        // print << fmt("is system header ",f_path.string()).color(fmt::Blue).endl();
        const std::string f_module {fmt((*mPath / f_path.stem()).string(), file.pcmModule)};
        const std::string f_objOutput {fmt((*oPath / f_path.stem()).string(), file.objFile)};
        
        const bool f_found = [this,&f_path](){
            for (const auto& i : IncludeMap)
            {
                for (const auto& m : i.second) {
                    if (m == f_path.string()) return true;
                }
            }
            return false;
        }();
        const bool f_inObject = [&f_objOutput,this]() {
            for (const auto& obj : Object) {
                if (obj == f_objOutput) {
                    return true;
                    break;
                }
            } return false;
        }();
        // std::string f_moduleOutput {fmt(" -o ", f_objOutput)};
        std::string f_srcInput     {fmt(f_path.string()," -c -fmodule-output=",f_module," -fmodules-reduced-bmi -fprebuilt-module-path=",(*mPath / ". ").string())};
        
        for (const auto& [mod , dep] : ModuleMap)
        {
            if (fs::path(mod).filename().string() == f_path.filename().string()) {
                for(const auto& d : dep)
                {
                    ModuleDeps[f_path.filename().string()].append(fmt(" -fmodule-file=",d,"=",(*mPath / d).string(),file.pcmModule," "));
                }
            }
        }
        
        if (!f_inObject)
        {
            Object.emplace_back(f_objOutput);
        } 
        const std::string f_cmd {fmt(Compiler, Options ,f_srcInput ,f_found ? compileInclude : "",ModuleDeps[f_path.filename().string()]," -o ",f_objOutput).clean()};
        if(cmdJson != nullptr) { cmdJson->addCompilecmd(f_path.parent_path().string(),f_cmd,f_path.string().c_str(),f_objOutput);}
        
        if (recompile) {
            print << fmt("recompiling "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } else if (!fs::exists(f_module))
        {
            print << fmt("compiling "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        } else if (fs::last_write_time(f_path) > fs::last_write_time(f_module))
        {
            print << fmt("updated "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
        }
    };

    void compileCpp(std::string_view inPath)
    {
        const auto* oPath = &OutPath->objPath;
        const auto* mPath = &OutPath->modulePath;
        const fs::path    f_path = inPath;
        const std::string f_objOutput {fmt((*oPath / f_path.filename().stem()).string(), file.objFile)};
        const bool f_isModule = file.isModule(f_path.extension().string());
        if (f_isModule) return;
        const std::string f_filein    {f_isModule ? fmt((*mPath / f_path.filename().stem()).string(),file.pcmModule ) : f_path.string()};
        // print << fmt("is module " ,f_isModule ? "true ": "false ",f_path.extension().string()).color(fmt::Bold_Red).endl();
        const bool f_includefound = std::find_if(IncludeMap.begin(), IncludeMap.end(), [&f_path](const auto& p) 
        {return p.second[0] == f_path.string();}) != IncludeMap.end();
        
        const bool f_inObject = [&f_objOutput,this]() {
            for (const auto& obj : Object) {
                if (obj == f_objOutput) {
                    return true;
                    break;
                }
            } return false;
        }();
        const bool f_inModuledep = [&f_isModule,&f_path,&mPath,this](){
            if (f_isModule) return true;
            for (const auto& [mod , dep] : ModuleMap) {
                    if (fs::path(mod).filename().string() == f_path.filename().string()) {
                        for(const auto& d : dep)
                        {
                            ModuleDeps[f_path.filename().string()].append(fmt(" -fmodule-file=",d,"=",(*mPath / d).string(),file.pcmModule," "));
                        }
                        return true;
                    }
                // print << fmt("have modules ",f_path.filename().string(),m.first).endl(); 
            }
            return false;
        }();
            
        const std::string f_cppOutput {fmt(f_isModule ?"":"-c ",f_filein, Modules.empty() ? "" : 
            fmt(" -fprebuilt-module-path=", (*mPath / ".").string()," "))};
            
        const std::string f_cmd {fmt(Compiler, Options ,f_cppOutput,f_includefound ? compileInclude : "",f_inModuledep ? ModuleDeps[f_path.filename().string()] : "",f_isModule?" -c -o ":" -o ", f_objOutput).clean()};
        
        if(cmdJson != nullptr && !f_isModule) { cmdJson->addCompilecmd(f_path.parent_path().string(),f_cmd,f_path.string().c_str(),f_objOutput);}
        // if(cmdJson != nullptr) { cmdJson->addCompilecmd(f_cmd);}
        
        if (!f_inObject)
        {
            Object.emplace_back(f_objOutput);
        } 

        
        if (recompile) {
            print << fmt("recompiling "_fmt.color(fmt::Bold_Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "error compiling "_fmt.color(fmt::Bold_Red);
            return;
        } else if (!fs::exists(f_objOutput))
        {
            print << fmt("compiling "_fmt.color(fmt::Bold_Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "error compiling "_fmt.color(fmt::Bold_Red);
            return;
        } else if (fs::last_write_time(f_path) > fs::last_write_time(f_objOutput))
        {
            print << fmt("updated "_fmt.color(fmt::Bold_Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "error compiling "_fmt.color(fmt::Bold_Red);
            return;
        }
    }
    
    
    void link(std::string_view target) {
        print << fmt("linking"_fmt.color(fmt::Bold_Green).endl());
        const std::string f_targetOut {fmt((OutPath->exePath / target).string())};
        const std::string f_Executable {fmt(" -o ", f_targetOut, file.platform)};
        std::string f_Object     {fmt(Modules.empty() ? "" : fmt(" -fprebuilt-module-path=", (OutPath->modulePath / ".").string()))};
        if (fs::exists(fmt(f_targetOut, file.platform).sv())) {
            fs::rename(fmt(f_targetOut, file.platform).str, fmt(f_targetOut, file.platform, ".old").str);
        }
        for (const auto& p : Object) {
            for (const auto& deps : Dependency)
            {
                if (fs::path(deps.first).stem().string() == fs::path(p).stem().string())
                {
                    print << fmt("check dependency in " , deps.first , " for " , p).endl();
                    f_Object.append(deps.second);   
                }
            }
            f_Object.append(fmt(" ",p).sv());
        }

        const std::string f_cmd {fmt(Compiler,f_Object,f_Executable).clean()};
        print << f_cmd << "\n";

        
        cmd << f_cmd.c_str() >> "linking error"_fmt.color(fmt::Bold_Red);
    }

    

    Project& dumpModuleMap() {
        print << "Dump Module Map"_fmt.color(fmt::Yellow).endl();
        for (const auto& [key, value] : ModuleMap) {
            print << "moduleMap (first) " << key << " (second) ";

            for (const auto& v : value) {
                print << v << " ";
            }
            print << "\n";
        }
        return *this;
    }

    Project& dumpProject() {
        print << " dump project"_fmt.color(fmt::Yellow).endl();
        for (const auto& p : ProjectFile) {
            print << p << "\n";
        }
        return *this;
    }

    Project& dumpInclude()
    {
        print << "dump include"_fmt.color(fmt::Yellow).endl();
        for (const auto& i : Include) {
            print << "include " << i << "\n";
        }
        return *this;
    }
    Project& dumpModule() {
        print << "dump module"_fmt.color(fmt::Yellow).endl();
        for (const auto& i : Modules) {
            print << "module " << i.first << " module name " << i.second << "\n";
        }
        return *this;
    }

    Project& dumpIncludeMap() {
        print << "dump include map "_fmt.color(fmt::Yellow).endl();
        for (const auto& [key, value] : IncludeMap) {
            print << "include " << key << " ";
            for (const auto& v : value) {
                print << v << " "; 
            }
            print << "\n";
        }
        return *this;
    }
    Project& dumpDependencies()
    {
        print << "dump dependencies "_fmt.color(fmt::Yellow).endl();
        for (const auto& dep : Dependency)
        {
            print << fmt("file " , dep.first , " depends on " , dep.second).endl();
        }
        return *this;
    }

    Project& dumpObjects() {
        print << "dump Objects"_fmt.color(fmt::Yellow).endl();
        for (const auto& p : Object) {
            print << p << "\n";
        }
        return *this;
    }
    // Project& dumpSysHeader() {
    //     print << "dump SysHeader"_fmt.color(fmt::Yellow).endl();
    //     for (const auto& p : SystemHeader) {
    //         print << p << "\n";
    //     }
    //     return *this;
    // }
    
};

#endif