
#include "Texture.hpp"

#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>

using namespace DirectX;

namespace
{
    DXGI_FORMAT ConvertColorspace(DXGI_FORMAT format)
    {
        if (format == DXGI_FORMAT_R8G8B8A8_UNORM)
        {
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        }

        return format;
    }
} // namespace

Texture::Texture(const std::string& filename)
{
    try
    {
        File file(filename.c_str());
        m_data = file.ReadAll();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error("Failed to load texture file");
    }
}

void Texture::Upload(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    ResourceUploadBatch upload(device);
    upload.Begin();
    winrt::check_hresult(CreateDDSTextureFromMemory(device, upload, m_data.data(), m_data.size(), m_resource.put()));
    auto finish = upload.End(commandQueue);
    finish.wait();
}

void Texture::AddToDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap* descriptorHeap, size_t index)
{
    m_srvDescriptorHeap = descriptorHeap;

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(
        descriptorHeap->GetCPUDescriptorHandleForHeapStart(), index,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = ConvertColorspace(m_resource->GetDesc().Format);
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = m_resource->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    device->CreateShaderResourceView(m_resource.get(), &srvDesc, hDescriptor);
}

void Texture::Bind(ID3D12GraphicsCommandList* commandList)
{
    CD3DX12_GPU_DESCRIPTOR_HANDLE hDescriptor(m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootDescriptorTable(0, hDescriptor);
}