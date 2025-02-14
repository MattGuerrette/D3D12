////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include "Example.hpp"
#include "File.hpp"
#include "Texture.hpp"

#include <SDL3/SDL_main.h>

#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>

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
    Vector2 TexCoord;
};

XM_ALIGNED_STRUCT(256) SceneConstantBuffer
{
    Matrix ModelViewProjection;
    float  DeltaTime;
    float  Time;
};

class HelloTexture final : public Example
{
public:
    explicit HelloTexture(bool fullscreen);
    HelloTexture(const HelloTexture& other) = delete;
    HelloTexture& operator=(const HelloTexture& other) = delete;

    ~HelloTexture() override;

    bool Load() override;

    void Update(const GameTimer& timer) override;

    void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) override;

private:
    void CreateRootSignature();

    void CreateBuffers();

    void CreatePipelineState();

    void LoadTexture();

    void UpdateUniforms(float deltaTime);

    winrt::com_ptr<ID3D12RootSignature>   m_rootSignature;
    winrt::com_ptr<ID3D12PipelineState>   m_pipelineState;
    winrt::com_ptr<ID3D12Resource>        m_vertexBuffer;
    winrt::com_ptr<ID3D12Resource>        m_indexBuffer;
    winrt::com_ptr<ID3D12DescriptorHeap>  m_srvDescriptorHeap;
    std::vector<std::unique_ptr<Texture>> m_textures;
    // winrt::com_ptr<ID3D12Resource>       m_texture;
    D3D12_VERTEX_BUFFER_VIEW              m_vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW               m_indexBufferView{};
    winrt::com_ptr<ID3D12Resource>        m_constBuffer;
    SceneConstantBuffer                   m_constBufferData{};
    UINT8*                                m_constBufferDataBegin{};
    float                                 m_rotationY = 0.0f;
    float                                 m_rotationX = 0.0f;
    float                                 m_cubeRotationY = 0.0f;
};

HelloTexture::HelloTexture(bool fullscreen) : Example("Hello, Texture", 800, 600, fullscreen)
{
}

HelloTexture::~HelloTexture() = default;

bool HelloTexture::Load()
{
    CreateRootSignature();

    CreateBuffers();

    CreatePipelineState();

    m_textures.push_back(std::make_unique<Texture>("dirt.dds"));
    m_textures.push_back(std::make_unique<Texture>("bricks.dds"));

    D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
    srvDescriptorHeapDesc.NumDescriptors = m_textures.size();
    srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    winrt::check_hresult(
        m_context->Device()->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)));

    for (uint32_t i = 0; i < m_textures.size(); ++i)
    {
        m_textures[i]->Upload(m_context->Device(), m_context->CommandQueue());
        m_textures[i]->AddToDescriptorHeap(m_context->Device(), m_srvDescriptorHeap.get(), i);
    }

    SDL_HideCursor();

    return true;
}

void HelloTexture::Update(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.GetElapsedSeconds());

    SDL_WarpMouseInWindow(m_window, GetFrameWidth() / 2, GetFrameHeight() / 2);

    m_rotationX -= m_mouse->RelativeX() * elapsed;
    m_rotationY -= m_mouse->RelativeY() * elapsed;

    m_camera->rotate(m_rotationY, m_rotationX);

    // Clamp m_rotationX between 75 degrees and -75 degrees
    m_rotationY = std::clamp(m_rotationY, -XMConvertToRadians(75.0f), XMConvertToRadians(75.0f));

    m_cubeRotationY += elapsed;
}

void HelloTexture::Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.GetElapsedSeconds());

    UpdateUniforms(elapsed);

    // Set the root signature
    commandList->SetGraphicsRootSignature(m_rootSignature.get());

    ID3D12DescriptorHeap* heaps[] = {m_srvDescriptorHeap.get()};
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    CD3DX12_GPU_DESCRIPTOR_HANDLE tex(
        m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 1,
        m_context->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    commandList->SetGraphicsRootDescriptorTable(0, tex);

    // Set the constant buffer view (For scene data such as camera and timing)
    commandList->SetGraphicsRootConstantBufferView(1, m_constBuffer->GetGPUVirtualAddress());

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

void HelloTexture::UpdateUniforms(float deltaTime)
{
    auto position = Vector3(0.0f, 0.0, -10.0f);
    auto rotationX = 0.0f;
    // DirectX::XMConvertToRadians(75.0f);
    auto rotationY = 0.0f;
    // m_cubeRotationY;
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
    m_constBufferData.DeltaTime = deltaTime;
    m_constBufferData.Time += deltaTime;
    memcpy(m_constBufferDataBegin, &m_constBufferData, sizeof(m_constBufferData));
}

void HelloTexture::CreateRootSignature()
{
    ID3D12Device* device = m_context->Device();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParams[2];
    rootParams[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
                                           D3D12_SHADER_VISIBILITY_VERTEX);

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams, m_context->Samplers().size(),
                               m_context->Samplers().data(), rootSignatureFlags);

    winrt::com_ptr<ID3DBlob> signature;
    winrt::com_ptr<ID3DBlob> error;
    winrt::check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion,
                                                               signature.put(), error.put()));
    winrt::check_hresult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                     IID_PPV_ARGS(&m_rootSignature)));
}

