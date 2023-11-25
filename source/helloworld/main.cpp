////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include "Example.hpp"
#include "FileUtil.hpp"

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
    HelloWorld();

    ~HelloWorld() override;

    bool Load() override;

    void Update(const GameTimer& timer) override;

    void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) override;

private:
    void CreateRootSignature();

    void CreateBuffers();

    void CreatePipelineState();

    void UpdateUniforms();

    winrt::com_ptr<ID3D12RootSignature> RootSignature;
    winrt::com_ptr<ID3D12PipelineState> PipelineState;
    winrt::com_ptr<ID3D12Resource> VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    winrt::com_ptr<ID3D12Resource> ConstantBuffer;
    SceneConstantBuffer ConstantBufferData;
    UINT8* ConstantBufferDataBegin;

    float RotationY = 0.0f;
};

HelloWorld::HelloWorld()
    : Example("Hello, Metal", 800, 600), VertexBufferView(), ConstantBufferDataBegin(nullptr)
{
}

HelloWorld::~HelloWorld() = default;

bool HelloWorld::Load()
{
    CreateRootSignature();

    CreateBuffers();

    CreatePipelineState();

    return true;
}

void HelloWorld::Update(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.GetElapsedSeconds());

    if (Mouse->LeftPressed())
    {
        RotationY += static_cast<float>(Mouse->RelativeX()) * elapsed;
    }

    //RotationY += elapsed;
}

void HelloWorld::Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer)
{
    UpdateUniforms();

    // Set the root signature
    commandList->SetGraphicsRootSignature(RootSignature.get());

    ID3D12DescriptorHeap* heaps[] = {CbvDescriptorHeap.get()};
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(
        0, CbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());


    // Set the pipeline state
    commandList->SetPipelineState(PipelineState.get());


    // Set the primitive topology
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set the vertex buffer
    commandList->IASetVertexBuffers(0, 1, &VertexBufferView);

    // Draw instanced
    commandList->DrawInstanced(3, 1, 0, 0);
}

void HelloWorld::UpdateUniforms()
{
    auto position = Vector3(0.0f, 0.0, -10.0f);
    auto rotationX = 0.0f;
    auto rotationY = RotationY;
    auto scaleFactor = 3.0f;

    const Vector3 xAxis = Vector3::Right;
    const Vector3 yAxis = Vector3::Up;

    Matrix xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
    Matrix yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
    Matrix rotation = xRot * yRot;

    Matrix translation = Matrix::CreateTranslation(position);
    Matrix scale = Matrix::CreateScale(scaleFactor);
    Matrix model = scale * rotation * translation;

    ConstantBufferData.ModelViewProjection = model * MainCamera->GetUniforms().ViewProjection;
    memcpy(ConstantBufferDataBegin, &ConstantBufferData, sizeof(ConstantBufferData));
}

void HelloWorld::CreateRootSignature()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    CD3DX12_ROOT_PARAMETER1 rootParams[1];
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
    winrt::check_hresult(
        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, signature.put(),
                                              error.put()));
    winrt::check_hresult(Device->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(&RootSignature)));
}

void HelloWorld::CreatePipelineState()
{
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    auto rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

    uint32_t vsSize, psSize;
    auto vertexShader = FileUtil::ReadAllBytes("SimpleShaderVS.bin", &vsSize);
    auto pixelShader = FileUtil::ReadAllBytes("SimpleShaderPS.bin", &psSize);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
    psoDesc.pRootSignature = RootSignature.get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader, vsSize);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader, psSize);
    psoDesc.RasterizerState = rasterDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    winrt::check_hresult(
        Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineState)));
}

void HelloWorld::CreateBuffers()
{
    // Define the geometry for a triangle.
    constexpr Vertex triangleVertices[] =
    {
        {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };

    constexpr UINT vertexBufferSize = sizeof(triangleVertices);
    const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    winrt::check_hresult(Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&VertexBuffer)
    ));

    // Copy triangle data to vertex buffer
    UINT8* vertexDataBegin;
    const CD3DX12_RANGE readRange(0, 0);
    winrt::check_hresult(VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, triangleVertices, sizeof(triangleVertices));
    VertexBuffer->Unmap(0, nullptr);

    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = sizeof(Vertex);
    VertexBufferView.SizeInBytes = vertexBufferSize;


    // Create the constant buffer.
    {
        constexpr UINT constantBufferSize = sizeof(SceneConstantBuffer); // CB size is required to be 256-byte aligned.

        const auto cbHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto cbHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        winrt::check_hresult(Device->CreateCommittedResource(
            &cbHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbHeapDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&ConstantBuffer)));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = ConstantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
        Device->CreateConstantBufferView(
            &cbvDesc, CbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay
        winrt::check_hresult(ConstantBuffer->Map(0, &readRange,
                                                 reinterpret_cast<void**>(&ConstantBufferDataBegin)));
        memcpy(ConstantBufferDataBegin, &ConstantBufferData, sizeof(ConstantBufferData));
    }
}

#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else
int main(const int argc, char** argv)
#endif
{
    const std::unique_ptr<HelloWorld> example = std::make_unique<HelloWorld>();
    return example->Run(argc, argv);
}
