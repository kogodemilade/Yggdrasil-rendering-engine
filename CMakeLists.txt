# cmake_minimum_required(VERSION 3.10)
# project(Yggdrasil)

# set(CMAKE_CXX_STANDARD 20)

# # Add your source files
# set(SOURCES
#     src/glad.c
#     src/main.cpp
#     src/engine.cpp
#     src/stb_impl.cpp
# )

# # Include directories
# include_directories(
#     include
#     include/glm
#     include/glad
#     include/stb
#     include/utils
# )

# # Add executable
# add_executable(Yggdrasil ${SOURCES})

# # Link libraries
# find_package(glfw3 REQUIRED)   # assuming you have GLFW installed
# target_link_libraries(Yggdrasil glfw)


cmake_minimum_required(VERSION 3.10)
project(YggProject)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(engine)
add_subdirectory(demo)
