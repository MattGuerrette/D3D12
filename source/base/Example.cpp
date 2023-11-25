////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
// #include "imgui.h"
// #include "imgui_impl_sdl2.h"
// #include "imgui_impl_metal.h"

#include "Example.hpp"

#include <SDL_syswm.h>
#include <fmt/format.h>

static bool shouldWarp = false;

namespace
{
    void FindCompatibleAdapter(IDXGIAdapter1** adapter, const winrt::com_ptr<IDXGIFactory4>& factory)
    {
        *adapter = nullptr;

        winrt::com_ptr<IDXGIAdapter1> temp;
        const winrt::com_ptr<IDXGIFactory6> factory6 = factory.as<IDXGIFactory6>();
        if (factory6)
        {
            for (UINT index = 0; SUCCEEDED(
                     factory6->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_UNSPECIFIED,
                         IID_PPV_ARGS(&temp)));
                 index++)
            {
                DXGI_ADAPTER_DESC1 desc;
                winrt::check_hresult(temp->GetDesc1(&desc));

                // Ignore software devices
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    continue;
                }


#ifdef _DEBUG
                // const std::string message = fmt::format("Direct3D Adapter ({}): VID:{}, PID:{} - %{}", index,
                //                                         desc.VendorId,
                //                                         desc.DeviceId, desc.Description);
                // OutputDebugStringA(message.c_str());
#endif

                break;
            }
        }

        if (!temp)
        {
            temp = nullptr;
            for (UINT adapterIndex = 0;
                 SUCCEEDED(factory->EnumAdapters1(
                     adapterIndex,
                     temp.put()));
                 adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                winrt::check_hresult(temp->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId,
                           desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }

        if (!temp)
        {
            throw std::runtime_error("No Direct3D device found.");
        }

        *adapter = temp.detach();
    }
}


Example::Example(const char* title, uint32_t width, uint32_t height)
    : BackBufferIndex(0)
      , BackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      , DepthBufferFormat(DXGI_FORMAT_D32_FLOAT)
      , FactoryFlags(0)
{
    // IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO();
    // io.IniFilename = nullptr;
    // io.IniSavingRate = 0.0f;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    // ImGui::StyleColorsDark();


    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL.\n");
        abort();
    }

    int flags = SDL_WINDOW_ALLOW_HIGHDPI;
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);
    width = mode.w;
    height = mode.h;
    Window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)width, (int)height, flags);
    if (!Window)
    {
        fprintf(stderr, "Failed to create SDL window.\n");
        abort();
    }
    Running = true;

    Width = width;
    Height = height;


    // ImGui_ImplMetal_Init(Device.get());
    // ImGui_ImplSDL2_InitForMetal(Window);

    CreateDeviceResources();

    CreateWindowDependentResources();

    Keyboard = std::make_unique<class Keyboard>();
    Mouse = std::make_unique<class Mouse>(Window);


    Timer.SetTargetElapsedSeconds(1.0f / static_cast<float>(mode.refresh_rate));
    Timer.SetFixedTimeStep(true);

    const auto actualWidth = GetFrameWidth();
    const auto actualHeight = GetFrameHeight();
    const float aspect = (float)actualWidth / (float)actualHeight;
    const float fov = XMConvertToRadians(75.0f);
    const float znear = 0.01f;
    const float zfar = 1000.0f;

    MainCamera = std::make_unique<Camera>(Vector3::Zero,
                                          Vector3::Forward,
                                          Vector3::Up,
                                          fov, aspect, znear, zfar);
}

Example::~Example()
{
    // Cleanup
    // ImGui_ImplMetal_Shutdown();
    // ImGui_ImplSDL2_Shutdown();
    // ImGui::DestroyContext();

    //WaitForGpu();

    if (Window != nullptr)
    {
        SDL_DestroyWindow(Window);
    }
    SDL_Quit();
}

void Example::CreateDeviceResources()
{
#ifdef _DEBUG
    winrt::com_ptr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
    else
    {
        OutputDebugStringW(L"Failed to enable Direct3D Debug Layer");
    }

    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
    {
        FactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

        winrt::check_hresult(
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
        winrt::check_hresult(
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));
    }
#endif

    winrt::check_hresult(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory)));

    winrt::com_ptr<IDXGIAdapter1> adapter;
    FindCompatibleAdapter(adapter.put(), Factory);


    winrt::check_hresult(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));
    Device->SetName(L"Device");

