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
    .addOptions("-O1 -Wall -Wextra -Wpedantic -Werror -fno-rtti -std=c++23")
    .addLdOptions("-fuse-ld=lld")
    .setProjectPath(rootPath)
    .addSourcePath("")
    .addSource("","build.cc")
    .setMain("build.cc")
    // .dumpProject()
    ;
    rebuild.compileCpp(rebuild.ProjectFile.getMain());
    rebuild.link(rebuild.ProjectFile.getMain());
    return 0;
}
int CompileFile(const std::string_view Name,const std::string_view From,std::span<const std::string_view> src,bool recompile)
{
    std::cout << fmt("Compiling {}\nFrom: {}\n" ,Name,From);
    const fs::path rootPath = fs::current_path();
    const fs::path exePath = rootPath / "bin";
    const fs::path outBuildPath = rootPath / ".build";
    const fs::path outProjectPath = rootPath / ".build" / Name;
    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath(rootPath)
    .setBuildfolder(outBuildPath)
    .setOutpath(outProjectPath);
    
    Project compile("build",outPath,Project::exe,recompile);
    current = &compile;
    compile.setCompiler("clang++")
    .addOptions("-Os -Wall -Wextra -Wpedantic -Werror -fno-rtti -std=c++23")
    .addLdOptions("-fuse-ld=lld")
    .setProjectPath(rootPath)
    .addSourcePath(From)
    .addSource(From,src)
    .setMain(src[0]).scanHeader().scanModule()
    .configureModuleFlags()
    .dumpProject()
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
    if(!getModule.empty()) {
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
    }
    for (auto& S : getSource) {
        compile.compileCpp(S);
    }
    compile.link(compile.ProjectFile.getMain());
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
    .addSourcePathList({
        "src",
        "src/core",
        "src/window"
    })
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
    .LinkLibrary("lib.win.ccm",{"gdi32","user32"})
    .LinkLibrary("renderer.ccm",{"opengl32"})
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

    if(compile.getCompileCommand() != nullptr) compile.getCompileCommand()->write(outPath.rootPath/"compile_commands.json");

    compile.link(compile.ProjectFile.getMain());
    
    return 0;
}
void exitImpl() {
    current->~Project();
}

template<typename T,std::size_t N>
constexpr std::array<T, N> appendArray(std::array<T, N <= 1 ? 0 : N - 1>& from,T&& add) {
    return [&]<std::size_t... I>(std::index_sequence<I...>){
        std::array<T,N> temp;
        ((temp[I] = std::move(from[I])),...);
        temp[N - 1] = std::move(add);
        return temp;
    }(std::make_index_sequence<N <= 1 ? 0 : N - 1>{});
}

struct Options{
    std::vector<std::string_view> opt;

    constexpr bool operator ==(std::string_view other) {
        return [&,this]{ 
            for (auto& S : opt) { if (S == other) {return true;}}
            return false;
        }();
    }
};
template<std::size_t N = 0>
struct argsParse {

    std::vector<std::string_view> args;
    std::array<Options, N> options;
    constexpr argsParse() : args() {}
    argsParse(std::size_t argc, const char* argv[],argsParse<N>&& other) : 
    args([&]() {
            std::span<const char*> argsSpan {argv,argc};
            auto argsRange = argsSpan | std::views::drop(1) | std::views::transform([](const auto s) {
                return std::string_view{s};
            }) | std::ranges::to<std::vector<std::string_view>>();
            return argsRange;
        }()),
    options(std::move(other.options)) 
    {
    }
    
    constexpr argsParse(argsParse<N <= 1 ? 0 : N - 1>&& other,Options&& value) : options(appendArray<Options,N>(other.options, std::move(value))) {
    }
    
    constexpr argsParse<N + 1> addOptions(Options opt) {
        return argsParse<N + 1>(std::move(*this),std::move(opt));
    }
};


auto main(int argc, const char* argv[]) -> int 
{
    std::cout << "CPP BUILD \n"_fmt.color(fmt::Bold_Purple);
    std::atexit(exitImpl);
    auto makeArgs = argsParse()
    .addOptions({{"-C","-compile"}})
    .addOptions({{"-S"}});
    auto cmd = argsParse(argc,argv,std::move(makeArgs));
    for(const auto& c : cmd.options) {
        for(const auto& cc : c.opt) {
            std::cout << cc << " ";
        }
    }
    std::cout << "\n";
    for(const auto& c : cmd.args) {
        std::cout << c << " ";
    }
    std::cout << "\n";
    
    std::string_view inputLine = cmd.args[0];
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
        else {
            const auto compile = std::ranges::find_if(cmd.args,[](auto& s) {
                return s == "-c"; 
            });
            const auto sourcePath = std::ranges::find_if(cmd.args,[](auto& s) {
                return s == "-S"; 
            });
            const std::size_t Pathidx {static_cast<std::size_t>(std::distance(cmd.args.begin(), sourcePath)) + 1};
            const std::size_t compileidx {static_cast<std::size_t>(std::distance(cmd.args.begin(), compile)) + 1};
            auto& Name = cmd.args[0];
            auto& sourceLocation = cmd.args[Pathidx];
            // std::span<const char*> srcList {argv + sourceidx, static_cast<std::size_t>(argc) - sourceidx};
            auto srcRange = cmd.args | std::views::drop(compileidx);
            CompileFile(Name,sourceLocation,srcRange,true);
            return 0;
        };    
    }

    return 0;
};