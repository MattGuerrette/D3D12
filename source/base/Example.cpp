////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Example.hpp"

#include <SDL3/SDL_properties.h>

#include <fmt/format.h>

Example::Example(const char* title, uint32_t width, uint32_t height, const bool fullscreen)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        fprintf(stderr, "Failed to initialize SDL.\n");
        abort();
    }

    int flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
    if (fullscreen)
    {
        int        numDisplays = 0;
        const auto displays = SDL_GetDisplays(&numDisplays);
        assert(numDisplays != 0);

        const auto mode = SDL_GetDesktopDisplayMode(displays[0]);
        width = mode->w;
        height = mode->h;
        SDL_free(displays);

        flags |= SDL_WINDOW_FULLSCREEN;
    }

    m_window = SDL_CreateWindow(title, static_cast<int>(width), static_cast<int>(height), flags);
    if (!m_window)
    {
        fprintf(stderr, "Failed to create SDL window.\n");
        abort();
    }
    m_running = true;

    auto hwnd = static_cast<HWND>(
        SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    m_context = std::make_unique<D3D12Context>(hwnd);
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>(m_window);

    // Timer.SetTargetElapsedSeconds(1.0f / static_cast<float>(mode.refresh_rate));
    m_timer.SetFixedTimeStep(false);

    const auto      actualWidth = GetFrameWidth();
    const auto      actualHeight = GetFrameHeight();
    const float     aspect = static_cast<float>(actualWidth) / static_cast<float>(actualHeight);
    constexpr float fov = XMConvertToRadians(75.0f);
    constexpr float nearPlane = 0.01f;
    constexpr float farPlane = 1000.0f;

    m_camera = std::make_unique<Camera>(Vector3::Zero, Vector3::Forward, Vector3::Up, fov, aspect, nearPlane, farPlane,
                                        800.0f, 600.0f);
}

Example::~Example()
{
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
}

uint32_t Example::GetFrameWidth() const
{
    int32_t w;
    SDL_GetWindowSizeInPixels(m_window, &w, nullptr);
    return w;
}

uint32_t Example::GetFrameHeight() const
{
    int32_t h;
    SDL_GetWindowSizeInPixels(m_window, nullptr, &h);
    return h;
}

int Example::Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    if (!Load())
    {
        return EXIT_FAILURE;
    }

    while (m_running)
    {
        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                m_running = false;
                continue;
            }

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                m_context->ResizeSwapChain();

                const auto      actualWidth = GetFrameWidth();
                const auto      actualHeight = GetFrameHeight();
                const float     aspect = static_cast<float>(actualWidth) / static_cast<float>(actualHeight);
                constexpr float fov = XMConvertToRadians(75.0f);
                constexpr float nearPlane = 0.01f;
                constexpr float farPlane = 1000.0f;
                m_camera->setProjection(fov, aspect, nearPlane, farPlane, actualWidth, actualHeight);
            }

            if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP)
            {
                m_keyboard->RegisterKeyEvent(&e.key);
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                m_mouse->RegisterMouseButton(&e.button);
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                m_mouse->RegisterMouseMotion(&e.motion);
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL)
            {
                m_mouse->RegisterMouseWheel(&e.wheel);
            }
        }

        const auto elapsed = static_cast<float>(m_timer.GetElapsedSeconds());
        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_LSHIFT) && m_mouse->LeftPressed() && m_mouse->RightPressed())
        {
            m_camera->moveForward(elapsed * m_mouse->RelativeY());
        }

        if (m_keyboard->IsKeyClicked(SDL_SCANCODE_ESCAPE))
        {
            Quit();
        }

        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_W))
        {
            m_camera->moveForward(elapsed);
        }

        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_S))
        {
            m_camera->moveBackward(elapsed);
        }

        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_A))
        {
            m_camera->strafeLeft(elapsed);
        }

        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_D))
        {
            m_camera->strafeRight(elapsed);
        }

        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_LEFT))
        {
            m_camera->rotate(0.0f, elapsed);
        }

        if (m_keyboard->IsKeyPressed(SDL_SCANCODE_RIGHT))
        {
            m_camera->rotate(0.0f, -elapsed);
        }

        m_timer.Tick([this]() { Update(m_timer); });

        m_context->BeginFrame();

        const auto commandList = m_context->CommandList();
        Render(commandList, m_timer);

        m_context->EndFrame();

        m_keyboard->Update();
        m_mouse->Update();
    }

    m_context->WaitForGpuCompletion();

    return 0;
}

void Example::Quit()
{
    m_running = false;
}
