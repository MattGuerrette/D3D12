
#pragma once

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <winrt/base.h>

#include "File.hpp"

class Texture
{
public:
    explicit Texture(const wchar_t* filename);

    void Upload(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
    void AddToDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap* descriptorHeap, size_t index);
    void Bind(ID3D12GraphicsCommandList* commandList);

private:
    std::vector<uint8_t>           m_data;
    winrt::com_ptr<ID3D12Resource> m_resource;
    ID3D12DescriptorHeap*          m_srvDescriptorHeap;
};