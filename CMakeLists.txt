cmake_minimum_required(VERSION 3.5)

# Project
project(360Stream)

# Sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
file(GLOB_RECURSE HEADERS include/*.hpp include/*.h)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Includes
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Flags
target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -O3 -Wall -Wextra -Werror -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wconversion -Wunreachable-code -Wno-error=maybe-uninitialized -Wno-error=sign-conversion -Wpedantic>
)

# Add Libraries
set(BUILD_EXAMPLE OFF)
add_subdirectory(lib/libuvc)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE uvc ${OpenCV_LIBS})
