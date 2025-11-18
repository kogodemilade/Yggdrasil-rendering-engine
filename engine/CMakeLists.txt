cmake_minimum_required(VERSION 3.10)

project(YggEngine)

add_library(Ygg STATIC
    src/engine.cpp
    src/stb_impl.cpp
    src/glad.c
)

# Include folder
target_include_directories(Ygg
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link external dependencies
target_link_libraries(Ygg
    PUBLIC glfw
)

