#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <ostream>
#include <ranges>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string_view>
#include <map>


namespace  fs = std::filesystem;

template<typename... Args> concept onlyStr = (std::convertible_to<Args, std::string_view> && ...);

enum {Not_color,
  Black,    Bold_Black,   High_Black,
  Red,      Bold_Red,     High_Red,
  Green,    Bold_Green,   High_Green,
  Yellow,   Bold_Yellow,  High_Yellow,
  Blue,     Bold_Blue,    High_Blue,
  Purple,   Bold_Purple,  High_Purple,
  Cyan,     Bold_Cyan,    High_Cyan,
  White,    Bold_White,   High_White,
};

/**
 * A table which associate each color
 * with a representation code
 */
std::map<int, std::string> color = {
  {Not_color,     "\033[0m"   },
  {Black,         "\033[0;0m" },
  {Red,           "\033[0;31m"},
  {Green,         "\033[0;32m"},
  {Yellow,        "\033[0;33m"},
  {Blue,          "\033[0;34m"},
  {Purple,        "\033[0;35m"},
  {Cyan,          "\033[0;36m"},
  {White,         "\033[0;37m"},
  {Bold_Black,    "\033[1;30m"},
  {Bold_Red,      "\033[1;31m"},
  {Bold_Green,    "\033[1;32m"},
  {Bold_Yellow,   "\033[1;33m"},
  {Bold_Blue,     "\033[1;34m"},
  {Bold_Purple,   "\033[1;35m"},
  {Bold_Cyan,     "\033[1;36m"},
  {White,         "\033[1;37m"},
  {High_Black,    "\033[0;90m"},
  {High_Red,      "\033[0;91m"},
  {High_Green,    "\033[0;92m"},
  {High_Yellow,   "\033[0;93m"},
  {High_Blue,     "\033[0;94m"},
  {High_Purple,   "\033[0;95m"},
  {High_Cyan,     "\033[0;96m"},
  {High_White,    "\033[0;97m"},
};



std::string fmt(onlyStr auto&&... args) {
    const size_t totalSize = [](auto&&... args) -> size_t {
        size_t size = 0;
        ((size += std::string_view(args).size()), ...);
        return size;
    }(args...);
    std::string temp {};
    temp.reserve(totalSize);

    (temp.append(std::forward<decltype(args)>(args)), ...);
    return temp; 
};

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(queue_mutex_);
                        cv_.wait(lock, [this] { return !tasks_.empty() || stop_; });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    template<typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(f));
        }
        cv_.notify_one();
    }
    int isEmpty() {return tasks_.empty();}
    private:
    std::vector<std::jthread> threads_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

template<typename F>
struct defer {
    F f;
    defer(F&& f) : f(f) {}
    ~defer() { f(); }
};

struct ProjectPath {
    fs::path path       {};
    fs::path sourcePath {};
    fs::path outPath    {};
    fs::path objPath    {};
    fs::path modulePath {};
    fs::path exePath    {};

    void setProjectPath(std::string_view in) {path = in;}
    void setSourcePath(std::string_view in) {sourcePath = this->path / in;}
    void setExePath(std::string_view exe) {exePath = this->path / exe;}
    void setOutpath(std::string_view out) {
        outPath = this->path / out;
        objPath = outPath / "obj";
        modulePath = outPath / "module";
        for (const auto& lm_dir : {outPath,objPath,modulePath})
        {
            if (!lm_dir.has_parent_path()) {
                std::cerr << "Error: Path has no parent path: " << lm_dir.string() << "\n";
                return;
            }
    
            if (!fs::exists(lm_dir)) {
                try {
                    if (fs::create_directory(lm_dir)) {
                    std::cout << "Directory created: " << lm_dir << std::endl;
                    } else {
                        std::cerr << "Failed to create directory: " << lm_dir << std::endl;
                    }
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Filesystem error: " << e.what() << std::endl;
                }
            } else {
                std::cout << "Directory already exists: " << lm_dir << std::endl;
            }
        }
    }
};

