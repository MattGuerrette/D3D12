cmake_minimum_required(VERSION 3.24)
project(D3D12)

set(CMAKE_CXX_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchExternal)

find_package(directx-headers CONFIG REQUIRED)
find_package(directx12-agility CONFIG REQUIRED)
find_package(directxtk12 CONFIG REQUIRED)
find_package(dstorage CONFIG REQUIRED)

include(CompileShaders)
file(GLOB_RECURSE HLSL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)
CompileHLSLShaders("${HLSL_SHADERS}")


# Examples
add_subdirectory(source)

