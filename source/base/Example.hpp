////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>

#include "Camera.hpp"
#include "D3D12Context.hpp"
#include "GameTimer.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"

class Example
{
public:
    explicit Example(const char* title, uint32_t width, uint32_t height, bool fullscreen = false);

    virtual ~Example();

    int Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv);

    void Quit();

    [[nodiscard]] uint32_t GetFrameWidth() const;

    [[nodiscard]] uint32_t GetFrameHeight() const;

    virtual bool Load() = 0;

    virtual void Update(const GameTimer& timer) = 0;

    virtual void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) = 0;

protected:
    static constexpr int FRAME_COUNT = 3;

    SDL_Window*                   m_window;
    std::unique_ptr<Camera>       m_camera;
    std::unique_ptr<Keyboard>     m_keyboard;
    std::unique_ptr<Mouse>        m_mouse;
    std::unique_ptr<D3D12Context> m_context;

private:
    GameTimer m_timer;
    bool      m_running;
};
