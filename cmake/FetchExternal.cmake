include(FetchContent)

FetchContent_Declare(sal
        GIT_REPOSITORY "https://github.com/MattGuerrette/sal.git"
        GIT_TAG main
)

FetchContent_Declare(directxmath
        GIT_REPOSITORY "https://github.com/Microsoft/DirectXMath.git"
        GIT_TAG dec2022
)

FetchContent_Declare(directxheaders
        GIT_REPOSITORY "https://github.com/microsoft/DirectX-Headers.git"
        GIT_TAG v1.610.2
)

FetchContent_Declare(imgui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG v1.89.9
)

FetchContent_Declare(sdl2
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG release-2.28.1
)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
        GIT_TAG 10.1.1
)

set(KTX_FEATURE_STATIC_LIBRARY ON)
set(KTX_FEATURE_TOOLS OFF)
set(KTX_FEATURE_TESTS OFF)
if (APPLE)
    set(BASISU_SUPPORT_SSE OFF)
endif ()
FetchContent_Declare(
        ktx
        GIT_REPOSITORY "https://github.com/KhronosGroup/KTX-Software.git"
        GIT_TAG v4.0.0
)
FetchContent_GetProperties(ktx)
if (NOT ktx_POPULATED)
    FetchContent_Populate(ktx)
    add_subdirectory(${ktx_SOURCE_DIR} ${ktx_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.10
)

FetchContent_MakeAvailable(sal directxmath directxheaders fmt imgui sdl2 cgltf)

include_directories(${sal_SOURCE_DIR} ${cgltf_SOURCE_DIR} ${directxmath_SOURCE_DIR}/Inc)
