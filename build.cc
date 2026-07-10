#include "build.hpp"

#include <functional>
#include <mutex>
#include <thread>
#include <queue>


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

    Project test("test",&outPath,Project::exe,true);

    #ifdef _WIN32
    test.setCompiler("clang++")
    .setOptions("-O2 -ffreestanding -flto -Wall -std=c++26 -fno-rtti")
    .setLdOptions("-s ")
    #elif __unix__
    test.setCompiler("clang++")
    .setOptions("-O0 -std=c++23 -nostdlib ")
    #endif
    .setProjectPath(rootPath.string())
    .addSourcePath("testlib")
    .addSource({(test.Path / test.getMainPath() / "staticMap.cc").string()})
    .setMain("staticMap.cc");
    // .getCppFile();
    
    #ifdef __unix__
    // test.addDependency("inprogress.cc",{"c++","c++abi"});
    #endif
    for (auto& i : test.ProjectFile.VIter())
    {
        test.compileCpp(*i);
    }

    test.link(test.ProjectFile.getMain());
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
    
    Project rebuild("build",&outPath,Project::exe,recompile);

    #ifdef _WIN32
    rebuild.setCompiler("clang++")
    .setOptions("-Os -flto=thin -fno-rtti -fuse-ld=lld -Wall -std=c++23")
    .setLdOptions("-s")
    #elif __unix__
    rebuild.setCompiler("clang++")
    .setOptions("-O3 -Wall -std=c++26 -stdlib=libc++ ")
    #endif
    .setProjectPath(rootPath.string())
    .addSourcePath(rootPath.string())
    .addSource({(rootPath/"build.cc")});
    rebuild.setMain("build.cc").dumpProject();
    #ifdef __unix__
    rebuild.addDependency("build.cc",{"c++","c++abi"});
    #endif
    for (auto& i : rebuild.ProjectFile.VIter())
    {
        rebuild.compileCpp(*i);
    }

    rebuild.link(rebuild.ProjectFile.getMain());
    return 0;
}

int compileProject(bool recompile)
{
        ThreadPool pool(std::thread::hardware_concurrency());
        const fs::path rootPath = fs::current_path();
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
        .setOptions("-O2 -flto=thin")
        .setProjectPath(rootPath / "example"/ "lib" / "glad")
        .setSourcePath("src")
        .addIncludefile((libGLAD.Path / "include").string())
        .getCFile()
        .dumpProject()
        .setMain("glad.c")
        .scanInclude()
        // #ifdef _WIN32
        // .addDependency("glad.c", {"opengl32"})
        // #elif __unix__
        // .addDependency("glad.c", {"GL"})
        // #endif
        ;
        for (auto& f : libGLAD.ProjectFile.VIter()) {
            libGLAD.compileC(*f);
        }
        libGLAD.link(libGLAD.ProjectFile.getMain());
        // Project meshoptimizer("meshoptimizer",&outPath,Project::staticLib,false);
        // meshoptimizer.setCompiler("clang++")
        // .setOptions("-O3 -std=c++23")
        // .setProjectPath(rootPath/"example"/"lib"/"meshoptimizer")
        // .addSourcePath(meshoptimizer.Path/"src")
        // .addIncludefile(meshoptimizer.Path/"src")
        // .addIncludefile(meshoptimizer.Path/"extern")
        // .addIncludefile(meshoptimizer.Path/"gltf")
        // .getCppFile();
        // thread_local auto* pp = &meshoptimizer.ProjectFile;
        // for(const auto& i : *pp) {
        //     meshoptimizer.compileCpp(i);
        // }
        // while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));}
        // meshoptimizer.link("meshoptimizer");

        Project mainProj("main",&outPath,Project::exe,recompile);
    
        #ifdef _WIN32
        mainProj.setCompiler("clang++")
        .setOptions(" -O2 -flto=thin -fno-rtti -fno-exceptions -fuse-ld=lld -std=c++26 -ftime-trace")
        .setLdOptions("-s ")
        #elif __unix__
        mainProj.setCompiler("clang++")
        .setOptions("-O3 -fno-exceptions  -stdlib=libc++ -std=c++26")
        #endif
        .addCompileCommand(&cmdJson)
        .setProjectPath((rootPath).string())
        .addSourcePath((mainProj.Path / "example").string());
        // .getLib(&meshoptimizer)
        print << fmt("Source Path: "_fmt.color(fmt::Red),mainProj.getMainPath()," root path: "_fmt.color(fmt::Blue),mainProj.Path.string(),"\n");
        mainProj.getLib(&libGLAD)
        .addIncludefile((mainProj.Path / mainProj.getMainPath() / "lib" / "RGFW").string())
        .addIncludefile((mainProj.Path / mainProj.getMainPath() / "lib" / "glad" / "include").string())
        .setResourcePath("res")
        .getCppFile();
    
        mainProj
        .setMain("main.cc").scanHeader().scanModule()
        #ifdef _WIN32
        .addDependency("lib.RGFW.ccm",{"gdi32","opengl32"})
        #elif __unix__
        .addDependency("lib.RGFW.ccm",{"X11", "Xrandr"})
        .addDependency("lib.std.ccm",{"c++","c++abi"})
        #endif
        .dumpProject();

        while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
        std::queue<std::reference_wrapper<File>> queue;
        for (auto& i : mainProj.ProjectFile) {
            if (i.second.fileType == File::Source) {
                continue;
            }
            queue.push(i.second);
        }
        while(!queue.empty()) {
            auto& modulef = queue.front().get();
            queue.pop();
            // const auto moduleReady = mainProj.isModuleExist(modulef);
            // print << "Compiling Module: "_fmt.color(fmt::Red) << modulef.Path <<"\n";
            if (mainProj.compileModule(modulef) < 0) {
                queue.emplace(modulef);
            }
        }
        
        while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
        
        for (auto& i : mainProj.ProjectFile) {
            // pool.enqueue ([&i,&mainProj]{mainProj.compileCpp(i);});
            // print << "File " << i.first << " with ID: " << std::to_string(i.second.ID) << " is: " << (i.second.compiled ? "Compiled" : "not Compiled") << "\n";
            mainProj.compileCpp(i.second);
        }
        while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};

        if(mainProj.cmdJson != nullptr) mainProj.cmdJson->write(outPath.rootPath/"compile_commands.json");

        mainProj.link(mainProj.ProjectFile.getMain());
    
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