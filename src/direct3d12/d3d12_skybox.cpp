#include "d3d12_skybox.hpp"

#include "DirectXTex/DirectXTex/DirectXTex.h"
#include "d3d12_shader.hpp"

#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;
using namespace winrt;
namespace fs = std::filesystem;

D3D12Skybox::D3D12Skybox()
    : indexCount_{0}
{
}

void D3D12Skybox::draw(ID3D12GraphicsCommandList* commandList,
                       ID3D12Resource* mvpConstantBuffer)
{
    commandList->SetGraphicsRootSignature(rootSignature_.get());
    commandList->SetPipelineState(pso_.get());

    // Root CBV
    commandList->SetGraphicsRootConstantBufferView(
        0,
        mvpConstantBuffer->GetGPUVirtualAddress());
    // Skybox texture SRV
    commandList->SetGraphicsRootDescriptorTable(1, textureSrvGpuHandle_);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

std::optional<D3D12Skybox> D3D12SkyboxBuilder::build(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    D3D12Skybox skybox;
    if (!createRootSignature(device, skybox) || !createPSO(device, skybox)
        || !createVertexBuffer(device, commandList, skybox)
        || !createIndexBuffer(device, commandList, skybox)
        || !createTexture(device,
                          commandList,
                          skybox,
                          srvCpuHandle,
                          srvGpuHandle))
    {
        return std::nullopt;
    }

    return skybox;
}

bool D3D12SkyboxBuilder::createRootSignature(ID3D12Device* device,
                                             D3D12Skybox& skybox)
{
    std::array<D3D12_ROOT_PARAMETER1, 2> rootParameters = {};

    // Projection-view matrix Constant Buffer
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Texture descriptor table
    D3D12_DESCRIPTOR_RANGE1 descriptorRange{};
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.NumDescriptors = 1;
    descriptorRange.BaseShaderRegister = 0;
    descriptorRange.RegisterSpace = 0;
    descriptorRange.OffsetInDescriptorsFromTableStart = 0;

    rootParameters[1].ParameterType
        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static sampler
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root Signature
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc;
    versionedDesc.Init_1_1(
        rootParameters.size(),
        rootParameters.data(),
        1,
        &sampler,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    com_ptr<ID3DBlob> signature;
    com_ptr<ID3DBlob> error;
    HRESULT hr
        = D3DX12SerializeVersionedRootSignature(&versionedDesc,
                                                D3D_ROOT_SIGNATURE_VERSION_1_1,
                                                signature.put(),
                                                error.put());
    if (FAILED(hr) || error)
    {
        return false;
    }

    hr = device->CreateRootSignature(0,
                                     signature->GetBufferPointer(),
                                     signature->GetBufferSize(),
                                     IID_PPV_ARGS(skybox.rootSignature_.put()));
    return SUCCEEDED(hr);
}

bool D3D12SkyboxBuilder::createPSO(ID3D12Device* device, D3D12Skybox& skybox)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

    // Root Signature
    psoDesc.pRootSignature = skybox.rootSignature_.get();

    // Input Layout
    const std::array<D3D12_INPUT_ELEMENT_DESC, 1> inputElementDesc
        = {D3D12_INPUT_ELEMENT_DESC{"POSITION",
                                    0,
                                    DXGI_FORMAT_R32G32B32_FLOAT,
                                    0,
                                    0,
                                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                    0}};
    psoDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{inputElementDesc.data(),
                                                  inputElementDesc.size()};

    // Shaders
    com_ptr<ID3DBlob> vertexShader;
    if (!D3D12Shader::CompileShader(
            "skybox.vs.hlsl",
            D3D12Shader::ShaderCompileType::VertexShader,
            vertexShader))
    {
        return false;
    }
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.get());

    com_ptr<ID3DBlob> pixelShader;
    if (!D3D12Shader::CompileShader("skybox.ps.hlsl",
                                    D3D12Shader::ShaderCompileType::PixelShader,
                                    pixelShader))
    {
        return false;
    }
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.get());

    // Topology type
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Rasterizer State
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Depth-Stencil State
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // Blend State
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0]
        = DXGI_FORMAT_R8G8B8A8_UNORM;  // TODO: add global constant that matches
                                       // swapChainDesc.Format
    psoDesc.DSVFormat
        = DXGI_FORMAT_D24_UNORM_S8_UINT;  // TODO: duplicate code from
                                          // d3d12_renderer.hpp
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    HRESULT hr
        = device->CreateGraphicsPipelineState(&psoDesc,
                                              IID_PPV_ARGS(skybox.pso_.put()));
    return SUCCEEDED(hr);
}

