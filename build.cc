#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <string>
#include <iostream>


template<typename... Args> concept onlyStr = (std::convertible_to<Args, std::string_view> && ...);
template<typename T> concept moveAble = 
    !std::is_const_v<std::remove_reference_t<T>> && 
    std::is_move_constructible_v<std::remove_reference_t<T>> && 
    std::is_rvalue_reference_v<T&&>;
template<moveAble T>
inline auto imove(T&& args) {return static_cast<std::remove_reference_t<decltype(args)>&&>(args);};


using cstr = const char*;
 
inline const std::string projectPath = std::filesystem::current_path().string();
template<onlyStr... Args>
std::string pathJoin(Args... args) {
    std::filesystem::path result = std::filesystem::current_path();
    ((result /= args), ...);
    return result.string();
}

std::string fmt(onlyStr auto&&... args) {
    std::string temp;

    try {
        size_t totalSize = 0;
        ((totalSize += std::string_view(args).size()), ...);
        
        temp.reserve(totalSize);

        // Fold expression to append args and the delimiter
        (temp.append(args), ...);
        
    } catch (const std::bad_alloc& e) {
        throw std::runtime_error("fmt: Memory allocation failed");
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt("fmt: Unhandled exception: {}", e.what()));
    } catch (...) {
        throw std::runtime_error("fmt: Unhandled exception");
    }

    return temp; 
}


void build(std::string_view file) {
    std::string_view Path = projectPath;
    std::string_view Compiler = "clang++ ";
    std::string_view Options = "-std=c++23 -O2 ";
    std::string Output = fmt(" -o ",  file, ".o ");
    std::string Executable = fmt(" -o ", file, ".out ");
    std::string Source = fmt(" -c ",  file, ".cc ");
    std::string Object = fmt( file, ".o ");
    std::printf("%s \n",fmt(Compiler, Options, Output, Source).c_str());
    std::system(fmt(Compiler, Options, Output, Source).c_str());
    std::printf("%s \n",fmt(Compiler, Options,Executable, Object).c_str());
    std::system(fmt(Compiler, Options,Executable, Object).c_str());
};

auto main() -> int {
    build("./build");
    return 0;
};