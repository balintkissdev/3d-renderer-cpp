#ifndef D3D12_RENDERER_HPP_
#define D3D12_RENDERER_HPP_

#include "direct3d12/d3d12_model.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "win32_window.hpp"

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <filesystem>
#include <winrt/base.h>

class Scene;

class D3D12Renderer : public Renderer
{
public:
    D3D12Renderer(Window& window,
                  const DrawProperties& drawProps,
                  const Camera& camera);
    ~D3D12Renderer() override;
    DISABLE_COPY_AND_MOVE(D3D12Renderer)

    bool init() final;
    void initImGuiBackend() final;
    void cleanup() final;
    void draw(const Scene& scene) final;
    void setVSyncEnabled(const bool vsyncEnabled) final;

private:
    static constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;
    struct alignas(CONSTANT_BUFFER_ALIGNMENT) MVPConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4X4 normalMatrix;
    };
    static_assert((sizeof(MVPConstantBuffer) % CONSTANT_BUFFER_ALIGNMENT) == 0,
                  "MVP Constant Buffer size must be 256-byte aligned");

    enum AdsPropertiesFlags : uint8_t
    {
        ADS_FLAG_DIFFUSE_ENABLED = 1,
        ADS_FLAG_SPECULAR_ENABLED = 2,
    };

    // See this link for HLSL Constant Buffer layout rules:
    // https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/
    struct alignas(CONSTANT_BUFFER_ALIGNMENT) MaterialConstantBuffer
    {
        DirectX::XMFLOAT3 color;
        // Bit packing instead of bools to avoid wasting 4 byte for each bool
        uint32_t adsPropertiesFlags;
        DirectX::XMFLOAT3 viewPos;
        float padding;
        DirectX::XMFLOAT3 lightDirection;
    };
    static_assert((sizeof(MaterialConstantBuffer) % CONSTANT_BUFFER_ALIGNMENT)
                      == 0,
                  "Material Constant Buffer size must be 256-byte aligned");

    static constexpr UINT FRAME_COUNT = 2;
    static constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT
        = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static constexpr size_t MAX_MODEL_SCENE_NODE_COUNT = 256;

    CD3DX12_VIEWPORT viewPort_;
    CD3DX12_RECT scissorRect_;

    winrt::com_ptr<ID3D12Device> device_;
    winrt::com_ptr<IDXGISwapChain3> swapChain_;
    winrt::com_ptr<ID3D12CommandQueue> commandQueue_;
    winrt::com_ptr<ID3D12CommandAllocator> commandAllocator_;
    winrt::com_ptr<ID3D12GraphicsCommandList> commandList_;
    winrt::com_ptr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    size_t rtvDescriptorHeapSize_;
    winrt::com_ptr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    winrt::com_ptr<ID3D12DescriptorHeap> cbvDescriptorHeap_;
    size_t cbvDescriptorHeapSize_;
    winrt::com_ptr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    std::array<winrt::com_ptr<ID3D12Resource>, FRAME_COUNT> renderTargets_;
    winrt::com_ptr<ID3D12Resource> depthStencil_;
    winrt::com_ptr<ID3D12RootSignature> rootSignature_;
    winrt::com_ptr<ID3D12PipelineState> pso_;
    std::vector<winrt::com_ptr<ID3D12Resource>> mvpConstantBuffers_;
    std::vector<winrt::com_ptr<ID3D12Resource>> materialConstantBuffers_;
    std::vector<uint8_t*> mvpCbvDataBegin_;
    std::vector<uint8_t*> materialCbvDataBegin_;

    winrt::com_ptr<ID3D12Fence> fence_;
    UINT frameIndex_;
    UINT64 fenceValue_;
    HANDLE fenceEvent_;

    bool vsyncEnabled_;
    std::vector<D3D12Model> models_;

    bool createDevice();
    bool createCommandObjects();
    bool createDescriptorHeaps();
    bool createRTVs();
    bool createDSV();
    bool createCBVs();
    template <typename ConstantBufferType>
    bool createConstantBufferAndView(
        const CD3DX12_HEAP_PROPERTIES& heapProperties,
        winrt::com_ptr<ID3D12Resource>& constantBuffer,
        UINT cbvDescriptorHeapIndex,
        uint8_t*& cbvDataBegin);
    bool createRootSignature();
    bool createPSO();
    enum class ShaderCompileType : uint8_t
    {
        VertexShader,
        PixelShader,
    };
    bool compileShader(const std::filesystem::path& shaderPath,
                       const ShaderCompileType shaderType,
                       winrt::com_ptr<ID3DBlob>& shaderOut);
    void createSyncObjects();
    bool loadAssets();

    void drawModels(const Scene& scene,
                    MVPConstantBuffer& mvpConstantBufferData,
                    MaterialConstantBuffer& materialConstantBufferData);
    void waitForPreviousFrame();
};

inline void D3D12Renderer::setVSyncEnabled(const bool vsyncEnabled)
{
    vsyncEnabled_ = vsyncEnabled;
}

#endif
