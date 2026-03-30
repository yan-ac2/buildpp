#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <ios>
#include <memory>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <variant>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string_view>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <optional>


namespace  fs = std::filesystem;
using namespace std::string_view_literals;
template<typename... Args> concept onlyStr = (std::convertible_to<Args, std::string_view> && ...);

template<typename T>
concept funcPtr = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

inline class cmdImpl {
    int ret;
    public:
    cmdImpl& run(const char* cmd) {
        ret = std::system(cmd);
        return *this;
    }
    cmdImpl& err(const char* msg) {
        if (ret) {
            std::cerr << msg << "\n";
            std::exit(1);
        }
        return *this;
    }
    cmdImpl& operator <<(const char* cmd) { return run(cmd);}
    cmdImpl& operator >>(const char* cmd) { return err(cmd);}
}cmd;

class fn 
{
    void* _fn;
    public:
    constexpr fn(funcPtr auto&& f) 
    {
        _fn = reinterpret_cast<void*>(f);
    }
    template<typename... Args>
    constexpr fn& run(funcPtr auto&& f, Args&&... args) {
        auto fn = reinterpret_cast<decltype(f)>(_fn);
        fn(std::forward<Args>(args)...);
        return *this;
    }
    constexpr fn& run(funcPtr auto&& f) {
        auto fn = reinterpret_cast<decltype(f)>(_fn);
        fn();
        return *this;
    }
    constexpr fn& operator=(funcPtr auto&& f) {_fn = reinterpret_cast<void*>(f); return *this;}
};

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
    constexpr fmt setColor(fmt::colors color) { 
        str.reserve((color == fmt::Black) ? 13 : 14);
        str.insert(0, this->getColor(color));
        str.append(this->getColor(Not_color));
        return *this;
    }
        
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

constexpr fmt operator""_fmt(const char* str,unsigned long long) { return fmt(str);}
struct fprint
{
    constexpr fprint& endl() {std::cout << std::endl;return *this;}
    constexpr fprint& operator <<(std::string_view in) {

        std::cout << in;
        return *this;
    }
    void operator <<(fprint& f) {f = *this;}
};
struct outputPath {
    private:
    constexpr outputPath& err(bool e = false,std::string_view msg = "") {
        if (e) {std::cerr << msg;throw 0;} 
        return *this;
    }
    public:
    fs::path rootPath   {};
    fs::path outPath    {};
    fs::path objPath    {};
    fs::path modulePath {};
    fs::path exePath    {};