class Project
{
    ProjectPath* projectPath;
    std::string platform {
    [this]{switch (outFile)
        {
            case exe:
                #ifdef _WIN32 
                return ".exe";
                #elif defined(__unix__) 
                return ".out";
                #endif
            case dynamicLib: return ".so";
            default: return "";
        }
    }()};
    std::string Main            {};
    std::string Options         {"-std=c++23 "};
    std::string Compiler        {"clang++ "};
    std::string compileInclude  {};
    public:
    enum fileType : int {
        exe,
        dynamicLib
    } outFile;

    
    
    std::vector<std::string> includePath {};
    std::vector<std::string> project     {};
    std::vector<std::string> object      {};
    std::vector<std::string> include     {};
    std::vector<std::pair<std::string, std::string>> dependency {};
    std::vector<std::pair<std::string, std::string>> modules    {};
    std::map<std::string,std::vector<std::string>>   includeMap {};
    std::map<std::string,std::vector<std::string>>   moduleMap  {};

    void clearModules() {modules.clear();}
    void setMain(std::string_view main) {Main = main;}
    
    Project(ProjectPath* path, fileType exe) {
        projectPath = path;
        outFile = exe;
        std::cout << "Project initialized at " << projectPath->path.string() << std::endl;
    };
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
            f_deps.append(fmt(" -l" , d , " "));
        }

        for (const auto& mod : project) {
            std::string filename = fs::path(mod).filename().string();
            std::cout << "check file " << filename << " against " << inFile << std::endl;
            if (filename == inFile) {
                std::cout << "dependency found for " << inFile << " in " << mod << " imp filename " << filename << std::endl;
                f_file = mod;
                break;
            }
        }
        dependency.emplace_back(f_file, f_deps);
    }
    void preCompile(std::string_view inpath) {
        fs::path f_path = inpath;

        auto f_found = std::find_if(includeMap.begin(), includeMap.end(), [&f_path](const auto& p) 
        {return p.second[0] == f_path.string();}) != includeMap.end();
        bool f_inModule = std::find_if(modules.begin(), modules.end(), [&f_path](const auto& p) 
        {return p.first == f_path.string();}) != modules.end();
    
        std::string f_objOutput; 
        std::string f_srcInput; 
        std::string f_cmd;

        const std::string f_module      {fmt((projectPath->modulePath / f_path.stem()).string(), ".pcm")};
        const std::string f_obj         {fmt((projectPath->objPath / f_path.stem()).string(), ".o")};
        // const std::string f_hasDeps =  [&f_path,this](const auto& p){
        // for (const auto& dep : dependency) {
        //     if (p == dep.first) {//std::cout << "this path " << path.string() << " deps " << dep.first << " " << dep.second << std::endl;
        //         return dep.second;
        //     }
        // } return std::string{};}
        // (f_path.string());

        f_objOutput.append(fmt(" -o ", f_module));
        f_srcInput.append (fmt(" -fprebuilt-module-path=",(projectPath->modulePath / ".").string()," --precompile ", f_path.string()," "));
        
        f_cmd = {fmt(Compiler, f_found ? compileInclude : "",Options, f_srcInput , f_objOutput )};
        
        this->object.push_back(f_obj);
        std::printf("%s \n",f_cmd.c_str());
        std::system(f_cmd.c_str());
    };

    
    void compileModule(std::string_view inpath) {
        fs::path path = inpath;
        
        const std::string module      {fmt((projectPath->modulePath / path.stem()).string() , ".pcm")};
        const std::string obj         {fmt((projectPath->objPath / path.stem()).string(), ".o")};
        const std::string objOutput   {fmt(" -fprebuilt-module-path=" ,this->projectPath->modulePath.string(), " -o ", obj)};
        const std::string moduleInput {fmt(" -c ",  module ,"  ")};
    
        std::string cmd {fmt(Compiler, Options,moduleInput, objOutput)};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
    };
    void compileMain()
    {
        const fs::path findMain = [this]{
            for (const auto& p : project) {
                if (p.find(Main) != std::string::npos) {
                    return fs::path(p);
                }
            }
            return fs::path{};
        }();
        std::string mainOutput {fmt(findMain.string()," -fprebuilt-module-path=", (projectPath->modulePath / ".").string(),
                                " -c -o ", (projectPath->objPath / findMain.filename().stem()).string() , ".o ")};
        
        std::string cmd {fmt(Compiler, Options,mainOutput)};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
        
        mainOutput = {fmt(findMain.string()," -fprebuilt-module-path=", (projectPath->modulePath / ".").string()," -c -o ", (projectPath->modulePath / findMain.filename().stem()).string() , ".pcm ")};
        cmd = {fmt(Compiler, Options,mainOutput)};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
    }
    void compileC(std::string_view inPath)
    {
        fs::path f_path = inPath;

        auto f_found = std::find_if(includeMap.begin(), includeMap.end(), [&f_path](const auto& p) 
        {return p.second[0] == f_path.string();}) != includeMap.end();
        const std::string obj {fmt((projectPath->objPath / f_path.stem()).string(), ".o")};
        
        std::string cmd {fmt("clang ",f_found ? compileInclude : "" ,fmt(" -c ", inPath, " -o ", obj))};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
    }
    
    void link(std::string_view target) {
        std::string Executable {fmt(" -o ", fmt((projectPath->exePath / target).string(), platform), " ")};
        std::string Object     {fmt(" -fprebuilt-module-path=", (projectPath->modulePath / ".").string(), " " ,
                                (projectPath->objPath / target).string(), ".o ")};
        
        for (auto& p : object) {
            for (const auto& deps : dependency)
            {
                if (fs::path(deps.first).stem().string() == fs::path(p).stem().string())
                {
                    std::cout << "check dependency " << deps.first << " against " << p << std::endl;
                    Object.append(deps.second);   
                }
            }
            Object.append(fmt(" ",p));
        }
        std::string cmd; 
        
        cmd = {fmt(Compiler,Object,Executable)};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());       
    }

    void addInclude(fs::path IncludePath) {
        if (IncludePath.empty()) {
            throw std::invalid_argument("compileIncludePath cannot be empty");
        } 

        if (!fs::exists(IncludePath)) {
            throw std::runtime_error(fmt("Include path ",IncludePath.string(), "does not exist"));
        }
        includePath.push_back(IncludePath.string());
        compileInclude.append(fmt("-I ",IncludePath.string(), " "));
    }

    void getCppFile() {
        if (!fs::exists(projectPath->sourcePath) || !fs::is_directory(projectPath->sourcePath)) {std::cerr << "Directory does not exist.\n"; return;}

        fs::directory_iterator iterator(projectPath->sourcePath);
        for (const auto& entry : iterator) {
            if (entry.is_regular_file()) {
                if (entry.path().extension() == ".ccm" || entry.path().extension() == ".cc") {
                    std::cout << "add project file " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    project.emplace_back(entry.path().string());
                }
            }
        }
        object.reserve  (project.size());
        modules.reserve (project.size());
        for (const auto& includeScan : includePath) {
            fs::recursive_directory_iterator iterator2(includeScan);
            for (const auto& entry : iterator2) {
                if (entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
                    //std::cout << "add include " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    includeMap[entry.path().filename().string()].emplace_back(entry.path().string());
                }
            }
        }
    }
    void getCFile() {
        if (!fs::exists(projectPath->sourcePath) || !fs::is_directory(projectPath->sourcePath)) {std::cerr << "Directory does not exist.\n"; return;}

        fs::directory_iterator iterator(projectPath->sourcePath);
        for (const auto& entry : iterator) {
            if (entry.is_regular_file()) {
                if (entry.path().extension() == ".c") {
                    std::cout << "add project file " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    project.emplace_back(entry.path().string());
                }
            }
        }
        object.reserve  (project.size());
        modules.reserve (project.size());
        for (const auto& includeScan : includePath) {
            fs::recursive_directory_iterator iterator2(includeScan);
            for (const auto& entry : iterator2) {
                if (entry.is_regular_file() && (entry.path().extension() == ".h")) {
                    //std::cout << "add include " << entry.path().filename().string() << " " << entry.path().string() << std::endl;
                    includeMap[entry.path().filename().string()].emplace_back(entry.path().string());
                }
            }
        }
    }

    void scanInclude() {
        const std::string_view importToken    {"import"};
        const std::string_view includeToken   {"#include"};
        const std::string_view exportToken    {"export module"};
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
                size_t pos = line.find(includeToken);
                if (pos == std::string::npos) {
                    //include.emplace_back(includeFile);
                    continue;
                }
                includeFound = {fs::path{line.substr(pos + includeToken.length())}.filename().string()};
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

    void scanModule() {
        const std::string_view importToken  {"import"};
        const std::string_view includeToken {"#include"};
        const std::string_view exportToken  {"export module"};
        for (const auto& p : project) {
            std::cout << "scan module " << p;
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
                size_t ipos = line.find(importToken);
                size_t epos = line.find(exportToken);
                std::string moduleName;
                if (ipos != std::string::npos && line.find('.') != std::string::npos) {
                    std::cout << " import module found in " << p << std::endl;
                    moduleName = line.substr(ipos + importToken.length());
                    moduleMap[moduleName].push_back(p);
                    //compile->modules.emplace_back(p,moduleName);
                }
                if (epos != std::string::npos) {
                    std::cout << " export module found in " << p << std::endl;
                    moduleName = line.substr(epos + exportToken.length());
                    moduleMap[moduleName].push_back(p);
                    modules.push_back ({p, moduleName});
                }
            }
            files.close();
        }
    }

    
    
};

