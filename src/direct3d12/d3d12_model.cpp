#include "d3d12_model.hpp"

#include "modelimporter.hpp"
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
    if (!ModelImporter::loadFromFile(filePath,
                                     vertices,
                                     indices,
                                     ModelImporter::Winding::Clockwise))
    {
        return std::nullopt;
    }

    // Vertex buffer
    const UINT vertexBufferSize = sizeof(vertices[0]) * vertices.size();
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
                                    IID_PPV_ARGS(model.vertexBuffer_.put()));

    // Copy data to vertex buffer
    Vertex* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    model.vertexBuffer_->Map(0,
                             &readRange,
                             reinterpret_cast<void**>(&vertexDataBegin));
    std::memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    model.vertexBuffer_->Unmap(0, nullptr);

    // Init vertex buffer view
    model.vertexBufferView_.BufferLocation
        = model.vertexBuffer_->GetGPUVirtualAddress();
    model.vertexBufferView_.StrideInBytes = stride;
    model.vertexBufferView_.SizeInBytes = vertexBufferSize;

    // Index buffer
    const UINT indexBufferSize = sizeof(indices[0]) * indices.size();

    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    device->CreateCommittedResource(&heapProperties,
                                    D3D12_HEAP_FLAG_NONE,
                                    &bufferDesc,
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr,
                                    IID_PPV_ARGS(model.indexBuffer_.put()));

    uint32_t* indexDataBegin;
    readRange = CD3DX12_RANGE(0, 0);
    model.indexBuffer_->Map(0,
                            &readRange,
                            reinterpret_cast<void**>(&indexDataBegin));
    std::memcpy(indexDataBegin, indices.data(), indexBufferSize);
    model.indexBuffer_->Unmap(0, nullptr);

    model.indexBufferView_.BufferLocation
        = model.indexBuffer_->GetGPUVirtualAddress();
    // Keep the index buffer format 32-bit instead of 16-bit, otherwise mesh
    // like the Stanford Bunny becomes an eldritch abomination.
    model.indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    model.indexBufferView_.SizeInBytes = indexBufferSize;
    model.indexCount_ = indices.size();

    return model;
}

D3D12Model::D3D12Model()
    : indexCount_{0}
{
}

D3D12Model::D3D12Model(D3D12Model&& other) noexcept = default;
D3D12Model& D3D12Model::operator=(D3D12Model&& other) noexcept = default;

