////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <winrt/base.h>

#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include <string>
#include <memory>

#include <SDL2/SDL.h>

#include "GameTimer.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"
#include "Camera.hpp"

class Example
{
public:
    SDL_Window* Window;
    Example(const char* title, uint32_t width, uint32_t height);

    virtual ~Example();

    int Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv);

    void Quit();

    [[nodiscard]] uint32_t GetFrameWidth() const;

    [[nodiscard]] uint32_t GetFrameHeight() const;

    virtual bool Load() = 0;

    virtual void Update(const GameTimer& timer) = 0;

    virtual void SetupUi(const GameTimer& timer);

    virtual void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) = 0;

    void WaitForGpu();

protected:
    static constexpr int BufferCount = 3;

    std::unique_ptr<Camera> MainCamera;

    // Keyboard and Mouse
    std::unique_ptr<class Keyboard> Keyboard;
    std::unique_ptr<class Mouse> Mouse;

    // D3D12
    winrt::com_ptr<ID3D12Device9> Device;
    winrt::com_ptr<ID3D12GraphicsCommandList> CommandList;
    winrt::com_ptr<ID3D12CommandQueue> CommandQueue;
    winrt::com_ptr<ID3D12CommandAllocator> CommandAllocator[BufferCount];
    winrt::com_ptr<IDXGIFactory4> Factory;
    winrt::com_ptr<IDXGISwapChain3> SwapChain;
    winrt::com_ptr<ID3D12Resource> RenderTarget[BufferCount];
    winrt::com_ptr<ID3D12Resource> DepthStencilTarget;
    winrt::com_ptr<ID3D12Fence> Fence;
    winrt::com_ptr<IDXGIInfoQueue> InfoQueue;

    winrt::handle FenceEvent;
    UINT64 FenceValue[BufferCount]{};
    UINT BackBufferIndex;
    winrt::com_ptr<ID3D12DescriptorHeap> RtvDescriptorHeap;
    winrt::com_ptr<ID3D12DescriptorHeap> DsvDescriptorHeap;
    winrt::com_ptr<ID3D12DescriptorHeap> CbvDescriptorHeap;
    UINT DescriptorSize{};
    D3D12_VIEWPORT Viewport{};
    D3D12_RECT Scissor{};
    DXGI_FORMAT BackBufferFormat;
    DXGI_FORMAT DepthBufferFormat;
    D3D_FEATURE_LEVEL FeatureLevel;
    DWORD FactoryFlags;

private:
    uint32_t Width;
    uint32_t Height;
    GameTimer Timer;
    bool Running;

    D3D12_VIEWPORT GetScreenViewport() const;
    D3D12_RECT GetScissorRect() const;

    void CreateDeviceResources();

    void CreateWindowDependentResources();

    void HandleDeviceLost();

    void MoveToNextFrame();

    void Prepare(D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_PRESENT,
                 D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_RENDER_TARGET);

    void Clear() const;

    void ExplicitTransition();
    void Present(D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_RENDER_TARGET);
};
