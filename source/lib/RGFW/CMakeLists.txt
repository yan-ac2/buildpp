project(RGFW CXX)

add_library(RGFW INTERFACE)
add_library(RGFW::RGFW ALIAS RGFW)

target_include_directories(RGFW INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})