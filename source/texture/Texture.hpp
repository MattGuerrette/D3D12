
#pragma once

#include <expected>

#include <winrt/base.h>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>

class Texture
{
public:
    static std::expected<std::unique_ptr<Texture>, int> CreateFromFile(const std::string path);

private:
    explicit Texture(ID3D12Device* device);

    winrt::com_ptr<ID3D12Resource> m_resource;
};