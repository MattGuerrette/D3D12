
add_library(base STATIC
        Camera.hpp
        Camera.cpp
        GameTimer.cpp
        GameTimer.cpp
        D3D12Context.cpp
        Example.hpp
        Example.cpp
        File.cpp)

target_include_directories(base PUBLIC .)
target_compile_options(base PUBLIC /utf-8)
target_link_libraries(base PUBLIC
        Microsoft::DirectX-Headers
        Microsoft::DirectX-Guids
        Microsoft::DirectX12-Agility
        Microsoft::DirectStorage
        Microsoft::DirectXTK12
        Microsoft::GameInput
        dxgi
        dxguid)