class depScanner
{
    Project* compile;

    public:
    void operator=(Project* p) {
        compile = p;
    }

    

    void reorderModules()
    {
        auto& modules = compile->modules;
        
        // 1. Setup tracking structures (Optimized: O(1) lookups)
        std::unordered_map<std::string, size_t> inDegree;
        std::unordered_map<std::string, std::vector<std::string>> dependents;

        // Initialize all modules in the provided list
        for (const auto& [key, value] : modules) {
                inDegree[value] += 0; // Number of imports (dependencies)
        }
        
        for (const auto& [key, value] : compile->moduleMap) {
            std::cout << "module map key " << key << " "; 
            for (const auto& dep : value) {
                std::cout << " depends on " << dep << " "; 
                dependents[dep].push_back(key);
            }
            std::cout << std::endl;
        }

        // 2. Calculate in-degree for each module
        for (const auto& [first, second] : dependents) {
            for (const auto& dep : second) {
                inDegree[dep]++;
            }
        }
        
        // 2. Queue modules with 0 dependencies
        std::queue<std::string> q;
        {
            auto maxValue = [](const auto& in) {int ret = 0; for (auto const& value : in) {if (value.second > ret) {ret = value.second;}} return ret;}(inDegree);
            for (int temp = 0; temp <= maxValue; temp++) {
                for (const auto& [name, degree] : inDegree) {
                    
                    if (degree == maxValue - temp) {
                        q.push(name);
                        std::cout << "temp " << temp << " max " << maxValue - temp << " name " << name << " degree " << degree << std::endl;
                        continue;
                    }
                }
            }
        }
        for (std::queue<std::string> temp = q; !temp.empty(); temp.pop()) {
            std::cout << "queue contains: " << temp.front() << std::endl;
        }

        std::vector<std::pair<std::string, std::string>> sortedOrder;
        while (!q.empty()) {
            std::string u = q.front();
            std::string moduleFile = [&u](auto& m) {
                for (const auto& [key, val] : m) {
                    if (val == u) {
                        return key;
                        }
                } return std::string{};
            }(modules);
            std::cout << "Processing module: " << u << " module " << moduleFile << std::endl;
            q.pop();
            sortedOrder.push_back({moduleFile, u});
        }
        compile->modules.clear();
        std::cout << "--- Optimized Compilation Order ---" << std::endl;
        for (const auto& sort : sortedOrder) {
            std::cout << sort.first << " " << sort.second << " " << std::endl;
            compile->modules.push_back ({sort.first, sort.second}); // Reorder modules based on sorted order
        }

        // for (const auto& [key, value] : dependents) {
        //     std::cout << "Dependents first " << key << ", Dependents second: ";
        //     for (const auto& d : value) {
        //         std::cout << d << " ";
        //     }
        //     std::cout << std::endl;
        // }
        
        
        // for (const auto& mod : inDegree) {
        //     std::cout << "indegree first: " << mod.first << ", In-Degree second: " << mod.second << std::endl;
        // }
    }
    
