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

auto fmt(onlyStr auto&&... args) -> std::string  {
    std::string temp;
    
    // Calculate total size using a fold expression and string_view
    size_t totalSize = 0;
    ((totalSize += std::string_view(args).size()), ...);
    
    // Perform exactly ONE allocation
    temp.reserve(totalSize);

    // Append all arguments using a fold expression
    (temp.append(std::forward<decltype(args)>(args)), ...);
    return temp; // Return a pointer to the first element of this; 
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

struct Platform {
    std::string platform {};
    enum _os
    {
        Windows = 1, 
        Linux = 2
    }os ;
    Platform(_os p) {
        platform = [&p]() -> std::string {
            switch (p)
            {
                case Platform::Windows:
                    return ".exe";
                    break;
                default:
                    return "";
            };
            
        }();
    }
    inline std::string_view get() {return platform;}
};

class Project
{
    public:
    std::string_view Compiler {"clang++ "};
    std::string Options {"-std=c++23 "};
    fs::path path       { fs::current_path()};
    fs::path sourcePath {path / "source"};
    fs::path outPath    {path / "build"};
    fs::path objPath    {outPath / "obj"};
    fs::path modulePath {outPath / "module"};
    std::string compileInclude {};
    std::vector<std::string> includePath {};
    
    std::vector<std::string> project {};
    std::vector<std::string> object {};
    std::vector<std::string> modules {};
    std::map<std::string,std::vector<std::string>> includeMap {};

    void preCompile(std::string_view inpath,std::string_view outpath) {
        fs::path path = inpath;
        fs::path out = outpath;
    
        std::string module {(out / "module" / path.stem()).string() + ".pcm"};
        std::string obj {(out / "obj" / path.stem()).string() + ".o"};
        std::string objOutput; 
        std::string srcInput; 
        bool inModule = std::find_if(modules.begin(), modules.end(), [&path](const auto& p) 
        {return p == path.string();}) != modules.end();
        if (inModule && path.filename().extension() == ".ccm") {
            objOutput.append(fmt(" -o ", module));
            srcInput.append(fmt(" --precompile ", path.string()," "));
        } else {
            objOutput.append(fmt(" -fprebuilt-module-path=",(fs::path("build") / "module" / "").string(), " --precompile -o ", module));
            srcInput.append(fmt("  ",  path.string(), " "));
        }
        auto found = std::find_if(includeMap.begin(), includeMap.end(), [&path](const auto& p) 
        {return p.second[0] == path.string();}) != includeMap.end();
        //std::cout << "\x1b[32m path \x1b[0m" << path.string() << "\x1b[32m found \x1b[0m" << found << std::endl;
        
        std::string cmd {fmt(Compiler, Options, srcInput ,found ? compileInclude : "", objOutput )};
        this->object.push_back(obj);
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
        
        objOutput = fmt(" -o ", module);
        srcInput = fmt(" --precompile ", module," ");
        cmd = fmt(Compiler, Options, srcInput ,found ? compileInclude : "", objOutput );
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
    };

    void compile(std::string_view inpath,std::string_view outpath) {
        fs::path path = inpath;
        fs::path out = outpath;
    
        std::string module {(out / "module" / path.stem()).string() + ".pcm"};
        std::string obj {(out / "obj" / path.stem()).string() + ".o"};
        std::string objOutput; 
        std::string srcInput; 
        bool inModule = std::find_if(modules.begin(), modules.end(), [&path](const auto& p) 
        {return p == path.string();}) != modules.end();
        if (inModule) {
            objOutput.append(fmt(" --precompile -o ", module));
            srcInput.append(fmt(" -c ", path.string()," "));
        } else {
            objOutput.append(fmt(" -fprebuilt-module-path=",(out / "module" / ".").string(), " -o ", obj));
            srcInput.append(fmt(" -c ",  path.string()));
        }
        auto found = std::find_if(includeMap.begin(), includeMap.end(), [&path](const auto& p) 
        {return p.second[0] == path.string();}) != includeMap.end();
        //std::cout << "\x1b[32m path \x1b[0m" << path.string() << "\x1b[32m found \x1b[0m" << found << std::endl;
        
        std::string cmd {fmt(Compiler, Options, srcInput ,found ? compileInclude : "", objOutput )};
        this->object.push_back(obj);
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
    };
    
    void compileModule(std::string_view inpath,std::string_view outpath) {
        fs::path path = inpath;
        fs::path dir = outpath;
        
        std::string module {(dir / "module" / path.stem()).string() + ".pcm"};
        std::string obj {(dir / "obj" / path.stem()).string() + ".o"};
        std::string objOutput {fmt(" -fprebuilt-module-path=" ,(fs::path("build")/"module").string(), " -o ", obj)};
        std::string moduleInput {fmt(" -c ",  module ,"  ")};
        auto found = std::find_if(includeMap.begin(), includeMap.end(), [&path](const auto& p) 
        {return p.second[0] == path.string();}) != includeMap.end();
    
        std::string cmd {fmt(Compiler, Options,moduleInput, found ? compileInclude : "", objOutput)};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());
    };
    
    void link(std::vector<std::string>& path,std::string_view target, Platform platform) {
        std::string Object;
        for (auto& p : path) {
            Object.append(fmt(" ", p));
        }
        std::string Executable {fmt(" -o ", fmt((outPath / target).string(), platform.get()), " ")};
        
        std::string cmd {fmt(Compiler, Options,Executable, Object)};
        std::printf("%s \n",cmd.c_str());
        std::system(cmd.c_str());       
    }

    void addInclude(fs::path compileIncludePath) {
        if (compileIncludePath.empty()) {
            throw std::invalid_argument("compileIncludePath cannot be empty");
        }
        fs::path compileIncludePathStr = compileIncludePath;
        if (!compileIncludePathStr.is_absolute()) {
            compileIncludePathStr = path / compileIncludePathStr;
        }
        if (!fs::exists(compileIncludePathStr)) {
            throw std::runtime_error(fmt("compileInclude path ",compileIncludePathStr.string(), "does not exist"));
        }
        includePath.push_back(compileIncludePathStr.string());
        compileInclude.append(fmt("-I ",compileIncludePathStr.string(), " "));
    }

    void getCppFile() {
        if (!fs::exists(sourcePath) || !fs::is_directory(sourcePath)) {std::cerr << "Directory does not exist.\n"; return;}

        fs::directory_iterator iterator(sourcePath);
        for (const auto& entry : iterator) {
            if (entry.is_regular_file()) {
                if (entry.path().extension() == ".ccm" || entry.path().extension() == ".cc") {
                    project.emplace_back(entry.path().string());
                }
            }
        }
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

    void dumpProject() {
        std::cout << "dump project" << std::endl;
        for (const auto& p : project) {
            std::cout << p << std::endl;
        }
    }
    void dumpModule() {
        std::cout << "dump module" << std::endl;
        for (const auto& i : modules) {
            std::cout << "module " << i << std::endl;
        }
    }
    void dumpInclude() {
        std::cout << "dump include" << std::endl;
        for (const auto& [key, value] : includeMap) {
            std::cout << "include " << key << " ";
            for (const auto& v : value) {
                std::cout << v << " "; 
            }
            std::cout << std::endl;
        }
    }
    
};

class dependencyScanner
{
    Project* compile;
    std::string_view importToken {"import"};
    std::string_view includeToken {"#include"};
    std::string_view exportToken {"export module"};
    std::vector<std::string> include {};
    std::map<std::string,std::vector<std::string>> moduleMap {};

    
    public:
    dependencyScanner(Project* p) {
        compile = p;
    }

    void scanInclude() {
        for (const auto& p : compile->project) {
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
                for (const auto& map : compile->includeMap) {
                    if (map.first == includeFound) {
                        std::cout << "include found " << includeFound << " in " << p << std::endl;
                        compile->includeMap[includeFound] = {p};
                        include.emplace_back(includeFound);
                    }
                }
            }
            files.close();
        }
        
        std::erase_if(compile->includeMap, [&](const auto& c) { 
            return std::find(include.begin(), include.end(), c.first) == include.end(); });
    }

    void scanModule() {
        for (const auto& p : compile->project) {
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
                size_t pos = line.find(importToken);
                std::string moduleName;
                if (pos != std::string::npos && line.find(';') != std::string::npos) {
                    moduleName = line.substr(pos + importToken.length());
                    moduleMap[moduleName].push_back(p);
                }
                
                pos = line.find(exportToken);
                if (pos != std::string::npos && line.find(';') != std::string::npos)
                {
                    compile->modules.push_back(p);
                }
                if (pos != std::string::npos && line.find(';') == std::string::npos) {
                    moduleName = line.substr(pos + exportToken.length());
                    moduleMap[moduleName].push_back(p);
                    compile->modules.push_back(p);
                }
            }
            files.close();
        }
        // for (const auto& mod : moduleMap) {
        //     compile->modules.push_back(mod.second[0]);
        // }
    }

    void reorderModules()
    {
        auto& modules = compile->modules;
        
        // 1. Setup tracking structures (Optimized: O(1) lookups)
        std::unordered_map<std::string, int> inDegree;
        std::unordered_map<std::string, std::vector<std::string>> dependents;

        // Initialize all modules in the provided list with 0 dependencies
        for (const auto& mod : modules) {
            inDegree[mod] = 0;
        }

        // 2. Build the dependency graph
        for (const auto& mod : modules) {
            auto it = moduleMap.find(mod);
            if (it != moduleMap.end()) {
                for (const auto& dep : it->second) {
                    if (inDegree.find(dep) != inDegree.end()) {
                        dependents[dep].push_back(mod); 
                        inDegree[mod]++;                
                    }
                }
            }
        }

        // Save a copy of the original dependency counts before we start decrementing them
        std::unordered_map<std::string, int> originalInDegree = inDegree;

        // Create a custom comparator for our priority queue.
        // We want the module with the HIGHEST original dependency count to go first.
        auto cmp = [&originalInDegree](const std::string& a, const std::string& b) {
            // std::priority_queue puts the "largest" element at the top.
            // Returning a < b means 'b' gets priority if it has a higher count.
            return originalInDegree[a] < originalInDegree[b];
        };

        // 3. Find all modules ready to load right now (0 dependencies)
        // Swap std::queue for std::priority_queue
        std::priority_queue<std::string, std::vector<std::string>, decltype(cmp)> readyQueue(cmp);
        
        for (const auto& pair : inDegree) {
            if (pair.second == 0) {
                readyQueue.push(pair.first);
            }
        }

        // 4. Process the queue to determine the final order
        std::vector<std::string> reordered;
        reordered.reserve(modules.size());

        while (!readyQueue.empty()) {
            // Priority queues use .top() instead of .front()
            std::string current = readyQueue.top(); 
            readyQueue.pop();
            
            reordered.push_back(current);

            auto depIt = dependents.find(current);
            if (depIt != dependents.end()) {
                for (const auto& dependent : depIt->second) {
                    inDegree[dependent]--;
                    if (inDegree[dependent] == 0) {
                        readyQueue.push(dependent); // This automatically sorts by original dependencies!
                    }
                }
            }
        }

        // 5. Catch circular dependencies
        if (reordered.size() != modules.size()) {
            throw std::runtime_error("Failed to reorder: Circular dependency detected!");
        }

        // 6. Apply the correctly ordered list back
        for (const auto& mod : reordered) {
            std::cout << "Reordered modules: " << mod << '\n'; // \n is faster than std::endl
        }
        modules = std::move(reordered);
    }

    void dumpModuleMap() {
        std::cout << "Dump Module Map" << std::endl;
        for (const auto& [key, value] : moduleMap) {
            if (key.find(".") == std::string::npos) {
                std::cout << "module " << key << " ";
            } else [[likely]] {
                std::cout << "subModule " << key << " ";
            }

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


};

auto main(int argc, const char** argv) -> int 
{
    Platform platform(Platform::Windows);
    Project c;
    c.addInclude(fs::path("source") / "lib" / "RGFW");
    c.addInclude(fs::path("source") / "lib" / "glad" / "include");
    c.getCppFile();
    // c.dumpProject();
    
    dependencyScanner scanner(&c);
    scanner.scanInclude();
    scanner.scanModule();
    scanner.reorderModules();
    scanner.dumpModuleMap();
    c.dumpModule();

    if (fs::exists(c.outPath) == false) {
        if (fs::create_directory(c.outPath) == false) {
            std::cerr << "Error: could not create directory " << c.outPath.string() << "\n";
            return EXIT_FAILURE;
        }
        if (fs::create_directory(c.objPath) == false) {
            std::cerr << "Error: could not create directory " << c.outPath.string() << "\n";
            return EXIT_FAILURE;
        }
        if (fs::create_directory(c.modulePath) == false) {
            std::cerr << "Error: could not create directory " << c.modulePath.string() << "\n";
            return EXIT_FAILURE;
        }
    }


    std::string test;
    defer endMessage ([&] {std::cout << "end \n"<< test;});
    // defer link ([&] {
    //     std::cout << "link" << std::endl;
    //     c.link(c.object, "main", platform);}
    // );
    
    c.object.reserve(c.project.size() -1);
    c.modules.reserve(c.project.size() -1);

    ThreadPool pool(std::thread::hardware_concurrency());
    //std::cout << std::thread::hardware_concurrency() << "\n";

    for (auto& i : c.modules) {
            c.preCompile(i,c.outPath.string());
    }
    for (auto& i : c.modules) {
        c.compileModule(i,c.outPath.string());
    }
    while (pool.isEmpty() == false) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "compile cpp \n";

    // for (auto& i : c.project) {
    //     if (fs::path(i).extension() == ".cc") {
    //         pool.enqueue([&i,&c] {c.compile(i,c.outPath.string());});
    //     }
    // }
    while (pool.isEmpty() == false) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // std::cout << "compile cpp \n";

    // for (auto& i : c.project) {
    //     if (fs::path(i).extension() == ".cc") {
    //         pool.enqueue([&i,&c] {c.compile(i,c.outPath.string());});
    //     }
    // }
    // while (pool.isEmpty() == false) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // std::cout << "compile module \n";
    // for (auto& i : c.module) {
    //     pool.enqueue([&i,&c]{c.compileModule(i, c.objPath.string());});
    //     //std::cout << i << "\n";
    // }
    
    
    return 0;
};