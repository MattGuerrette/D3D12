////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <string>

#include <Windows.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>

#include "Camera.hpp"
#include "D3D12Context.hpp"
#include "GameTimer.hpp"

class Example
{
public:
    explicit Example(const wchar_t* title, uint32_t width, uint32_t height, bool fullscreen = false);

    virtual ~Example();

    int Run([[maybe_unused]] int argc, [[maybe_unused]] wchar_t** argv);

    void Quit();

    [[nodiscard]] uint32_t GetFrameWidth() const;

    [[nodiscard]] uint32_t GetFrameHeight() const;

    void OnResize();

    virtual bool Load() = 0;

    virtual void Update(const GameTimer& timer) = 0;

    virtual void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) = 0;

protected:
    static constexpr int FRAME_COUNT = 3;

    HWND                                    m_window;
    std::unique_ptr<Camera>                 m_camera;
    std::unique_ptr<D3D12Context>           m_context;
    std::unique_ptr<DirectX::Keyboard>      m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;
    std::unique_ptr<DirectX::Mouse>         m_mouse;
    DirectX::Mouse::ButtonStateTracker      m_mouseTracker;

private:
    GameTimer m_timer;
    bool      m_running;
};
