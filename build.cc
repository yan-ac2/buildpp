#include "build.hpp"

#include <filesystem>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <string_view>
#include <thread>


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
        print << "exiting"_fmt.color(fmt::Bold_Green).endl();
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

int test()
{ 
    print << "compile test"_fmt.color(fmt::Bold_Green).endl();
    const fs::path rootPath = ".";
    const fs::path exePath = rootPath / "bin";

    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath(exePath)
    .setOutfolder(rootPath / ".build")
    .setOutpath(rootPath / ".build" / "test");

    Project test(&outPath,true,Project::exe);

    #ifdef _WIN32
    test.setCompiler("clang++").setOptions("-O3 -Wall -std=c++23")
    #elif __unix__
    test.setCompiler("clang++")
    .setOptions("-O0 -std=c++23 -nostdlib ")
    #endif
    .setProjectPath(rootPath)
    .addSourcePath("testlib")
    .setMain((test.SourcePath[0] / "inprogress.cc").string())
    .addSource({(test.SourcePath[0] / "inprogress.cc").string()})
    .getCppFile();
    
    #ifdef __unix__
    // test.addDependency("inprogress.cc",{"c++","c++abi"});
    #endif
    for (const auto& i : test.ProjectFile)
    {
        test.compileCpp(i);
    }

    test.link("test");
    return 0;
    
}


int selfCompile(bool recompile)
{
    print << "compile self"_fmt.color(fmt::Bold_Green).endl();

    const fs::path rootPath = ".";
    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath(rootPath)
    .setOutfolder(rootPath / ".build")
    .setOutpath(rootPath/".build" / "self");

    Project rebuild(&outPath,recompile);

    #ifdef _WIN32
    rebuild.setCompiler("clang++").setOptions("-Wall -std=c++23")
    #elif __unix__
    rebuild.setCompiler("clang++")
    .setOptions("-O3 -Wall -std=c++23 -stdlib=libc++ ")
    #endif
    .setProjectPath(rootPath)
    .addSourcePath(rootPath)
    .setMain((rebuild.Path / "build.cc").string())
    .addSource({"build.cc"})
    .getCppFile();
    
    #ifdef __unix__
    rebuild.addDependency("build.cc",{"c++","c++abi"});
    #endif
    for (const auto& i : rebuild.ProjectFile)
    {
        rebuild.compileCpp(i);
    }

    rebuild.link("build");
    return 0;
}

int compileProject(bool recompile)
{
        ThreadPool pool(std::thread::hardware_concurrency());
        const fs::path rootPath = ".";
        const fs::path exePath = rootPath / "bin";

        compileCommand cmdJson;
        outputPath outPath;
        outPath.setRootPath(rootPath)
        .setExePath(exePath)
        .setOutfolder(rootPath / ".build")
        .setOutpath(rootPath / ".build" / "project");

    
        cProject libGLAD(&outPath, cProject::staticLib,recompile);
        #ifdef _WIN32
        libGLAD.setCompiler("clang")
        #elif __unix__
        libGLAD.setCompiler("clang")
        #endif
        .addCompileCommand(&cmdJson)
        .setOptions("-O0")
        .setProjectPath(rootPath / "example"/ "lib" / "glad")
        .setSourcePath("src")
        .setMain((libGLAD.sourcePath / "glad.c").string())
        .addInclude(libGLAD.path / "include")
        .getCFile()
        .scanInclude()
        #ifdef _WIN32
        .addDependency("glad.c", {"opengl32"});
        #elif __unix__
        .addDependency("glad.c", {"GL"});
        #endif
    
        Project mainProj(&outPath,recompile);
    
        #ifdef _WIN32
        mainProj.setCompiler("clang++")
        .setOptions("-O3 -std=c++23 -fopenmp=libomp")
        #elif __unix__
        mainProj.setCompiler("clang++")
        .setOptions("-O3 -fno-exceptions -stdlib=libc++ -std=c++23")
        #endif
        .addCompileCommand(&cmdJson)
        .setProjectPath((rootPath / "example"))
        .addSourcePath(mainProj.Path)
        .setMain("main.cc")
        
        //mainProj.addInclude(rootPath / "usr" / "include" / "X11" );
        .addIncludefile(mainProj.SourcePath[0]/ "lib" / "RGFW")
        .addIncludefile(mainProj.SourcePath[0]/ "lib" / "glad" / "include")
        .getLib(&libGLAD)
        .getCppFile();
    
        mainProj.scanHeader().scanModule()
        #ifdef _WIN32
        .addDependency("lib.RGFW.ccm",{"gdi32","opengl32"})
        .addDependency("lib.renderer.ccm",{"omp"})
        #elif __unix__
        .addDependency("lib.RGFW.ccm",{"X11", "Xrandr"})
        .addDependency("lib.std.ccm",{"c++","c++abi"})
        #endif
        .dumpProject()
        .dumpIncludeMap()
        .dumpModule()
        .dumpDependencies()
        .dumpModuleMap()
        .dumpInclude();
    
        // defer end([&mainProj]{
        //     std::cout << "linking"_fmt.setColor(fmt::Bold_Green) << std::endl;
        //     mainProj.link("main");
        // });
        // for (const auto& i : libGLAD.project) {
        //     pool.enqueue ([&i,&libGLAD]{libGLAD.compileC(i);});
        // }
        // while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
        // for (const auto& i : mainProj.SystemHeader) {
        //     mainProj.compileModule(i,true);
        // }
        for (const auto& f : libGLAD.project) {
            libGLAD.compileC(f);
        }
        while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
        std::queue<std::string> queue;
        for (const auto& i : mainProj.Modules) {queue.push(i.first);}
        while(!queue.empty()) {
            const fs::path modulef = queue.front();
            queue.pop();
            const auto moduleReady = mainProj.isModuleExist(modulef.string());
            if (moduleReady) {
                mainProj.compileModule(modulef);
            } else {
                queue.push(modulef.string());
            }
        }
        while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
        
        for (const auto& i : mainProj.ProjectFile) {
            // pool.enqueue ([&i,&mainProj]{mainProj.compileCpp(i);});
            mainProj.compileCpp(i);
        }
        while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};

        if(mainProj.cmdJson != nullptr) mainProj.cmdJson->write(outPath.rootPath/"compile_commands.json");

        mainProj.link("main");
    
    return 0;
}

auto main(int argc, const char* argv[]) -> int 
{
    print << "CPP BUILD \n"_fmt.color(fmt::Bold_Purple).sv();

    std::string inputLine = argv[1];
    if (argc < 2) {return 1;} else 
    {
        if (inputLine.empty()) {return 0;}
        if (inputLine == "-compile") {compileProject(false); return 0;}
        if (inputLine == "-recompile") {compileProject(true); return 0;}
        if (inputLine == "-self") {selfCompile(false); return 0;}
        if (inputLine == "-recompileself") {selfCompile(true); return 0;}
        if (inputLine == "-test") {test(); return 0;}
        else return 0;    
    }

    return 0;
};