    void dumpModuleMap() {
        std::cout << "Dump Module Map" << std::endl;
        for (const auto& [key, value] : compile->moduleMap) {
            std::cout << "moduleMap (first) " << key << " (second) ";

            for (const auto& v : value) {
                std::cout << v << " ";
            }
            std::cout << std::endl;
        }
    }

    void dumpIncludeMap() {
        std::cout << "Dump Include Map" << std::endl;
        for (const auto& [key, value] : compile->includeMap) {
            std::cout << "include " << key << " ";
            for (const auto& v : value) {
                std::cout << v << " ";
            }
            std::cout << std::endl;
        }
    }

    void dumpProject() {
        std::cout << " dump project" << std::endl;
        for (const auto& p : compile->project) {
            std::cout << p << std::endl;
        }
    }
    void dumpModule() {
        std::cout << " dump module" << std::endl;
        for (const auto& i : compile->modules) {
            std::cout << "module " << i.first << " module name " << i.second << std::endl;
        }
    }
    void dumpInclude() {
        std::cout << " dump include" << std::endl;
        for (const auto& [key, value] : compile->includeMap) {
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
        for (const auto& dep : compile->dependency)
        {
            std::cout << "file " << dep.first << " depends on " << dep.second << " " << std::endl;
        }

    }

};

auto main(int argc, const char** argv) -> int 
{
    ThreadPool pool(std::thread::hardware_concurrency());
    const fs::path rootPath = ".";
    depScanner scanner;
    ProjectPath GLADPath;
    GLADPath.setProjectPath((rootPath /"source"/"lib"/"glad").c_str());
    GLADPath.setExePath("");
    GLADPath.setSourcePath("src");
    GLADPath.setOutpath("_build");

    ProjectPath mainPath;
    mainPath.setProjectPath(".");
    mainPath.setExePath("");
    mainPath.setSourcePath("source");
    mainPath.setOutpath("_build");

    Project libGLAD(&GLADPath, Project::exe);
    libGLAD.setMain((fs::path("stc") / "glad.c").c_str());
    libGLAD.addInclude(GLADPath.path / "include");
    libGLAD.getCFile();
    libGLAD.scanInclude();
    libGLAD.addDependency("lib/glad.c", {"GL"});

    Project mainProj(&mainPath, Project::exe);
    mainProj.setMain("main.cc");

    //mainProj.addInclude(rootPath / "usr" / "include" / "X11" );
    mainProj.addInclude(mainPath.sourcePath  / "lib" / "RGFW");
    mainProj.addInclude(mainPath.sourcePath  / "lib" / "glad" / "include");
    mainProj.getCppFile();
    mainProj.scanInclude();
    mainProj.scanModule();
    mainProj.addDependency("lib.RGFW.ccm",{"GL", "X11", "Xrandr"});
    // mainProj.dumpProject();
    scanner = &mainProj;
    scanner.reorderModules();
    scanner.dumpModule();
    scanner.dumpDependencies();
    for (const auto& i : libGLAD.project) {
        libGLAD.compileC(i);
    }
    //std::cout << std::thread::hardware_concurrency() << "\n";
    // for (auto& i : mainProj.modules) {
    //     mainProj.preCompile(i.first);
    // }
    // pool.enqueue ([&mainProj]{mainProj.compileMain();});
    // for (auto& i : mainProj.modules) {
    //     pool.enqueue([&i,&mainProj]{mainProj.compileModule(i.first);});
    // }
    // while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    // std::cout << "link" << std::endl;
    // mainProj.link("main");
    return 0;
};