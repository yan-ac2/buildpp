#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <type_traits>
#include <string_view>


namespace  fs = std::filesystem;


template<typename... Args> concept onlyStr = (std::convertible_to<Args, std::string_view> && ...);
template<typename T> concept moveAble = 
    !std::is_const_v<std::remove_reference_t<T>> && 
    std::is_move_constructible_v<std::remove_reference_t<T>> && 
    std::is_rvalue_reference_v<T&&>;
template<moveAble T>
inline auto imove(T&& args) {return static_cast<std::remove_reference_t<decltype(args)>&&>(args);};


template<onlyStr... Args>
std::string pathJoin(Args... args) {
    std::filesystem::path result = std::filesystem::current_path();
    ((result /= args), ...);
    return result.string();
}

auto fmt = [](auto&&... args) -> std::string  {
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

std::string_view Compiler {"clang++ "};
std::string_view Options {"-std=c++23 -O2 "};

void compile(std::string_view path) {
    std::string file {std::filesystem::path(path).string()};
    std::string Output {fmt(" -o ",  file, ".o ")};
    std::string Source {fmt(" -c ",  file, ".cc ")};

    std::string cmd {fmt(Compiler, Options, Output, Source)};
    std::printf("%s \n",cmd.c_str());
    std::system(cmd.c_str());
};

enum class Platform : int 
{
    Windows = 1, 
    Linux = 2
};

inline std::string_view getExtension(Platform platform) {
    switch (platform)
    {
        case Platform::Windows:
            return ".exe";
            break;
        default:
            return "";
    }
}

void link(std::string_view path,std::string_view target, Platform platform) {
    std::filesystem::path file {path};
    std::cout << std::filesystem::exists(fmt(file.string(), getExtension(platform))) << std::endl;
    std::string Executable {fmt(" -o ", fmt(file.string(), getExtension(platform), ".new"), " ")};
    std::string Object {fmt( file.string(), ".o ")};
    
    std::string cmd {fmt(Compiler, Options,Executable, Object)};
    std::printf("check %s \n",cmd.c_str());
    std::system(cmd.c_str());       
}

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
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

void getFileWithExtension(std::string_view inpath,std::vector<std::string>& container) {
    fs::path path = fmt("." , fs::current_path().root_directory().string(), inpath);
    std::cout << path <<"\n";
    if (!fs::exists(path) || !fs::is_directory(path)) {std::cerr << "Directory does not exist.\n"; return;}
    
    fs::directory_iterator iterator(path);
    
    for (const auto& entry : iterator) {
        if (entry.is_regular_file()&&(entry.path().extension() == ".cc" || entry.path().extension() == ".ixx")) {
            container.emplace_back(entry.path().string());
        }
    }
}

void getFile(std::string_view path,auto& container) {
    std::string file {std::filesystem::path(path).string()};
    container.push_back(file);
}

template<typename F>
struct defer {
    F f;
    ~defer() { f(); }
};

auto main(int argc, const char** argv) -> int {
    std::vector<std::string> project {};
    defer end {[&] {std::cout << "end \n";}};
    
    getFileWithExtension("source", project);
    //ThreadPool pool(std::thread::hardware_concurrency());

    for (auto& i : project) {
       //pool.enqueue([i] {compile(i);});
        std::cout << i << "\n"; 
    }
    
    return 0;
};