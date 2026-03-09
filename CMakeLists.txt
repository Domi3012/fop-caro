cmake_minimum_required(VERSION 3.10)
project(GameCaro C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# tat cac vi du va game cua raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

# them raylib
add_subdirectory(external/raylib)


# build
file(GLOB SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

# link thu vien raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)