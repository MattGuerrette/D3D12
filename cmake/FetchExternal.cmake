include(FetchContent)


FetchContent_Declare(imgui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG v1.89.9
)

FetchContent_Declare(sdl3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG main
)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
        GIT_TAG 10.1.1
)

FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.10
)

FetchContent_MakeAvailable(fmt imgui sdl3 cgltf)

include_directories(${cgltf_SOURCE_DIR})
