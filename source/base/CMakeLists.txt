
add_library(base STATIC
        Camera.hpp
        Camera.cpp
        Keyboard.hpp
        Keyboard.cpp
        Mouse.hpp
        Mouse.cpp
        GameTimer.cpp
        GameTimer.cpp
        SimpleMath.cpp
        D3D12Context.cpp
        Example.hpp
        Example.cpp
        File.cpp
        "File.cpp")

target_include_directories(base PUBLIC .)
target_compile_options(base PUBLIC /utf-8)
target_link_libraries(base PUBLIC
        fmt::fmt
        SDL3::SDL3
        Microsoft::DirectX-Headers
        Microsoft::DirectX-Guids
        Microsoft::DirectX12-Agility
        Microsoft::DirectStorage
        dxgi
        dxguid)