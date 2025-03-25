////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <memory>

#include "Example.hpp"
#include "File.hpp"

using namespace DirectX::SimpleMath;

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
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

struct StaticGeometry
{
    winrt::com_ptr<ID3D12Resource> vertexBuffer;
    winrt::com_ptr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW       vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW        indexBufferView;
};

class Mesh
{
public:
    explicit Mesh(const wchar_t* filename);

private:
    StaticGeometry m_geometry;
};

Mesh::Mesh(const wchar_t* filename)
{
    File file(filename);

    const auto bytes = file.Data();
}

class HelloMesh final : public Example
{
public:
    explicit HelloMesh(bool fullscreen);
    HelloMesh(const HelloMesh& other) = delete;
    HelloMesh& operator=(const HelloMesh& other) = delete;

    ~HelloMesh() override;

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
    winrt::com_ptr<ID3D12Resource>       m_indexBuffer;
    winrt::com_ptr<ID3D12DescriptorHeap> m_cbvDescriptorHeap;
    D3D12_VERTEX_BUFFER_VIEW             m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW              m_indexBufferView;
    winrt::com_ptr<ID3D12Resource>       m_constBuffer;
    SceneConstantBuffer                  m_constBufferData;
    UINT8*                               m_constBufferDataBegin;
    float                                m_rotationY = 0.0f;
    float                                m_rotationX = 0.0f;
    float                                m_cubeRotationY = 0.0f;
};

HelloMesh::HelloMesh(bool fullscreen)
    : Example(L"Hello, Mesh", 800, 600, fullscreen), m_vertexBufferView(), m_constBufferDataBegin(nullptr)
{
}

HelloMesh::~HelloMesh() = default;

bool HelloMesh::Load()
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

    return true;
}

void HelloMesh::Update(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.ElapsedSeconds());

    m_mouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

    auto ms = m_mouse->GetState();

    m_rotationX -= ms.x * elapsed;
    m_rotationY -= ms.y * elapsed;

    m_camera->rotate(m_rotationY, m_rotationX);

    // Clamp m_rotationX between 75 degrees and -75 degrees
    m_rotationY = std::clamp(m_rotationY, -XMConvertToRadians(75.0f), XMConvertToRadians(75.0f));

    m_cubeRotationY += elapsed;
}

void HelloMesh::Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer)
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
    // Set the index buffer
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // Draw indexed geometry
    commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void HelloMesh::UpdateUniforms()
{
    auto position = Vector3(0.0f, 0.0, -10.0f);
    auto rotationX = 0.0f;
    auto rotationY = m_cubeRotationY;
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

void HelloMesh::CreateRootSignature()
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

void HelloMesh::CreatePipelineState()
{
    ID3D12Device* device = m_context->Device();

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    auto rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterDesc.FrontCounterClockwise = TRUE;

    std::vector<uint8_t> vertexShader;
    std::vector<uint8_t> pixelShader;
    try
    {
        File vs(L"SimpleShaderVS.bin");
        File ps(L"SimpleShaderPS.bin");

        vertexShader = vs.Data();
        pixelShader = ps.Data();
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

void HelloMesh::CreateBuffers()
{
    ID3D12Device* device = m_context->Device();

    // Define the geometry for a cube.
    constexpr Vertex cubeVertices[] = {
        // Front Face
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

        // Back Face
        {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

        // Top Face
        {{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

        // Bottom Face
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

        // Left Face
        {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},

        // Right Face
        {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
    };

    // Define indices for a cube so that each triangle is front-facing
    constexpr uint16_t cubeIndices[] = {
        0,  1,  2,  0,  2,  3,  // Front Face
        4,  5,  6,  4,  6,  7,  // Back Face
        8,  9,  10, 8,  10, 11, // Top Face
        12, 13, 14, 12, 14, 15, // Bottom Face
        16, 17, 18, 16, 18, 19, // Left Face
        20, 21, 22, 20, 22, 23  // Right Face
    };

    constexpr UINT vertexBufferSize = sizeof(cubeVertices);
    const auto     heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto     resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    winrt::check_hresult(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                         IID_PPV_ARGS(&m_vertexBuffer)));
    UINT8*              vertexDataBegin;
    const CD3DX12_RANGE readRange(0, 0);
    winrt::check_hresult(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, cubeVertices, sizeof(cubeVertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    constexpr UINT indexBufferSize = sizeof(cubeIndices);
    const auto     indexHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto     indexResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    winrt::check_hresult(device->CreateCommittedResource(&indexHeapProps, D3D12_HEAP_FLAG_NONE, &indexResourceDesc,
                                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                         IID_PPV_ARGS(&m_indexBuffer)));

    // Copy cube index data to index buffer
    UINT8* indexDataBegin;
    winrt::check_hresult(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexDataBegin)));
    memcpy(indexDataBegin, cubeIndices, sizeof(cubeIndices));
    m_indexBuffer->Unmap(0, nullptr);
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    bool fullscreen = false;
    if (argc > 1)
    {
        for (int i = 0; i < argc; i++)
        {
            if (wcscmp(argv[i], L"--fullscreen") == 0)
            {
                fullscreen = true;
            }
        }
    }
    const auto example = std::make_unique<HelloMesh>(fullscreen);
    return example->Run(argc, argv);
}
