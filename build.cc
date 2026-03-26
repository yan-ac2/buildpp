#include "build.h"
#include <filesystem>


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

int selfCompile()
{
    std::cout << "compile self" << std::endl;
    const fs::path rootPath = ".";
    outPath outPath;
    outPath.setRootPath(rootPath.string());
    outPath.setExePath("");
    outPath.setOutpath("_buildself");

    Project rebuild(&outPath,Project::exe);
    rebuild.setProjectPath(".");
    rebuild.setSourcePath("");
    rebuild.setMain((rebuild.sourcePath / "build.cc").string());
    rebuild.getCppFile();
    rebuild.addDependency("build.cc",{"c++","c++abi"});
    for (const auto& i : rebuild.project)
    {
        rebuild.compileCpp(i);
    }
    rebuild.link("build");
    return 0;
}

int compileProject()
{
    ThreadPool pool(std::thread::hardware_concurrency());
    const fs::path rootPath = ".";
    depScanner scanner;
    outPath outPath;
    outPath.setRootPath(".");
    outPath.setExePath("");
    outPath.setOutpath("_build");

    
    cProject libGLAD(&outPath, cProject::staticLib);
    libGLAD.setProjectPath((rootPath / "example" / "source" / "lib" / "glad").c_str());
    libGLAD.setSourcePath("src");
    libGLAD.setMain((libGLAD.sourcePath / "glad.c").c_str());
    libGLAD.addInclude(libGLAD.path / "include");
    libGLAD.getCFile();
    libGLAD.scanInclude();
    libGLAD.addDependency("glad.c", {"GL"});
    
    Project mainProj(&outPath, Project::exe);
    mainProj.setProjectPath((rootPath / "example").c_str());
    mainProj.setSourcePath("source");
    mainProj.setMain("main.cc");
    
    //mainProj.addInclude(rootPath / "usr" / "include" / "X11" );
    mainProj.addInclude(mainProj.sourcePath  / "lib" / "RGFW");
    mainProj.addInclude(mainProj.sourcePath  / "lib" / "glad" / "include");
    mainProj.getCppFile();
    mainProj.scanInclude();
    mainProj.scanModule();
    mainProj.addDependency("RGFW.ccm",{"GL", "X11", "Xrandr"});
    mainProj.addDependency("lib.std.ccm",{"c++","c++abi"});
    // mainProj.dumpProject();
    scanner = &mainProj;
    scanner.reorderModules();
    mainProj.dumpModule();
    mainProj.dumpDependencies();

    for (const auto& i : libGLAD.project) {
        libGLAD.compileC(i);
    }
    mainProj.getLib(&libGLAD);

    for (auto& i : mainProj.modules) {
        mainProj.compilePcm(i.first);
    }
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    // for (auto& i : mainProj.modules) {
    //     pool.enqueue([&i,&mainProj]{mainProj.compileModule(i.first);});
    // }

    //while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    for (const auto& i : mainProj.project) {
        mainProj.compileCpp(i);
    }
    std::cout << "link" << std::endl;
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    mainProj.link("main");
    return 0;
}

auto main(int argc, const char* argv[]) -> int 
{
    std::string inputLine = argv[1];
    if (argc < 2) {return 1;} else
    {
        if (inputLine.empty()) {return 0;}
        if (inputLine == "-compile") {compileProject(); return 0;}
        if (inputLine == "-self") {selfCompile(); return 0;}
        else return 0;    
    }
    return 0;
};