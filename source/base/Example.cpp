////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Example.hpp"

#include <format>

using namespace DirectX::SimpleMath;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

Example::Example(const wchar_t* title, uint32_t width, uint32_t height, const bool fullscreen)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"Example";
    RegisterClassExW(&wcex);

    RECT rc = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    m_window = CreateWindowW(L"Example", title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
                             rc.bottom - rc.top, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    ShowWindow(m_window, SW_SHOW);

    SetWindowLongPtr(m_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    m_running = true;
    m_context = std::make_unique<D3D12Context>(m_window);

    const auto      actualWidth = GetFrameWidth();
    const auto      actualHeight = GetFrameHeight();
    const float     aspect = static_cast<float>(actualWidth) / static_cast<float>(actualHeight);
    constexpr float fov = DirectX::XMConvertToRadians(75.0f);
    constexpr float nearPlane = 0.01f;
    constexpr float farPlane = 1000.0f;
    m_camera = std::make_unique<Camera>(Vector3::Zero, Vector3::Forward, Vector3::Up, fov, aspect, nearPlane, farPlane,
                                        800.0f, 600.0f);
    m_keyboard = std::make_unique<DirectX::Keyboard>();
    m_mouse = std::make_unique<DirectX::Mouse>();
    m_mouse->SetWindow(m_window);
}

Example::~Example()
{
    DestroyWindow(m_window);
}

uint32_t Example::GetFrameWidth() const
{
    RECT rect = {};
    GetClientRect(m_window, &rect);
    return rect.right - rect.left;
}

uint32_t Example::GetFrameHeight() const
{
    RECT rect = {};
    GetClientRect(m_window, &rect);
    return rect.bottom - rect.top;
}

int Example::Run([[maybe_unused]] int argc, [[maybe_unused]] wchar_t** argv)
{
    if (!Load())
    {
        return EXIT_FAILURE;
    }

    while (m_running)
    {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        m_timer.Tick([this]() {
            Update(m_timer);

            auto lastKb = m_keyboardTracker.GetLastState();
            auto kb = m_keyboard->GetState();

            auto ms = m_mouse->GetState();
            auto lastMs = m_mouseTracker.GetLastState();

            if (kb.Escape && !lastKb.Escape)
            {
                Quit();
            }

            const auto elapsed = m_timer.ElapsedSeconds();

            if (kb.W)
            {
                m_camera->moveForward(elapsed);
            }

            if (kb.S)
            {
                m_camera->moveBackward(elapsed);
            }

            if (kb.A)
            {
                m_camera->strafeLeft(elapsed);
            }

            if (kb.D)
            {
                m_camera->strafeRight(elapsed);
            }

            m_keyboardTracker.Update(kb);
            m_mouseTracker.Update(ms);
        });

        m_context->BeginFrame();

        const auto commandList = m_context->CommandList();
        Render(commandList, m_timer);

        m_context->EndFrame();
    }

    m_context->WaitForGpuCompletion();

    return 0;
}

void Example::Quit()
{
    m_running = false;
}

void Example::OnResize()
{
    if (!m_context)
        return;

    m_context->WaitForGpuCompletion();

    m_context->ResizeSwapChain();

    const auto      actualWidth = GetFrameWidth();
    const auto      actualHeight = GetFrameHeight();
    const float     aspect = static_cast<float>(actualWidth) / static_cast<float>(actualHeight);
    constexpr float fov = DirectX::XMConvertToRadians(75.0f);
    constexpr float nearPlane = 0.01f;
    constexpr float farPlane = 1000.0f;
    m_camera->setProjection(fov, aspect, nearPlane, farPlane, actualWidth, actualHeight);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto example = reinterpret_cast<Example*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_CLOSE:
        example->Quit();
        break;

    case WM_ACTIVATE:
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_ACTIVATEAPP:
        if (example)
        {
            DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
            DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        }
        break;

    case WM_SIZE: {
        if (!example)
            break;

        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        if (width != 0 && height != 0)
        {
            example->OnResize();
        }
        break;
    }
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}