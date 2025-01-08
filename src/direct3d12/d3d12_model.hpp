#ifndef D3D12MODEL_HPP_
#define D3D12MODEL_HPP_

#include <d3d12.h>
#include <filesystem>
#include <optional>
#include <winrt/base.h>

/// Representation of 3D model (currently mesh only).
///
/// Non-copyable, move-only. Mesh face vertices reside in GPU memory.
/// Vertices are referred by indices to avoid storing duplicated vertices.
class D3D12Model
{
public:
    /// Factory method loading a model file and initializing buffers.
    static std::optional<D3D12Model> create(
        ID3D12Device* device,
        const std::filesystem::path& filePath);

    D3D12Model(const D3D12Model& other) = delete;
    D3D12Model& operator=(const D3D12Model& other) = delete;
    D3D12Model(D3D12Model&& other) noexcept;
    D3D12Model& operator=(D3D12Model&& other) noexcept;

    [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& vertexBuffer() const;
    [[nodiscard]] const D3D12_INDEX_BUFFER_VIEW& indexBuffer() const;
    [[nodiscard]] size_t indexCount() const;

private:
    D3D12Model();

    winrt::com_ptr<ID3D12Resource> vertexBuffer_;
    winrt::com_ptr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;
    size_t indexCount_;
};

inline const D3D12_VERTEX_BUFFER_VIEW& D3D12Model::vertexBuffer() const
{
    return vertexBufferView_;
}

inline const D3D12_INDEX_BUFFER_VIEW& D3D12Model::indexBuffer() const
{
    return indexBufferView_;
}

inline size_t D3D12Model::indexCount() const
{
    return indexCount_;
}

#endif
