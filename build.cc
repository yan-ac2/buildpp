#include "build.h"
#include <cstdlib>


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
        std::cout << "exiting"_fmt.setColor(fmt::Bold_Green) << std::endl;
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
{ try 
    {
        std::cout << "compile test"_fmt.setColor(fmt::Bold_Green) << std::endl;
        outputPath outPath;
        outPath.setRootPath(".").
        setExePath("").
        setOutpath("_buildtest");

        Project test(&outPath,Project::exe,true);

        #ifdef _WIN32
        test.setCompiler("clang++").setOptions("-O0 -Wall -std=c++23")
        #elif __unix__
        .setCompiler("clang++-20");
        .setOptions("-O3 -std=c++23 -stdlib=libc++ ");
        #endif
        .setProjectPath(".")
        .setSourcePath("")
        .setMain((test._sourcePath / "test.cc").string())
        .addSource({"sptr.cc"})
        .getCppFile();
        
        #ifdef __unix__
        test.addDependency("test.cc",{"c++","c++abi"});
        #endif
        for (const auto& i : test._project)
        {
            test.compileCpp(i);
        }

        test.link("test");
        return 0;
    }catch(int)
    {
        std::cerr << "test failed"_fmt.setColor(fmt::Bold_Red) << std::endl;
        return 1;
    }
}


int selfCompile(bool recompile)
{
    try {
    
        std::cout << "compile self"_fmt.setColor(fmt::Bold_Green) << std::endl;
        outputPath outPath;
        outPath.setRootPath(".")
        .setExePath("").
        setOutpath("_buildself");
    
        Project rebuild(&outPath,Project::exe,recompile);
    
        #ifdef _WIN32
        rebuild.setCompiler("clang++").setOptions("-O0 -Wall -std=c++23")
        #elif __unix__
        .setCompiler("clang++-20");
        .setOptions("-O3 -std=c++23 -stdlib=libc++ ");
        #endif
        .setProjectPath(".")
        .setSourcePath("")
        .setMain((rebuild._sourcePath / "build.cc").string())
        .addSource({"build.cc"})
        .getCppFile();
        
        #ifdef __unix__
        rebuild.addDependency("build.cc",{"c++","c++abi"});
        #endif
        for (const auto& i : rebuild._project)
        {
            rebuild.compileCpp(i);
        }
    
        rebuild.link("build");
        return 0;
    } catch (int) {
        std::cerr << "self failed"_fmt.setColor(fmt::Bold_Red) << std::endl;
        return 1;
    }
}

int compileProject(bool recompile)
{
    ThreadPool pool(std::thread::hardware_concurrency());
    const fs::path rootPath = ".";
    
    outputPath outPath;
    outPath.setRootPath(".");
    outPath.setExePath("");
    outPath.setOutpath("_build");

    
    cProject libGLAD(&outPath, cProject::staticLib,recompile);
    #ifdef _WIN32
    libGLAD.setCompiler("clang");
    #elif __unix__
    libGLAD.setCompiler("clang-20");
    #endif
    libGLAD.setOptions("-O0");
    libGLAD.setProjectPath((rootPath / "example" / "source" / "lib" / "glad").string());
    libGLAD.setSourcePath("src");
    libGLAD.setMain((libGLAD.sourcePath / "glad.c").string());
    libGLAD.addInclude(libGLAD.path / "include");
    libGLAD.getCFile();
    libGLAD.scanInclude();
    #ifdef _WIN32
    libGLAD.addDependency("glad.c", {"opengl32"});
    #elif __unix__
    libGLAD.addDependency("glad.c", {"GL"});
    #endif
    Project mainProj(&outPath, Project::exe,false);

    #ifdef _WIN32
    mainProj.setCompiler("clang++")
    .setOptions("-O0 -std=c++23")
    #elif __unix__
    mainProj.setCompiler("clang++-20")
    .setOptions("-O3 -stdlib=libc++ -std=c++23")
    #endif

    .setProjectPath((rootPath / "example").string())
    .setSourcePath("source")
    .setMain("main.cc")
    .addIncludePath((libGLAD.path / "include" / "glad").string())
    
    //mainProj.addInclude(rootPath / "usr" / "include" / "X11" );
    .addInclude(mainProj._sourcePath  / "lib" / "RGFW")
    .addInclude(mainProj._sourcePath  / "lib" / "glad" / "include")
    .getCppFile();
    mainProj.scanHeader().scanModule();
    #ifdef _WIN32
    mainProj.addDependency("lib.RGFW.ccm",{"opengl32", "gdi32"});
    #elif __unix__
    mainProj.addDependency("lib.RGFW.ccm",{"GL", "X11", "Xrandr"});
    mainProj.addDependency("lib.std.ccm",{"c++","c++abi"});
    #endif
    // mainProj.dumpProject();
    mainProj.dumpIncludeMap()
    .dumpModule()
    .dumpDependencies()
    .dumpModuleMap()
    .dumpInclude();

    // defer end([&mainProj]{
    //     std::cout << "linking"_fmt.setColor(fmt::Bold_Green) << std::endl;
    //     mainProj.link("main");
    // });
    for (const auto& i : libGLAD.project) {
        pool.enqueue ([&i,&libGLAD]{libGLAD.compileC(i);});
    }
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    mainProj.getLib(&libGLAD);

    mainProj.compileModule();
    
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    
    for (const auto& i : mainProj._project) {
        pool.enqueue ([&i,&mainProj]{mainProj.compileCpp(i);});
    }
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(1000));};

    
    mainProj.link("main");
    return 0;
}
int stuff(int a,int b)
{
    std::cout << "stuff " << a << " " << b << std::endl;
    return a + b;
};
float stuff2(int a,int b)
{
    std::cout << "stuff " << a * b << " " << b * a << std::endl;
    return a + b;
};

auto main(int argc, const char* argv[]) -> int 
{
    fprint out;
    out << "CPP BUILD"_fmt.setColor(fmt::Bold_Purple).sv() << out.endl();
    //cmd << "secho hello" >> "err"_fmt.setColor(fmt::Bold_Yellow);
    //sys("sdecho hello");
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