#include "build.h"

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
    rebuild.compileMain();
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
    libGLAD.dumpDependencies();
    
    
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
        mainProj.preCompile(i.first);
    }
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    for (auto& i : mainProj.modules) {
        pool.enqueue([&i,&mainProj]{mainProj.compileModule(i.first);});
    }
    while (!pool.isEmpty()) {std::this_thread::sleep_for(std::chrono::milliseconds(100));};
    mainProj.compileMain();
    std::cout << "link" << std::endl;
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