void HelloTexture::CreatePipelineState()
{
    ID3D12Device* device = m_context->Device();

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Color),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, TexCoord),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    auto rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;

    std::vector<uint8_t> vertexShader;
    std::vector<uint8_t> pixelShader;
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

void HelloTexture::CreateBuffers()
{
    ID3D12Device* device = m_context->Device();

    // Define the geometry for a cube.
    const Vertex cubeVertices[] = {
        // Back Face
        {{1.0f, 1.0f, -1.0f}, Vector4{DirectX::Colors::Black}, {0.0f, 0.0f}},
        {{-1.0f, 1.0f, -1.0f}, Vector4{DirectX::Colors::White}, {1.0f, 0.0f}},
        {{1.0f, -1.0f, -1.0f}, Vector4{DirectX::Colors::LimeGreen}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f}, Vector4{DirectX::Colors::Purple}, {1.0f, 1.0f}},

        // Front Face
        {{-1.0f, 1.0f, 1.0f}, Vector4{DirectX::Colors::Black}, {0.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}, Vector4{DirectX::Colors::White}, {1.0f, 0.0f}},
        {{-1.0f, -1.0f, 1.0f}, Vector4{DirectX::Colors::LimeGreen}, {0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}, Vector4{DirectX::Colors::Purple}, {1.0f, 1.0f}},

        //// Top Face
        {{-1.0f, 1.0f, -1.0f}, Vector4{DirectX::Colors::Black}, {0.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, Vector4{DirectX::Colors::White}, {1.0f, 0.0f}},
        {{-1.0f, 1.0f, 1.0f}, Vector4{DirectX::Colors::LimeGreen}, {0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}, Vector4{DirectX::Colors::Purple}, {1.0f, 1.0f}},

        // Bottom Face
        {{1.0f, -1.0f, -1.0f}, Vector4{DirectX::Colors::Black}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, Vector4{DirectX::Colors::White}, {1.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, Vector4{DirectX::Colors::LimeGreen}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}, Vector4{DirectX::Colors::Purple}, {1.0f, 1.0f}},

        //// Left Face
        {{-1.0f, 1.0f, -1.0f}, Vector4{DirectX::Colors::Black}, {0.0f, 0.0f}},
        {{-1.0f, 1.0f, 1.0f}, Vector4{DirectX::Colors::White}, {1.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, Vector4{DirectX::Colors::LimeGreen}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}, Vector4{DirectX::Colors::Purple}, {1.0f, 1.0f}},

        // Right Face
        {{1.0f, 1.0f, 1.0f}, Vector4{DirectX::Colors::Black}, {0.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, Vector4{DirectX::Colors::White}, {1.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, Vector4{DirectX::Colors::LimeGreen}, {0.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f}, Vector4{DirectX::Colors::Purple}, {1.0f, 1.0f}},
    };

    // Define indices for a cube so that each triangle is front-facing
    constexpr uint16_t cubeIndices[] = {
        0,  1,  2,  2,  1,  3,  // Front Face
        4,  5,  6,  6,  5,  7,  // Back Face
        8,  9,  10, 10, 9,  11, // Top Face
        12, 13, 14, 14, 13, 15, // Bottom Face
        16, 17, 18, 18, 17, 19, // Left Face
        20, 21, 22, 22, 21, 23  // Right Face
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

        //// Map and initialize the constant buffer. We don't unmap this until the
        //// app closes. Keeping things mapped for the lifetime of the resource is okay
        winrt::check_hresult(m_constBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constBufferDataBegin)));
        memcpy(m_constBufferDataBegin, &m_constBufferData, sizeof(m_constBufferData));
    }
}

void HelloTexture::LoadTexture()
{
    /*ResourceUploadBatch upload(m_context->Device());
    upload.Begin();

    File file("dirt.dds");
    auto ddsData = file.ReadAll();
    winrt::check_hresult(
        CreateDDSTextureFromMemory(m_context->Device(), upload, ddsData.data(), ddsData.size(), m_texture.put()));

    auto finish = upload.End(m_context->CommandQueue());
    finish.wait();

    auto format = m_texture->GetDesc().Format;
    printf("Test\n");*/
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
    const auto example = std::make_unique<HelloTexture>(fullscreen);
    return example->Run(argc, argv);
}
