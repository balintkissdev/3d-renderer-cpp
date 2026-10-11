#include "d3d12_model.hpp"

#include "meshimporter.hpp"
#include "utils.hpp"

#include <cstddef>
#include <d3dx12.h>
#include <utility>

namespace fs = std::filesystem;

std::optional<D3D12Model> D3D12Model::create(ID3D12Device* device,
                                             const fs::path& filePath)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    D3D12Model model;
    if (!MeshImporter::loadFromFile(filePath,
                                    vertices,
                                    indices,
                                    MeshImporter::Winding::Clockwise))
    {
        return std::nullopt;
    }

    // Vertex buffer
    const UINT vertexBufferSize
        = static_cast<UINT>(sizeof(vertices[0]) * vertices.size());
    const UINT stride = sizeof(Vertex);

    // TODO: Research better (default) heap usage
    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc
        = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    device->CreateCommittedResource(&heapProperties,
                                    D3D12_HEAP_FLAG_NONE,
                                    &bufferDesc,
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr,
                                    IID_PPV_ARGS(model.m_vertexBuffer.put()));

    // Copy data to vertex buffer
    Vertex* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    model.m_vertexBuffer->Map(0,
                              &readRange,
                              reinterpret_cast<void**>(&vertexDataBegin));
    std::memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    model.m_vertexBuffer->Unmap(0, nullptr);

    // Init vertex buffer view
    model.m_vertexBufferView.BufferLocation
        = model.m_vertexBuffer->GetGPUVirtualAddress();
    model.m_vertexBufferView.StrideInBytes = stride;
    model.m_vertexBufferView.SizeInBytes = vertexBufferSize;

    // Index buffer
    const UINT indexBufferSize
        = static_cast<UINT>(sizeof(indices[0]) * indices.size());

    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    device->CreateCommittedResource(&heapProperties,
                                    D3D12_HEAP_FLAG_NONE,
                                    &bufferDesc,
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr,
                                    IID_PPV_ARGS(model.m_indexBuffer.put()));

    uint32_t* indexDataBegin;
    readRange = CD3DX12_RANGE(0, 0);
    model.m_indexBuffer->Map(0,
                             &readRange,
                             reinterpret_cast<void**>(&indexDataBegin));
    std::memcpy(indexDataBegin, indices.data(), indexBufferSize);
    model.m_indexBuffer->Unmap(0, nullptr);

    model.m_indexBufferView.BufferLocation
        = model.m_indexBuffer->GetGPUVirtualAddress();
    // Keep the index buffer format 32-bit instead of 16-bit, otherwise mesh
    // like the Stanford Bunny becomes an eldritch abomination.
    model.m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    model.m_indexBufferView.SizeInBytes = indexBufferSize;
    model.m_indexCount = static_cast<UINT>(indices.size());

    return model;
}

D3D12Model::D3D12Model()
    : m_indexCount{0}
{
}

D3D12Model::D3D12Model(D3D12Model&& other) noexcept = default;
D3D12Model& D3D12Model::operator=(D3D12Model&& other) noexcept = default;

