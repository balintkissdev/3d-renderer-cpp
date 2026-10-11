#ifndef D3D12_SKYBOX_HPP_
#define D3D12_SKYBOX_HPP_

#include "utils.hpp"

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <winrt/base.h>

class D3D12Skybox
{
public:
    friend class D3D12SkyboxBuilder;

    D3D12Skybox();

    DISABLE_COPY(D3D12Skybox)
    D3D12Skybox(D3D12Skybox&& other) noexcept = default;
    D3D12Skybox& operator=(D3D12Skybox&& other) noexcept = default;

    void draw(ID3D12GraphicsCommandList* commandList,
              ID3D12Resource* mvpConstantBuffer);

private:
    winrt::com_ptr<ID3D12Resource> m_vertexBuffer;
    winrt::com_ptr<ID3D12Resource> m_indexBuffer;
    winrt::com_ptr<ID3D12Resource> m_vertexUploadBuffer;
    winrt::com_ptr<ID3D12Resource> m_indexUploadBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    UINT m_indexCount;

    winrt::com_ptr<ID3D12Resource> m_textureResource;
    winrt::com_ptr<ID3D12Resource> m_textureUploadBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE m_textureSrvGpuHandle;

    winrt::com_ptr<ID3D12RootSignature> m_rootSignature;
    winrt::com_ptr<ID3D12PipelineState> m_pso;
};

class D3D12SkyboxBuilder
{
public:
    std::optional<D3D12Skybox> build(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

private:
    bool createRootSignature(ID3D12Device* device, D3D12Skybox& skybox);
    bool createPSO(ID3D12Device* device, D3D12Skybox& skybox);
    bool createVertexBuffer(ID3D12Device* device,
                            ID3D12GraphicsCommandList* commandList,
                            D3D12Skybox& skybox);
    bool createIndexBuffer(ID3D12Device* device,
                           ID3D12GraphicsCommandList* commandList,
                           D3D12Skybox& skybox);
    bool createTexture(ID3D12Device* device,
                       ID3D12GraphicsCommandList* commandList,
                       D3D12Skybox& skybox,
                       CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
                       CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);
};

#endif