    outputPath& setRootPath(std::string_view root) { rootPath = root;return *this;}
    outputPath& setExePath(std::string_view exe) {
        if(rootPath.empty()) {return err(true,"root path is empty\n");} 
        exePath = rootPath / exe;
        return err();
    }
    void setOutpath(std::string_view out) {
        if(rootPath.empty()) { err(true,"root path is empty\n");}
        outPath = rootPath / out;
        objPath = outPath / "obj";
        modulePath = outPath / "module";
        for (const auto& lm_dir : {outPath,objPath,modulePath})
        {
            if (!lm_dir.has_parent_path()) {
                err(true,fmt("Error: Path has no parent path: " ,lm_dir.string()));
            }
    
            if (!fs::exists(lm_dir)) 
            {
                if (fs::create_directory(lm_dir)) {
                    std::cout << "Directory created: " << lm_dir << std::endl;
                } else {
                    err(true,fmt("Failed to create directory: "));
                }        
            } else {
                std::cout << "Directory already exists: "_fmt.setColor(fmt::Bold_Yellow) << lm_dir << std::endl;
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
    std::map<std::string,std::vector<std::string_view>>   includeMap {};

    cProject(outputPath* path, projectType exe,bool recomp = false) {
        projectOutPath = path;
        outFile = exe;
        recompile = recomp;
        std::cout << "Project initialized at "_fmt.setColor(fmt::Bold_Green) << this->path << std::endl;
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
        std::cout << "add dependency for " << inFile << " with deps: ";
        for (const auto& d : inDeps) {
            std::cout << d << " ";
        }        std::cout << std::endl;
        std::string f_file;
        std::string f_deps; 

        size_t totalSize = 0;
        for (const auto& d : inDeps) {
            totalSize += d.size() + 1; // +1 for space
        }
        
        f_deps.reserve(totalSize);
        for (const auto& d : inDeps) {
            f_deps.append(fmt(" -l" , d , " ").sv());
        }

        for (const auto& mod : project) {
            std::string filename = fs::path(mod).filename().string();
            std::cout << "check file " << filename << " against " << inFile << std::endl;
            if (filename == inFile) {
                f_file = mod;
                break;
            }
        }
        dependency.emplace_back(f_file, f_deps);
    }

    void addInclude(fs::path IncludePath) {
        if (IncludePath.empty()) {
            throw std::invalid_argument("compileIncludePath cannot be empty");
        } 

        if (!fs::exists(IncludePath)) {
            throw std::runtime_error(fmt("Include path ",IncludePath.string(), "does not exist").str);
        }
        includePath.push_back(IncludePath.string());
        compileInclude.append(fmt("-I ",IncludePath.string(), " ").sv());
    }


    void scanInclude() {
        
        for (const auto& p : project) {
            if (p.empty()) {
                std::cerr << "Error: Empty project path" << std::endl;
                continue;
            }
            
            std::ifstream files(p);
            if (!files.is_open()) {
                std::cerr << "Error: Unable to open file " << p << std::endl;
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
                        std::cout << "include found " << includeFound << " in " << p << std::endl;
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
        if (!fs::exists(sourcePath) || !fs::is_directory(sourcePath)) {std::cerr << "Directory does not exist.\n"; return;}
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
        const std::string Executable {fmt(" -o ", fmt((projectOutPath->exePath / target).string(), file.platform).sv(), " ").sv()};
        std::string Object     {fmt(" -fprebuilt-module-path=", (projectOutPath->modulePath / ".").string(), " " ,
            (projectOutPath->objPath / target).string(), file.objFile).sv()};
        
        for (auto& p : object) {
            for (const auto& deps : dependency)
            {
                if (fs::path(deps.first).stem().string() == fs::path(p).stem().string())
                {
                    Object.append(deps.second);   
                }
            }
            Object.append(fmt(" ",p).sv());
        }
        
        std::string cmd {fmt(Compiler,Object,Executable).sv()};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());       
    }

    void compileC(std::string_view inPath)
    {
        fs::path f_path = inPath;
        
        const std::string obj {fmt((projectOutPath->objPath / f_path.stem()).string(), file.objFile).sv()};

        auto f_found = std::find_if(includeMap.begin(), includeMap.end(), [&f_path](const auto& p) 
        {return p.second[0] == f_path.string();}) != includeMap.end();
        
        std::string cmd {fmt(Compiler, f_found ? compileInclude : "" ,fmt(" -c ", inPath, " -o ", obj).sv()).sv()};
        object.push_back(obj);
        if (recompile)
        {
            std::cout << "recompile "_fmt.setColor(fmt::Green) << cmd << std::endl;
            std::system(cmd.c_str());
            return;
        }else if (!fs::exists(obj))
        {
            std::system(cmd.c_str());
            return;
        } else if (fs::last_write_time(f_path) >= fs::last_write_time(obj))
        {
            std::system(cmd.c_str());
            return;
        }
    }

    void dumpProject() {
        std::cout << " dump project" << std::endl;
        for (const auto& p : project) {
            std::cout << p << std::endl;
        }
    }

    void dumpIncludeMap() {
        std::cout << " dump include" << std::endl;
        for (const auto& [key, value] : includeMap) {
            std::cout << "include " << key << " ";
            for (const auto& v : value) {
                std::cout << v << " "; 
            }
            std::cout << std::endl;
        }
    }
    void dumpDependencies()
    {
        std::cout << " dump dependencies " << std::endl;
        for (const auto& dep : dependency)
        {
            std::cout << "file " << dep.first << " depends on " << dep.second << " " << std::endl;
        }

    }

    void dumpObjects() {
        std::cout << " dump Objects" << std::endl;
        for (const auto& p : object) {
            std::cout << p << std::endl;
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
    std::map<std::string_view,std::vector<std::string_view>>   _includeMap {};
    std::map<std::string_view,std::vector<std::string_view>>   _moduleMap  {};

    constexpr Project& setMain(std::string_view main)     {_Main = main; return *this;}
    constexpr Project& setCompiler(std::string_view comp) {_Compiler = fmt(comp," ").sv(); return *this;}
    constexpr Project& setOptions (std::string_view opt)  {_Options = fmt(" ",opt," ").sv(); return *this;}
    constexpr Project& setProjectPath(std::string_view in){_path = in; return *this;}
    constexpr Project& setSourcePath(std::string_view in) {_sourcePath = _path / in; return *this;}
    constexpr Project& addIncludePath(std::string_view in) {_includePath.emplace_back(in); return *this;}
    constexpr Project& addSource(std::initializer_list<std::string_view> in)    {
        _project.reserve(in.size());
        for (const auto& i : in) {
            if (!fs::exists(_path / i)) {std::cerr << "source file "_fmt.setColor(fmt::Red) << i << " does not exist" << std::endl; continue;}
            _project.emplace_back((_path / i).string()); 
        }
        return *this;
    }

    
    constexpr Project(outputPath* path, projectType exe = Project::exe,bool recomp = false) {
        OutPath = path;
        outFile = exe;
        recompile = recomp;
        std::cout << "Project initialized at "_fmt.setColor(fmt::Green) << this->_path << std::endl;
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
        std::cout << "add dependency for " << inFile << " with deps: ";
        for (const auto& d : inDeps) {
            std::cout << d << " ";
        }        std::cout << std::endl;
        std::string f_file;
        std::string f_deps; 

        size_t totalSize = 0;
        for (const auto& d : inDeps) {
            totalSize += d.size() + 1; // +1 for space
        }
        
        f_deps.reserve(totalSize);
        for (const auto& d : inDeps) {
            f_deps.append(fmt(" -l" , d , " ").sv());
        }

        for (const auto& mod : _project) {
            std::string filename = fs::path(mod).filename().string();
            std::cout << "check file " << filename << " against " << inFile << std::endl;
            if (filename == inFile) {
                std::cout << "dependency found for " << inFile << " in " << mod << " imp filename " << filename << std::endl;
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
        
        while (!queue.empty())
        {
            const fs::path f_path {queue.front()};
            const auto modMap = _moduleMap.find(queue.front())->second;
            bool nosub = [&modMap,&mPath,this] -> bool{
                bool ret = true;
                if (!modMap.empty())
                {
                    for (const auto& m : modMap)
                    {
                        // std::cout << "check module "_fmt.setColor(fmt::Bold_Green) << queue.front() << " is exist "_fmt.setColor(fmt::Blue) << fmt((mPath / fs::path(m).stem()).string(), file.pcmModule) 
                        // << " = " << fs::exists(fmt((mPath / fs::path(m).stem()).string(), file.pcmModule).sv()) << std::endl; 
                        if (!fs::exists(fmt((mPath / fs::path(m).stem()).string(), file.pcmModule).sv())) {ret = false;}
                    }
                }
                return ret;
            }();

            if (recompile || nosub)
            {
                //std::cout << "compiling " << queue.front() << std::endl;
                // const bool f_inModule = std::find_if(modules.begin(), modules.end(), [&f_path](const auto& p) 
                // {return p.first == f_path.string();}) != modules.end();
                const bool f_found = std::find_if(_includeMap.begin(), _includeMap.end(), [&f_path](const auto& p) 
                {return p.second[0] == f_path.string();}) != _includeMap.end();
                
                const std::string f_module       {fmt((mPath / f_path.stem()).string(), file.pcmModule).sv()};
                const std::string f_moduleOutput {fmt(" -o ", f_module).sv()};
                const std::string f_srcInput     {fmt(" -fprebuilt-module-path=",(mPath / ".").string()," --precompile ", f_path.string()," ").sv()};

                const std::string f_cmd          {fmt(_Compiler, _Options , f_found ? _compileInclude : "", f_srcInput , f_moduleOutput ).sv()};
                
                //this->object.push_back(f_obj);
                if (recompile) {
                    std::cout << "recompiling "_fmt.setColor(fmt::Green) << f_cmd << std::endl;
                    cmd << f_cmd.c_str() >> "recompile error"_fmt.setColor(fmt::Red);
                    queue.pop();
                    continue;
                } else if (!fs::exists(f_module))
                {
                    std::cout << "compiling "_fmt.setColor(fmt::Green) << f_cmd << std::endl;
                    cmd << f_cmd.c_str() >> "recompile error"_fmt.setColor(fmt::Red);
                    queue.pop();
                    continue;
                } else if (fs::last_write_time(f_path) > fs::last_write_time(f_module))
                {
                    std::cout << "updated "_fmt.setColor(fmt::Green) << f_cmd << std::endl;
                    cmd << f_cmd.c_str() >> "recompile error"_fmt.setColor(fmt::Red);
                    queue.pop();
                    continue;
                }
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
        const std::string f_objOutput {fmt((OutPath->objPath / f_path.filename().stem()).string() , file.objFile).sv()};

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
            fmt(" -fprebuilt-module-path=", OutPath->modulePath.string()).sv()," -c -o ").sv()};

        const std::string f_cmd {fmt(_Compiler, _Options,f_includefound ? _compileInclude : "",f_cppOutput, f_objOutput).sv()};
        
        if (!f_inObject)
        {
            _object.emplace_back(f_objOutput);
        } 

        
        if (recompile) {
            std::cout << "recompiling "_fmt.setColor(fmt::Bold_Green) << f_cmd << std::endl;
            std::system(f_cmd.c_str());
            return;
        } else if (!fs::exists(f_objOutput))
        {
            std::cout << "compiling "_fmt.setColor(fmt::Bold_Green) << f_cmd << std::endl;
            std::system(f_cmd.c_str());
            return;
        } else if (fs::last_write_time(f_path) > fs::last_write_time(f_objOutput))
        {
            std::cout << "updated "_fmt.setColor(fmt::Bold_Green) << f_cmd << std::endl;
            std::system(f_cmd.c_str());
            return;
        }
    }
    
    
    void link(std::string_view target) {
        std::cout << "linking"_fmt.setColor(fmt::Bold_Green) << std::endl;
        const std::string targetOut {fmt((OutPath->exePath / target).string()).sv()};
        const std::string Executable {fmt(" -o ", targetOut, file.platform).sv()};
        std::string Object     {fmt(_modules.empty() ? "" : fmt(" -fprebuilt-module-path=", (OutPath->modulePath / ".").string()).sv(), " " ).sv()};
        if (fs::exists(fmt(targetOut, file.platform).sv())) {
            fs::rename(fmt(targetOut, file.platform).sv(), fmt(targetOut, file.platform, ".old").sv());
        }
        for (const auto& p : _object) {
            for (const auto& deps : _dependency)
            {
                if (fs::path(deps.first).stem().string() == fs::path(p).stem().string())
                {
                    std::cout << "check dependency in " << deps.first << " for " << p << std::endl;
                    Object.append(deps.second);   
                }
            }
            Object.append(fmt(" ",p).sv());
        }

        const std::string cmd {fmt(_Compiler,Object,Executable).sv()};
        std::printf("%s \n",cmd.c_str());
        repeat:
        for (const auto& i : _object)
        {
            if(!fs::exists(i)) { goto repeat;}
        }
        
        std::system(cmd.c_str());       
    }

    Project& addInclude(fs::path IncludePath) {
        if (IncludePath.empty()) {
            throw std::invalid_argument("compileIncludePath cannot be empty");
        } 

        if (!fs::exists(IncludePath)) {
            throw std::runtime_error(fmt("Include path ",IncludePath.string(), "does not exist").str);
        }
        _includePath.push_back(IncludePath.string());
        _compileInclude.append(fmt("-I ",IncludePath.string(), " ").sv());
        return *this;
    }

    void getCppFile() {
        if (!fs::exists(_sourcePath) || !fs::is_directory(_sourcePath)) {std::cerr << "Directory does not exist.\n"; return;}

        fs::directory_iterator iterator(_sourcePath);
        if (_project.empty())
        {
            for (const auto& entry : iterator) {
                if (entry.is_regular_file() && (file.isModule(entry.path().extension().string()) || file.isCpp(entry.path().extension().string()))) {
                    
                    std::cout << "add project file " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
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

                    std::cout << "add include " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    _include.push_back(entry.path().filename().string());
                    //_includeMap[entry.path().filename().string()].emplace_back(entry.path().string());
                }
            }
        }
    }
    

    Project& scanHeader() {
        std::cout << "Scanning Files"_fmt.setColor(fmt::Bold_Green) << std::endl;
        for (const auto& p : _project) {
            std::cout << "scan " << p << " ";
            if (p.empty()) {
                std::cerr << "Error: Empty project path" << std::endl;
                continue;
            }
            
            std::ifstream files(p);
            if (!files.is_open()) {
                std::cerr << "Error: Unable to open file " << p << std::endl;
                continue;
            }

            std::string line;
            std::string includeFound;
            std::string moduleName;

            while (std::getline(files,line)) {
                size_t epos = line.find(file.exportToken);
                if (epos != std::string::npos) {
                    std::cout << " export module found in " << p << std::endl;
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
                    //include.emplace_back(includeFile);
                    includeFound = {fs::path{line.substr(pos + file.includeToken.length())}.filename().string()};
                    std::erase_if(includeFound, [](char c) { return c == '"' || c == '<' || c == '>' || c == ' '; });
                    //std::cout << " include module found in " << p << " " << includeFound << std::endl;
                    
                    for (const auto& map : _include) {
                        if (map == includeFound) {
                            std::cout << "include found "_fmt.setColor(fmt::Bold_Red) << map << " in " << p << std::endl;
                            _includeMap[map] = {p};
                            //_include.emplace_back(includeFound);
                        }
                    }
                    //continue;
                }
                
            }
            files.close();
        }
        // for (auto it = _include.begin(); it != _include.end(); it++) {
        //     const auto& i = *it;
        //     bool found = false;
        //     for (const auto& p : _includeMap) {
        //         if (i != p.first) {found = true;}
        //     }
        //     if (found) { _include.erase(it);}
        // }
        //std::erase_if(_include, [&](const auto& c) {return std::find(_includeMap.begin()->first, _includeMap.end()->first, c) == _includeMap.end()->first; });
        return *this;
    }

    Project& scanModule() {
        for (const auto& p : _project) {
            const auto& name = fs::path(p).filename().stem().string();
            std::cout << "Scan module " << p << std::endl;
            if (p.empty()) {
                std::cerr << "Error: Empty project path" << std::endl;
                continue;
            }

            std::ifstream files(p);
            if (!files.is_open()) {
                std::cerr << "Error: Unable to open file " << p << std::endl;
                continue;
            }

            std::string line;
            while (std::getline(files, line)) {
                size_t ipos = line.find(file.importToken);
                std::string moduleName;
                if (ipos != std::string::npos && line.find('.') != std::string::npos) {
                    moduleName = line.substr(ipos + file.importToken.length());
                    std::cout << "import module "_fmt.setColor(fmt::Bold_Blue) << moduleName << " found in " << p << std::endl;
                    for (const auto& i : _modules)
                    {
                        if (i.second == moduleName)
                        {
                            std::cout << "compare module "_fmt.setColor(fmt::Bold_Red) << i.second << " found in " << std::string_view(p).substr(0,p.find_last_of(".")) << std::endl;
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
        std::cout << "Dump Module Map"_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& [key, value] : _moduleMap) {
            std::cout << "moduleMap (first) " << key << " (second) ";

            for (const auto& v : value) {
                std::cout << v << " ";
            }
            std::cout << std::endl;
        }
        return *this;
    }

    Project& dumpProject() {
        std::cout << " dump project"_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& p : _project) {
            std::cout << p << std::endl;
        }
        return *this;
    }

    Project& dumpInclude()
    {
        std::cout << "dump include"_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& i : _include) {
            std::cout << "include " << i << std::endl;
        }
        return *this;
    }
    Project& dumpModule() {
        std::cout << "dump module"_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& i : _modules) {
            std::cout << "module " << i.first << " module name " << i.second << std::endl;
        }
        return *this;
    }

    Project& dumpIncludeMap() {
        std::cout << "dump include map "_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& [key, value] : _includeMap) {
            std::cout << "include " << key << " ";
            for (const auto& v : value) {
                std::cout << v << " "; 
            }
            std::cout << std::endl;
        }
        return *this;
    }
    Project& dumpDependencies()
    {
        std::cout << "dump dependencies "_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& dep : _dependency)
        {
            std::cout << "file " << dep.first << " depends on " << dep.second << " " << std::endl;
        }
        return *this;
    }

    Project& dumpObjects() {
        std::cout << "dump Objects"_fmt.setColor(fmt::Yellow) << std::endl;
        for (const auto& p : _object) {
            std::cout << p << std::endl;
        }
        return *this;
    }
    
};
