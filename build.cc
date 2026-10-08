#include "build.hpp"

#include <functional>
// #include <mutex>
// #include <thread>
#include <queue>
#include <cstdint>


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

int selfCompile(bool recompile)
{
    std::cout << sformat("{}\n",addColors("compile self",strColors::Bold_Green));
    const fs::path rootPath = fs::current_path();
    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath("")
    .setBuildfolder(".build")
    .setOutpath("self");
    outPath.ensurePath();
    
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

int CompileFile(const std::string_view Name,const std::string_view From,const std::string_view oPath,Project::projectType type,std::span<const std::string_view> options,std::span<const std::string_view> ldoptions,
    std::span<const std::string_view> IncludePath,std::span<const std::string_view> src,const std::span<const std::string_view> libraryList,bool recompile)
{
    std::cout << sformat("Compiling {}\nFrom: {}\n" ,Name,From);
    const fs::path rootPath = fs::current_path();
    outputPath outPath;
    outPath.setRootPath(rootPath)
    .setExePath(oPath)
    .setBuildfolder(".build")
    .setOutpath(Name);
    outPath.ensurePath();
    
    Project compile("build",outPath,type,recompile);
    current = &compile;
    compile.setCompiler("clang++")
    .addOptions(options)
    .addLdOptions(ldoptions)
    .setProjectPath(rootPath)
    .addSourcePath(From)
    .addSource(From,src)
    .addIncludePathList(IncludePath)
    .setMain(src[0]).scanHeader().scanModule()
    .LinkLibrary(src[0], libraryList)
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
    compileCommand cmdJson;
    outputPath outPath;
    outPath.setRootPath(rootPath).setExePath("bin").setBuildfolder(".build").setOutpath("Project");
    outPath.ensurePath();
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
    std::cout << sformat("{} forced Exit\n",addColors("Error:",strColors::Bold_Red));
}
template <std::size_t N = 0>
struct Options {
    std::array<std::vector<std::string_view>, N> options;

    Options() = default;
    explicit Options(std::array<std::vector<std::string_view>, N> opts) 
        : options(std::move(opts)) {}

    template <std::size_t... Is>
    auto append_impl(std::vector<std::string_view>&& new_opt, std::index_sequence<Is...>) {
        return Options<N + 1>{
            std::array<std::vector<std::string_view>, N + 1>{
                std::move(options[Is])..., 
                std::move(new_opt)
            }
        };
    }

    [[nodiscard]] Options<N + 1> addOptions(std::vector<std::string_view> opt) && {
        return append_impl(std::move(opt), std::make_index_sequence<N>{});
    }

    bool operator==(std::string_view target) const {
        for (const auto& group : options) {
            for (const auto& sv : group) {
                if (sv == target) return true;
            }
        }
        return false;
    }

    auto begin() { return options.begin(); }
    auto end() { return options.end(); }
};

struct ParsedArgs {
    std::string_view projectName;
    std::string_view sourcePath;
    std::string_view outputPath;
    Project::projectType outType;
    std::vector<std::string_view> options;
    std::vector<std::string_view> ldoptions;
    std::vector<std::string_view> sources;
    std::vector<std::string_view> libraries;
    std::vector<std::string_view> IncludePath;
};

ParsedArgs parseCommandLine(std::span<const std::string_view> args) {
    ParsedArgs result;
    std::string_view currentFlag;
    bool inStr = false;
    for (std::string_view arg : args) {
        bool startsQuote = (!inStr && arg.front() == '<');
        if (startsQuote) {
            inStr = true;
            arg.remove_prefix(1); // Strip leading quote
        }

        bool endsQuote = (inStr && !arg.empty() && arg.back() == '>');
        if (endsQuote) {
            arg.remove_suffix(1); // Strip trailing quote
        }
        if (!inStr && !arg.empty() && arg[0] == '-' ) {
            currentFlag = arg; // Switch current active flag
            continue;
        }

        if (currentFlag == "-P") {
            result.projectName = arg;
        } else if (currentFlag == "-S") {
            result.sourcePath = arg;
        } else if (currentFlag == "-C") {
            result.sources.push_back(arg);
        } else if (currentFlag == "-L") {
            result.libraries.push_back(arg);
        } else if (currentFlag == "-I") {
            result.libraries.push_back(arg);
        } else if (currentFlag == "-Ld") {
            result.ldoptions.push_back(arg);
        } else if (currentFlag == "-CXX") {
            result.options.push_back(arg);
        } else if (currentFlag == "-O") {
            result.outputPath = arg;
        } else if (currentFlag == "-type") {
            if (arg == "exe") {
                result.outType = Project::exe;
            }
            if (arg == "static") {
                result.outType = Project::staticLib;
            }
        }
        if (endsQuote) {
            inStr = false;
        }
    }

    return result;
}

template<std::size_t N = 0>
struct argsParse {
    std::vector<std::string_view> args;
    Options<N> options;
    argsParse(std::size_t argc, const char* argv[],Options<N>&& other) : options(std::move(other)) 
    {
        args.reserve(argc);
        for(auto sv : std::span<const char*>{argv + 1,argc - 1}) args.push_back(std::string_view{sv});
    }
    
};




auto main(int argc, const char* argv[]) -> int 
{
    std::cout << sformat("{}\n",addColors("CPP BUILD",strColors::Bold_Purple));
    std::atexit(exitImpl);
    auto makeOptions = Options()
    .addOptions({"-C","-compile"})
    .addOptions({"-S"});
    auto cmd = argsParse(argc,argv,std::move(makeOptions));
    for(const auto& c : cmd.options) {
        for(const auto& cc : c) {
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
            selfCompile(true); 
            return 0;
        }
        if (inputLine == "-P") {
            const auto parsed = parseCommandLine(cmd.args);
            CompileFile(parsed.projectName,parsed.sourcePath,parsed.outputPath,
                parsed.outType,parsed.options,parsed.ldoptions,parsed.IncludePath,
                parsed.sources,parsed.libraries,true);
            return 0;
        };    
    }

    return 0;
};