#ifdef _DEBUG
    winrt::com_ptr<ID3D12InfoQueue> infoQueue = Device.as<ID3D12InfoQueue>();
    if (infoQueue)
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        infoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    winrt::check_hresult(Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&CommandQueue)));
    CommandQueue->SetName(L"CommandQueue");

    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = BufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    winrt::check_hresult(Device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&RtvDescriptorHeap)));

    DescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    winrt::check_hresult(Device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&DsvDescriptorHeap)));


    D3D12_DESCRIPTOR_HEAP_DESC cbvDescriptorHeapDesc = {};
    cbvDescriptorHeapDesc.NumDescriptors = 1;
    cbvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    winrt::check_hresult(Device->CreateDescriptorHeap(&cbvDescriptorHeapDesc, IID_PPV_ARGS(&CbvDescriptorHeap)));

    for (UINT i = 0; i < BufferCount; i++)
    {
        winrt::check_hresult(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                            IID_PPV_ARGS(&CommandAllocator[i])));

        wchar_t name[25] = {};
        swprintf_s(name, L"RTV[%u] Allocator", i);
        CommandAllocator[i]->SetName(name);
    }

    winrt::check_hresult(Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
                                                    IID_PPV_ARGS(&CommandList)));


    winrt::check_hresult(Device->CreateFence(FenceValue[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    FenceValue[0]++;
    Fence->SetName(L"Fence");

    FenceEvent.attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!FenceEvent)
    {
        throw std::runtime_error("Failed to create fence event");
    }
}

void Example::MoveToNextFrame()
{
    const UINT64 currentFenceValue = FenceValue[BackBufferIndex];
    winrt::check_hresult(CommandQueue->Signal(Fence.get(), currentFenceValue));

    // Update back buffer index
    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    // Wait for frame to be ready
    if (Fence->GetCompletedValue() < FenceValue[BackBufferIndex])
    {
        winrt::check_hresult(Fence->SetEventOnCompletion(FenceValue[BackBufferIndex], FenceEvent.get()));
        WaitForSingleObjectEx(FenceEvent.get(), INFINITE, FALSE);
    }

    // Update fence value for next frame
    FenceValue[BackBufferIndex] = currentFenceValue + 1;
}

void Example::CreateWindowDependentResources()
{
    WaitForGpu();

    for (UINT i = 0; i < BufferCount; i++)
    {
        RenderTarget[i] = nullptr;
        DepthStencilTarget = nullptr;
        FenceValue[i] = FenceValue[BackBufferIndex];
    }

    const UINT backBufferWidth = Width;
    const UINT backBufferHeight = Height;
    const DXGI_FORMAT backBufferFormat = BackBufferFormat;

    // Swapchain already exists, resize it
    if (SwapChain)
    {
        HRESULT result = SwapChain->ResizeBuffers(
            BufferCount,
            backBufferWidth,
            backBufferHeight,
            BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                      static_cast<unsigned int>((result == DXGI_ERROR_DEVICE_REMOVED)
                                                    ? Device->GetDeviceRemovedReason()
                                                    : result));
            OutputDebugStringA(buff);
#endif

            // Recreate device and swapchain
            HandleDeviceLost();
            return;
        }
        winrt::check_hresult(result);
    }
    else
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = BufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;


        winrt::com_ptr<IDXGISwapChain1> swapChain;

        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        SDL_GetWindowWMInfo(Window, &info);

        winrt::check_hresult(Factory->CreateSwapChainForHwnd(CommandQueue.get(), info.info.win.window, &swapChainDesc,
                                                             nullptr, nullptr, swapChain.put()));
        SwapChain = swapChain.as<IDXGISwapChain3>();

        // Disable ALT-ENTER fullscreen
        winrt::check_hresult(Factory->MakeWindowAssociation(info.info.win.window, DXGI_MWA_NO_ALT_ENTER));
    }
    winrt::check_hresult(SwapChain->SetRotation(DXGI_MODE_ROTATION_IDENTITY));


    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
        RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < BufferCount; i++)
    {
        winrt::check_hresult(SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTarget[i])));

        wchar_t name[25] = {};
        swprintf_s(name, L"RTV[%u]", i);
        RenderTarget[i]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
        renderTargetViewDesc.Format = BackBufferFormat;
        renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;


        Device->CreateRenderTargetView(RenderTarget[i].get(),
                                       &renderTargetViewDesc, rtvDescriptor);

        rtvDescriptor.Offset(1, DescriptorSize);
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    if (DepthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DepthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1,
            1
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClear = {};
        depthClear.Format = DepthBufferFormat;
        depthClear.DepthStencil.Depth = 1.0f;
        depthClear.DepthStencil.Stencil = 0;
        winrt::check_hresult(Device->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClear,
            IID_PPV_ARGS(&DepthStencilTarget)
        ));
        DepthStencilTarget->SetName(L"Depth Stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = DepthBufferFormat;
        depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        Device->CreateDepthStencilView(DepthStencilTarget.get(), &depthStencilViewDesc,
                                       DsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

void Example::Prepare(const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after)
{
    winrt::check_hresult(CommandAllocator[BackBufferIndex]->Reset());
    winrt::check_hresult(CommandList->Reset(CommandAllocator[BackBufferIndex].get(), nullptr));

    if (before != after)
    {
        // Transition state
        const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            RenderTarget[BackBufferIndex].get(),
            before, after);
        CommandList->ResourceBarrier(1, &barrier);
    }
}


void Example::Clear() const
{
    //PIXBeginEvent(CommandList.get(), PIX_COLOR_DEFAULT, L"Clear");

    const auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(BackBufferIndex), DescriptorSize
    );
    const auto dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    CommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::CornflowerBlue, 0, nullptr);
    CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    const auto viewport = GetScreenViewport();
    const auto scissor = GetScissorRect();
    CommandList->RSSetViewports(1, &viewport);
    CommandList->RSSetScissorRects(1, &scissor);

    //PIXEndEvent(CommandList.get());
}

