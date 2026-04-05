#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <source_location>


namespace  fs = std::filesystem;
using namespace std::string_view_literals;
template<typename... Args> concept onlyStr = (std::convertible_to<Args, std::string_view> && ...);

template<typename T>
concept funcPtr = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

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

static struct implPrint
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

    outputPath& setRootPath(std::string_view root) { rootPath = root; return *this;}
    outputPath& setExePath(std::string_view exe) {
        if(rootPath.empty()) {return err(true,"root path is empty");} 
        exePath = rootPath / exe;
        return err();
    }
    void setOutpath(std::string_view out) {
        if(rootPath.empty()) { err(true,"root path is empty");}
        outPath = rootPath / out;
        objPath = outPath / "obj";
        modulePath = outPath / "module";
        for (const auto& lm_dir : {outPath,objPath,modulePath})
        {
            if (!lm_dir.has_parent_path()) {
                err(true,fmt("Error: Path has no parent path: " ,lm_dir.string()," "));
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
            if (i == file) {return true; break;}
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
        recompile = recomp;
        print << fmt("Project initialized at "_fmt.color(fmt::Bold_Green) , this->path.string());
    };

    void setMain(std::string_view main)         {Main = main;}
    void setCompiler(std::string_view comp)     {Compiler = fmt(comp, " ").sv();}
    void setOptions (std::string_view opt)      {Options = fmt(opt, " ").sv();}
    void setProjectPath(std::string_view in)    {path = in;}
    void setSourcePath(std::string_view in)     {sourcePath = path / in;}
    void setSource(std::initializer_list<std::string_view> in) {
        project.reserve(in.size());
        for (const auto& i : in) {
            project.push_back((sourcePath / i).string());
        }
    }

    void addDependency(std::string_view inFile, std::initializer_list<std::string_view> inDeps){
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
            print << fmt("check file " , filename , " against " , inFile , "\n");
            if (filename == inFile) {
                f_file = mod;
                break;
            }
        }
        dependency.emplace_back(f_file, f_deps);
    }

    void addInclude(fs::path IncludePath) {
        if (IncludePath.empty()) {err(true,"compileIncludePath cannot be empty");} 

        if (!fs::exists(IncludePath)) {err(true,fmt("Include path ",IncludePath.string(), "does not exist").cstr());}

        includePath.push_back(IncludePath.string());
        compileInclude.append(fmt("-I ",IncludePath.string(), " ").sv());
    }


    void scanInclude() {
        
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
        
        std::erase_if(includeMap, [&](const auto& c) { 
            return std::find(include.begin(), include.end(), c.first) == include.end(); });
    }

    void getCFile() {
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
    }

    void link(std::string_view target) {
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
    }

    void compileC(std::string_view inPath)
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

        const std::string f_cmd {fmt(Compiler, f_found ? compileInclude : "" ,fmt(" -c ", inPath, " -o ", f_obj))};
        object.push_back(f_obj);
        if (recompile)
        {
            print << fmt("recompile "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
            return;
        }else if (!fs::exists(f_obj))
        {
            print << fmt("compile "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "compile error "_fmt.color(fmt::Red);
            return;
        } else if (fs::last_write_time(f_path) >= fs::last_write_time(f_obj))
        {
            print << fmt("updating "_fmt.color(fmt::Green) , f_cmd , "\n");
            cmd << f_cmd.c_str() >> "recompile error "_fmt.color(fmt::Red);
            return;
        }
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
    outputPath* OutPath;
    const fileExtension file;
    std::string _Main            {};
    std::string _Options         {};
    std::string _Compiler        {};
    std::string _compileInclude  {};

    constexpr Project& err(bool e = false,std::string_view msg = "",std::string fn = std::source_location::current().function_name()) {
        if (e) {print << fmt (msg , "at" , __FILE__ , "\n"); std::exit(1);} 
        return *this;
    }
    public:
    bool recompile;
    
    enum projectType : char {
        exe,
        dynamicLib
    } outFile;

    
    fs::path _path       {};
    fs::path _sourcePath {};
    std::vector<std::string> _includePath {};
    std::vector<std::string> _project     {};
    std::vector<std::string> _object      {};
    std::vector<std::string> _include     {};
    std::vector<std::pair<std::string, std::string>> _dependency {};
    std::vector<std::pair<std::string, std::string>> _modules    {};
    std::unordered_map<std::string_view,std::vector<std::string_view>>   _includeMap {};
    std::unordered_map<std::string_view,std::vector<std::string_view>>   _moduleMap  {};

    constexpr Project& setMain(std::string_view main)     {_Main = main; return *this;}
    constexpr Project& setCompiler(std::string_view comp) {_Compiler = fmt(comp," ").sv(); return *this;}
    constexpr Project& setOptions (std::string_view opt)  {_Options = fmt(" ",opt," ").sv(); return *this;}
    constexpr Project& setProjectPath(std::string_view in){_path = in; return *this;}
    constexpr Project& setSourcePath(std::string_view in) {_sourcePath = _path / in; return *this;}
    constexpr Project& addIncludePath(std::string_view in) {_includePath.emplace_back(in); return *this;}
    constexpr Project& addSource(std::initializer_list<std::string_view> in)    {
        _project.reserve(in.size());
        for (const auto& i : in) {
            if (!fs::exists(_path / i)) { err (true,fmt("source file "_fmt.color(fmt::Red) , i , " does not exist" ));}
            _project.emplace_back((_path / i).string()); 
        }
        return *this;
    }

    
    constexpr Project(outputPath* path, projectType exe = Project::exe,bool recomp = false) {
        OutPath = path;
        outFile = exe;
        recompile = recomp;
        print << fmt("Project initialized at "_fmt.color(fmt::Green) , this->_path.string(),"\n" );
    };

    Project& getLib(cProject* cProj)
    {
        if (cProj->outFile == cProject::staticLib) {
            if (cProj->object.size() > 1) {
                this->_object.reserve(cProj->object.size());
            }
            if (cProj->dependency.size() > 1) {
                this->_dependency.reserve(cProj->dependency.size());
            }
            for (const auto& obj : cProj->object) {
                this->_object.emplace_back(obj);
            }
            for (const auto& dep : cProj->dependency) {
                this->_dependency.emplace_back(dep);
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

        size_t totalSize = 0;
        for (const auto& d : inDeps) {
            totalSize += d.size() + 1; // +1 for space
        }
        
        f_deps.reserve(totalSize);
        for (const auto& d : inDeps) {
            f_deps.append(fmt(" -l" , d , " "));
        }

        for (const auto& mod : _project) {
            std::string filename = fs::path(mod).filename().string();
            print << fmt("check file " , filename , " against " , inFile , "\n");
            if (filename == inFile) {
                print << fmt("dependency found for " , inFile , " in " , mod , " imp filename " , filename , "\n");
                f_file = mod;
                break;
            }
        }
        _dependency.emplace_back(f_file, f_deps);
        return *this;
    }

    void compileModule() {
        std::queue<std::string> queue;
        const auto& mPath = OutPath->modulePath;
        for (const auto& i : _modules) {queue.push(i.first);}
        
        for (;!queue.empty();)
        {
            const fs::path f_path {queue.front()};
            const auto modMap = _moduleMap.find(queue.front())->second;
            bool nosub = [&modMap,&mPath,this] -> bool{

                if (!modMap.empty())
                {
                    for (const auto& m : modMap)
                    {
                        // std::cout << "check module "_fmt.color(fmt::Bold_Green) << queue.front() << " is exist "_fmt.color(fmt::Blue) << fmt((mPath / fs::path(m).stem()).string(), file.pcmModule) 
                        // << " = " << fs::exists(fmt((mPath / fs::path(m).stem()).string(), file.pcmModule).sv()) << std::endl; 
                        if (!fs::exists(fmt((mPath / fs::path(m).stem()).string(), file.pcmModule).sv())) {return false;}
                    }
                }
                return true;
            }();

            if (nosub)
            {
                // const bool f_inModule = std::find_if(modules.begin(), modules.end(), [&f_path](const auto& p) 
                // {return p.first == f_path.string();}) != modules.end();
                // const bool f_found = std::find_if(_includeMap.begin(), _includeMap.end(), [&f_path](const auto& p) 
                // {return p.second[0] == f_path.string();}) != _includeMap.end();
                
                const bool f_found = [this,&f_path](){
                    for (const auto& i : _includeMap)
                    {
                        for (const auto& m : i.second) {
                            if (m == f_path.string()) return true;
                        }
                    }
                    return false;
                }();

                const std::string f_module       {fmt((mPath / f_path.stem()).string(), file.pcmModule)};
                const std::string f_moduleOutput {fmt(" -o ", f_module)};
                const std::string f_srcInput     {fmt(" -fprebuilt-module-path=",(mPath / ".").string()," --precompile ", f_path.string()," ")};

                const std::string f_cmd          {fmt(_Compiler, _Options , f_found ? _compileInclude : "", f_srcInput , f_moduleOutput )};
                
                //this->object.push_back(f_obj);
                if (recompile) {
                    print << fmt("recompiling "_fmt.color(fmt::Green) , f_cmd , "\n");
                    cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
                    queue.pop();
                    continue;
                } else if (!fs::exists(f_module))
                {
                    print << fmt("compiling "_fmt.color(fmt::Green) , f_cmd , "\n");
                    cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
                    queue.pop();
                    continue;
                } else if (fs::last_write_time(f_path) > fs::last_write_time(f_module))
                {
                    print << fmt("updated "_fmt.color(fmt::Green) , f_cmd , "\n");
                    cmd << f_cmd.c_str() >> "recompile error"_fmt.color(fmt::Red);
                    queue.pop();
                    continue;
                }
                // std::cout << "compiling " << f_path << std::endl;
                queue.pop();
            } 
            else
            {
                //std::cout << "requeue " << queue.front() << std::endl;
                queue.push(queue.front());
                queue.pop();
            }
        }
        
    };

    void compileCpp(std::string_view inPath)
    {
        const fs::path f_path = inPath;
        const std::string f_objOutput {fmt((OutPath->objPath / f_path.filename().stem()).string() , file.objFile)};

        const bool f_includefound = std::find_if(_includeMap.begin(), _includeMap.end(), [&f_path](const auto& p) 
        {return p.second[0] == f_path.string();}) != _includeMap.end();
        const bool f_inObject = [&f_objOutput,this]() {
            for (const auto& obj : _object) {
                if (obj == f_objOutput) {
                    return true;
                    break;
                }
            }
            return false;
        }();
            
        const std::string f_cppOutput {fmt( " ",f_path.string(), _modules.empty() ? "" : 
            fmt(" -fprebuilt-module-path=", OutPath->modulePath.string()).sv()," -c -o ")};

        const std::string f_cmd {fmt(_Compiler, _Options,f_includefound ? _compileInclude : "",f_cppOutput, f_objOutput)};
        
        if (!f_inObject)
        {
            _object.emplace_back(f_objOutput);
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
        const std::string targetOut {fmt((OutPath->exePath / target).string())};
        const std::string Executable {fmt(" -o ", targetOut, file.platform)};
        std::string Object     {fmt(_modules.empty() ? "" : fmt(" -fprebuilt-module-path=", (OutPath->modulePath / ".").string()), " " )};
        if (fs::exists(fmt(targetOut, file.platform).sv())) {
            fs::rename(fmt(targetOut, file.platform).sv(), fmt(targetOut, file.platform, ".old").sv());
        }
        for (const auto& p : _object) {
            for (const auto& deps : _dependency)
            {
                if (fs::path(deps.first).stem().string() == fs::path(p).stem().string())
                {
                    print << fmt("check dependency in " , deps.first , " for " , p).endl();
                    Object.append(deps.second);   
                }
            }
            Object.append(fmt(" ",p).sv());
        }

        const std::string f_cmd {fmt(_Compiler,Object,Executable)};
        print << f_cmd << "\n";

        cmd << f_cmd.c_str();
    }

    Project& addInclude(fs::path IncludePath) {
        if (IncludePath.empty())         { err(true,"IncludePath cannot be empty"_fmt.color(fmt::Bold_Red));} 

        if (!fs::exists(IncludePath)) { err(true,fmt("Include path ",IncludePath.string(), "does not exist").color(fmt::Bold_Red));}
        
        _includePath.push_back(IncludePath.string());
        _compileInclude.append(fmt("-I ",IncludePath.string(), " "));
        return *this;
    }

    void getCppFile() {
        if (!fs::exists(_sourcePath) || !fs::is_directory(_sourcePath)) {err (true, "Directory does not exist."_fmt.color(fmt::Bold_Red));}

        fs::directory_iterator iterator(_sourcePath);
        if (_project.empty())
        {
            for (const auto& entry : iterator) {
                if (entry.is_regular_file() && (file.isModule(entry.path().extension().string()) || file.isCpp(entry.path().extension().string()))) {
                    
                    print << fmt("add project file " , entry.path().filename().string() , " " , entry.path().string()).endl();
                    _project.emplace_back(entry.path().string());
                }
            }
            if (_project.size() > 1) {
                _object.reserve(_project.size());
                _modules.reserve(_project.size());
            }
        }
        for (const auto& includeScan : _includePath) {
            fs::directory_iterator iterator2(includeScan);
            for (const auto& entry : iterator2) {
                if (entry.is_regular_file() && file.isCppHeader(entry.path().extension().string()) ) {

                    print << fmt("add include " , entry.path().filename().string() , " " , entry.path().string()).endl();
                    _include.push_back(entry.path().filename().string());
                    
                }
            }
        }
    }
    

    Project& scanHeader() {
        print << "Scanning Files"_fmt.color(fmt::Bold_Green).endl();
        for (const auto& p : _project) {
            print << "scan " << p << " ";
            if (p.empty()) {err(true,"Error: Empty project path"_fmt.color(fmt::Bold_Red));}
            
            std::ifstream files(p);
            if (!files.is_open()) {err (true,fmt("Error: Unable to open file "_fmt.color(fmt::Bold_Red) , p));}

            std::string line;
            std::string includeFound;
            std::string moduleName;

            while (std::getline(files,line)) {
                size_t epos = line.find(file.exportToken);
                if (epos != std::string::npos) {
                    print << fmt(" export module found in " , p ).endl();
                    moduleName = line.substr(epos + file.exportToken.length());
                    _moduleMap[p];
                    _modules.push_back ({p, moduleName});
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
                    
                    for (const auto& map : _include) {
                        if (map == includeFound) {
                            print << fmt("include found "_fmt.color(fmt::Blue) , map , " in " , p);
                            _includeMap[map] = {p};
                            
                        }
                    }
                }
                
            }
            files.close();
        }

        return *this;
    }

    Project& scanModule() {
        for (const auto& p : _project) {
            const auto& name = fs::path(p).filename().stem().string();
            print << fmt("Scan module " , p ).endl();
            if (p.empty())        {err(true ,"Error: Empty project path"); }

            std::ifstream files(p);
            if (!files.is_open()) {err(true,"Error: Unable to open file "_fmt.color(fmt::Bold_Red) , p);}

            std::string line;
            while (std::getline(files, line)) {
                size_t ipos = line.find(file.importToken);
                std::string moduleName;
                if (ipos != std::string::npos && line.find('.') != std::string::npos) {
                    moduleName = line.substr(ipos + file.importToken.length());
                    print << fmt("import module "_fmt.color(fmt::Bold_Blue) , moduleName , " found in " , p).endl();
                    for (const auto& i : _modules)
                    {
                        if (i.second == moduleName)
                        {
                            print << fmt("compare module "_fmt.color(fmt::Bold_Cyan) , i.second , " found in " , std::string_view(p).substr(0,p.find_last_of("."))).endl();
                            _moduleMap[p].push_back(std::string_view(i.first).substr(p.find(name)));
                        }
                    }
                }
            }
            files.close();
        }
        return *this;
    }

    Project& dumpModuleMap() {
        print << "Dump Module Map"_fmt.color(fmt::Yellow).endl();
        for (const auto& [key, value] : _moduleMap) {
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
        for (const auto& p : _project) {
            print << p << "\n";
        }
        return *this;
    }

    Project& dumpInclude()
    {
        print << "dump include"_fmt.color(fmt::Yellow).endl();
        for (const auto& i : _include) {
            print << "include " << i << "\n";
        }
        return *this;
    }
    Project& dumpModule() {
        print << "dump module"_fmt.color(fmt::Yellow).endl();
        for (const auto& i : _modules) {
            print << "module " << i.first << " module name " << i.second << "\n";
        }
        return *this;
    }

    Project& dumpIncludeMap() {
        print << "dump include map "_fmt.color(fmt::Yellow).endl();
        for (const auto& [key, value] : _includeMap) {
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
        for (const auto& dep : _dependency)
        {
            print << fmt("file " , dep.first , " depends on " , dep.second).endl();
        }
        return *this;
    }

    Project& dumpObjects() {
        print << "dump Objects"_fmt.color(fmt::Yellow).endl();
        for (const auto& p : _object) {
            print << p << "\n";
        }
        return *this;
    }
    
};