bool D3D12SkyboxBuilder::createVertexBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    D3D12Skybox& skybox)
{
    // clang-format off
    const XMFLOAT3 vertices[] = {
        // Front face (+Z)
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        // Back face (-Z)
        { 1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        // Top face (+Y)
        {-1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        // Bottom face (-Y)
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},
        // Right face (+X)
        { 1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f,  1.0f},
        // Left face (-X)
        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f}
    };
    // clang-format on
    const UINT vertexBufferSize = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    CD3DX12_RESOURCE_DESC bufferDesc
        = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    // Vertex buffer
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(skybox.vertexBuffer_.put()));
    if (FAILED(hr))
    {
        return false;
    }

    // Upload buffer
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(skybox.vertexUploadBuffer_.put()));
    if (FAILED(hr))
    {
        return false;
    }

    uint8_t* mappedData = nullptr;
    hr = skybox.vertexUploadBuffer_->Map(0,
                                         nullptr,
                                         reinterpret_cast<void**>(&mappedData));
    if (FAILED(hr))
    {
        return false;
    }
    std::memcpy(mappedData, vertices, vertexBufferSize);
    skybox.vertexUploadBuffer_->Unmap(0, nullptr);

    commandList->CopyResource(skybox.vertexBuffer_.get(),
                              skybox.vertexUploadBuffer_.get());

    CD3DX12_RESOURCE_BARRIER uploadToVertexBufferTransition
        = CD3DX12_RESOURCE_BARRIER::Transition(
            skybox.vertexBuffer_.get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList->ResourceBarrier(1, &uploadToVertexBufferTransition);

    // Vertex buffer view
    skybox.vertexBufferView_.BufferLocation
        = skybox.vertexBuffer_->GetGPUVirtualAddress();
    skybox.vertexBufferView_.StrideInBytes = sizeof(XMFLOAT3);
    skybox.vertexBufferView_.SizeInBytes = vertexBufferSize;

    return true;
}

bool D3D12SkyboxBuilder::createIndexBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    D3D12Skybox& skybox)
{
    // clang-format off
    const std::array<UINT, 36> indices = {
        // Front
        0, 1, 2,
        0, 2, 3,
        // Back
        4, 5, 6,
        4, 6, 7,
        // Top
        8, 9, 10,
        8, 10, 11,
        // Bottom
        12, 13, 14,
        12, 14, 15,
        // Right
        16, 17, 18,
        16, 18, 19,
        // Left
        20, 21, 22,
        20, 22, 23
    };
    // clang-format on

    skybox.indexCount_ = indices.size();
    const UINT indexBufferSize = sizeof(indices);

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    CD3DX12_RESOURCE_DESC bufferDesc
        = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    // Index buffer
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(skybox.indexBuffer_.put()));
    if (FAILED(hr))
    {
        return false;
    }

    // Upload buffer
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(skybox.indexUploadBuffer_.put()));
    if (FAILED(hr))
    {
        return false;
    }

    uint8_t* mappedData = nullptr;
    hr = skybox.indexUploadBuffer_->Map(0,
                                        nullptr,
                                        reinterpret_cast<void**>(&mappedData));
    if (FAILED(hr))
    {
        return false;
    }
    std::memcpy(mappedData, indices.data(), indexBufferSize);
    skybox.indexUploadBuffer_->Unmap(0, nullptr);
    commandList->CopyResource(skybox.indexBuffer_.get(),
                              skybox.indexUploadBuffer_.get());

    CD3DX12_RESOURCE_BARRIER uploadToIndexBufferTransition
        = CD3DX12_RESOURCE_BARRIER::Transition(
            skybox.indexBuffer_.get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_INDEX_BUFFER);
    commandList->ResourceBarrier(1, &uploadToIndexBufferTransition);

    // Index buffer view
    skybox.indexBufferView_.BufferLocation
        = skybox.indexBuffer_->GetGPUVirtualAddress();
    skybox.indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    skybox.indexBufferView_.SizeInBytes = indexBufferSize;

    return true;
}

bool D3D12SkyboxBuilder::createTexture(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    D3D12Skybox& skybox,
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    // TODO: Move to generic texture loader system
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return false;
    }

    // TODO: Research CreateDDSTextureFromFileEx() and work submission with
    // ResourceUploadBatch()
    // TODO: Add pipeline to precook JPG files to DDS
    DirectX::ScratchImage image;
    hr = DirectX::LoadFromDDSFile(L"assets/skybox/skybox.dds",
                                  DDS_FLAGS_NONE,
                                  nullptr,
                                  image);
    if (FAILED(hr))
    {
        return false;
    }
    const DirectX::TexMetadata& metadata = image.GetMetadata();

    // Texture heap
    CD3DX12_HEAP_PROPERTIES textureHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    constexpr int cubemapFaceCount = 6;
    CD3DX12_RESOURCE_DESC textureDesc
        = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format,
                                       metadata.width,
                                       metadata.height,
                                       cubemapFaceCount,
                                       metadata.mipLevels);
    hr = device->CreateCommittedResource(
        &textureHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(skybox.textureResource_.put()));
    if (FAILED(hr))
    {
        return false;
    }

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    hr = DirectX::PrepareUpload(device,
                                image.GetImages(),
                                image.GetImageCount(),
                                metadata,
                                subresources);
    if (FAILED(hr))
    {
        return false;
    }

    // Upload buffer
    const UINT64 uploadBufferSize
        = ::GetRequiredIntermediateSize(skybox.textureResource_.get(),
                                        0,
                                        subresources.size());
    CD3DX12_HEAP_PROPERTIES textureUploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadDesc
        = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    hr = device->CreateCommittedResource(
        &textureUploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(skybox.textureUploadBuffer_.put()));
    if (FAILED(hr))
    {
        return false;
    }

    ::UpdateSubresources(commandList,
                         skybox.textureResource_.get(),
                         skybox.textureUploadBuffer_.get(),
                         0,
                         0,
                         subresources.size(),
                         subresources.data());

    CD3DX12_RESOURCE_BARRIER uploadToSrvTransition
        = CD3DX12_RESOURCE_BARRIER::Transition(
            skybox.textureResource_.get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &uploadToSrvTransition);

    // Cubemap SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    device->CreateShaderResourceView(skybox.textureResource_.get(),
                                     &srvDesc,
                                     srvCpuHandle);
    skybox.textureSrvGpuHandle_ = srvGpuHandle;

    return true;
}

