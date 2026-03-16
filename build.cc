#include <cstdlib>
#include <cstdio>
#include <filesystem>
 
auto projectPath = std::filesystem::current_path();
template<typename... Args> concept allStr = (std::is_same_v<Args, const char*> && ...);
const char* fmt(const char* format, allStr auto... args) {
    static char buffer[512];
    std::snprintf(buffer, sizeof(buffer), format, args...);
    return buffer;
}

void build() {
    const char* Path = projectPath.c_str();
    std::printf("%s \n",fmt("%s \n", Path));
    std::printf("%s \n",fmt("clang -std=c++23 -o %s -c %s -v", Path, Path));
    //std::system(fmt("clang -std=c++23 -o %s -c %s -v", projectPath, projectPath));
};

auto main() -> int {
    build();
    return 0;
};