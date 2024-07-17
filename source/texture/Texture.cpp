
#include "Texture.hpp"

#include <DDSTextureLoader.h>

Texture::Texture(ID3D12Device* device)
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