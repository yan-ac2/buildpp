project (glad C)

add_library (glad src/glad.c)
add_library (glad::glad ALIAS glad)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)