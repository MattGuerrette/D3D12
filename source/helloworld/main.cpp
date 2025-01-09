////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include "Example.hpp"
#include "File.hpp"

#include <SDL3/SDL_main.h>

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 614;
}

extern "C"
{
    __declspec(dllexport) extern const auto* D3D12SDKPath = u8".\\D3D12\\";
}

using namespace DirectX;

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector3 Position;
    Vector4 Color;
};

XM_ALIGNED_STRUCT(256) SceneConstantBuffer
{
    Matrix ModelViewProjection;
};

class HelloWorld final : public Example
{
public:
    explicit HelloWorld(bool fullscreen);
    HelloWorld(const HelloWorld& other) = delete;
    HelloWorld& operator=(const HelloWorld& other) = delete;

    ~HelloWorld() override;

    bool Load() override;

    void Update(const GameTimer& timer) override;

    void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) override;

private:
    void CreateRootSignature();

    void CreateBuffers();

    void CreatePipelineState();

    void UpdateUniforms();

    winrt::com_ptr<ID3D12RootSignature>  m_rootSignature;
    winrt::com_ptr<ID3D12PipelineState>  m_pipelineState;
    winrt::com_ptr<ID3D12Resource>       m_vertexBuffer;
    winrt::com_ptr<ID3D12DescriptorHeap> m_cbvDescriptorHeap;
    D3D12_VERTEX_BUFFER_VIEW             m_vertexBufferView;
    winrt::com_ptr<ID3D12Resource>       m_constBuffer;
    SceneConstantBuffer                  m_constBufferData;
    UINT8*                               m_constBufferDataBegin;
    float                                m_rotationY = 0.0f;
    float                                m_rotationX = 0.0f;
};

HelloWorld::HelloWorld(bool fullscreen)
    : Example("Hello, D3D12", 800, 600, fullscreen), m_vertexBufferView(), m_constBufferDataBegin(nullptr)
{
}

HelloWorld::~HelloWorld() = default;

bool HelloWorld::Load()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvDescriptorHeapDesc = {};
    cbvDescriptorHeapDesc.NumDescriptors = 1;
    cbvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    winrt::check_hresult(
        m_context->Device()->CreateDescriptorHeap(&cbvDescriptorHeapDesc, IID_PPV_ARGS(&m_cbvDescriptorHeap)));

    CreateRootSignature();

    CreateBuffers();

    CreatePipelineState();

    SDL_HideCursor();

    return true;
}

void HelloWorld::Update(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.GetElapsedSeconds());

    SDL_WarpMouseInWindow(m_window, GetFrameWidth() / 2, GetFrameHeight() / 2);

    m_rotationX -= m_mouse->RelativeX() * elapsed;
    m_rotationY -= m_mouse->RelativeY() * elapsed;

    m_camera->rotate(m_rotationY, m_rotationX);

    // Clamp m_rotationX between 75 degrees and -75 degrees
    m_rotationY = std::clamp(m_rotationY, -XMConvertToRadians(75.0f), XMConvertToRadians(75.0f));
}

void HelloWorld::Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer)
{
    UpdateUniforms();

    // Set the root signature
    commandList->SetGraphicsRootSignature(m_rootSignature.get());

    ID3D12DescriptorHeap* heaps[] = {m_cbvDescriptorHeap.get()};
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(0, m_cbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    // Set the pipeline state
    commandList->SetPipelineState(m_pipelineState.get());

    // Set the primitive topology
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set the vertex buffer
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

    // Draw instanced
    commandList->DrawInstanced(3, 1, 0, 0);
}

void HelloWorld::UpdateUniforms()
{
    auto position = Vector3(0.0f, 0.0, -10.0f);
    auto rotationX = 0.0f;
    auto rotationY = 0.0f;
    auto scaleFactor = 3.0f;

    const Vector3 xAxis = Vector3::Right;
    const Vector3 yAxis = Vector3::Up;

    Matrix xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
    Matrix yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
    Matrix rotation = xRot * yRot;

    Matrix translation = Matrix::CreateTranslation(position);
    Matrix scale = Matrix::CreateScale(scaleFactor);
    Matrix model = scale * rotation * translation;

    m_constBufferData.ModelViewProjection = model * m_camera->viewProjection();
    memcpy(m_constBufferDataBegin, &m_constBufferData, sizeof(m_constBufferData));
}

void HelloWorld::CreateRootSignature()
{
    ID3D12Device* device = m_context->Device();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    CD3DX12_ROOT_PARAMETER1   rootParams[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    rootParams[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, rootSignatureFlags);

    winrt::com_ptr<ID3DBlob> signature;
    winrt::com_ptr<ID3DBlob> error;
    winrt::check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion,
                                                               signature.put(), error.put()));
    winrt::check_hresult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                     IID_PPV_ARGS(&m_rootSignature)));
}

void HelloWorld::CreatePipelineState()
{
    ID3D12Device* device = m_context->Device();

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    auto rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

    std::vector<std::byte> vertexShader;
    std::vector<std::byte> pixelShader;
    try
    {
        File vs("SimpleShaderVS.bin");
        File ps("SimpleShaderPS.bin");

        vertexShader = vs.ReadAll();
        pixelShader = ps.ReadAll();
    }
    catch (const std::exception& e)
    {
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
    psoDesc.pRootSignature = m_rootSignature.get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.data(), vertexShader.size());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.data(), pixelShader.size());
    psoDesc.RasterizerState = rasterDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_context->BackBufferFormat();
    psoDesc.SampleDesc.Count = 1;
    winrt::check_hresult(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void HelloWorld::CreateBuffers()
{
    ID3D12Device* device = m_context->Device();

    // Define the geometry for a triangle.
    constexpr Vertex triangleVertices[] = {{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                                           {{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                                           {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    constexpr UINT vertexBufferSize = sizeof(triangleVertices);
    const auto     heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto     resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    winrt::check_hresult(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                         IID_PPV_ARGS(&m_vertexBuffer)));

    // Copy triangle data to vertex buffer
    UINT8*              vertexDataBegin;
    const CD3DX12_RANGE readRange(0, 0);
    winrt::check_hresult(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, triangleVertices, sizeof(triangleVertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    // Create the constant buffer.
    {
        constexpr UINT constantBufferSize = sizeof(SceneConstantBuffer); // CB size is required to be 256-byte aligned.

        const auto cbHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto cbHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        winrt::check_hresult(device->CreateCommittedResource(&cbHeapProps, D3D12_HEAP_FLAG_NONE, &cbHeapDesc,
                                                             D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                             IID_PPV_ARGS(&m_constBuffer)));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = m_constBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
        device->CreateConstantBufferView(&cbvDesc, m_cbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay
        winrt::check_hresult(m_constBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constBufferDataBegin)));
        memcpy(m_constBufferDataBegin, &m_constBufferData, sizeof(m_constBufferData));
    }
}

int main(const int argc, char** argv)
{
    bool fullscreen = false;
    if (argc > 1)
    {
        for (int i = 0; i < argc; i++)
        {
            if (strcmp(argv[i], "--fullscreen") == 0)
            {
                fullscreen = true;
            }
        }
    }
    const auto example = std::make_unique<HelloWorld>(fullscreen);
    return example->Run(argc, argv);
}
