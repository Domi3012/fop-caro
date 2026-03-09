cmake_minimum_required(VERSION 3.10)

project(GameCaro C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix Git Bash sh.exe issue
if(WIN32)
    set(CMAKE_SH "CMAKE_SH-NOTFOUND")
endif()

# tat vi du, game cua raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

# Add raylib
add_subdirectory(external/raylib)

# Collect source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include headers
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Windows libraries required by raylib
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm
        gdi32
    )
endif()