D3D12_VIEWPORT Example::GetScreenViewport() const
{
    // TODO: Manage through member updated on resize
    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = viewport.TopLeftY = 0.f;
    viewport.Width = static_cast<float>(Width);
    viewport.Height = static_cast<float>(Height);
    viewport.MinDepth = D3D12_MIN_DEPTH;
    viewport.MaxDepth = D3D12_MAX_DEPTH;
    return viewport;
}

D3D12_RECT Example::GetScissorRect() const
{
    // TODO: Manage through member updated on resize
    D3D12_RECT rect{};
    rect.left = rect.top = 0;
    rect.right = static_cast<LONG>(Width);
    rect.bottom = static_cast<LONG>(Height);
    return rect;
}

void Example::ExplicitTransition()
{
    // Transition state
    const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        RenderTarget[BackBufferIndex].get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    CommandList->ResourceBarrier(1, &barrier);
}


void Example::Present(const D3D12_RESOURCE_STATES before)
{
    //if (before != D3D12_RESOURCE_STATE_PRESENT)
    //{
    //    // Transition state
    //    const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    //        RenderTarget[BackBufferIndex].get(),
    //        before, D3D12_RESOURCE_STATE_PRESENT);
    //    CommandList->ResourceBarrier(1, &barrier);
    //}

    if (const HRESULT result = SwapChain->Present(1, 0); result ==
        DXGI_ERROR_DEVICE_REMOVED || result ==
        DXGI_ERROR_DEVICE_RESET)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                  static_cast<unsigned int>((result == DXGI_ERROR_DEVICE_REMOVED)
                                                ? Device->GetDeviceRemovedReason()
                                                : result));
        OutputDebugStringA(buff);
#endif
        HandleDeviceLost();
    }
    else
    {
        winrt::check_hresult(result);

        MoveToNextFrame();

        if (!Factory->IsCurrent())
        {
            // Create a new factory
            Factory = nullptr;
            winrt::check_hresult(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory)));
        }
    }
}

void Example::WaitForGpu()
{
    if (CommandQueue && Fence && FenceEvent)
    {
        UINT64 fenceValue = FenceValue[BackBufferIndex];
        if (SUCCEEDED(CommandQueue->Signal(Fence.get(), fenceValue)))
        {
            if (SUCCEEDED(Fence->SetEventOnCompletion(fenceValue, FenceEvent.get())))
            {
                WaitForSingleObjectEx(FenceEvent.get(), INFINITE, FALSE);

                FenceValue[BackBufferIndex]++;
            }
        }
    }
}

