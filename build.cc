#include "build.hpp"

#include <functional>
// #include <mutex>
// #include <thread>
#include <queue>


[[maybe_unused]] inline Project* current {nullptr};
// class ThreadPool {
// public:
//     explicit ThreadPool(size_t num_threads) {
//         for (size_t i = 0; i < num_threads; ++i) {
//             threads_.emplace_back([this] {
//                 while (true) {
//                     std::function<void()> task;
//                     {
//                         std::unique_lock lock(queue_mutex_);
//                         cv_.wait(lock, [this] { return !tasks_.empty() || stop_; });
//                         if (stop_ && tasks_.empty()) return;
//                         task = std::move(tasks_.front());
//                         tasks_.pop();
//                     }
//                     task();
//                 }
//             });
//         }
//     }

//     ~ThreadPool() {
//         {
//             std::unique_lock lock(queue_mutex_);
//             stop_ = true;
//         }
//         cv_.notify_all();
//         print << "exiting"_fmt.color(fmt::Bold_Green).endl();
//         for (auto& thread : threads_) {
//             thread.join();
//         }
//     }

//     template<typename F>
//     void enqueue(F&& f) {
//         {
//             std::unique_lock lock(queue_mutex_);
//             tasks_.emplace(std::forward<F>(f));
//         }
//         cv_.notify_one();
//     }
//     int isEmpty() {return tasks_.empty();}
//     private:
//     std::vector<std::jthread> threads_;
//     std::queue<std::function<void()>> tasks_;
//     mutable std::mutex queue_mutex_;
//     std::condition_variable cv_;
//     bool stop_ = false;
// };


int test()
{ 
    std::cout << "compile test"_fmt.color(fmt::Bold_Green).endl();
    const fs::path rootPath = fs::current_path();
    const fs::path exePath = rootPath / "bin";
    const fs::path outBuildPath = rootPath / ".build";
    const fs::path outProjectPath = rootPath / ".build" / "test";

    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath(exePath)
    .setBuildfolder(outBuildPath)
    .setOutpath(outProjectPath);

    Project test("test",outPath,Project::exe,true);
    current = &test;
    test.setCompiler("clang++")
    .addOptions("-O2 -Wall -Wextra -Wpedantic -Werror -std=c++20 -fno-rtti")
    .addLdOptions("-s ")
    .setProjectPath(rootPath.string())
    .addSourcePath("testlib")
    .addSource("testlib","match_test.cc")
    .setMain("match_test.cc");
    test.compileCpp(test.ProjectFile.getMain());

    test.link(test.ProjectFile.getMain());
    return 0;
    
}


int selfCompile(bool recompile)
{
    std::cout << "compile self"_fmt.color(fmt::Bold_Green).endl();
    const fs::path rootPath = fs::current_path();
    const fs::path exePath = rootPath / "bin";
    const fs::path outBuildPath = rootPath / ".build";
    const fs::path outProjectPath = rootPath / ".build" / "self";
    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath(rootPath)
    .setBuildfolder(outBuildPath)
    .setOutpath(outProjectPath);
    
    Project rebuild("build",outPath,Project::exe,recompile);
    current = &rebuild;
    rebuild.setCompiler("clang++")
    .addOptions("-Os -Wall -Wextra -Wpedantic -Werror -fno-rtti -std=c++23")
    .addLdOptions("-fuse-ld=lld")
    .setProjectPath(rootPath)
    .addSourcePath("")
    .addSource("","build.cc")
    .setMain("build.cc")
    .dumpProject();

    rebuild.compileCpp(rebuild.ProjectFile.getMain());
    rebuild.link(rebuild.ProjectFile.getMain());
    return 0;
}

int compileProject(bool recompile)
{
    const fs::path rootPath = fs::current_path();
    const fs::path exePath = rootPath / "bin";
    const fs::path outBuildPath = rootPath / ".build";
    const fs::path outProjectPath = rootPath / ".build" / "Project";
    compileCommand cmdJson;
    outputPath outPath;
    outPath.setRootPath(rootPath).setExePath(exePath).setBuildfolder(outBuildPath).setOutpath(outProjectPath);
    Project compile("Main",outPath,Project::exe,recompile);
    current = &compile;
    compile.setCompiler("clang++")
    .addOptions("-O2 -Wall -Wextra -Wpedantic -Werror -flto=thin -fno-rtti -fno-exceptions -std=c++26")
    .addLdOptions("-fuse-ld=lld")
    .addCompileCommand(&cmdJson)
    .setProjectPath(rootPath)
    .addSourcePath("src")
    .addSourcePath("src/core")
    .addSourcePath("src/window")
    .addSource("src", {
        "main.cc",
        "lib.ui.ccm",
        "lib.image.ccm",
    })
    .addSource("src/core", "*")
    .addSource("src/window", {
        "lib.win.ccm",
        "winCommon.ccm",
        "platform.ccm",
        "renderer.ccm",
        "keyboard.ccm",
    })
    .addIncludePathList({
        "src",
        "src/window"
    })
    .getHeaderFile()
    .setResourcePath("res")
    .setMain("main.cc").scanHeader().scanModule()
    .addLinkLibrary("lib.win.ccm",{"gdi32","user32"})
    .addLinkLibrary("renderer.ccm",{"opengl32"})
    .configureModuleFlags().dumpProject()
    ;

    auto getProjectFile = compile.ProjectFile | std::views::values; 
    auto getModule = getProjectFile 
    | std::views::filter([](const auto& File){
        const bool isSource = File.fileType == File::Source;
        const bool isModuleImpl = File.fileType == File::ModuleImpl;
        return !isSource || isModuleImpl;
    }); 
    auto getSource = getProjectFile 
    | std::views::filter([](const auto& File){
        const bool isSource = File.fileType == File::Source;
        const bool isModuleImpl = File.fileType == File::ModuleImpl;
        return isSource || isModuleImpl;
    });
    
    std::queue<std::reference_wrapper<File>> queue;
    for (auto& i : getModule) {
        queue.push(i);
    }
    while(!queue.empty()) {
        auto& modulef = queue.front().get();
        queue.pop();
        if (compile.compileModule(modulef) == false) {
            // std::cout << "Compiled Module: "_fmt.color(fmt::Red) << modulef.Name <<"\n";
            queue.emplace(modulef);
        }
    }
    
    for (auto& i : getSource) {
        compile.compileCpp(i);
    }

    if(compile.cmdJson != nullptr) compile.cmdJson->write(outPath.rootPath/"compile_commands.json");

    compile.link(compile.ProjectFile.getMain());
    
    return 0;
}
void exitImpl() {
    current->~Project();
}

auto main(int argc, const char* argv[]) -> int 
{
    std::cout << "CPP BUILD \n"_fmt.color(fmt::Bold_Purple);
    std::atexit(exitImpl);
    
    std::string inputLine = argv[1];
    if (argc < 2) {return 1;} else 
    {
        if (inputLine.empty()) {return 0;}
        if (inputLine == "-compile") {
            compileProject(false);
            return 0;
        }
        if (inputLine == "-recompile") {
            compileProject(true); 
            return 0;
        }
        if (inputLine == "-self") {
            selfCompile(false); 
            return 0;
        }
        if (inputLine == "-recompileself") {
            selfCompile(true); 
            return 0;
        }
        if (inputLine == "-test") {
            test(); 
            return 0;
        }
        else return 0;    
    }

    return 0;
};