
#include "Example.hpp"

#include "File.hpp"
#include "Texture.hpp"

#include <SDL3/SDL_main.h>

#include <directxtk12/DDSTextureLoader.h>

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 614;
}

extern "C"
{
    __declspec(dllexport) extern const auto* D3D12SDKPath = u8".\\D3D12\\";
}

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector3 Position;
    Vector2 Color;
};

class TextureExample : public Example
{
public:
    TextureExample();
    TextureExample(const TextureExample& other) = delete;
    TextureExample& operator=(const TextureExample& other) = delete;

    ~TextureExample() override;

    bool Load() override;

    void Update(const GameTimer& timer) override;

    void Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer) override;

private:
    void CreateRootSignature();

    void CreateBuffers();

    void CreateTexture();

    winrt::com_ptr<ID3D12RootSignature> m_rootSignature;
    winrt::com_ptr<ID3D12Resource>      m_texture;
    winrt::com_ptr<ID3D12Resource>      m_vertexBuffer;
};

TextureExample::TextureExample() : Example("Textures", 800, 600)
{
}

TextureExample::~TextureExample() = default;

bool TextureExample::Load()
{
    CreateRootSignature();

    CreateTexture();
    return true;
}

void TextureExample::Update(const GameTimer& timer)
{
}

void TextureExample::Render(ID3D12GraphicsCommandList* commandList, const GameTimer& timer)
{
}

void TextureExample::CreateRootSignature()
{
    ID3D12Device* device = m_context->Device();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1]{};
    CD3DX12_ROOT_PARAMETER1   rootParams[1]{};

    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    rootParams[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, rootSignatureFlags);

    winrt::com_ptr<ID3DBlob> signature;
    winrt::com_ptr<ID3DBlob> error;
    winrt::check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion,
                                                               signature.put(), error.put()));
    winrt::check_hresult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                     IID_PPV_ARGS(&m_rootSignature)));
}

void TextureExample::CreateBuffers()
{
    ID3D12Device* device = m_context->Device();
}

void TextureExample::CreateTexture()
{
    try
    {
        File file("example.dds");

        ID3D12Device* device = m_context->Device();

        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        const auto                          bytes = file.ReadAll();
        winrt::check_hresult(
            DirectX::LoadDDSTextureFromMemory(device, bytes.data(), bytes.size(), m_texture.put(), subresources));
    }
    catch (const winrt::hresult_error& e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "HRESULT Exception", winrt::to_string(e.message()).c_str(),
                                 m_window);
    }
    catch (const std::exception& e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Exception", e.what(), m_window);
    }
}

int main(int argc, char* argv[])
{
    winrt::init_apartment();

    std::unique_ptr<TextureExample> example = std::make_unique<TextureExample>();
    return example->Run(argc, argv);
}