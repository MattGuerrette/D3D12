
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
        Example.hpp
        Example.cpp
        FileUtil.hpp
        FileUtil.cpp
)

target_include_directories(base PUBLIC .)
target_link_libraries(base PUBLIC fmt SDL2::SDL2-static SDL2::SDL2main Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility Microsoft::DirectStorage dxgi dxguid)