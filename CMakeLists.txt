cmake_minimum_required(VERSION 3.25)
project(sh++ CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_subdirectory(libs/fmt)

file(GLOB SRC src/*.cc src/*.hh)
add_executable(sh++ ${SRC})
target_compile_options(sh++ PRIVATE -Wall -Wextra -Wconversion -Werror=return-type -fdiagnostics-color=always)
target_include_directories(sh++ PRIVATE libs/fmt/include libs/clopts/include)
target_link_libraries(sh++ PRIVATE fmt)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(sh++ PRIVATE -g -O0)
else()
    target_compile_options(sh++ PRIVATE -O3)
endif()