void Example::HandleDeviceLost()
{
    for (UINT i = 0; i < BufferCount; i++)
    {
        CommandAllocator[i] = nullptr;
        RenderTarget[i] = nullptr;
    }

    DepthStencilTarget = nullptr;
    CommandQueue = nullptr;
    CommandList = nullptr;
    Fence = nullptr;
    RtvDescriptorHeap = nullptr;
    DsvDescriptorHeap = nullptr;
    SwapChain = nullptr;
    Device = nullptr;
    Factory = nullptr;

#ifdef _DEBUG
    {
        winrt::com_ptr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
        {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL,
                                     static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_SUMMARY |
                                         DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    // Create device resources
    CreateDeviceResources();

    // Create window dependent resources
    CreateWindowDependentResources();
}


uint32_t Example::GetFrameWidth() const
{
    int32_t w;
    SDL_Metal_GetDrawableSize(Window, &w, nullptr);
    return w;
}

uint32_t Example::GetFrameHeight() const
{
    int32_t h;
    SDL_Metal_GetDrawableSize(Window, nullptr, &h);


    return h;
}

int Example::Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    if (!Load())
    {
        return EXIT_FAILURE;
    }

#if defined(__IPHONEOS__) || defined(__TVOS__)
	SDL_iOSSetAnimationCallback(Window, 1, AnimationCallback, this);
#else
    while (Running)
    {
        SDL_Event e;
        SDL_ShowCursor(SDL_ENABLE);

        while (SDL_PollEvent(&e))
        {
            //ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT)
            {
                Running = false;
                continue;
            }
            if (e.type == SDL_WINDOWEVENT)
            {
                switch (e.window.event)
                {
                case SDL_WINDOWEVENT_SHOWN:
                {
                    break;
                }
                }
            }



            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
                Keyboard->RegisterKeyEvent(&e.key);
            }
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                Mouse->RegisterMouseButton(&e.button);
            }
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                Mouse->RegisterMouseButton(&e.button);
            }
            if (e.type == SDL_MOUSEMOTION)
            {
                Mouse->RegisterMouseMotion(&e.motion);
            }
            if (e.type == SDL_MOUSEWHEEL)
            {
                Mouse->RegisterMouseWheel(&e.wheel);
            }
        }


        const auto elapsed = static_cast<float>(Timer.GetElapsedSeconds());
        if (Keyboard->IsKeyPressed(SDL_SCANCODE_LSHIFT) && Mouse->LeftPressed() && Mouse->RightPressed())
        {
            if (!shouldWarp)
            {
                Mouse->Warp();
            }
            //MainCamera->MoveForward(Mouse->WheelY() * 10.0f * elapsed);
            MainCamera->MoveForward(elapsed * Mouse->RelativeY());
        }
        else
        {
            if (shouldWarp)
            {
                shouldWarp = false;
            }

            MainCamera->RotateY(Mouse->WheelX() * 10.0f * elapsed);
        }

        if (Keyboard->IsKeyClicked(SDL_SCANCODE_ESCAPE))
        {
            Quit();
        }


        if (Keyboard->IsKeyPressed(SDL_SCANCODE_W))
        {
            MainCamera->MoveForward(elapsed);
        }

        if (Keyboard->IsKeyPressed(SDL_SCANCODE_S))
        {
            MainCamera->MoveBackward(elapsed);
        }

        if (Keyboard->IsKeyPressed(SDL_SCANCODE_A))
        {
            MainCamera->StrafeLeft(elapsed);
        }

        if (Keyboard->IsKeyPressed(SDL_SCANCODE_D))
        {
            MainCamera->StrafeRight(elapsed);
        }

        if (Keyboard->IsKeyPressed(SDL_SCANCODE_LEFT))
        {
            MainCamera->RotateY(elapsed);
        }

        if (Keyboard->IsKeyPressed(SDL_SCANCODE_RIGHT))
        {
            MainCamera->RotateY(-elapsed);
        }

        Timer.Tick([this]()
        {
            Update(Timer);
        });

        Prepare();
        Clear();

        const auto commandList = CommandList.get();
        Render(commandList, Timer);

        ExplicitTransition();
        winrt::check_hresult(commandList->Close());
        CommandQueue->ExecuteCommandLists(1, CommandListCast(&commandList));
        Present();

        //Mouse->Warp();
        Keyboard->Update();
        Mouse->Update();
    }
#endif

    WaitForGpu();

    return 0;
}

void Example::SetupUi(const GameTimer& timer)
{
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    // ImGui::SetNextWindowPos(ImVec2(10, 10));
    // ImGui::SetNextWindowSize(ImVec2(125 * ImGui::GetIO().DisplayFramebufferScale.x, 0), ImGuiCond_FirstUseEver);
    // ImGui::Begin("Metal Example",
    // 	nullptr,
    // 	ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    // ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(Window), timer.GetFramesPerSecond());
    // ImGui::Text("Press Esc to Quit");
    // ImGui::End();
    // ImGui::PopStyleVar();
}

void Example::Quit()
{
    Running = false;
}
