cmake_minimum_required(VERSION 3.24)
project(D3D12)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchExternal)

include(CompileShaders)
file(GLOB_RECURSE HLSL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)
CompileHLSLShaders("${HLSL_SHADERS}")


# Examples
add_subdirectory(source)

